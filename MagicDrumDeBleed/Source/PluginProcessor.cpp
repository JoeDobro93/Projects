#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Genuine log mapping (not a power skew): every part of the knob moves
    // the value by the same *ratio*, so the bottom of the range never turns
    // into a stretch where the knob turns but nothing visibly changes.
    juce::NormalisableRange<float> logHzRange (float minHz, float maxHz)
    {
        return { minHz, maxHz,
            [] (float s, float e, float n) { return s * std::pow (e / s, n); },
            [] (float s, float e, float v) { return std::log (v / s) / std::log (e / s); } };
    }

    // Two-segment log range keeping a chosen value at 12 o'clock.
    // wholeNumbers snaps every produced value to integers (times in ms).
    juce::NormalisableRange<float> logRange (float min, float centre, float max,
                                             bool wholeNumbers = false)
    {
        using Remap = juce::NormalisableRange<float>::ValueRemapFunction;
        return { min, max,
            [centre] (float s, float e, float n)
            {
                return n < 0.5f ? s * std::pow (centre / s, n * 2.0f)
                                : centre * std::pow (e / centre, n * 2.0f - 1.0f);
            },
            [centre] (float s, float e, float v)
            {
                return v < centre ? 0.5f * std::log (v / s) / std::log (centre / s)
                                  : 0.5f + 0.5f * std::log (v / centre) / std::log (e / centre);
            },
            wholeNumbers ? Remap ([] (float s, float e, float v)
                                  { return juce::jlimit (s, e, (float) juce::roundToInt (v)); })
                         : Remap() };
    }

    constexpr float kNotchDefaultFreqs[5] = { 200.0f, 200.0f, 400.0f, 800.0f, 1600.0f };   // K1 matches Focus

    // The gate always ducks the parallel copy to silence when open; a
    // shallower duck would leave bleed-removal partially active during hits.
    constexpr double kReductionDb = -96.0;
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout MagicDrumDeBleedAudioProcessor::createParameterLayout()
{
    using juce::AudioParameterFloat;
    using juce::AudioParameterInt;
    using juce::AudioParameterBool;
    using juce::AudioParameterChoice;
    using juce::ParameterID;

    // Value text includes the unit (e.g. "-24.0 dB", "1.20 kHz") so the knob
    // read-outs and host parameter displays are self-describing.
    const auto dB = juce::AudioParameterFloatAttributes()
        .withLabel ("dB")
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " dB"; });

    const auto ms = juce::AudioParameterFloatAttributes()
        .withLabel ("ms")
        .withStringFromValueFunction ([] (float v, int)
        {   // String(v, 0) would print the raw float — round explicitly
            return (v < 10.0f ? juce::String (v, 1) : juce::String (juce::roundToInt (v))) + " ms";
        });

    const auto msInt = juce::AudioParameterFloatAttributes()
        .withLabel ("ms")
        .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v)) + " ms"; });

    const auto hz = juce::AudioParameterFloatAttributes()
        .withLabel ("Hz")
        .withStringFromValueFunction ([] (float v, int)
        {
            return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " kHz"
                                : juce::String (v, 1) + " Hz";
        })
        .withValueFromStringFunction ([] (const juce::String& text)
        {
            const float f = text.getFloatValue();
            return text.containsIgnoreCase ("k") ? f * 1000.0f : f;
        });

    const auto pct = juce::AudioParameterFloatAttributes()
        .withLabel ("%")
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1) + " %"; });

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // ---- Gate stage (the event detector; hard close, no gain of its own) ----
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                    juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -40.0f, dB));
    p.push_back (std::make_unique<AudioParameterInt>   (ParameterID { ParamIDs::lookahead, 1 }, "Lookahead",
                    0, 10, 5, juce::AudioParameterIntAttributes()
                                .withLabel ("ms")
                                .withStringFromValueFunction ([] (int v, int) { return juce::String (v) + " ms"; })));
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::rmsWindow, 1 }, "RMS Window",
                        logRange (1.0f, 10.0f, 100.0f), 10.0f, ms));
    }
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hold, 1 }, "Hold",
                        logRange (5.0f, 40.0f, 200.0f, true), 7.0f, msInt));
    }
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hysteresis, 1 }, "Hysteresis",
                    juce::NormalisableRange<float> (0.0f, 24.0f), 8.0f, dB));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::midiTrigger, 1 }, "MIDI Trigger", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::contrast, 1 }, "Selectivity",
                    juce::NormalisableRange<float> (0.0f, 24.0f), 24.0f,
                    juce::AudioParameterFloatAttributes()
                        .withLabel ("dB")
                        .withStringFromValueFunction ([] (float v, int)
                        { return v >= 23.75f ? juce::String ("Off") : juce::String (v, 1) + " dB"; })));

    // ---- Compressor stage (the duck, keyed by the gated sidechain) ----
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::compThreshold, 1 }, "Comp Threshold",
                    juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -52.0f, dB));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::compRatio, 1 }, "Ratio",
                    juce::StringArray { "4:1", "10:1", "20:1", "100:1", "-1:1", "-2:1", "Full" }, 3));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::compAttack, 1 }, "Comp Attack",
                    logRange (0.1f, 2.0f, 30.0f), 0.1f, ms));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::compRelease, 1 }, "Comp Release",
                    logRange (5.0f, 100.0f, 1000.0f, true), 10.0f, msInt));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::compRmsWindow, 1 }, "Comp RMS Window",
                    logRange (1.0f, 10.0f, 100.0f), 10.0f, ms));

    // ---- Sidechain ----
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::scEnable, 1 }, "SC Filter", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::scFreq, 1 }, "SC Frequency",
                    logHzRange (30.0f, 2000.0f), 200.0f, hz));
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::scQ, 1 }, "SC Q",
                        logRange (0.3f, 1.9f, 12.0f), 2.871f,   // 0.5 oct
                        juce::AudioParameterFloatAttributes()
                            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })));
    }
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scType, 1 }, "Trigger Filter Type",
                    juce::StringArray { "High Pass", "Low Pass", "Bandpass" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::scSlope, 1 }, "Trigger Filter Slope",
                    juce::NormalisableRange<float> (6.0f, 24.0f, 6.0f), 12.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("dB/oct")
                        .withStringFromValueFunction ([] (float v, int) { return juce::String ((int) v) + " dB/oct"; })));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::scExternal, 1 }, "External Sidechain", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::learnCeiling, 1 }, "Learn Ceiling",
                    logHzRange (200.0f, 2000.0f), 1000.0f, hz));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::linkK1, 1 }, "Link to K1", true));

    // ---- EQ: HPF / LPF ----
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::hpfOn, 1 }, "HPF On", false));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::hpfFreq, 1 }, "HPF Freq",
                    logHzRange (20.0f, 15000.0f), 800.0f, hz));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::hpfSlope, 1 }, "HPF Slope",
                    juce::StringArray { "6 dB/oct", "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, 1));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::lpfOn, 1 }, "LPF On", false));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::lpfFreq, 1 }, "LPF Freq",
                    logHzRange (50.0f, 20000.0f), 20000.0f, hz));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lpfSlope, 1 }, "LPF Slope",
                    juce::StringArray { "6 dB/oct", "12 dB/oct", "24 dB/oct", "36 dB/oct", "48 dB/oct" }, 1));

    // ---- EQ: notch bands 1..5 ----
    for (int i = 0; i < 5; ++i)
    {
        const juce::String num (i + 1);
        p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::notchOn (i), 1 },
                        "Notch " + num + " On", i == 0));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::notchFreq (i), 1 },
                        "Notch " + num + " Freq", logHzRange (20.0f, 20000.0f), kNotchDefaultFreqs[i], hz));
        {
            p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::notchQ (i), 1 },
                            "Notch " + num + " Q", logRange (0.1f, 1.0f, 40.0f), 1.0f,
                            juce::AudioParameterFloatAttributes()
                                .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 2); })));
        }
        p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::notchGain (i), 1 },
                        "Notch " + num + " Ring", juce::NormalisableRange<float> (0.0f, 20.0f), 9.8f,
                        juce::AudioParameterFloatAttributes()
                            .withStringFromValueFunction ([] (float v, int) { return juce::String (v, 1); })));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::notchShape (i), 1 },
                        "Notch " + num + " Shape", juce::StringArray { "Bell", "Proportional Q", "Band Shelf" }, 1));
    }

    // ---- EQ gate ----
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::eqGateOn, 1 }, "EQ Gate", true));
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::eqGateHold, 1 }, "EQ Gate Hold",
                        logRange (1.0f, 150.0f, 2000.0f, true), 120.0f, msInt));
    }
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::eqGateRelease, 1 }, "EQ Gate Release",
                        logRange (5.0f, 800.0f, 5000.0f, true), 100.0f, msInt));
    }
    // Comp-mode tail blend: hits between T and T+range engage the tail
    // partially (base% at T ramping to 100% at T+range) — the rumble fix.
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::tailRange, 1 }, "Tail Range",
                    juce::NormalisableRange<float> (0.0f, 12.0f), 3.0f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::tailBase, 1 }, "Tail Base",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, pct));

    // ---- Output / monitoring ----
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::intensity, 1 }, "Intensity",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::monitorMode, 1 }, "Monitor",
                    juce::StringArray { "Normal", "Sidechain", "Processing", "Delta" }, 0));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::compBypass, 1 }, "Gate Bypass", false));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::eqBypass, 1 }, "EQ Bypass", false));

    return { p.begin(), p.end() };
}

