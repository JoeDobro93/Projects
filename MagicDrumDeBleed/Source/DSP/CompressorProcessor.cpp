#include "CompressorProcessor.h"
#include "BiquadFilter.h"   // for mdd::exactlyEqual

namespace mdd
{

namespace
{
    // The hysteresis zone only sustains a level that is still FALLING (a hit
    // decaying through it). Bleed parked steadily inside the zone stops
    // falling, so it releases normally instead of holding the gate open.
    // "Falling" = the level trails its 6 ms lag by > 0.15 dB (≈ 25 dB/s;
    // drum decays run 40–300 dB/s, bleed wobble is far slower).
    constexpr double kHystLagSeconds = 0.006;
    constexpr double kHystFallEpsDb  = 0.15;
}

void WindowedRms::setWindow (double windowMs)
{
    if (exactlyEqual (windowMs, currentWindowMs))
        return;

    currentWindowMs = windowMs;
    const int newLength = juce::jlimit (1, (int) buffer.size(),
                                        (int) std::lround (windowMs * 0.001 * sr));
    if (newLength != length)
    {
        length = newLength;
        reset();
    }
}

void CompressorProcessor::prepare (double sampleRate, int numChannels, int maxDelaySamples,
                                   int maxMarginSamples)
{
    sr = sampleRate;

    delays.resize ((size_t) juce::jmax (1, numChannels));
    for (auto& d : delays)
        d.prepare (maxDelaySamples);

    gainRing.assign ((size_t) juce::jmax (1, maxMarginSamples), 0.0);
    envRing.assign ((size_t) juce::jmax (1, maxMarginSamples), 0.0);

    gateRms.prepare (sampleRate);
    compRms.prepare (sampleRate);
    hystLagCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, kHystLagSeconds * sr));
    pkRelCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, 0.012 * sr));

    reset();
}

void CompressorProcessor::reset()
{
    for (auto& d : delays)
        d.reset();

    gateRms.reset();
    compRms.reset();

    fastMeanSq = 0.0;
    broadMeanSq = 0.0;
    broadPkSq = 0.0;
    fastPkSq = 0.0;
    vetoLatch = false;
    slowDbLag = -120.0;
    currentGainDb = 0.0;
    holdCounter = 0;
    gateOpen = false;
    eqGateEnv = 0.0;
    eqGateHoldCounter = 0;
    std::fill (gainRing.begin(), gainRing.end(), 0.0);
    std::fill (envRing.begin(), envRing.end(), 0.0);
    ringIdx = 0;
    bypassRemaining = 0;
    vetoBlockedSamples = 0;
    lastBlockGrDb = 0.0f;
}

void CompressorProcessor::setCompParameters (double newCompThresholdDb, double grPerOverDb,
                                             double rmsWindowMs, double attackMs, double releaseMs)
{
    compThresholdDb = newCompThresholdDb;
    compGrPerDb = grPerOverDb;
    compRms.setWindow (rmsWindowMs);
    compAttackCoeff  = 1.0 - std::exp (-1.0 / juce::jmax (1.0, attackMs  * 0.001 * sr));
    compReleaseCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, releaseMs * 0.001 * sr));
}

void CompressorProcessor::setEqGateParameters (double holdMs, double releaseMs,
                                               double newTailRangeDb, double newTailBase01)
{
    tailRangeDb = newTailRangeDb;
    tailBase01 = juce::jlimit (0.0, 1.0, newTailBase01);
    eqGateHoldSamples = (int) std::lround (holdMs * 0.001 * sr);
    // Coefficient chosen so the envelope falls ~60 dB (to 0.001) within the
    // release time — "fully cancels" on the user's timescale.
    const double relSamples = juce::jmax (1.0, releaseMs * 0.001 * sr);
    eqGateReleaseCoeff = 1.0 - std::exp (-6.9078 / relSamples);
}

void CompressorProcessor::setParameters (double newThresholdDb, double newReductionDb,
                                         int newLookaheadSamples, int newMarginSamples,
                                         double rmsWindowMs, double holdMs,
                                         double newHysteresisDb, double newContrastDb)
{
    thresholdDb = newThresholdDb;
    reductionDb = newReductionDb;
    hysteresisDb = newHysteresisDb;
    contrastDb = newContrastDb;

    lookaheadSamples = juce::jmax (0, newLookaheadSamples);
    marginSamples = juce::jlimit (0, (int) gainRing.size(), newMarginSamples);
    for (auto& d : delays)
        d.setDelay (lookaheadSamples + marginSamples);

    gateRms.setWindow (rmsWindowMs);

    // Fast opening detector tracks the Smoothing knob so raising Smoothing
    // still steadies the open decision, but never slower than a few ms.
    const double fastTauSamples = juce::jmax (1.0, juce::jlimit (0.5, 3.0, rmsWindowMs / 6.0) * 0.001 * sr);
    fastCoeff = 1.0 - std::exp (-1.0 / fastTauSamples);

    holdSamples = (int) std::lround (holdMs * 0.001 * sr);
}

