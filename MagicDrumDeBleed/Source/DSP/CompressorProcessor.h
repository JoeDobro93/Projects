#pragma once

/*
    CompressorProcessor — the fixed-reduction gate at the heart of the
    parallel null-cancellation path.

    This is NOT a ratio compressor. When the detector exceeds the threshold,
    a fixed gain reduction (reductionDb) is applied to the audio, regardless
    of how far above threshold the detector is. The envelope is smoothed by
    an attack ramp sized to the lookahead, a hold timer and a release ramp.

    Detection (v2.2) uses a fast/slow split so marginal hits neither click
    nor cut short:
      - OPEN on a fast RMS (~rmsWindow/6, 0.5-3 ms) OR the slow RMS
        crossing the threshold — ghost notes and low-frequency kicks are
        caught near their true onset, preserving the lookahead margin.
        Opening can additionally require the band to dominate the unfiltered
        sidechain (the Selectivity contrast veto) so off-frequency bleed
        can't trigger it.
      - CLOSE on the slow RMS only, with hysteresis (the Hysteresis knob):
        once open, the gate sustains while the level is still falling through
        the zone below the threshold, so a barely-over hit rings out through
        its own decay like a loud one. The sustain requires a falling level,
        so bleed parked steadily inside the zone releases normally.
    Gain only ever moves through a full open -> hold -> release cycle; there
    is deliberately no partial/soft-knee state (v1.0.1: a partial opening
    with no gate cycle audibly popped on vetoed tom hits).

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
                        double rmsWindowMs, double holdMs, double releaseMs,
                        double hysteresisDb, double contrastDb);

    // EQ-gate envelope timing (shares the detector/threshold/lookahead).
    void setEqGateParameters (double holdMs, double releaseMs);

    /*  Delays `audio` in place by the lookahead amount, computes the gain
        envelope from `detector` (mono, un-delayed, already sidechain-filtered
        by the caller) and applies it to all channels.

        applyGain == false implements the compressor-bypass: the lookahead
        delay still runs (so path alignment and latency never change) but the
        gain stays at 0 dB and the envelope state is kept released.
    */
    /*  eqGateEnv (optional): per-sample 0..1 envelope for the EQ gate — 1
        while the detector is above threshold (fast attack inside the
        lookahead window), then holds and releases towards 0. Driven by the
        exact same RMS detector as the main gate.
    */
    /*  detectorBroad: the same sidechain BEFORE the trigger filter — the
        contrast (Selectivity) veto compares the filtered band against it, so
        off-frequency bleed that happens to poke over the threshold cannot
        open the gate. Pass the filtered signal again when unavailable.

        forceOpen (optional): per-sample non-zero = a MIDI note is holding the
        gate open. Treated exactly like a detector crossing (same hold and
        release once it clears) and overrides threshold AND the contrast
        veto — manual events are authoritative. */
    void process (juce::AudioBuffer<double>& audio, const double* detector,
                  const double* detectorBroad,
                  int numSamples, bool applyGain, double* eqGateEnv = nullptr,
                  const unsigned char* forceOpen = nullptr);

    // Most negative gain value (dB) seen during the last process() call — for the GR meter.
    float getCurrentGainReductionDb() const noexcept   { return lastBlockGrDb; }

    // Highest detector RMS (dB) seen during the last process() call — for the
    // input meter, directly comparable to the threshold.
    float getCurrentDetectorRmsDb() const noexcept     { return lastBlockRmsDb; }

    // Highest fast (opening) detector level of the last block — what actually
    // fires the gate; the history view overlays it on the smoothed trace.
    float getCurrentFastDetectorDb() const noexcept    { return lastBlockFastDb; }

    // Highest off-band (veto reference) level of the last block — the level
    // the Selectivity comparison runs against, for the history view.
    float getCurrentOffbandDb() const noexcept         { return lastBlockOffDb; }

private:
    void setRmsWindow (double windowMs);
    void rebuildRmsSum();

    double sr = 44100.0;

    std::vector<MonoDelay> delays;          // one per channel

    // Slow RMS detector (circular buffer of squared samples + running sum);
    // window = the Smoothing knob. Governs closing and the meter.
    std::vector<double> rmsBuffer;
    double rmsSum = 0.0;
    int rmsLength = 1, rmsIndex = 0, rmsRefreshCounter = 0;
    double currentRmsWindowMs = -1.0;

    // Fast RMS detector (single-pole mean-square) — opening only. The broad
    // twin runs on the unfiltered sidechain for the contrast veto. Both are
    // peak-held (instant attack, shared release) BEFORE the off-band
    // subtraction, so a hit's own bandpass ring-up can't read as off-band.
    double fastMeanSq = 0.0, fastCoeff = 1.0;
    double broadMeanSq = 0.0;
    double broadPkSq = 0.0, fastPkSq = 0.0;      // peak-held inputs of the veto reference
    double pkRelCoeff = 1.0;
    bool vetoLatch = false;                      // event classified off-band at onset

    // Lagged copy of the slow level (dB) — the hysteresis falling test.
    double slowDbLag = -120.0, hystLagCoeff = 1.0;

    // Envelope
    double currentGainDb = 0.0;
    int holdCounter = 0;
    bool gateOpen = false;

    // EQ-gate envelope (0..1 linear)
    double eqGateEnv = 0.0;
    int eqGateHoldCounter = 0, eqGateHoldSamples = 0;
    double eqGateReleaseCoeff = 1.0;

    // Cached parameters
    double thresholdDb = -20.0, reductionDb = -24.0;
    int lookaheadSamples = 0, holdSamples = 0;
    double attackCoeff = 1.0, releaseCoeff = 1.0;
    double hysteresisDb = 8.0, contrastDb = 24.0;

    float lastBlockGrDb = 0.0f;
    float lastBlockRmsDb = -120.0f;
    float lastBlockFastDb = -120.0f;
    float lastBlockOffDb = -120.0f;
};

} // namespace mdd
