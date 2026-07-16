#include "LearnAnalyzer.h"

namespace mdd
{

void LearnAnalyzer::prepare (double sampleRate)
{
    sr = sampleRate;
    capture.assign ((size_t) juce::jmax (kFftSize, (int) std::ceil (kMaxSeconds * sr)), 0.0f);
    captureCount = 0;
    capturing.store (false);
}

void LearnAnalyzer::startCapture()
{
    const juce::SpinLock::ScopedLockType sl (lock);
    captureCount = 0;
    capturing.store (true);
}

void LearnAnalyzer::stopCapture()
{
    capturing.store (false);
}

void LearnAnalyzer::pushSamples (const double* samples, int numSamples)
{
    if (! capturing.load())
        return;

    const juce::SpinLock::ScopedTryLockType sl (lock);
    if (! sl.isLocked())
        return;                                     // never block the audio thread

    const int space = (int) capture.size() - captureCount;
    const int toCopy = juce::jmin (space, numSamples);
    for (int i = 0; i < toCopy; ++i)
        capture[(size_t) (captureCount + i)] = (float) samples[i];
    captureCount += toCopy;                         // buffer full: silently stop taking more
}

double LearnAnalyzer::analyse (double ceilingHz)
{
    const juce::SpinLock::ScopedLockType sl (lock);

    const int skip = (int) std::lround (kSkipMs * 0.001 * sr);
    const int usable = captureCount - skip;
    if (usable < 1024)
        return -1.0;                                // not enough material captured

    std::vector<float> fftData ((size_t) kFftSize * 2, 0.0f);
    std::vector<double> averaged ((size_t) kFftSize / 2, 0.0);

    const int hop = kFftSize / 2;
    int frames = 0;

    for (int start = skip; start < captureCount && frames < 256; start += hop)
    {
        const int available = captureCount - start;
        const int frameLen  = juce::jmin (kFftSize, available);
        if (frameLen < 1024 && frames > 0)
            break;                                  // ignore a tiny trailing remainder

        std::fill (fftData.begin(), fftData.end(), 0.0f);
        std::copy (capture.begin() + start, capture.begin() + start + frameLen, fftData.begin());

        window.multiplyWithWindowingTable (fftData.data(), (size_t) kFftSize);
        fft.performFrequencyOnlyForwardTransform (fftData.data(), true);

        for (size_t bin = 0; bin < averaged.size(); ++bin)
            averaged[bin] += (double) fftData[bin];
        ++frames;

        if (frameLen < kFftSize)
            break;
    }

    if (frames == 0)
        return -1.0;

    // Search below the ceiling only (analysis-domain low-pass), above 30 Hz.
    const double binHz = sr / (double) kFftSize;
    const int minBin = juce::jmax (1, (int) std::ceil (30.0 / binHz));
    const int maxBin = juce::jmin ((int) averaged.size() - 2, (int) std::floor (ceilingHz / binHz));
    if (maxBin <= minBin)
        return -1.0;

    int peakBin = minBin;
    for (int bin = minBin; bin <= maxBin; ++bin)
        if (averaged[(size_t) bin] > averaged[(size_t) peakBin])
            peakBin = bin;

    if (averaged[(size_t) peakBin] <= 1.0e-9 * (double) frames)
        return -1.0;                                // effectively silence

    // Parabolic interpolation around the peak for sub-bin accuracy.
    const double m0 = averaged[(size_t) (peakBin - 1)];
    const double m1 = averaged[(size_t) peakBin];
    const double m2 = averaged[(size_t) (peakBin + 1)];
    const double denom = m0 - 2.0 * m1 + m2;
    double offset = 0.0;
    if (std::abs (denom) > 1.0e-20)
        offset = juce::jlimit (-0.5, 0.5, 0.5 * (m0 - m2) / denom);

    return ((double) peakBin + offset) * binHz;
}

} // namespace mdd
