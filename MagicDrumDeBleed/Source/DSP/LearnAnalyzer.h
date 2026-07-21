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

private:
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
