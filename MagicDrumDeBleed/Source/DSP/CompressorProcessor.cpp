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

    // Soft-knee span below the threshold: the fast detector's lead over the
    // slow one pre-opens the gate proportionally inside this zone.
    constexpr double kKneeDb = 6.0;

    // The knee only engages when the fast detector leads the slow one by this
    // much. Real onsets lead by 7 dB+; the fast detector's ripple on steady
    // low-frequency bleed stays under it, so bleed in the knee cannot leak.
    constexpr double kKneeLeadDb = 4.0;
}

void CompressorProcessor::prepare (double sampleRate, int numChannels, int maxLookaheadSamples)
{
    sr = sampleRate;

    delays.resize ((size_t) juce::jmax (1, numChannels));
    for (auto& d : delays)
        d.prepare (maxLookaheadSamples);

    // Worst-case RMS window is 100 ms.
    rmsBuffer.assign ((size_t) juce::jmax (1, (int) std::ceil (0.1 * sr) + 1), 0.0);
    currentRmsWindowMs = -1.0;
    setRmsWindow (10.0);
    hystLagCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, kHystLagSeconds * sr));
    broadPkRelCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, 0.012 * sr));

    reset();
}

void CompressorProcessor::reset()
{
    for (auto& d : delays)
        d.reset();

    std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0);
    rmsSum = 0.0;
    rmsIndex = 0;
    rmsRefreshCounter = 0;

    fastMeanSq = 0.0;
    broadMeanSq = 0.0;
    broadPkSq = 0.0;
    slowDbLag = -120.0;
    currentGainDb = 0.0;
    holdCounter = 0;
    gateOpen = false;
    eqGateEnv = 0.0;
    eqGateHoldCounter = 0;
    lastBlockGrDb = 0.0f;
}

void CompressorProcessor::setEqGateParameters (double holdMs, double releaseMs)
{
    eqGateHoldSamples = (int) std::lround (holdMs * 0.001 * sr);
    // Coefficient chosen so the envelope falls ~60 dB (to 0.001) within the
    // release time — "fully cancels" on the user's timescale.
    const double relSamples = juce::jmax (1.0, releaseMs * 0.001 * sr);
    eqGateReleaseCoeff = 1.0 - std::exp (-6.9078 / relSamples);
}

void CompressorProcessor::setRmsWindow (double windowMs)
{
    if (exactlyEqual (windowMs, currentRmsWindowMs))
        return;

    currentRmsWindowMs = windowMs;
    const int newLength = juce::jlimit (1, (int) rmsBuffer.size(),
                                        (int) std::lround (windowMs * 0.001 * sr));
    if (newLength != rmsLength)
    {
        rmsLength = newLength;
        // Window length changed: clear and start accumulating fresh. A brief
        // detector dip on a manual parameter tweak is inaudible.
        std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0);
        rmsSum = 0.0;
        rmsIndex = 0;
        rmsRefreshCounter = 0;
    }
}

void CompressorProcessor::rebuildRmsSum()
{
    // Periodically recompute the running sum exactly to stop floating-point
    // drift from accumulating over hours of processing.
    double sum = 0.0;
    for (int i = 0; i < rmsLength; ++i)
        sum += rmsBuffer[(size_t) i];
    rmsSum = sum;
}

void CompressorProcessor::setParameters (double newThresholdDb, double newReductionDb,
                                         int newLookaheadSamples, double rmsWindowMs,
                                         double holdMs, double releaseMs,
                                         double attackMs, double newHysteresisDb, double newContrastDb)
{
    thresholdDb = newThresholdDb;
    reductionDb = newReductionDb;
    hysteresisDb = newHysteresisDb;
    contrastDb = newContrastDb;

    lookaheadSamples = juce::jmax (0, newLookaheadSamples);
    for (auto& d : delays)
        d.setDelay (lookaheadSamples);

    setRmsWindow (rmsWindowMs);

    // Fast opening detector — the Attack knob sets its time constant.
    const double fastTauSamples = juce::jmax (1.0, juce::jlimit (0.1, 20.0, attackMs) * 0.001 * sr);
    fastCoeff = 1.0 - std::exp (-1.0 / fastTauSamples);

    holdSamples = (int) std::lround (holdMs * 0.001 * sr);

    // Attack: exponential ramp fast enough to hit the reduction target within
    // the lookahead window (5 time constants ≈ full settle).
    const double attackTauSamples = juce::jmax (1.0, (double) lookaheadSamples / 5.0);
    attackCoeff = 1.0 - std::exp (-1.0 / attackTauSamples);

    const double releaseTauSamples = juce::jmax (1.0, releaseMs * 0.001 * sr);
    releaseCoeff = 1.0 - std::exp (-1.0 / releaseTauSamples);
}