//==============================================================================
MagicDrumDeBleedAudioProcessor::MagicDrumDeBleedAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                        .withInput  ("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    auto raw = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };

    pThreshold    = raw (ParamIDs::threshold);
    pLookahead    = raw (ParamIDs::lookahead);
    pRmsWindow    = raw (ParamIDs::rmsWindow);
    pHold         = raw (ParamIDs::hold);
    pHysteresis   = raw (ParamIDs::hysteresis);
    pContrast     = raw (ParamIDs::contrast);
    pMidiTrigger  = raw (ParamIDs::midiTrigger);
    pCompThreshold = raw (ParamIDs::compThreshold);
    pCompRatio    = raw (ParamIDs::compRatio);
    pCompAttack   = raw (ParamIDs::compAttack);
    pCompRelease  = raw (ParamIDs::compRelease);
    pCompRmsWindow = raw (ParamIDs::compRmsWindow);
    pScEnable     = raw (ParamIDs::scEnable);
    pScFreq       = raw (ParamIDs::scFreq);
    pScQ          = raw (ParamIDs::scQ);
    pScType       = raw (ParamIDs::scType);
    pScSlope      = raw (ParamIDs::scSlope);
    pScExternal   = raw (ParamIDs::scExternal);
    pLearnCeiling = raw (ParamIDs::learnCeiling);
    pHpfOn        = raw (ParamIDs::hpfOn);
    pHpfFreq      = raw (ParamIDs::hpfFreq);
    pHpfSlope     = raw (ParamIDs::hpfSlope);
    pLpfOn        = raw (ParamIDs::lpfOn);
    pLpfFreq      = raw (ParamIDs::lpfFreq);
    pLpfSlope     = raw (ParamIDs::lpfSlope);

    for (int i = 0; i < 5; ++i)
    {
        pNotchOn[i]    = raw (ParamIDs::notchOn (i));
        pNotchFreq[i]  = raw (ParamIDs::notchFreq (i));
        pNotchQ[i]     = raw (ParamIDs::notchQ (i));
        pNotchGain[i]  = raw (ParamIDs::notchGain (i));
        pNotchShape[i] = raw (ParamIDs::notchShape (i));
    }

    pEqGateOn      = raw (ParamIDs::eqGateOn);
    pEqGateHold    = raw (ParamIDs::eqGateHold);
    pEqGateRelease = raw (ParamIDs::eqGateRelease);
    pTailRange     = raw (ParamIDs::tailRange);
    pTailBase      = raw (ParamIDs::tailBase);

    pIntensity   = raw (ParamIDs::intensity);
    pMonitorMode = raw (ParamIDs::monitorMode);
    pCompBypass  = raw (ParamIDs::compBypass);
    pEqBypass    = raw (ParamIDs::eqBypass);

    spectrumFifoBuffer.resize (kSpectrumFifoSize, 0.0f);

    apvts.addParameterListener (ParamIDs::linkK1, this);
    apvts.addParameterListener (ParamIDs::scFreq, this);
    apvts.addParameterListener (ParamIDs::notchFreq (0), this);
}

