#pragma once

/*
    EQProcessor — the 7-band double-precision IIR EQ on the parallel path.

    Band indices:
        0        HPF  (6/12/24/36/48 dB/oct)
        1        LPF  (6/12/24/36/48 dB/oct)
        2 .. 6   Notch bands 1..5 (bell / flat-bottom, negative gain only)

    Frequencies notched OUT of the parallel signal are the frequencies that
    survive cancellation in the final output — i.e. what the listener keeps.

    Shapes (all plain biquad topologies, no linear phase / FIR):
      - Bell:           one RBJ peaking filter.
      - Proportional Q: RBJ peaking with Q scaled by cut depth — wide when
                        shallow, surgical when deep (iZotope-style).
      - Band Shelf:     dry/notch blended band-reject — an exactly-flat floor
                        at the set depth with steep, contained walls.
*/

#include <juce_audio_basics/juce_audio_basics.h>
#include "BiquadFilter.h"

namespace mdd
{

// Ring level UI unit (0 = no ring, 20 = max) → internal parallel-path cut.
inline double ringToGainDb (double units) noexcept   { return -2.4 * units; }
inline double gainDbToRing (double gainDb) noexcept  { return -gainDb / 2.4; }

struct BandParams
{
    bool   enabled = false;
    double freqHz  = 1000.0;
    double q       = 1.0;      // keep bands only
    double gainDb  = -24.0;    // keep bands only
    int    shape   = 0;        // keep bands: 0 bell, 1 proportional-Q, 2 band shelf
    int    slope   = 1;        // HPF/LPF: index into {6,12,24,36,48} dB/oct

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
    static constexpr int kMaxStages  = 5;               // band shelf: 2 shelf pairs + centre notch

    void prepare (double sampleRate, int numChannels);
    void reset();

    // Recomputes coefficients only when something actually changed.
    void setBandParameters (int bandIndex, const BandParams& params);

    void process (juce::AudioBuffer<double>& audio, int numSamples);

    // Coefficient design shared with the UI (EQ-curve drawing), so the curve
    // is always computed from the same maths as the audio path.
    // bandKind: 0 = HPF, 1 = LPF, 2 = notch. Returns the number of stages.
    static int computeCoefficients (const BandParams& params, int bandKind,
                                    double sampleRate, BiquadFilter::Coeffs (&out)[kMaxStages]);

private:
    void updateBandCoefficients (int bandIndex);

    struct Band
    {
        BandParams params;
        int numStages = 1;                                  // 1..kMaxStages cascaded biquads
        BiquadFilter filters[kMaxStages][kMaxChannels];     // [stage][channel]
    };

    Band bands[kNumBands];
    double sr = 44100.0;
    int channels = 2;
};

} // namespace mdd
