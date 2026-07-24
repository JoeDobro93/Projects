#pragma once

/*
    CompressorProcessor — the unified duck engine at the heart of the
    parallel null-cancellation path (v1.3: one engine, no modes).

    Two stages run in series on the sidechain:

    1. GATE — the event detector. The filtered detector must qualify as an
       event before the compressor is allowed to hear anything:
         - OPEN on a fast RMS (~rmsWindow/6, 0.5-3 ms) OR the slow RMS
           crossing the gate threshold, optionally vetoed by the
           Selectivity contrast test (the focused band must dominate the
           off-band remainder of the mic at the event's onset — the veto
           LATCHES per event so off-drum hits stay vetoed even when their
           sympathetic buzz later leaks into the band).
         - CLOSE on the slow RMS only, with hysteresis: once open, the
           gate sustains while the level is still FALLING through the zone
           below the threshold (a barely-over hit rings out through its own
           decay), plus the additive Hold time. Closing is HARD — the gate
           has no release and applies no gain of its own.
         - A MIDI note (forceOpen) opens it unconditionally.
       Its only job is deciding WHEN the sidechain is live.

    2. COMPRESSOR — the duck. The gated sidechain (filtered detector while
       the gate is open, silence while closed) feeds the compressor's own
       windowed RMS (the Comp RMS knob). GR target =
       −grPerOverDb × (dB over the comp threshold), clamped at the full
       reduction: standard ratios R map to grPerOverDb = 1−1/R (the copy is
       squeezed toward the threshold — hits escape the null partially
       attenuated, the classic parallel bleed trick); the negative ratios
       −N:1 use 1+N, pushing the copy N dB BELOW the threshold per dB over;
       Full (grPerOverDb ≥ kFullRatioSlope) is the binary-gate law — any
       amount over ducks the full −96 dB. The comp's attack/release
       envelope is the ONLY gain smoothing in the engine: when the gate
       slams shut the sidechain dies, the RMS drains within its window and
       the duck releases at the comp Release rate — the release role of the
       classic gate, performed in the audio domain.

    The comp threshold is independent of the gate threshold: gate low +
    comp higher = the old compressor behaviour; both matched + Full ratio =
    the old gate. In between, quiet events that pass the gate escape the
    null only as far as the ratio law lets them — false triggers just over
    the line stay nearly cancelled.

    The tail (EQ-gate) keys on the comp threshold too: a hit peaking `over`
    dB past it engages the tail at base + (1−base)·over/range, capped at 1
    (range < 0.05 = always full), and the envelope holds the event's
    maximum before the hold/fade cycle.

    Lookahead: the audio passing through this processor is delayed by the
    lookahead amount; the detector is analysed *un-delayed*, so the duck
    has already reached its target by the time the transient arrives at
    the output of the internal delay line.

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

// Boxcar mean-square detector: circular buffer of squared samples with a
// running sum, rebuilt exactly once per window so floating-point drift
// can't accumulate over hours of processing. Worst-case window is 100 ms.
class WindowedRms
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        buffer.assign ((size_t) juce::jmax (1, (int) std::ceil (0.1 * sr) + 1), 0.0);
        currentWindowMs = -1.0;
        setWindow (10.0);
        reset();
    }

    void reset()
    {
        std::fill (buffer.begin(), buffer.end(), 0.0);
        sum = 0.0;
        index = 0;
        refreshCounter = 0;
    }

    // Window length changes clear the buffer and start accumulating fresh —
    // a brief detector dip on a manual parameter tweak is inaudible.
    void setWindow (double windowMs);

    inline double pushMeanSquare (double x) noexcept
    {
        const double sq = x * x;
        sum += sq - buffer[(size_t) index];
        buffer[(size_t) index] = sq;
        if (++index >= length)
            index = 0;
        if (++refreshCounter >= length)
        {
            refreshCounter = 0;
            double exact = 0.0;
            for (int i = 0; i < length; ++i)
                exact += buffer[(size_t) i];
            sum = exact;
        }
        return juce::jmax (0.0, sum) / (double) length;
    }

private:
    std::vector<double> buffer { 1, 0.0 };
    double sum = 0.0;
    int length = 1, index = 0, refreshCounter = 0;
    double currentWindowMs = -1.0;
    double sr = 44100.0;
};

class CompressorProcessor
{
public:
    // grPerOverDb at or above this means Full ratio: the binary −96 duck.
    static constexpr double kFullRatioSlope = 1.0e6;

    void prepare (double sampleRate, int numChannels, int maxDelaySamples,
                  int maxMarginSamples = 0);
    void reset();

    /*  Gate-stage parameters (the event detector).
        marginSamples: the audio is delayed lookahead + margin, and the gain/
        tail envelopes are applied margin samples LATE — so decisions still
        lead the audio by exactly the lookahead. A gate opening that was held
        back by the Selectivity veto (level qualified, veto said no) applies
        UNDELAYED for one margin window instead, recovering the head start the
        veto's settle time consumed — no more flammed attacks at partial
        Amount. The caller passes margin = totalDelay − lookahead, so the sum
        (and the plugin's reported latency) never moves with any knob. */
    void setParameters (double thresholdDb, double reductionDb, int lookaheadSamples,
                        int marginSamples,
                        double rmsWindowMs, double holdMs,
                        double hysteresisDb, double contrastDb);

    /*  EQ-gate envelope timing. tailRangeDb/tailBase01: a hit peaking `over`
        dB past the COMP threshold engages the tail at
        base + (1−base)·over/range, capped at 1 — false triggers just over
        the line no longer ring the full tail. The envelope holds the
        event's MAX engagement, then fades from there. range < 0.05 =
        always full (legacy). */
    void setEqGateParameters (double holdMs, double releaseMs,
                              double tailRangeDb = 0.0, double tailBase01 = 1.0);

    /*  Compressor-stage parameters (the duck): its own absolute threshold,
        the ratio law's slope (see kFullRatioSlope), its own RMS window and
        the attack/release envelope — the engine's only gain smoothing. */
    void setCompParameters (double compThresholdDb, double grPerOverDb,
                            double rmsWindowMs, double attackMs, double releaseMs);

    /*  Delays `audio` in place by the lookahead amount, computes the gain
        envelope from `detector` (mono, un-delayed, already sidechain-filtered
        by the caller) and applies it to all channels.

        gateActive == false (the TRIGGER stage's Bypass) bypasses the GATE
        STAGE ONLY: every sample counts as an event — the compressor's feed
        stays live, exactly as if the gate threshold sat at minimum. The
        compressor keeps ducking (and cancelling sub-threshold bleed)
        normally, and the lookahead delay still runs so path alignment and
        latency never change.
    */
    /*  eqGateEnv (optional): per-sample 0..1 envelope for the EQ gate — it
        engages (with the tail blend) while the gated sidechain is over the
        comp threshold, then holds and releases towards 0.
    */
    /*  detectorBroad: the same sidechain BEFORE the trigger filter — the
        contrast (Selectivity) veto compares the filtered band against it, so
        off-frequency bleed that happens to poke over the threshold cannot
        open the gate. Pass the filtered signal again when unavailable.

        forceOpen (optional): per-sample non-zero = a MIDI note is holding the
        GATE open (no threshold, no veto; hold and hysteresis then run as
        normal once it clears). The compressor still tracks the real level —
        a forced-open passage only escapes the null as far as the signal
        pushes past the comp threshold. */
    void process (juce::AudioBuffer<double>& audio, const double* detector,
                  const double* detectorBroad,
                  int numSamples, bool gateActive, double* eqGateEnv = nullptr,
                  const unsigned char* forceOpen = nullptr);

    // Most negative gain value (dB) seen during the last process() call — for the GR meter.
    float getCurrentGainReductionDb() const noexcept   { return lastBlockGrDb; }

    // Highest gate (slow) detector RMS (dB) seen during the last process()
    // call — for the input meter, directly comparable to the gate threshold.
    float getCurrentDetectorRmsDb() const noexcept     { return lastBlockRmsDb; }

    // Highest fast (opening) detector level of the last block — what actually
    // fires the gate; the history view overlays it on the smoothed trace.
    float getCurrentFastDetectorDb() const noexcept    { return lastBlockFastDb; }

