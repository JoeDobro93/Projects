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

int LearnAnalyzer::averageSpectrum (std::vector<double>& averaged)
{
    const int skip = (int) std::lround (kSkipMs * 0.001 * sr);
    const int usable = captureCount - skip;
    if (usable < 1024)
        return 0;                                   // not enough material captured

    std::vector<float> fftData ((size_t) kFftSize * 2, 0.0f);
    averaged.assign ((size_t) kFftSize / 2, 0.0);

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
    return frames;
}

double LearnAnalyzer::analyse (double ceilingHz)
{
    const juce::SpinLock::ScopedLockType sl (lock);

    std::vector<double> averaged;
    const int frames = averageSpectrum (averaged);
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

std::vector<LearnAnalyzer::Resonance> LearnAnalyzer::analyseResonances (double ceilingHz, int maxCount)
{
    const juce::SpinLock::ScopedLockType sl (lock);

    std::vector<double> averaged;
    const int frames = averageSpectrum (averaged);
    if (frames == 0)
        return {};

    const double binHz = sr / (double) kFftSize;
    const int minBin = juce::jmax (2, (int) std::ceil (30.0 / binHz));
    const int maxBin = juce::jmin ((int) averaged.size() - 2, (int) std::floor (ceilingHz / binHz));
    if (maxBin <= minBin)
        return {};

    // Every local maximum, refined for sub-bin accuracy.
    struct Cand { double hz, mag; };
    std::vector<Cand> cands;
    for (int bin = minBin; bin <= maxBin; ++bin)
    {
        const double m0 = averaged[(size_t) (bin - 1)];
        const double m1 = averaged[(size_t) bin];
        const double m2 = averaged[(size_t) (bin + 1)];
        if (m1 < m0 || m1 < m2 || m1 <= 1.0e-9 * (double) frames)
            continue;
        const double denom = m0 - 2.0 * m1 + m2;
        double offset = 0.0;
        if (std::abs (denom) > 1.0e-20)
            offset = juce::jlimit (-0.5, 0.5, 0.5 * (m0 - m2) / denom);
        cands.push_back ({ ((double) bin + offset) * binHz, m1 });
    }
    if (cands.empty())
        return {};

    std::sort (cands.begin(), cands.end(), [] (const Cand& a, const Cand& b) { return a.mag > b.mag; });

    // Greedy pick with spacing padding; the first pick is the fundamental
    // and sets the floor for everything after it.
    auto spacing = [] (double f) { return juce::jmax (12.0, 0.06 * f); };
    const double floorRatio = std::pow (10.0, kResonanceFloorDb / 20.0);
    std::vector<Resonance> out;
    double fundMag = 0.0;
    for (const auto& c : cands)
    {
        if ((int) out.size() >= maxCount)
            break;
        if (fundMag > 0.0 && c.mag < fundMag * floorRatio)
            break;                                  // sorted: everything after is quieter
        bool clash = false;
        for (const auto& p : out)
            if (std::abs (c.hz - p.hz) < spacing (p.hz))
            {
                clash = true;                       // absorbed into a stronger centre
                break;
            }
        if (clash)
            continue;
        if (out.empty())
            fundMag = c.mag;
        out.push_back ({ c.hz, 20.0 * std::log10 (c.mag / fundMag) });
    }
    return out;
}

} // namespace mdd
