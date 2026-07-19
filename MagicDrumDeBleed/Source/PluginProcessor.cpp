#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::NormalisableRange<float> logHzRange (float minHz, float maxHz)
    {
        juce::NormalisableRange<float> r (minHz, maxHz, 0.01f);
        r.setSkewForCentre (std::sqrt (minHz * maxHz));
        return r;
    }

    constexpr float kNotchDefaultFreqs[5] = { 100.0f, 200.0f, 400.0f, 800.0f, 1600.0f };
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
        .withStringFromValueFunction ([] (float v, int) { return juce::String (v, v < 10.0f ? 1 : 0) + " ms"; });

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

    // ---- Compressor ----
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::threshold, 1 }, "Threshold",
                    juce::NormalisableRange<float> (-60.0f, 0.0f, 0.1f), -20.0f, dB));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::reduction, 1 }, "Reduction Target",
                    juce::NormalisableRange<float> (-96.0f, 0.0f, 0.1f), -96.0f, dB));
    p.push_back (std::make_unique<AudioParameterInt>   (ParameterID { ParamIDs::lookahead, 1 }, "Lookahead",
                    1, 20, 5, juce::AudioParameterIntAttributes()
                                .withLabel ("ms")
                                .withStringFromValueFunction ([] (int v, int) { return juce::String (v) + " ms"; })));
    {
        juce::NormalisableRange<float> r (1.0f, 100.0f, 0.1f);  r.setSkewForCentre (10.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::rmsWindow, 1 }, "RMS Window", r, 10.0f, ms));
    }
    {
        juce::NormalisableRange<float> r (5.0f, 200.0f, 1.0f);  r.setSkewForCentre (40.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::hold, 1 }, "Hold", r, 20.0f, ms));
    }
    {
        juce::NormalisableRange<float> r (5.0f, 200.0f, 1.0f);  r.setSkewForCentre (60.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::release, 1 }, "Release", r, 100.0f, ms));
    }

    // ---- Sidechain ----
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::scEnable, 1 }, "SC Filter", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::scFreq, 1 }, "SC Frequency",
                    logHzRange (30.0f, 2000.0f), 200.0f, hz));
    {
        juce::NormalisableRange<float> r (0.3f, 12.0f, 0.01f);  r.setSkewForCentre (1.9f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::scQ, 1 }, "SC Q", r, 1.5f));
    }
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::scType, 1 }, "Trigger Filter Type",
                    juce::StringArray { "High Pass", "Low Pass", "Bandpass" }, 2));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::scSlope, 1 }, "Trigger Filter Slope",
                    juce::NormalisableRange<float> (6.0f, 24.0f, 6.0f), 12.0f,
                    juce::AudioParameterFloatAttributes().withLabel ("dB/oct")
                        .withStringFromValueFunction ([] (float v, int) { return juce::String ((int) v) + " dB/oct"; })));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::learnCeiling, 1 }, "Learn Ceiling",
                    logHzRange (200.0f, 2000.0f), 1000.0f, hz));
    p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::linkK1, 1 }, "Link to K1", false));

    // ---- EQ: HPF / LPF ----
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::hpfOn, 1 }, "HPF On", true));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::hpfFreq, 1 }, "HPF Freq",
                    logHzRange (20.0f, 15000.0f), 800.0f, hz));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::hpfSlope, 1 }, "HPF Slope",
                    juce::StringArray { "6 dB/oct", "12 dB/oct" }, 1));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::lpfOn, 1 }, "LPF On", false));
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::lpfFreq, 1 }, "LPF Freq",
                    logHzRange (50.0f, 20000.0f), 20000.0f, hz));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::lpfSlope, 1 }, "LPF Slope",
                    juce::StringArray { "6 dB/oct", "12 dB/oct" }, 1));

    // ---- EQ: notch bands 1..5 ----
    for (int i = 0; i < 5; ++i)
    {
        const juce::String num (i + 1);
        p.push_back (std::make_unique<AudioParameterBool>  (ParameterID { ParamIDs::notchOn (i), 1 },
                        "Notch " + num + " On", false));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::notchFreq (i), 1 },
                        "Notch " + num + " Freq", logHzRange (20.0f, 20000.0f), kNotchDefaultFreqs[i], hz));
        {
            juce::NormalisableRange<float> r (0.1f, 40.0f, 0.01f);  r.setSkewForCentre (1.0f);
            p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::notchQ (i), 1 },
                            "Notch " + num + " Q", r, 1.0f));
        }
        p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::notchGain (i), 1 },
                        "Notch " + num + " Gain", juce::NormalisableRange<float> (-48.0f, 0.0f, 0.1f), -24.0f, dB));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::notchShape (i), 1 },
                        "Notch " + num + " Shape", juce::StringArray { "Bell", "Proportional Q", "Band Shelf" }, 0));
    }

    // ---- EQ gate ----
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamIDs::eqGateOn, 1 }, "EQ Gate", true));
    {
        juce::NormalisableRange<float> r (0.0f, 2000.0f, 1.0f);  r.setSkewForCentre (150.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::eqGateHold, 1 }, "EQ Gate Hold", r, 0.0f, ms));
    }
    {
        juce::NormalisableRange<float> r (5.0f, 5000.0f, 1.0f);  r.setSkewForCentre (800.0f);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { ParamIDs::eqGateRelease, 1 }, "EQ Gate Release", r, 1000.0f, ms));
    }

    // ---- Output / monitoring ----
    p.push_back (std::make_unique<AudioParameterFloat>  (ParameterID { ParamIDs::intensity, 1 }, "Intensity",
                    juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, pct));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::monitorMode, 1 }, "Monitor",
                    juce::StringArray { "Normal", "Sidechain", "Processing", "Delta" }, 0));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::compBypass, 1 }, "Comp Bypass", false));
    p.push_back (std::make_unique<AudioParameterBool>   (ParameterID { ParamIDs::eqBypass, 1 }, "EQ Bypass", false));

    return { p.begin(), p.end() };
}