MagicDrumDeBleedAudioProcessor::~MagicDrumDeBleedAudioProcessor()
{
    apvts.removeParameterListener (ParamIDs::linkK1, this);
    apvts.removeParameterListener (ParamIDs::scFreq, this);
    apvts.removeParameterListener (ParamIDs::notchFreq (0), this);
}

void MagicDrumDeBleedAudioProcessor::parameterChanged (const juce::String& id, float newValue)
{
    auto setReal = [this] (const juce::String& pid, float real)
    {
        if (auto* p = apvts.getParameter (pid))
            p->setValueNotifyingHost (p->convertTo0to1 (real));
    };

    // While the external sidechain drives the trigger, Focus and K1 live in
    // different signal domains (key vs this track) — the link is suspended:
    // the toggle stays as set, but neither snap nor mirror runs.
    if (pScExternal->load() > 0.5f)
        return;

    if (id == ParamIDs::linkK1)
    {
        // Turning the link on snaps K1 onto the current Focus frequency.
        if (newValue > 0.5f && ! linkSyncing.exchange (true))
        {
            setReal (ParamIDs::notchFreq (0), pScFreq->load());
            linkSyncing.store (false);
        }
        return;
    }

    // Mirror Focus ↔ K1 frequency. The flag breaks the notification loop:
    // the mirrored set re-enters this callback synchronously.
    if (apvts.getRawParameterValue (ParamIDs::linkK1)->load() < 0.5f || linkSyncing.exchange (true))
        return;
    setReal (id == ParamIDs::scFreq ? ParamIDs::notchFreq (0) : juce::String (ParamIDs::scFreq), newValue);
    linkSyncing.store (false);
}

bool MagicDrumDeBleedAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();

    if (in != out)
        return false;
    if (in != juce::AudioChannelSet::mono() && in != juce::AudioChannelSet::stereo())
        return false;

    // Key input: mono, stereo, or off (host's choice).
    if (layouts.inputBuses.size() > 1)
    {
        const auto& sc = layouts.getChannelSet (true, 1);
        if (! sc.isDisabled() && sc != juce::AudioChannelSet::mono() && sc != juce::AudioChannelSet::stereo())
            return false;
    }
    return true;
}