void CompressorProcessor::process (juce::AudioBuffer<double>& audio, const double* detector,
                                   const double* detectorBroad,
                                   int numSamples, bool applyGain, double* eqGateEnvOut,
                                   const unsigned char* forceOpen)
{
    const int numChannels = juce::jmin (audio.getNumChannels(), (int) delays.size());
    if (detectorBroad == nullptr)
        detectorBroad = detector;
    float minGainDb = 0.0f;
    float maxRmsDb = -120.0f;
    float maxFastDb = -120.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- Gate RMS detector (un-delayed, UNGATED sidechain) ----
        const double det = detector[i];
        const double meanSquare = gateRms.pushMeanSquare (det);
        const double rmsDb = 10.0 * std::log10 (meanSquare + 1.0e-30);
        maxRmsDb = juce::jmax (maxRmsDb, (float) rmsDb);

        fastMeanSq += fastCoeff * (det * det - fastMeanSq);
        const double fastDb = 10.0 * std::log10 (fastMeanSq + 1.0e-30);
        maxFastDb = juce::jmax (maxFastDb, (float) fastDb);
        const bool falling = slowDbLag - rmsDb > kHystFallEpsDb;
        slowDbLag += hystLagCoeff * (rmsDb - slowDbLag);

        // ---- Broad (unfiltered sidechain) follower — the veto reference ----
        const double broad = detectorBroad[i];
        broadMeanSq += fastCoeff * (broad * broad - broadMeanSq);

        const bool forced = forceOpen != nullptr && forceOpen[i] != 0;

        // ---- Contrast (Selectivity) veto: a hit may only OPEN the gate if
        // the focused band dominates the OFF-BAND remainder of the mic
        // (energy subtraction of the two same-speed followers, so no extra
        // filters). Bleed from another drum is loud at ITS frequency, so
        // the off-band level towers over the band — regardless of absolute
        // level — while a soft on-target hit is quiet everywhere BUT the
        // band. BOTH follower inputs are peak-held (instant attack, ~12 ms
        // release) and subtracted AFTER the hold: for a pure in-band hit the
        // two peaks match, so the trigger filter's own ring-up (band lags
        // broad by ~2Q/omega) collapses to the floor instead of reading as
        // off-band energy that vetoes soft on-target hits — while ripple on
        // low-frequency bleed still can't blink the veto off mid-event
        // (both holds ride their ripple tops together).
        //
        // The decision then LATCHES per event: hard off-drum hits grow
        // in-band content late (sympathetic snare buzz, 2nd harmonics), so
        // an event that STARTS off-band stays vetoed until it either fades
        // or the band convincingly takes over (a real hit landing on top).
        // At the knob's maximum the veto is off.
        broadPkSq = broadMeanSq > broadPkSq
                        ? broadMeanSq
                        : broadPkSq + pkRelCoeff * (broadMeanSq - broadPkSq);
        fastPkSq = fastMeanSq > fastPkSq
                       ? fastMeanSq
                       : fastPkSq + pkRelCoeff * (fastMeanSq - fastPkSq);
        const double offSq = juce::jmax (broadPkSq - fastPkSq, broadPkSq * 0.001);
        const double offDb = 10.0 * std::log10 (offSq + 1.0e-30);
        bool contrastOk = true;
        if (contrastDb < 23.75)
        {
            const double excess = offDb - fastDb;
            if (! vetoLatch)
                vetoLatch = excess > contrastDb && offDb > thresholdDb - 6.0;
            else if (excess < contrastDb - 6.0 || offDb < thresholdDb - 12.0)
                vetoLatch = false;
            contrastOk = ! vetoLatch && excess <= contrastDb;
        }
        else
            vetoLatch = false;

        // ---- Gate state: open fast, close slow with hysteresis, hard shut.
        // A MIDI note (forceOpen) is authoritative: no threshold, no veto.
        const bool levelQualifies = juce::jmax (fastDb, rmsDb) > thresholdDb;
        if (! forced && levelQualifies && ! contrastOk)
            ++vetoBlockedSamples;                       // veto is eating head start
        else if (! levelQualifies)
            vetoBlockedSamples = 0;
        if (forced || (contrastOk && levelQualifies))
        {
            if (! gateOpen && ! forced && vetoBlockedSamples > 0)
                bypassRemaining = marginSamples;        // veto-late open: apply NOW
            vetoBlockedSamples = 0;
            gateOpen = true;
            holdCounter = holdSamples;
        }
        else if (gateOpen)
        {
            if (rmsDb > thresholdDb - hysteresisDb && falling)
                holdCounter = holdSamples;  // sustain through the hit's decay
            else if (holdCounter > 0)
                --holdCounter;
            else
                gateOpen = false;
        }

        // ---- Compressor stage: the duck, keyed by the GATED sidechain.
        // While the gate is open the comp RMS hears the filtered detector;
        // shut, it drains to silence within its window and the duck
        // releases — so nothing that isn't an event can ever push the copy
        // down, and everything that is fades out at the comp Release rate.
        const double compMean = compRms.pushMeanSquare (gateOpen ? det : 0.0);
        const double compDb = 10.0 * std::log10 (compMean + 1.0e-30);
        const double overNow = compDb - compThresholdDb;
        const double targetDb = overNow > 0.0
                                    ? juce::jmax (reductionDb, -compGrPerDb * overNow)
                                    : 0.0;
        const bool openNow = overNow > 0.0;

        // ---- Smooth the envelope (attack towards reduction, release back to 0 dB) ----
        if (applyGain)
        {
            const double coeff = (targetDb < currentGainDb) ? compAttackCoeff : compReleaseCoeff;
            currentGainDb += coeff * (targetDb - currentGainDb);
        }
        else
        {
            // Trigger bypassed: the duck is pinned fully engaged — the
            // parallel copy is fully ducked, so no bleed is removed and the
            // dry signal passes untouched.
            currentGainDb = reductionDb;
        }

        // ---- EQ-gate envelope: engage (within lookahead), hold, release.
        // Keys on the comp threshold — the tail's hold starts counting the
        // moment the gated sidechain stops qualifying.
        if (! applyGain)
        {
            eqGateEnv = 1.0;               // trigger bypassed: tail rings too
        }
        else if (openNow)
        {
            // Partial engagement scaled by how far past the comp threshold
            // the event peaks (Tail Range / Tail Base); the env holds the
            // event's maximum.
            double engage = 1.0;
            if (tailRangeDb > 0.05)
                engage = juce::jlimit (0.0, 1.0,
                                       tailBase01 + (1.0 - tailBase01) * (overNow / tailRangeDb));
            eqGateEnv = juce::jmax (eqGateEnv, engage);
            eqGateHoldCounter = eqGateHoldSamples;
        }
        else if (eqGateHoldCounter > 0)
        {
            // Hold at the level the event REACHED — pinning 1.0 here would
            // undo the partial engagement the moment the signal dropped
            // below the threshold (the bug behind "the TAIL meter shoots to
            // 100% no matter what Tail range/base are set to").
            --eqGateHoldCounter;
        }
        else
        {
            eqGateEnv += eqGateReleaseCoeff * (0.0 - eqGateEnv);
        }

        // ---- Envelope-application margin: gain AND tail env are applied
        // marginSamples late (audio is delayed lookahead + margin, so
        // decisions still lead the audio by exactly the lookahead); a
        // veto-late open bypasses the ring for one window — applied fresh,
        // it recovers the head start the veto's settle time consumed.
        double applyDb = currentGainDb;
        double applyEnv = eqGateEnv;
        if (marginSamples > 0)
        {
            const double gOld = gainRing[(size_t) ringIdx];
            const double eOld = envRing[(size_t) ringIdx];
            gainRing[(size_t) ringIdx] = currentGainDb;
            envRing[(size_t) ringIdx] = eqGateEnv;
            if (++ringIdx >= marginSamples)
                ringIdx = 0;
            if (bypassRemaining > 0)
                --bypassRemaining;
            else
            {
                applyDb = gOld;
                applyEnv = eOld;
            }
        }
        const double gain = std::pow (10.0, applyDb / 20.0);
        minGainDb = juce::jmin (minGainDb, (float) currentGainDb);
        if (eqGateEnvOut != nullptr)
            eqGateEnvOut[i] = applyEnv;

        // ---- Delay the audio by the lookahead and apply the gain ----
        for (int ch = 0; ch < numChannels; ++ch)
        {
            double* data = audio.getWritePointer (ch);
            data[i] = delays[(size_t) ch].processSample (data[i]) * gain;
        }
    }

    lastBlockGrDb = minGainDb;
    lastBlockRmsDb = maxRmsDb;
    lastBlockFastDb = maxFastDb;
}

} // namespace mdd
