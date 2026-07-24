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
    // Gate stage: the event detector. It decides WHEN the compressor's
    // sidechain is live — it applies no gain of its own (hard close).
    inline constexpr const char* threshold    = "threshold";
    inline constexpr const char* lookahead    = "lookahead";
    inline constexpr const char* rmsWindow    = "rmsWindow";
    inline constexpr const char* hold         = "hold";
    inline constexpr const char* hysteresis   = "hysteresis";  // close this far below threshold (dB)
    inline constexpr const char* contrast     = "contrast";    // selectivity: max off-band excess (dB)
    inline constexpr const char* midiTrigger  = "midiTrigger"; // notes force the gate open

    // Compressor stage: the duck on the parallel copy, keyed by the GATED
    // sidechain. Its own absolute threshold and RMS window; its release is
    // the engine's only fade — the classic gate release, in audio domain.
    inline constexpr const char* compThreshold = "compThreshold";
    inline constexpr const char* compRatio    = "compRatio";   // choice: 4:1..100:1, -1:1, -2:1, Full
    inline constexpr const char* compAttack   = "compAttack";
    inline constexpr const char* compRelease  = "compRelease";
    inline constexpr const char* compRmsWindow = "compRmsWindow";

    inline constexpr const char* scEnable     = "scEnable";
    inline constexpr const char* scFreq       = "scFreq";
    inline constexpr const char* scQ          = "scQ";
    inline constexpr const char* scType       = "scType";    // 0 HP, 1 LP, 2 BP
    inline constexpr const char* scSlope      = "scSlope";   // 6/12/18/24 dB/oct (HP/LP only)
    inline constexpr const char* scExternal   = "scExternal"; // detector = the Sidechain input bus
    inline constexpr const char* learnCeiling = "learnCeiling";
    inline constexpr const char* linkK1       = "linkK1";    // K1 freq follows Focus (suspended on ext SC)

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

    inline constexpr const char* eqGateOn      = "eqGateOn";
    inline constexpr const char* eqGateHold    = "eqGateHold";
    inline constexpr const char* eqGateRelease = "eqGateRelease";
    inline constexpr const char* tailRange     = "tailRange";   // comp: dB over T for full tail
    inline constexpr const char* tailBase      = "tailBase";    // comp: tail blend AT the threshold

    inline constexpr const char* intensity    = "intensity";
    inline constexpr const char* monitorMode  = "monitorMode";
    inline constexpr const char* compBypass   = "compBypass";   // GATE bypass (ID kept for state compat)
    inline constexpr const char* eqBypass     = "eqBypass";
    // Whole-plugin bypass that keeps the engine AND the latency running
    // (smooth A/B): internally Amount is forced to 0, so the output is the
    // time-aligned dry signal.
    inline constexpr const char* globalBypass = "globalBypass";
}