//==============================================================================
void MagicDrumDeBleedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateCached = sampleRate;
    maxLookaheadSamples = (int) std::lround (kMaxLookaheadMs * 0.001 * sampleRate);
    baseDelaySamples = maxLookaheadSamples;
    marginSamples = (int) std::lround (kEnvMarginMs * 0.001 * sampleRate);
    currentTotalDelay = -1;

    const int numCh = juce::jlimit (1, 2, getMainBusNumInputChannels());

    compressor.prepare (sampleRate, numCh, baseDelaySamples + marginSamples,
                        baseDelaySamples + marginSamples);
    eq.prepare (sampleRate, numCh);
    scFilter.prepare (sampleRate);
    learnAnalyzer.prepare (sampleRate);
    scLearnAnalyzer.prepare (sampleRate);

    dryDelays.resize ((size_t) numCh);
    for (auto& d : dryDelays)
        d.prepare (baseDelaySamples + marginSamples);

    const int block = juce::jmax (16, samplesPerBlock);
    conversionBuffer.setSize (numCh, block);
    parallelBuffer.setSize (numCh, block);
    dryBuffer.setSize (numCh, block);
    preEqBuffer.setSize (numCh, block);
    detectorRaw.assign ((size_t) block, 0.0);
    detectorFiltered.assign ((size_t) block, 0.0);
    scRaw.assign ((size_t) block, 0.0);
    scActiveBlock = false;
    extScActive.store (false);
    forceMask.assign ((size_t) block, 0);
    std::fill (std::begin (heldKeys), std::end (heldKeys), false);
    heldCount = 0;
    eqGateEnvBuffer.assign ((size_t) block, 0.0);

    // Dry display follower: ~1.7 ms one-pole, matching the detector traces'
    // default responsiveness.
    dryDispMeanSq = 0.0;
    dryDispCoeff = 1.0 - std::exp (-1.0 / juce::jmax (1.0, 0.00167 * sampleRate));

    intensitySmoothed.reset (sampleRate, 0.05);
    intensitySmoothed.setCurrentAndTargetValue (pIntensity->load() * 0.01);

    spectrumFifo.reset();

    for (auto& st : soloStages)
        for (auto& f : st)
            f.reset();
    soloCachedBand = -2;

    currentLookaheadSamples = -1;   // force latency + delay refresh
    updateParametersForBlock();
}