void CompressorProcessor::process (juce::AudioBuffer<double>& audio, const double* detector,
                                   const double* detectorBroad,
                                   int numSamples, bool applyGain, double* eqGateEnvOut)
{
    const int numChannels = juce::jmin (audio.getNumChannels(), (int) delays.size());
    if (detectorBroad == nullptr)
        detectorBroad = detector;
    float minGainDb = 0.0f;
    float maxRmsDb = -120.0f;
    float maxFastDb = -120.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- RMS detector (un-delayed sidechain) ----
        const double det = detector[i];
        rmsSum += det * det - rmsBuffer[(size_t) rmsIndex];
        rmsBuffer[(size_t) rmsIndex] = det * det;
        if (++rmsIndex >= rmsLength)
            rmsIndex = 0;
        if (++rmsRefreshCounter >= rmsLength)
        {
            rmsRefreshCounter = 0;
            rebuildRmsSum();
        }

        const double meanSquare = juce::jmax (0.0, rmsSum) / (double) rmsLength;
        const double rmsDb = 10.0 * std::log10 (meanSquare + 1.0e-30);
        maxRmsDb = juce::jmax (maxRmsDb, (float) rmsDb);

        fastMeanSq += fastCoeff * (det * det - fastMeanSq);
        const double fastDb = 10.0 * std::log10 (fastMeanSq + 1.0e-30);
        maxFastDb = juce::jmax (maxFastDb, (float) fastDb);
        const bool falling = slowDbLag - rmsDb > kHystFallEpsDb;
        slowDbLag += hystLagCoeff * (rmsDb - slowDbLag);

        // ---- Contrast (Selectivity) veto: a hit may only OPEN the gate if
        // the focused band holds enough of the whole mic's energy. Bleed
        // from another drum is loud at ITS frequency, so the unfiltered
        // level towers over the band and the veto blocks it — regardless of
        // absolute level. A soft on-target hit is quiet everywhere BUT the
        // band, so it passes. At the knob's maximum the veto is off.
        // The broadband reference is peak-held (instant attack, ~12 ms
        // release): fast followers ripple several dB on low-frequency
        // content, and a momentary downward ripple of the reference must
        // not blink the veto off mid-bleed.
        const double broad = detectorBroad[i];
        broadMeanSq += fastCoeff * (broad * broad - broadMeanSq);
        broadPkSq = broadMeanSq > broadPkSq
                        ? broadMeanSq
                        : broadPkSq + broadPkRelCoeff * (broadMeanSq - broadPkSq);
        const bool contrastOk = contrastDb >= 23.75
            || 10.0 * std::log10 (broadPkSq + 1.0e-30) - fastDb <= contrastDb;

        // ---- Gate state: open fast, close slow with hysteresis ----
        if (contrastOk && juce::jmax (fastDb, rmsDb) > thresholdDb)
        {
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

        // ---- Soft knee: transient pre-open inside the 6 dB below threshold.
        // Uses the fast detector's lead over the slow one, so it ramps the
        // gate open during an attack's rise but is zero for steady bleed.
        double open01 = gateOpen ? 1.0 : 0.0;
        if (! gateOpen && contrastOk && fastDb > thresholdDb - kKneeDb && fastDb > rmsDb + kKneeLeadDb)
        {
            auto knee01 = [this] (double db)
            { return juce::jlimit (0.0, 1.0, (db - (thresholdDb - kKneeDb)) / kKneeDb); };
            open01 = juce::jmax (0.0, knee01 (fastDb) - knee01 (rmsDb));
        }
        const double targetDb = reductionDb * open01;

        // ---- Smooth the envelope (attack towards reduction, release back to 0 dB) ----
        if (applyGain)
        {
            const double coeff = (targetDb < currentGainDb) ? attackCoeff : releaseCoeff;
            currentGainDb += coeff * (targetDb - currentGainDb);
        }
        else
        {
            // Trigger bypassed: the gate is forced permanently OPEN — the
            // parallel copy is fully ducked, so no bleed is removed and the
            // dry signal passes untouched.
            currentGainDb = reductionDb;
        }

        const double gain = std::pow (10.0, currentGainDb / 20.0);
        minGainDb = juce::jmin (minGainDb, (float) currentGainDb);

        // ---- EQ-gate envelope: instant engage (within lookahead), hold, release.
        // Follows the same open state as the gate (fast open, hysteresis close),
        // so the tail's hold starts counting when the gate actually shuts.
        if (eqGateEnvOut != nullptr)
        {
            if (! applyGain)
            {
                eqGateEnv = 1.0;           // trigger bypassed: tail rings too
            }
            else if (gateOpen)
            {
                eqGateEnv = 1.0;               // instant, full engagement
                eqGateHoldCounter = eqGateHoldSamples;
            }
            else if (eqGateHoldCounter > 0)
            {
                --eqGateHoldCounter;
                eqGateEnv = 1.0;
            }
            else
            {
                eqGateEnv += eqGateReleaseCoeff * (0.0 - eqGateEnv);
            }
            eqGateEnvOut[i] = eqGateEnv;
        }

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