//==============================================================================
MagicDrumDeBleedAudioProcessor::MagicDrumDeBleedAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameterLayout())
{
    auto raw = [this] (const juce::String& id) { return apvts.getRawParameterValue (id); };

    pThreshold    = raw (ParamIDs::threshold);
    pReduction    = raw (ParamIDs::reduction);
    pLookahead    = raw (ParamIDs::lookahead);
    pRmsWindow    = raw (ParamIDs::rmsWindow);
    pHold         = raw (ParamIDs::hold);
    pRelease      = raw (ParamIDs::release);
    pScEnable     = raw (ParamIDs::scEnable);
    pScFreq       = raw (ParamIDs::scFreq);
    pScQ          = raw (ParamIDs::scQ);
    pScType       = raw (ParamIDs::scType);
    pScSlope      = raw (ParamIDs::scSlope);
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

    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

//==============================================================================
void MagicDrumDeBleedAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sampleRateCached = sampleRate;
    maxLookaheadSamples = (int) std::ceil (0.020 * sampleRate) + 1;   // 20 ms cap

    const int numCh = juce::jlimit (1, 2, getTotalNumInputChannels());

    compressor.prepare (sampleRate, numCh, maxLookaheadSamples);
    eq.prepare (sampleRate, numCh);
    scFilter.prepare (sampleRate);
    learnAnalyzer.prepare (sampleRate);

    dryDelays.resize ((size_t) numCh);
    for (auto& d : dryDelays)
        d.prepare (maxLookaheadSamples);

    const int block = juce::jmax (16, samplesPerBlock);
    conversionBuffer.setSize (numCh, block);
    parallelBuffer.setSize (numCh, block);
    dryBuffer.setSize (numCh, block);
    preEqBuffer.setSize (numCh, block);
    detectorRaw.assign ((size_t) block, 0.0);
    detectorFiltered.assign ((size_t) block, 0.0);
    eqGateEnvBuffer.assign ((size_t) block, 0.0);

    intensitySmoothed.reset (sampleRate, 0.05);
    intensitySmoothed.setCurrentAndTargetValue (pIntensity->load() * 0.01);

    spectrumFifo.reset();

    for (auto& f : soloFilters)
        f.reset();
    soloCachedBand = -2;

    currentLookaheadSamples = -1;   // force latency + delay refresh
    updateParametersForBlock();
}

