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
                                      double sampleRate, BiquadFilter::Coeffs (&out)[kMaxStages])
{
    if (bandKind == 0 || bandKind == 1)
    {
        // Butterworth cascades: slope index 0..4 = 6/12/24/36/48 dB/oct.
        static const double kQ2[] = { 0.70710678 };
        static const double kQ4[] = { 0.54119610, 1.30656296 };
        static const double kQ6[] = { 0.51763809, 0.70710678, 1.93185165 };
        static const double kQ8[] = { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };
        static const double* kQs[] = { nullptr, kQ2, kQ4, kQ6, kQ8 };

        const int idx = juce::jlimit (0, 4, p.slope);
        if (idx == 0)
        {
            out[0] = bandKind == 0 ? BiquadFilter::makeFirstOrderHighpass (sampleRate, p.freqHz)
                                   : BiquadFilter::makeFirstOrderLowpass  (sampleRate, p.freqHz);
            return 1;
        }
        for (int s = 0; s < idx; ++s)
            out[s] = bandKind == 0 ? BiquadFilter::makeHighpass (sampleRate, p.freqHz, kQs[idx][s])
                                   : BiquadFilter::makeLowpass  (sampleRate, p.freqHz, kQs[idx][s]);
        return idx;
    }

    // ---- Keep bands: Ozone-style cut shapes ----
    switch (p.shape)
    {
        case 0:  // Bell — classic RBJ peaking cut, Q as set
            out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q, p.gainDb);
            break;

        case 1:  // Proportional Q — bell whose width tightens as the cut deepens.
        {        // 3 dB reference calibrated against Ozone: broader than Bell
                 // only for very shallow cuts, distinctly narrower above.
            const double t = std::clamp (std::pow (std::abs (p.gainDb) / 3.0, 0.7), 0.4, 4.0);
            out[0] = BiquadFilter::makePeaking (sampleRate, p.freqHz, p.q * t, p.gainDb);
            break;
        }

        case 2:  // Band Shelf — a genuinely flat-topped cut between two edges
        default: // (matches Ozone): two cascaded ±gain S=1 shelf pairs give the
        {        // plateau; drive is iterated so the floor hits the target and
                 // bounded so narrow bands don't smear. Depth beyond what the
                 // shelves reach (below −20 dB: invisible on every display
                 // scale, inaudible in ring level) comes from a centre notch.
            const double bw = (2.0 / std::log (2.0)) * std::asinh (1.0 / (2.0 * p.q));
            const double s = std::pow (2.0, 0.5 * bw);
            const double fLo = p.freqHz / s, fHi = p.freqHz * s;

            const double plateau = std::max (p.gainDb, -std::min (20.0, 14.0 * bw));
            const double gpMin = -10.0 * bw;
            double gp = std::max (gpMin, plateau * 0.5);
            double ctr = 0.0;
            for (int it = 0; it < 10; ++it)
            {
                out[0] = BiquadFilter::makeHighShelf (sampleRate, fLo,  gp);
                out[1] = BiquadFilter::makeHighShelf (sampleRate, fHi, -gp);
                out[2] = out[0];
                out[3] = out[1];
                ctr = 0.0;
                for (int i = 0; i < 4; ++i)
                    ctr += 20.0 * std::log10 (BiquadFilter::magnitudeAt (out[i], p.freqHz, sampleRate));
                if (ctr < plateau + 0.2 || gp <= gpMin + 0.01)
                    break;
                gp = std::max (gpMin, gp + (plateau - ctr) * 0.35);
            }

            const double rem = p.gainDb - ctr;               // depth the shelves didn't reach
            if (rem < -0.1)
            {
                out[4] = BiquadFilter::makeBlendedNotch (sampleRate, p.freqHz, p.q, rem);
                return 5;
            }
            return 4;
        }
    }
    return 1;
}

void EQProcessor::updateBandCoefficients (int bandIndex)
{
    auto& band = bands[bandIndex];
    const int kind = bandIndex == kBandHPF ? 0 : (bandIndex == kBandLPF ? 1 : 2);

    BiquadFilter::Coeffs coeffs[kMaxStages];
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
