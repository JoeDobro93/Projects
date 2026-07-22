#pragma once

/*
    LearnAnalyzer — captures incoming audio while "Learn" is active, then
    finds the dominant frequency below a user-set ceiling.

    Usage (thread contract):
      - Audio thread calls pushSamples() every block while capturing is on.
        It uses a try-lock, so it never blocks the audio thread.
      - The UI turns capturing off, then calls analyse() on the message
        thread. analyse() takes the lock for real, so it cannot race the
        audio thread's writes.

    Analysis: skip the first ~5 ms of the capture (broadband attack click),
    Hann-window overlapping FFT frames, average the magnitude spectra,
    ignore every bin above the learn ceiling (equivalent to low-passing the
    analysis), then return the highest-magnitude bin refined by parabolic
    interpolation.
*/

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace mdd
{

class LearnAnalyzer
{
public:
    void prepare (double sampleRate);

    void startCapture();
    void stopCapture();
    bool isCapturing() const noexcept   { return capturing.load(); }

    // Audio thread. Mono samples, un-filtered input.
    void pushSamples (const double* samples, int numSamples);

    // Message thread, after stopCapture(). Returns detected frequency in Hz,
    // or -1.0 if there wasn't enough usable material.
    double analyse (double ceilingHz);

    /*  Resonance centre below ceilingHz. levelDb is relative to the loudest
        (the fundamental, always first at 0 dB). */
    struct Resonance { double hz; double levelDb; };

    /*  Message thread, after stopCapture(). Finds up to maxCount resonance
        centres below ceilingHz, loudest first: local spectrum maxima with
        parabolic refinement, greedily picked with a spacing padding of
        max(12 Hz, 6 %) so clustered bins collapse to one centre, and a
        floor of kResonanceFloorDb below the fundamental (drum-shell
        overtones and snare ring sit well above it; noise doesn't).
        Returns empty if there wasn't enough usable material. */
    std::vector<Resonance> analyseResonances (double ceilingHz, int maxCount = 8);

    static constexpr double kResonanceFloorDb = -30.0;

private:
    // Caller must hold the lock. Returns averaged frame count (0 = too little).
    int averageSpectrum (std::vector<double>& averaged);

    static constexpr int    kFftOrder   = 12;              // 4096-point FFT
    static constexpr int    kFftSize    = 1 << kFftOrder;
    static constexpr double kMaxSeconds = 8.0;             // capture cap
    static constexpr double kSkipMs     = 5.0;             // skip attack click

    juce::dsp::FFT fft { kFftOrder };
    juce::dsp::WindowingFunction<float> window { (size_t) kFftSize,
                                                 juce::dsp::WindowingFunction<float>::hann };

    juce::SpinLock lock;
    std::vector<float> capture;
    int captureCount = 0;
    std::atomic<bool> capturing { false };
    double sr = 44100.0;
};

} // namespace mdd
