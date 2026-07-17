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

    // ---- Notch bands: three selectable shapes, all single-biquad ----
    switch (p.shape)
    {
        case 0:  // Bell — rounded peaking dip, Q as set
            out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q, p.gainDb);
            break;

        case 1:  // Flat — wide, flat-bottomed band-reject (steeper walls than a bell)
            out[0] = BiquadFilter::makeBlendedNotch (sampleRate, p.freqHz, p.q * 0.6, p.gainDb);
            break;

        case 2:  // Notch — narrow, deep band-reject
        default:
            out[0] = BiquadFilter::makeBlendedNotch (sampleRate, p.freqHz, p.q * 1.6, p.gainDb);
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
