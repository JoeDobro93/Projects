#pragma once

/*
    CompressorProcessor — the fixed-reduction gate at the heart of the
    parallel null-cancellation path.

    This is NOT a ratio compressor. When the RMS of the (externally
    filtered) detector signal exceeds the threshold, a fixed gain reduction
    (reductionDb) is applied to the audio, regardless of how far above
    threshold the detector is. The engage/disengage envelope is binary,
    smoothed by an attack ramp sized to the lookahead, a hold timer and a
    release ramp.

    Lookahead: the audio passing through this processor is delayed by the
    lookahead amount; the detector is analysed *un-delayed*, so the gain
    reduction has already reached its target by the time the transient
    arrives at the output of the internal delay line.

    All state and maths are double precision.
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <cmath>

namespace mdd
{

// Simple double-precision integer-sample delay line (also used by the
// PluginProcessor for the dry path, so both delays always match exactly).
class MonoDelay
{
public:
    void prepare (int maxDelaySamples)
    {
        buffer.assign ((size_t) juce::jmax (1, maxDelaySamples + 1), 0.0);
        writeIndex = 0;
    }

    void setDelay (int samples) noexcept
    {
        delaySamples = juce::jlimit (0, (int) buffer.size() - 1, samples);
    }

    void reset() { std::fill (buffer.begin(), buffer.end(), 0.0); }

    inline double processSample (double x) noexcept
    {
        buffer[(size_t) writeIndex] = x;
        int readIndex = writeIndex - delaySamples;
        if (readIndex < 0)
            readIndex += (int) buffer.size();
        const double y = buffer[(size_t) readIndex];
        if (++writeIndex >= (int) buffer.size())
            writeIndex = 0;
        return y;
    }

private:
    std::vector<double> buffer { 1, 0.0 };
    int writeIndex = 0;
    int delaySamples = 0;
};

class CompressorProcessor
{
public:
    void prepare (double sampleRate, int numChannels, int maxLookaheadSamples);
    void reset();

    void setParameters (double thresholdDb, double reductionDb, int lookaheadSamples,
                        double rmsWindowMs, double holdMs, double releaseMs);

    /*  Delays `audio` in place by the lookahead amount, computes the gain
        envelope from `detector` (mono, un-delayed, already sidechain-filtered
        by the caller) and applies it to all channels.

        applyGain == false implements the compressor-bypass: the lookahead
        delay still runs (so path alignment and latency never change) but the
        gain stays at 0 dB and the envelope state is kept released.
    */
    void process (juce::AudioBuffer<double>& audio, const double* detector,
                  int numSamples, bool applyGain);

    // Most negative gain value (dB) seen during the last process() call — for the GR meter.
    float getCurrentGainReductionDb() const noexcept   { return lastBlockGrDb; }

    // Highest detector RMS (dB) seen during the last process() call — for the
    // input meter, directly comparable to the threshold.
    float getCurrentDetectorRmsDb() const noexcept     { return lastBlockRmsDb; }

private:
    void setRmsWindow (double windowMs);
    void rebuildRmsSum();

    double sr = 44100.0;

    std::vector<MonoDelay> delays;          // one per channel

    // RMS detector (circular buffer of squared samples + running sum)
    std::vector<double> rmsBuffer;
    double rmsSum = 0.0;
    int rmsLength = 1, rmsIndex = 0, rmsRefreshCounter = 0;
    double currentRmsWindowMs = -1.0;

    // Envelope
    double currentGainDb = 0.0;
    int holdCounter = 0;

    // Cached parameters
    double thresholdDb = -20.0, reductionDb = -24.0;
    int lookaheadSamples = 0, holdSamples = 0;
    double attackCoeff = 1.0, releaseCoeff = 1.0;

    float lastBlockGrDb = 0.0f;
    float lastBlockRmsDb = -120.0f;
};

} // namespace mdd
