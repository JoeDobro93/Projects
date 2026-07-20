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
        default: // (matches Ozone): two cascaded ±gain shelf pairs give the
        {        // plateau; drive is iterated so the floor hits the target and
                 // bounded so narrow bands don't smear. Depth beyond what the
                 // shelves reach (below −20 dB: invisible on every display
                 // scale, inaudible in ring level) comes from a centre notch.
            const double bw = (2.0 / std::log (2.0)) * std::asinh (1.0 / (2.0 * p.q));
            const double s = std::pow (2.0, 0.5 * bw + 0.16);
            const double fLo = p.freqHz / s, fHi = p.freqHz * s;

            const double plateau = std::max (p.gainDb, -std::min (20.0, 14.0 * bw));
            const double gpMin = -10.0 * bw;

            // A shallow wide bell (5th stage) flattens the plateau's residual
            // dome. The bell spans the ±0.20·bw probe points itself, so its
            // gain must be scaled by its own shoulder/centre response ratio w,
            // and flattening moves the whole top to ctr − sag/(1−w) — which is
            // therefore the level the drive iteration has to aim at.
            const double fSh = std::pow (2.0, 0.20 * bw);
            const double qc = 1.0 / (2.0 * std::sinh (0.5 * std::log (2.0) * std::max (0.5, 0.8 * bw)));
            const auto probe = BiquadFilter::makePeaking (sampleRate, p.freqHz, qc, -1.0);
            const double w = std::log10 (BiquadFilter::magnitudeAt (probe, p.freqHz * fSh, sampleRate))
                           / std::log10 (BiquadFilter::magnitudeAt (probe, p.freqHz, sampleRate));
            const double flatGain = 1.0 / std::max (0.25, 1.0 - w);

            auto pairDbAt = [&] (double f)
            {
                double m = 0.0;
                for (int i = 0; i < 4; ++i)
                    m += 20.0 * std::log10 (BiquadFilter::magnitudeAt (out[i], f, sampleRate));
                return m;
            };

            double gp = std::max (gpMin, plateau * 0.5);
            double ctr = 0.0, cg = 0.0;
            for (int it = 0; it < 14; ++it)
            {
                // Corner Q sizes each shelf's corner lobe to cancel its own
                // slow S=1 tail, so the response returns to 0 dB within about
                // an octave of the edges instead of drifting for several:
                // 0.80 is the wide-band optimum; narrow bands and hard-driven
                // shelves cancel best slightly higher (any leftover there is
                // a sub-0.4 dB boost, which the cut-depth canvas clips at the
                // zero line, rather than a visible spurious cut).
                const double sq = std::min (0.90, 0.80 + 0.08 * std::max (0.0, 1.0 - bw)
                                                + 0.02 * std::max (0.0, -gp - 6.0));
                out[0] = BiquadFilter::makeHighShelf (sampleRate, fLo,  gp, sq);
                out[1] = BiquadFilter::makeHighShelf (sampleRate, fHi, -gp, sq);
                out[2] = out[0];
                out[3] = out[1];
                ctr = pairDbAt (p.freqHz);
                const double sag = ctr - 0.5 * (pairDbAt (p.freqHz * fSh) + pairDbAt (p.freqHz / fSh));
                cg = std::clamp (-sag * flatGain, -6.5, 6.5);
                // Level of the corrected plateau's shoulders — the target the
                // drive has to hit (with the clamped bell folded in, so the
                // loop stays honest when the correction saturates).
                const double top = ctr - sag + w * cg;
                if (std::abs (top - plateau) < 0.04 || (gp <= gpMin + 0.01 && top > plateau))
                    break;
                gp = std::max (gpMin, gp + (plateau - top) * 0.5);
            }

            int n = 4;
            double centre = ctr;
            if (std::abs (cg) > 0.02)
            {
                out[n++] = BiquadFilter::makePeaking (sampleRate, p.freqHz, qc, cg);
                centre += cg;
            }
            const double rem = p.gainDb - centre;            // depth the shelves didn't reach
            if (rem < -0.05)
                out[n++] = BiquadFilter::makeBlendedNotch (sampleRate, p.freqHz, p.q, rem);
            return n;
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
