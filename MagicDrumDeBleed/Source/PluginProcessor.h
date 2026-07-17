#pragma once

/*
    PluginProcessor — parallel null-cancellation engine.

    Signal flow (see README for the full picture):

        input ──┬────────────────► dry delay (= lookahead) ────────────┐
                │                                                      ▼
                ├─► [flip ▸ compressor ▸ EQ ▸ intensity] ──────────► SUM ──► output
                │        (the polarity flip is linear, so it is
                │         folded into the final sum as a subtraction)
                └─► detector (un-delayed, optional bandpass) ─► compressor sidechain

    With the compressor idle the parallel path is an exact inverted copy of
    the dry path, so the sum cancels to silence. The compressor ducks the
    parallel path on drum hits, letting transients through; EQ notches keep
    chosen frequencies (drum body/ring) out of the parallel path so they
    survive in the output.

    All internal processing is double precision; float hosts are converted
    at the block boundary. Latency (= lookahead) is reported through
    setLatencySamples().
*/

#include <JuceHeader.h>
#include "DSP/CompressorProcessor.h"
#include "DSP/SidechainFilter.h"
#include "DSP/EQProcessor.h"
#include "DSP/LearnAnalyzer.h"

namespace ParamIDs
{
    inline constexpr const char* threshold    = "threshold";
    inline constexpr const char* reduction    = "reduction";
    inline constexpr const char* lookahead    = "lookahead";
    inline constexpr const char* rmsWindow    = "rmsWindow";
    inline constexpr const char* hold         = "hold";
    inline constexpr const char* release      = "release";

    inline constexpr const char* scEnable     = "scEnable";
    inline constexpr const char* scFreq       = "scFreq";
    inline constexpr const char* scQ          = "scQ";
    inline constexpr const char* learnCeiling = "learnCeiling";

    inline constexpr const char* hpfOn        = "hpfOn";
    inline constexpr const char* hpfFreq      = "hpfFreq";
    inline constexpr const char* hpfSlope     = "hpfSlope";
    inline constexpr const char* lpfOn        = "lpfOn";
    inline constexpr const char* lpfFreq      = "lpfFreq";
    inline constexpr const char* lpfSlope     = "lpfSlope";

    // Notch bands use 0-based indices 0..4 (displayed as 1..5).
    inline juce::String notchOn    (int i)   { return "notch" + juce::String (i + 1) + "On"; }
    inline juce::String notchFreq  (int i)   { return "notch" + juce::String (i + 1) + "Freq"; }
    inline juce::String notchQ     (int i)   { return "notch" + juce::String (i + 1) + "Q"; }
    inline juce::String notchGain  (int i)   { return "notch" + juce::String (i + 1) + "Gain"; }
    inline juce::String notchShape (int i)   { return "notch" + juce::String (i + 1) + "Shape"; }

    inline constexpr const char* intensity    = "intensity";
    inline constexpr const char* monitorMode  = "monitorMode";
    inline constexpr const char* compBypass   = "compBypass";
    inline constexpr const char* eqBypass     = "eqBypass";
}

class MagicDrumDeBleedAudioProcessor : public juce::AudioProcessor
{
public:
    enum MonitorMode
    {
        monitorNormal = 0,      // dry + parallel (final output)
        monitorSidechain,       // solo the filtered detector signal
        monitorProcessing,      // solo the processed parallel path, un-flipped
        monitorDelta            // input minus final output (what was removed)
    };

    MagicDrumDeBleedAudioProcessor();
    ~MagicDrumDeBleedAudioProcessor() override = default;

    // ---- AudioProcessor ----
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&,  juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;
    bool supportsDoublePrecisionProcessing() const override    { return true; }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                            { return true; }

    const juce::String getName() const override                { return JucePlugin_Name; }
    bool acceptsMidi() const override                          { return false; }
    bool producesMidi() const override                         { return false; }
    bool isMidiEffect() const override                         { return false; }
    double getTailLengthSeconds() const override               { return 0.0; }

