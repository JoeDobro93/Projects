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

void EQProcessor::updateBandCoefficients (int bandIndex)
{
    auto& band = bands[bandIndex];
    const auto& p = band.params;

    if (bandIndex == kBandHPF)
    {
        band.numStages = 1;
        const auto c = (p.slope == 0) ? BiquadFilter::makeFirstOrderHighpass (sr, p.freqHz)
                                      : BiquadFilter::makeHighpass (sr, p.freqHz, 0.70710678118654752);
        for (int ch = 0; ch < kMaxChannels; ++ch)
            band.filters[0][ch].setCoefficients (c);
        return;
    }

    if (bandIndex == kBandLPF)
    {
        band.numStages = 1;
        const auto c = (p.slope == 0) ? BiquadFilter::makeFirstOrderLowpass (sr, p.freqHz)
                                      : BiquadFilter::makeLowpass (sr, p.freqHz, 0.70710678118654752);
        for (int ch = 0; ch < kMaxChannels; ++ch)
            band.filters[0][ch].setCoefficients (c);
        return;
    }

    // ---- Notch bands ----
    if (p.shape == 0)
    {
        // Bell: single peaking filter with negative gain.
        band.numStages = 1;
        const auto c = BiquadFilter::makePeaking (sr, p.freqHz, p.q, p.gainDb);
        for (int ch = 0; ch < kMaxChannels; ++ch)
            band.filters[0][ch].setCoefficients (c);
    }
    else
    {
        // Flat-bottom: two peaking filters straddling the centre. The offset
        // scales with the band's bandwidth so the composite keeps roughly the
        // requested depth with steeper walls and a flatter floor.
        band.numStages = 2;
        const double bwOctaves = (2.0 / std::log (2.0)) * std::asinh (1.0 / (2.0 * p.q));
        const double spread    = std::pow (2.0, bwOctaves * 0.20);
        const double stageQ    = p.q * 1.6;
        const double stageGain = p.gainDb * 0.62;

        const auto c0 = BiquadFilter::makePeaking (sr, p.freqHz / spread, stageQ, stageGain);
        const auto c1 = BiquadFilter::makePeaking (sr, p.freqHz * spread, stageQ, stageGain);
        for (int ch = 0; ch < kMaxChannels; ++ch)
        {
            band.filters[0][ch].setCoefficients (c0);
            band.filters[1][ch].setCoefficients (c1);
        }
    }
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