class MagicDrumDeBleedAudioProcessor : public juce::AudioProcessor,
                                       private juce::AudioProcessorValueTreeState::Listener
{
public:
    // Total plugin delay never moves with the Lookahead knob: the knob
    // repositions decisions inside a fixed window and the envelope ring
    // absorbs the remainder. Base window = max Lookahead; engaging
    // Selectivity adds the flam-recovery margin (the ONLY thing that
    // changes latency, and only when crossing that boundary).
    static constexpr int kMaxLookaheadMs = 10;
    static constexpr int kEnvMarginMs    = 10;

    enum MonitorMode
    {
        monitorNormal = 0,      // dry + parallel (final output)
        monitorSidechain,       // solo the filtered detector signal
        monitorProcessing,      // solo the processed parallel path, un-flipped
        monitorDelta            // input minus final output (what was removed)
    };

    MagicDrumDeBleedAudioProcessor();
    ~MagicDrumDeBleedAudioProcessor() override;

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
    bool acceptsMidi() const override                          { return true; }
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
    // Two capture streams run together: the MAIN input (keep-band learns —
    // the signal being processed) and, while the external sidechain is
    // active, the KEY input (trigger-filter learns).
    void   startLearn()                 { learnAnalyzer.startCapture(); scLearnAnalyzer.startCapture(); }
    bool   isLearning() const           { return learnAnalyzer.isCapturing(); }
    // Trigger learn: stops capture, returns the detected Hz (from the KEY
    // when the external sidechain is active, else the main input) or -1.
    double finishLearnAndAnalyse();
    // Band learns: stops capture, returns the MAIN input's LP<1 kHz
    // resonance centres (loudest first) — never the sidechain.
    std::vector<mdd::LearnAnalyzer::Resonance> finishLearnAndAnalyseResonances();
    // After a finish...(): the KEY capture's fundamental (or -1). Lets the
    // Simple view / Learn all train the trigger filter on the key while the
    // keep bands come from the main input.
    double sidechainFundamentalAfterLearn();

    // True while the detector actually runs on the Sidechain input bus
    // (parameter on AND the host supplies the bus).
    bool isExternalSidechainActive() const  { return extScActive.load(); }

    // ---- Metering / analysis feeds for the UI ----
    float getGainReductionDb() const    { return grDb.load(); }
    float getDetectorRmsDb() const      { return detectorRmsDb.load(); }   // threshold-comparable input level
    float getFastDetectorDb() const     { return fastDetectorDb.load(); }  // opening detector (fast follower)
    float getDryDb() const              { return dryDb.load(); }            // raw (unfiltered) input level
    bool  getMidiForced() const         { return midiForcedFlag.load() > 0.5f; } // MIDI note holding the gate open
    float getEqGateReductionDb() const  { return eqGateDb.load(); }        // 0 = EQ fully engaged
    float getOutputPeakDb() const       { return outputPeakDb.load(); }
    float getIntensity01() const        { return pIntensity->load() * 0.01f; }
    int   readSpectrumSamples (float* dest, int maxSamples);   // mono parallel-path samples

    // Spectrum tap point: true = after the EQ (default), false = before it.

    // Band solo audition (UI-only, not saved): -1 = off, 0..6 = band index.
    // Plays exactly what that band KEEPS — dry minus the band's own filter
    // (|1−H| of just this band) — bypassing the gate, tail gate and Amount.
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
    // K1-link: mirrors Focus ↔ K1 frequency while ParamIDs::linkK1 is on.
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    std::atomic<bool> linkSyncing { false };

    void updateParametersForBlock();
    void processInternal (juce::AudioBuffer<double>& buffer);
    void updateOutputPeak (const juce::AudioBuffer<double>& buffer, int numChannels, int numSamples);
    void pushSpectrumSamples (const double* mono, int numSamples);

    // Mono-mixes the Sidechain input bus into scRaw for this block (sets
    // scActiveBlock / extScActive). Falls back to the main input when the
    // parameter is off or the host provides no bus.
    template <typename SampleType>
    void extractSidechain (juce::AudioBuffer<SampleType>& buffer, int numSamples);

    // DSP blocks
    mdd::CompressorProcessor    compressor;
    mdd::EQProcessor            eq;
    mdd::SidechainFilter        scFilter;
    mdd::LearnAnalyzer          learnAnalyzer;    // captures the MAIN input
    mdd::LearnAnalyzer          scLearnAnalyzer;  // captures the KEY input
    std::vector<mdd::MonoDelay> dryDelays;

    // Work buffers (allocated in prepareToPlay)
    juce::AudioBuffer<double> conversionBuffer;   // float-host staging
    juce::AudioBuffer<double> parallelBuffer;
    juce::AudioBuffer<double> dryBuffer;
    juce::AudioBuffer<double> preEqBuffer;        // parallel path before the EQ (gate blend)
    std::vector<double> detectorRaw, detectorFiltered, eqGateEnvBuffer;
    std::vector<double> scRaw;                    // mono key input (external sidechain)
    bool scActiveBlock = false;
    std::atomic<bool> extScActive { false };

    // Dry display trace: the MAIN input's level (what bypassed would sound
    // like) — independent of the detector, which may be the key input.
    double dryDispMeanSq = 0.0, dryDispCoeff = 1.0;

    juce::SmoothedValue<double> intensitySmoothed;

    // Spectrum feed to the editor (single producer / single consumer)
    static constexpr int kSpectrumFifoSize = 1 << 14;
    juce::AbstractFifo spectrumFifo { kSpectrumFifoSize };
    std::vector<float> spectrumFifoBuffer;

    std::atomic<float> grDb { 0.0f };
    std::atomic<float> detectorRmsDb { -120.0f };
    std::atomic<float> fastDetectorDb { -120.0f };
    std::atomic<float> dryDb { -120.0f };
    std::atomic<float> midiForcedFlag { 0.0f };
    std::atomic<float> eqGateDb { 0.0f };
    std::atomic<float> outputPeakDb { -120.0f };

    // Band-solo audition state + the band's own filter stages per channel
    std::atomic<int> soloBand { -1 };
    mdd::BiquadFilter soloStages[mdd::EQProcessor::kMaxStages][2];
    int soloNumStages = 1;
    int soloCachedBand = -2;
    mdd::BandParams soloCachedParams;

    // Cached raw parameter pointers
    std::atomic<float> *pThreshold, *pLookahead, *pRmsWindow, *pHold;
    std::atomic<float> *pScEnable, *pScFreq, *pScQ, *pScType, *pScSlope, *pScExternal, *pLearnCeiling;
    std::atomic<float> *pHpfOn, *pHpfFreq, *pHpfSlope, *pLpfOn, *pLpfFreq, *pLpfSlope;
    std::atomic<float> *pNotchOn[5], *pNotchFreq[5], *pNotchQ[5], *pNotchGain[5], *pNotchShape[5];
    std::atomic<float> *pIntensity, *pMonitorMode, *pCompBypass, *pEqBypass, *pGlobalBypass;
    std::atomic<float> *pEqGateOn, *pEqGateHold, *pEqGateRelease, *pTailRange, *pTailBase;
    std::atomic<float> *pHysteresis, *pContrast, *pMidiTrigger;
    std::atomic<float> *pCompThreshold, *pCompRatio, *pCompAttack, *pCompRelease, *pCompRmsWindow;
    void buildForceMask (const juce::MidiBuffer& midi, int numSamples);
    std::vector<unsigned char> forceMask;
    bool heldKeys[128] = {};                 // set, not a counter: self-heals lost note-offs
    int heldCount = 0;
    bool wasPlaying = false;
    double lastPpq = -1.0e9;

    // Per-block cached control state
    double sampleRateCached = 44100.0;
    int    maxLookaheadSamples = 0;
    int    baseDelaySamples = 0;        // kMaxLookaheadMs in samples
    int    marginSamples = 0;           // kEnvMarginMs in samples
    int    currentTotalDelay = -1;
    int    currentLookaheadSamples = -1;
    int    monitorModeCached = monitorNormal;
    int    soloBandCached = -1;
    bool   compBypassCached = false, eqBypassCached = false, scEnabledCached = true;
    bool   eqGateOnCached = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MagicDrumDeBleedAudioProcessor)
};