void MagicDrumDeBleedAudioProcessor::updateParametersForBlock()
{
    // ---- Lookahead / total delay. The knob never changes latency (the
    // ring absorbs total − lookahead); the ONLY thing that does is engaging
    // Selectivity, which adds the flam-recovery margin. ----
    const bool selectivityOn = pContrast->load() < 23.75f;
    const int lookaheadMs = (int) pLookahead->load();
    const int lookahead = juce::jlimit (0, maxLookaheadSamples,
                                        (int) std::lround (lookaheadMs * 0.001 * sampleRateCached));
    const int totalDelay = baseDelaySamples + (selectivityOn ? marginSamples : 0);
    if (totalDelay != currentTotalDelay)
    {
        currentTotalDelay = totalDelay;
        for (auto& d : dryDelays)
            d.setDelay (totalDelay);
        setLatencySamples (totalDelay);
    }
    currentLookaheadSamples = lookahead;

    // ---- Gate + compressor stages / sidechain ----
    compressor.setParameters (pThreshold->load(), kReductionDb, lookahead,
                              totalDelay - lookahead,
                              pRmsWindow->load(), pHold->load(),
                              pHysteresis->load(), pContrast->load());
    compressor.setEqGateParameters (pEqGateHold->load(), pEqGateRelease->load(),
                                    pTailRange->load(), pTailBase->load() * 0.01f);
    {
        // GR per dB over the comp threshold: standard ratios reduce by 1-1/R
        // (copy squeezed toward the threshold); the negative ratios -N:1
        // reduce by 1+N, pushing the copy N dB BELOW the threshold per dB
        // over; Full is the binary-gate law (any amount over = full duck).
        static constexpr double kGrPerDb[7] = { 0.75, 0.90, 0.95, 0.99, 2.0, 3.0,
                                                mdd::CompressorProcessor::kFullRatioSlope };
        compressor.setCompParameters (pCompThreshold->load(),
                                      kGrPerDb[juce::jlimit (0, 6, (int) pCompRatio->load())],
                                      pCompRmsWindow->load(),
                                      pCompAttack->load(), pCompRelease->load());
    }
    eqGateOnCached = pEqGateOn->load() > 0.5f;
    scFilter.setParameters ((int) pScType->load(), pScFreq->load(), pScQ->load(), pScSlope->load());
    scEnabledCached = pScEnable->load() > 0.5f;

    // ---- EQ bands ----
    mdd::BandParams bp;

    bp.enabled = pHpfOn->load() > 0.5f;
    bp.freqHz  = pHpfFreq->load();
    bp.slope   = (int) pHpfSlope->load();
    bp.q = 0.70710678118654752; bp.gainDb = 0.0; bp.shape = 0;
    eq.setBandParameters (mdd::EQProcessor::kBandHPF, bp);

    bp.enabled = pLpfOn->load() > 0.5f;
    bp.freqHz  = pLpfFreq->load();
    bp.slope   = (int) pLpfSlope->load();
    eq.setBandParameters (mdd::EQProcessor::kBandLPF, bp);

    for (int i = 0; i < 5; ++i)
    {
        mdd::BandParams nb;
        nb.enabled = pNotchOn[i]->load() > 0.5f;
        nb.freqHz  = pNotchFreq[i]->load();
        nb.q       = pNotchQ[i]->load();
        nb.gainDb  = mdd::ringToGainDb (pNotchGain[i]->load());
        nb.shape   = (int) pNotchShape[i]->load();
        nb.slope   = 0;
        eq.setBandParameters (mdd::EQProcessor::kFirstNotch + i, nb);
    }

    // ---- Output / monitoring ----
    intensitySmoothed.setTargetValue (juce::jlimit (0.0, 1.0, (double) pIntensity->load() * 0.01));
    monitorModeCached = (int) pMonitorMode->load();
    compBypassCached  = pCompBypass->load() > 0.5f;
    eqBypassCached    = pEqBypass->load() > 0.5f;

    // ---- Band-solo: the band's own filter (kept ring = dry − band(dry)) ----
    soloBandCached = soloBand.load();
    if (soloBandCached >= 0)
    {
        mdd::BandParams sp;
        sp.enabled = true;
        if (soloBandCached == 0)      { sp.freqHz = pHpfFreq->load(); sp.slope = (int) pHpfSlope->load(); }
        else if (soloBandCached == 1) { sp.freqHz = pLpfFreq->load(); sp.slope = (int) pLpfSlope->load(); }
        else
        {
            const int nb = soloBandCached - 2;
            sp.freqHz = pNotchFreq[nb]->load();
            sp.q      = pNotchQ[nb]->load();
            sp.gainDb = mdd::ringToGainDb (pNotchGain[nb]->load());
            sp.shape  = (int) pNotchShape[nb]->load();
        }

        if (soloBandCached != soloCachedBand || sp != soloCachedParams)
        {
            const bool bandChanged = soloBandCached != soloCachedBand;
            soloCachedBand  = soloBandCached;
            soloCachedParams = sp;

            const int kind = soloBandCached == 0 ? 0 : (soloBandCached == 1 ? 1 : 2);
            mdd::BiquadFilter::Coeffs cs[mdd::EQProcessor::kMaxStages];
            soloNumStages = mdd::EQProcessor::computeCoefficients (sp, kind, sampleRateCached, cs);
            for (int st = 0; st < soloNumStages; ++st)
                for (auto& f : soloStages[st])
                {
                    f.setCoefficients (cs[st]);
                    if (bandChanged)
                        f.reset();
                }
        }
    }
    else
    {
        soloCachedBand = -2;   // force a refresh next time solo engages
    }
}

//==============================================================================
void MagicDrumDeBleedAudioProcessor::buildForceMask (const juce::MidiBuffer& midi, int n)
{
    // Per-sample mask of "a MIDI note is holding the gate open".
    //
    // Hosts drop matching note-offs when a routed MIDI track is muted, or on
    // seeks and loops — so held notes are a per-note SET (playing the same
    // note again cannot double-count, and its next note-off always clears
    // it), and any transport stop or backwards jump releases everything.
    if ((int) forceMask.size() < n)
        forceMask.resize ((size_t) n, 0);

    auto clearHeld = [this]
    {
        std::fill (std::begin (heldKeys), std::end (heldKeys), false);
        heldCount = 0;
    };

    if (auto* ph = getPlayHead())
    {
        if (const auto pos = ph->getPosition())
        {
            const bool playing = pos->getIsPlaying();
            const double ppq = pos->getPpqPosition().orFallback (lastPpq);
            if ((wasPlaying && ! playing) || (playing && ppq < lastPpq - 1.0e-6))
                clearHeld();
            wasPlaying = playing;
            lastPpq = ppq;
        }
    }

    if (pMidiTrigger->load() < 0.5f)
    {
        clearHeld();
        std::fill (forceMask.begin(), forceMask.begin() + n, (unsigned char) 0);
        midiForcedFlag.store (0.0f);
        return;
    }

    int idx = 0;
    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        const bool on  = msg.isNoteOn();
        const bool off = msg.isNoteOff() || msg.isAllNotesOff() || msg.isAllSoundOff();
        if (! on && ! off)
            continue;
        const int pos = juce::jlimit (idx, n, (int) meta.samplePosition);
        std::fill (forceMask.begin() + idx, forceMask.begin() + pos, (unsigned char) (heldCount > 0 ? 1 : 0));
        idx = pos;
        if (on)
        {
            const int note = msg.getNoteNumber();
            if (! heldKeys[note]) { heldKeys[note] = true; ++heldCount; }
        }
        else if (msg.isNoteOff())
        {
            const int note = msg.getNoteNumber();
            if (heldKeys[note]) { heldKeys[note] = false; heldCount = juce::jmax (0, heldCount - 1); }
        }
        else
            clearHeld();
    }
    std::fill (forceMask.begin() + idx, forceMask.begin() + n, (unsigned char) (heldCount > 0 ? 1 : 0));

    bool any = false;
    for (int i = 0; i < n && ! any; ++i)
        any = forceMask[(size_t) i] != 0;
    midiForcedFlag.store (any ? 1.0f : 0.0f);
}

