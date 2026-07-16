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

    // ---- Notch bands ----
    if (p.shape == 0)
    {
        // Bell: single peaking filter with negative gain.
        out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q, p.gainDb);
        return 1;
    }

    // Flat-bottom: two peaking filters straddling the centre. The offset
    // scales with the band's bandwidth so the composite keeps roughly the
    // requested depth with steeper walls and a flatter floor.
    const double bwOctaves = (2.0 / std::log (2.0)) * std::asinh (1.0 / (2.0 * p.q));
    const double spread    = std::pow (2.0, bwOctaves * 0.20);
    const double stageQ    = p.q * 1.6;
    const double stageGain = p.gainDb * 0.62;

    out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz / spread, stageQ, stageGain);
    out[1] = BiquadFilter::makePeaking (sampleRate, p.freqHz * spread, stageQ, stageGain);
    return 2;
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
