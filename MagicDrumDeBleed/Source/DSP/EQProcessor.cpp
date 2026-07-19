#include "EQProcessor.h"

namespace mdd
{

void EQProcessor::prepare (double sampleRate, int numChannels)
{
    sr = sampleRate;
    channels = juce::jlimit (1, kMaxChannels, numChannels);

    for (int b = 0; b < kNumBands; ++b)
        updateBandCoefficients (b);

    reset();
}

void EQProcessor::reset()
{
    for (auto& band : bands)
        for (auto& stage : band.filters)
            for (auto& f : stage)
                f.reset();
}

void EQProcessor::setBandParameters (int bandIndex, const BandParams& params)
{
    if (bandIndex < 0 || bandIndex >= kNumBands)
        return;

    auto& band = bands[bandIndex];
    if (band.params == params)
        return;

    // Reset filter state when a band is switched on so it starts clean.
    const bool turningOn = params.enabled && ! band.params.enabled;

    band.params = params;
    updateBandCoefficients (bandIndex);

    if (turningOn)
        for (auto& stage : band.filters)
            for (auto& f : stage)
                f.reset();
}

int EQProcessor::computeCoefficients (const BandParams& p, int bandKind,
                                      double sampleRate, BiquadFilter::Coeffs (&out)[2])
{
    if (bandKind == 0)
    {
        out[0] = (p.slope == 0) ? BiquadFilter::makeFirstOrderHighpass (sampleRate, p.freqHz)
                                : BiquadFilter::makeHighpass (sampleRate, p.freqHz, 0.70710678118654752);
        return 1;
    }

    if (bandKind == 1)
    {
        out[0] = (p.slope == 0) ? BiquadFilter::makeFirstOrderLowpass (sampleRate, p.freqHz)
                                : BiquadFilter::makeLowpass (sampleRate, p.freqHz, 0.70710678118654752);
        return 1;
    }

    // ---- Keep bands: Ozone-style cut shapes ----
    switch (p.shape)
    {
        case 0:  // Bell — classic RBJ peaking cut, Q as set
            out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q, p.gainDb);
            break;

        case 1:  // Proportional Q — bell whose width tightens as the cut deepens
        {
            const double t = std::clamp (std::pow (std::abs (p.gainDb) / 18.0, 0.7), 0.1, 3.0);
            out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q * t, p.gainDb);
            break;
        }

        case 2:  // Band Shelf — dry/notch blend: exact flat floor at gainDb,
        default: // walls steeper and more contained than a bell at depth.
            out[0] = BiquadFilter::makeBlendedNotch (sampleRate, p.freqHz, p.q, p.gainDb);
            break;
    }
    return 1;
}

void EQProcessor::updateBandCoefficients (int bandIndex)
{
    auto& band = bands[bandIndex];
    const int kind = bandIndex == kBandHPF ? 0 : (bandIndex == kBandLPF ? 1 : 2);

    BiquadFilter::Coeffs coeffs[2];
    band.numStages = computeCoefficients (band.params, kind, sr, coeffs);

    for (int stage = 0; stage < band.numStages; ++stage)
        for (int ch = 0; ch < kMaxChannels; ++ch)
            band.filters[stage][ch].setCoefficients (coeffs[stage]);
}

void EQProcessor::process (juce::AudioBuffer<double>& audio, int numSamples)
{
    const int numChannels = juce::jmin (audio.getNumChannels(), channels);

    for (auto& band : bands)
    {
        if (! band.params.enabled)
            continue;

        for (int stage = 0; stage < band.numStages; ++stage)
        {
            for (int ch = 0; ch < numChannels; ++ch)
            {
                double* data = audio.getWritePointer (ch);
                auto& filter = band.filters[stage][ch];

                for (int i = 0; i < numSamples; ++i)
                    data[i] = filter.processSample (data[i]);
            }
        }
    }
}

} // namespace mdd