template <typename SampleType>
void MagicDrumDeBleedAudioProcessor::extractSidechain (juce::AudioBuffer<SampleType>& buffer, int n)
{
    scActiveBlock = false;
    if (pScExternal->load() > 0.5f && getBusCount (true) > 1)
    {
        auto bus = getBusBuffer (buffer, true, 1);
        const int ch = bus.getNumChannels();
        // Guard against hosts passing fewer channels than the layout claims.
        if (ch > 0 && buffer.getNumChannels() >= getMainBusNumInputChannels() + ch)
        {
            if ((int) scRaw.size() < n)
                scRaw.resize ((size_t) n, 0.0);
            const double inv = 1.0 / (double) ch;
            for (int i = 0; i < n; ++i)
            {
                double sum = 0.0;
                for (int c = 0; c < ch; ++c)
                    sum += (double) bus.getReadPointer (c)[i];
                scRaw[(size_t) i] = sum * inv;
            }
            scActiveBlock = true;
        }
    }
    extScActive.store (scActiveBlock);
}

void MagicDrumDeBleedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n   = buffer.getNumSamples();
    const int nCh = juce::jmin (getMainBusNumInputChannels(), buffer.getNumChannels(),
                                conversionBuffer.getNumChannels());

    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (n == 0 || nCh == 0)
        return;

    if (conversionBuffer.getNumSamples() < n)
        conversionBuffer.setSize (conversionBuffer.getNumChannels(), n, false, false, true);

    for (int ch = 0; ch < nCh; ++ch)
    {
        const float* src = buffer.getReadPointer (ch);
        double* dst = conversionBuffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            dst[i] = (double) src[i];
    }

    buildForceMask (midi, n);
    extractSidechain (buffer, n);
    juce::AudioBuffer<double> view (conversionBuffer.getArrayOfWritePointers(), nCh, n);
    processInternal (view);

    for (int ch = 0; ch < nCh; ++ch)
    {
        const double* src = conversionBuffer.getReadPointer (ch);
        float* dst = buffer.getWritePointer (ch);
        for (int i = 0; i < n; ++i)
            dst[i] = (float) src[i];
    }
}

void MagicDrumDeBleedAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (n == 0)
        return;

    buildForceMask (midi, n);
    extractSidechain (buffer, n);
    juce::AudioBuffer<double> view (buffer.getArrayOfWritePointers(),
                                    juce::jmin (getMainBusNumInputChannels(), buffer.getNumChannels()), n);
    processInternal (view);
}

