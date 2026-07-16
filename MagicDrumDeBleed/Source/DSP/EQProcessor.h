#pragma once

/*
    EQProcessor — the 7-band double-precision IIR EQ on the parallel path.

    Band indices:
        0        HPF  (6 or 12 dB/oct)
        1        LPF  (6 or 12 dB/oct)
        2 .. 6   Notch bands 1..5 (bell / flat-bottom, negative gain only)

    Frequencies notched OUT of the parallel signal are the frequencies that
    survive cancellation in the final output — i.e. what the listener keeps.

    Shapes (both plain biquad topologies, no linear phase / FIR):
      - Bell:        one RBJ peaking filter.
      - Flat-bottom: two cascaded RBJ peaking filters straddling the centre
                     frequency — steeper walls and a flatter, wider rejection
                     floor at approximately the same depth as the bell.
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include "BiquadFilter.h"

namespace mdd
{

struct BandParams
{
    bool   enabled = false;
    double freqHz  = 1000.0;
    double q       = 4.0;      // notches only
    double gainDb  = -24.0;    // notches only
    int    shape   = 0;        // notches: 0 = bell, 1 = flat-bottom
    int    slope   = 1;        // HPF/LPF: 0 = 6 dB/oct, 1 = 12 dB/oct

    bool operator== (const BandParams& o) const noexcept
    {
        return enabled == o.enabled && exactlyEqual (freqHz, o.freqHz) && exactlyEqual (q, o.q)
            && exactlyEqual (gainDb, o.gainDb) && shape == o.shape && slope == o.slope;
    }
    bool operator!= (const BandParams& o) const noexcept   { return ! (*this == o); }
};

class EQProcessor
{
public:
    static constexpr int kNumBands   = 7;
    static constexpr int kBandHPF    = 0;
    static constexpr int kBandLPF    = 1;
    static constexpr int kFirstNotch = 2;
    static constexpr int kMaxChannels = 2;

    void prepare (double sampleRate, int numChannels);
    void reset();

    // Recomputes coefficients only when something actually changed.
    void setBandParameters (int bandIndex, const BandParams& params);

    void process (juce::AudioBuffer<double>& audio, int numSamples);

    // Coefficient design shared with the UI (EQ-curve drawing), so the curve
    // is always computed from the same maths as the audio path.
    // bandKind: 0 = HPF, 1 = LPF, 2 = notch. Returns the number of stages.
    static int computeCoefficients (const BandParams& params, int bandKind,
                                    double sampleRate, BiquadFilter::Coeffs (&out)[2]);

private:
    void updateBandCoefficients (int bandIndex);

    struct Band
    {
        BandParams params;
        int numStages = 1;                                  // 1 or 2 cascaded biquads
        BiquadFilter filters[2][kMaxChannels];              // [stage][channel]
    };

    Band bands[kNumBands];
    double sr = 44100.0;
    int channels = 2;
};

} // namespace mdd