void MagicDrumDeBleedAudioProcessor::updateParametersForBlock()
{
    // ---- Lookahead / latency ----
    const int lookaheadMs = (int) pLookahead->load();
    const int lookahead = juce::jlimit (0, maxLookaheadSamples,
                                        (int) std::lround (lookaheadMs * 0.001 * sampleRateCached));
    if (lookahead != currentLookaheadSamples)
    {
        currentLookaheadSamples = lookahead;
        for (auto& d : dryDelays)
            d.setDelay (lookahead);
        setLatencySamples (lookahead);
    }

    // ---- Compressor / sidechain ----
    compressor.setParameters (pThreshold->load(), pReduction->load(), lookahead,
                              pRmsWindow->load(), pHold->load(), pRelease->load());
    compressor.setEqGateParameters (pEqGateHold->load(), pEqGateRelease->load());
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
        nb.gainDb  = pNotchGain[i]->load();
        nb.shape   = (int) pNotchShape[i]->load();
        nb.slope   = 0;
        eq.setBandParameters (mdd::EQProcessor::kFirstNotch + i, nb);
    }

    // ---- Output / monitoring ----
    intensitySmoothed.setTargetValue (juce::jlimit (0.0, 1.0, (double) pIntensity->load() * 0.01));
    monitorModeCached = (int) pMonitorMode->load();
    compBypassCached  = pCompBypass->load() > 0.5f;
    eqBypassCached    = pEqBypass->load() > 0.5f;
    spectrumPostEqCached = spectrumPostEq.load();

    // ---- Band-solo listen filter ----
    soloBandCached = soloBand.load();
    if (soloBandCached >= 0)
    {
        double freq = 1000.0, q = 4.0;
        int slope = 1;
        if (soloBandCached == 0)      { freq = pHpfFreq->load(); slope = (int) pHpfSlope->load(); }
        else if (soloBandCached == 1) { freq = pLpfFreq->load(); slope = (int) pLpfSlope->load(); }
        else
        {
            const int n = soloBandCached - 2;
            freq = pNotchFreq[n]->load();
            q    = pNotchQ[n]->load();
        }

        if (soloBandCached != soloCachedBand || ! mdd::exactlyEqual (freq, soloCachedFreq)
            || ! mdd::exactlyEqual (q, soloCachedQ) || slope != soloCachedSlope)
        {
            const bool bandChanged = soloBandCached != soloCachedBand;
            soloCachedBand  = soloBandCached;
            soloCachedFreq  = freq;
            soloCachedQ     = q;
            soloCachedSlope = slope;

            mdd::BiquadFilter::Coeffs c;
            if (soloBandCached == 0)
                c = slope == 0 ? mdd::BiquadFilter::makeFirstOrderHighpass (sampleRateCached, freq)
                               : mdd::BiquadFilter::makeHighpass (sampleRateCached, freq, 0.70710678118654752);
            else if (soloBandCached == 1)
                c = slope == 0 ? mdd::BiquadFilter::makeFirstOrderLowpass (sampleRateCached, freq)
                               : mdd::BiquadFilter::makeLowpass (sampleRateCached, freq, 0.70710678118654752);
            else
                c = mdd::BiquadFilter::makeBandpass (sampleRateCached, freq, q);

            for (auto& f : soloFilters)
            {
                f.setCoefficients (c);
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
void MagicDrumDeBleedAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n   = buffer.getNumSamples();
    const int nCh = juce::jmin (buffer.getNumChannels(), conversionBuffer.getNumChannels());

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

void MagicDrumDeBleedAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, n);

    if (n == 0)
        return;

    processInternal (buffer);
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

    // ---- 1. Detector: mono mix of the un-delayed input ----
    const double invCh = 1.0 / (double) nCh;
    for (int i = 0; i < n; ++i)
    {
        double sum = 0.0;
        for (int ch = 0; ch < nCh; ++ch)
            sum += buffer.getReadPointer (ch)[i];
        detectorRaw[(size_t) i] = sum * invCh;
    }

    if (learnAnalyzer.isCapturing())
        learnAnalyzer.pushSamples (detectorRaw.data(), n);

    for (int i = 0; i < n; ++i)
        detectorFiltered[(size_t) i] = scEnabledCached ? scFilter.processSample (detectorRaw[(size_t) i])
                                                       : detectorRaw[(size_t) i];

    // ---- 2. Parallel path: delay (lookahead) ▸ gate ▸ EQ ----
    for (int ch = 0; ch < nCh; ++ch)
        parallelBuffer.copyFrom (ch, 0, buffer, ch, 0, n);

    compressor.process (parallelBuffer, detectorFiltered.data(), n, ! compBypassCached,
                        eqGateEnvBuffer.data());
    grDb.store (compressor.getCurrentGainReductionDb());
    detectorRmsDb.store (compressor.getCurrentDetectorRmsDb());

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

    // ---- Band solo audition: bandpass the processed signal, mute the dry ----
    if (soloBandCached >= 0)
    {
        intensitySmoothed.skip (n);

        for (int ch = 0; ch < nCh; ++ch)
        {
            const double* par = parallelBuffer.getReadPointer (ch);
            double* out = buffer.getWritePointer (ch);
            auto& filter = soloFilters[juce::jmin (ch, 1)];
            for (int i = 0; i < n; ++i)
                out[i] = filter.processSample (par[i]);
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

    // Level of the signal being subtracted (REDUCTION meter feed).
    {
        double peak = 0.0;
        for (int ch = 0; ch < nCh; ++ch)
            peak = juce::jmax (peak, parallelBuffer.getMagnitude (ch, 0, n));
        peak *= (double) pIntensity->load() * 0.01;
        removedPeakDb.store ((float) (20.0 * std::log10 (juce::jmax (1.0e-6, peak))));
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
    return learnAnalyzer.analyse (pLearnCeiling->load());
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
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
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