void MagicDrumDeBleedAudioProcessor::processInternal (juce::AudioBuffer<double>& buffer)
{
    const int n   = buffer.getNumSamples();
    const int nCh = juce::jmin (buffer.getNumChannels(),
                                parallelBuffer.getNumChannels(),
                                (int) dryDelays.size());
    if (nCh == 0)
        return;

    // Defensive: hosts occasionally exceed the prepared block size.
    if (parallelBuffer.getNumSamples() < n)
    {
        parallelBuffer.setSize (parallelBuffer.getNumChannels(), n, false, false, true);
        dryBuffer.setSize (dryBuffer.getNumChannels(), n, false, false, true);
        preEqBuffer.setSize (preEqBuffer.getNumChannels(), n, false, false, true);
        detectorRaw.resize ((size_t) n, 0.0);
        detectorFiltered.resize ((size_t) n, 0.0);
        eqGateEnvBuffer.resize ((size_t) n, 0.0);
    }

    updateParametersForBlock();

    // ---- 1. Detectors: mono mix of the un-delayed MAIN input, plus the
    // Dry display trace (the main level — what bypassed would sound like).
    // With the external sidechain active the trigger/veto detectors run on
    // the KEY (scRaw) instead; the main mix still feeds the Dry trace,
    // spectrum and keep-band Learn.
    const double invCh = 1.0 / (double) nCh;
    float dryDispMax = -120.0f;
    for (int i = 0; i < n; ++i)
    {
        double sum = 0.0;
        for (int ch = 0; ch < nCh; ++ch)
            sum += buffer.getReadPointer (ch)[i];
        const double mono = sum * invCh;
        detectorRaw[(size_t) i] = mono;
        dryDispMeanSq += dryDispCoeff * (mono * mono - dryDispMeanSq);
        dryDispMax = juce::jmax (dryDispMax, (float) (10.0 * std::log10 (dryDispMeanSq + 1.0e-30)));
    }
    dryDb.store (dryDispMax);

    const double* detectorKey = scActiveBlock ? scRaw.data() : detectorRaw.data();

    if (learnAnalyzer.isCapturing())
    {
        learnAnalyzer.pushSamples (detectorRaw.data(), n);
        if (scActiveBlock)
            scLearnAnalyzer.pushSamples (scRaw.data(), n);
    }

    for (int i = 0; i < n; ++i)
        detectorFiltered[(size_t) i] = scEnabledCached ? scFilter.processSample (detectorKey[i])
                                                       : detectorKey[i];

    // ---- 2. Parallel path: delay (lookahead) ▸ gate ▸ EQ ----
    for (int ch = 0; ch < nCh; ++ch)
        parallelBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    compressor.process (parallelBuffer, detectorFiltered.data(), detectorKey, n, ! compBypassCached,
                        eqGateEnvBuffer.data(), forceMask.data());
    grDb.store (compressor.getCurrentGainReductionDb());
    detectorRmsDb.store (compressor.getCurrentDetectorRmsDb());
    fastDetectorDb.store (compressor.getCurrentFastDetectorDb());

    // ---- 3. Dry path: exactly the same integer-sample delay (always ticks) ----
    for (int ch = 0; ch < nCh; ++ch)
    {
        const double* src = buffer.getReadPointer (ch);
        double* dst = dryBuffer.getWritePointer (ch);
        auto& delay = dryDelays[(size_t) ch];
        for (int i = 0; i < n; ++i)
            dst[i] = delay.processSample (src[i]);
    }

    // ---- Spectrum feed: the DRY input (blue layer; "kept" is derived in UI) ----
    pushSpectrumSamples (detectorRaw.data(), n);

    // ---- Band solo audition: only this band's kept ring — the dry signal
    // minus the band's own filter output (no gate, no tail gate, no Amount).
    if (soloBandCached >= 0)
    {
        intensitySmoothed.skip (n);

        for (int ch = 0; ch < nCh; ++ch)
        {
            const double* dry = dryBuffer.getReadPointer (ch);
            double* out = buffer.getWritePointer (ch);
            const int chIdx = juce::jmin (ch, 1);
            for (int i = 0; i < n; ++i)
            {
                double f = dry[i];
                for (int st = 0; st < soloNumStages; ++st)
                    f = soloStages[st][chIdx].processSample (f);
                out[i] = dry[i] - f;
            }
        }
        updateOutputPeak (buffer, nCh, n);
        return;
    }

    // ---- 4. EQ + gate blend ----
    const bool eqActive   = ! eqBypassCached;
    const bool gateActive = eqActive && eqGateOnCached;

    if (gateActive)
        for (int ch = 0; ch < nCh; ++ch)
            preEqBuffer.copyFrom (ch, 0, parallelBuffer, ch, 0, n);

    if (eqActive)
        eq.process (parallelBuffer, n);

    if (gateActive)
    {
        // EQ gate: crossfade the parallel path between the EQ'd signal
        // (envelope = 1, drum sounding) and the raw inverted-cancelling
        // signal (envelope = 0, silence between hits).
        float minEnvDb = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const double env = eqGateEnvBuffer[(size_t) i];
            for (int ch = 0; ch < nCh; ++ch)
            {
                double* par = parallelBuffer.getWritePointer (ch);
                par[i] = par[i] * env + preEqBuffer.getReadPointer (ch)[i] * (1.0 - env);
            }
            minEnvDb = juce::jmin (minEnvDb,
                                   (float) (20.0 * std::log10 (juce::jmax (1.0e-4, env))));
        }
        eqGateDb.store (minEnvDb);
    }
    else
    {
        eqGateDb.store (0.0f);
    }

    // ---- 5. Compose the output ----
    // The parallel path's polarity flip is linear, so it is applied here as
    // the subtraction:  normal output = dry − intensity · processed.
    switch (monitorModeCached)
    {
        case monitorSidechain:
        {
            intensitySmoothed.skip (n);
            for (int ch = 0; ch < nCh; ++ch)
            {
                double* out = buffer.getWritePointer (ch);
                for (int i = 0; i < n; ++i)
                    out[i] = detectorFiltered[(size_t) i];
            }
            break;
        }

        case monitorProcessing:
        {
            for (int i = 0; i < n; ++i)
            {
                const double iv = intensitySmoothed.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                    buffer.getWritePointer (ch)[i] = iv * parallelBuffer.getReadPointer (ch)[i];
            }
            break;
        }

        case monitorDelta:
        {
            // Literally: (time-aligned) input minus the final Normal output.
            for (int i = 0; i < n; ++i)
            {
                const double iv = intensitySmoothed.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                {
                    const double dry    = dryBuffer.getReadPointer (ch)[i];
                    const double normal = dry - iv * parallelBuffer.getReadPointer (ch)[i];
                    buffer.getWritePointer (ch)[i] = dry - normal;
                }
            }
            break;
        }

        case monitorNormal:
        default:
        {
            for (int i = 0; i < n; ++i)
            {
                const double iv = intensitySmoothed.getNextValue();
                for (int ch = 0; ch < nCh; ++ch)
                    buffer.getWritePointer (ch)[i] = dryBuffer.getReadPointer (ch)[i]
                                                   - iv * parallelBuffer.getReadPointer (ch)[i];
            }
            break;
        }
    }

    updateOutputPeak (buffer, nCh, n);
}