private:
    double sr = 44100.0;

    std::vector<MonoDelay> delays;          // one per channel

    // Gate slow RMS (the Smoothing knob) — governs closing and the meter —
    // and the compressor's own RMS fed by the GATED sidechain (Comp RMS).
    WindowedRms gateRms, compRms;

    // Fast RMS detector (single-pole mean-square) — gate opening only. The
    // broad twin runs on the unfiltered sidechain for the contrast veto.
    // Both are peak-held (instant attack, shared release) BEFORE the
    // off-band subtraction, so a hit's own bandpass ring-up can't read as
    // off-band.
    double fastMeanSq = 0.0, fastCoeff = 1.0;
    double broadMeanSq = 0.0;
    double broadPkSq = 0.0, fastPkSq = 0.0;      // peak-held inputs of the veto reference
    double pkRelCoeff = 1.0;
    bool vetoLatch = false;                      // event classified off-band at onset

    // Lagged copy of the slow level (dB) — the hysteresis falling test.
    double slowDbLag = -120.0, hystLagCoeff = 1.0;

    // Gate state + the duck envelope
    double currentGainDb = 0.0;
    int holdCounter = 0;
    bool gateOpen = false;

    // Envelope-application margin (see setParameters): rings hold the gain
    // and tail envelopes for marginSamples; veto-late opens bypass them.
    std::vector<double> gainRing { 1, 0.0 }, envRing { 1, 0.0 };
    int marginSamples = 0, ringIdx = 0;
    int bypassRemaining = 0;
    int vetoBlockedSamples = 0;

    // EQ-gate envelope (0..1 linear)
    double eqGateEnv = 0.0;
    int eqGateHoldCounter = 0, eqGateHoldSamples = 0;
    double eqGateReleaseCoeff = 1.0;
    double tailRangeDb = 0.0, tailBase01 = 1.0;

    // Compressor stage
    double compThresholdDb = -52.0;
    double compGrPerDb = 0.99;
    double compAttackCoeff = 1.0, compReleaseCoeff = 1.0;

    // Cached gate parameters
    double thresholdDb = -20.0, reductionDb = -24.0;
    int lookaheadSamples = 0, holdSamples = 0;
    double hysteresisDb = 8.0, contrastDb = 24.0;

    float lastBlockGrDb = 0.0f;
    float lastBlockRmsDb = -120.0f;
    float lastBlockFastDb = -120.0f;
};

} // namespace mdd