    int getNumPrograms() override                              { return 1; }
    int getCurrentProgram() override                           { return 0; }
    void setCurrentProgram (int) override                      {}
    const juce::String getProgramName (int) override           { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---- Parameters ----
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ---- Learn (called from the UI / message thread) ----
    void   startLearn()                 { learnAnalyzer.startCapture(); }
    bool   isLearning() const           { return learnAnalyzer.isCapturing(); }
    double finishLearnAndAnalyse();     // stops capture, returns detected Hz or -1

    // ---- Metering / analysis feeds for the UI ----
    float getGainReductionDb() const    { return grDb.load(); }
    float getDetectorRmsDb() const      { return detectorRmsDb.load(); }   // threshold-comparable input level
    int   readSpectrumSamples (float* dest, int maxSamples);   // mono parallel-path samples

    // Spectrum tap point: true = after the EQ (default), false = before it.
    void setSpectrumPostEq (bool postEq)   { spectrumPostEq.store (postEq); }
    bool isSpectrumPostEq() const          { return spectrumPostEq.load(); }

    // Band solo audition (UI-only, not saved): -1 = off, 0..6 = band index.
    // Solos a bandpass at the band's frequency/Q on the processed signal
    // (band gain intentionally NOT applied), muting the dry path.
    void setSoloBand (int band)            { soloBand.store (band); }
    int  getSoloBand() const               { return soloBand.load(); }

    // ---- UI-state persistence (saved inside the plugin state) ----
    bool  isDarkTheme() const           { return (bool) apvts.state.getProperty ("themeDark", true); }
    void  setDarkTheme (bool dark)      { apvts.state.setProperty ("themeDark", dark, nullptr); }

    bool  isSimpleView() const          { return (bool) apvts.state.getProperty ("simpleView", false); }
    void  setSimpleView (bool simple)   { apvts.state.setProperty ("simpleView", simple, nullptr); }

    // Each view remembers its own last window size.
    juce::Point<int> getAdvancedSize() const;
    void  setAdvancedSize (int w, int h);
    juce::Point<int> getSimpleSize() const;
    void  setSimpleSize (int w, int h);

private:
    void updateParametersForBlock();
    void processInternal (juce::AudioBuffer<double>& buffer);
    void pushSpectrumSamples (const juce::AudioBuffer<double>& parallel, int numChannels, int numSamples);

    // DSP blocks
    mdd::CompressorProcessor    compressor;
    mdd::EQProcessor            eq;
    mdd::SidechainFilter        scFilter;
    mdd::LearnAnalyzer          learnAnalyzer;
    std::vector<mdd::MonoDelay> dryDelays;

    // Work buffers (allocated in prepareToPlay)
    juce::AudioBuffer<double> conversionBuffer;   // float-host staging
    juce::AudioBuffer<double> parallelBuffer;
    juce::AudioBuffer<double> dryBuffer;
    std::vector<double> detectorRaw, detectorFiltered;

    juce::SmoothedValue<double> intensitySmoothed;

    // Spectrum feed to the editor (single producer / single consumer)
    static constexpr int kSpectrumFifoSize = 1 << 14;
    juce::AbstractFifo spectrumFifo { kSpectrumFifoSize };
    std::vector<float> spectrumFifoBuffer;

    std::atomic<float> grDb { 0.0f };
    std::atomic<float> detectorRmsDb { -120.0f };
    std::atomic<bool>  spectrumPostEq { true };

    // Band-solo audition state + its listen filter (one biquad per channel)
    std::atomic<int> soloBand { -1 };
    mdd::BiquadFilter soloFilters[2];
    int    soloCachedBand = -2;
    double soloCachedFreq = -1.0, soloCachedQ = -1.0;
    int    soloCachedSlope = -1;

    // Cached raw parameter pointers
    std::atomic<float> *pThreshold, *pReduction, *pLookahead, *pRmsWindow, *pHold, *pRelease;
    std::atomic<float> *pScEnable, *pScFreq, *pScQ, *pLearnCeiling;
    std::atomic<float> *pHpfOn, *pHpfFreq, *pHpfSlope, *pLpfOn, *pLpfFreq, *pLpfSlope;
    std::atomic<float> *pNotchOn[5], *pNotchFreq[5], *pNotchQ[5], *pNotchGain[5], *pNotchShape[5];
    std::atomic<float> *pIntensity, *pMonitorMode, *pCompBypass, *pEqBypass;

    // Per-block cached control state
    double sampleRateCached = 44100.0;
    int    maxLookaheadSamples = 0;
    int    currentLookaheadSamples = -1;
    int    monitorModeCached = monitorNormal;
    int    soloBandCached = -1;
    bool   compBypassCached = false, eqBypassCached = false, scEnabledCached = true;
    bool   spectrumPostEqCached = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicDrumDeBleedAudioProcessor)
};