void MagicDrumDeBleedAudioProcessor::updateOutputPeak (const juce::AudioBuffer<double>& buffer,
                                                       int numChannels, int numSamples)
{
    double peak = 0.0;
    for (int ch = 0; ch < numChannels; ++ch)
        peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
    outputPeakDb.store ((float) (20.0 * std::log10 (juce::jmax (1.0e-6, peak))));
}

//==============================================================================
void MagicDrumDeBleedAudioProcessor::pushSpectrumSamples (const double* mono, int numSamples)
{
    int start1, size1, start2, size2;
    spectrumFifo.prepareToWrite (numSamples, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        spectrumFifoBuffer[(size_t) (start1 + i)] = (float) mono[i];
    for (int i = 0; i < size2; ++i)
        spectrumFifoBuffer[(size_t) (start2 + i)] = (float) mono[size1 + i];

    spectrumFifo.finishedWrite (size1 + size2);   // whatever didn't fit is dropped
}

int MagicDrumDeBleedAudioProcessor::readSpectrumSamples (float* dest, int maxSamples)
{
    int start1, size1, start2, size2;
    spectrumFifo.prepareToRead (maxSamples, start1, size1, start2, size2);

    for (int i = 0; i < size1; ++i)
        dest[i] = spectrumFifoBuffer[(size_t) (start1 + i)];
    for (int i = 0; i < size2; ++i)
        dest[size1 + i] = spectrumFifoBuffer[(size_t) (start2 + i)];

    spectrumFifo.finishedRead (size1 + size2);
    return size1 + size2;
}

//==============================================================================
double MagicDrumDeBleedAudioProcessor::finishLearnAndAnalyse()
{
    learnAnalyzer.stopCapture();
    scLearnAnalyzer.stopCapture();
    // Trigger learn listens to whatever the trigger listens to.
    return (extScActive.load() ? scLearnAnalyzer : learnAnalyzer).analyse (pLearnCeiling->load());
}

std::vector<mdd::LearnAnalyzer::Resonance> MagicDrumDeBleedAudioProcessor::finishLearnAndAnalyseResonances()
{
    learnAnalyzer.stopCapture();
    scLearnAnalyzer.stopCapture();
    // Keep bands always come from the signal being processed.
    return learnAnalyzer.analyseResonances (1000.0);
}

double MagicDrumDeBleedAudioProcessor::sidechainFundamentalAfterLearn()
{
    scLearnAnalyzer.stopCapture();
    return scLearnAnalyzer.analyse (pLearnCeiling->load());
}

//==============================================================================
juce::Point<int> MagicDrumDeBleedAudioProcessor::getAdvancedSize() const
{
    return { (int) apvts.state.getProperty ("advWidth", 0),
             (int) apvts.state.getProperty ("advHeight", 0) };
}

void MagicDrumDeBleedAudioProcessor::setAdvancedSize (int w, int h)
{
    apvts.state.setProperty ("advWidth", w, nullptr);
    apvts.state.setProperty ("advHeight", h, nullptr);
}

juce::Point<int> MagicDrumDeBleedAudioProcessor::getSimpleSize() const
{
    return { (int) apvts.state.getProperty ("simpleWidth", 0),
             (int) apvts.state.getProperty ("simpleHeight", 0) };
}

void MagicDrumDeBleedAudioProcessor::setSimpleSize (int w, int h)
{
    apvts.state.setProperty ("simpleWidth", w, nullptr);
    apvts.state.setProperty ("simpleHeight", h, nullptr);
}

void MagicDrumDeBleedAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MagicDrumDeBleedAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            linkSyncing.store (true);      // params load in undefined order
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            linkSyncing.store (false);
        }
}

//==============================================================================
juce::AudioProcessorEditor* MagicDrumDeBleedAudioProcessor::createEditor()
{
    return new MagicDrumDeBleedAudioProcessorEditor (*this);
}

// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MagicDrumDeBleedAudioProcessor();
}
