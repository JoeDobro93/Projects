#pragma once
/*  SidechainFilter — mono detector filter. Three types:
      0 = High Pass, 1 = Low Pass (slope 6/12/18/24 dB/oct, cascaded
          Butterworth sections), 2 = Bandpass (Q-controlled, the default).
    Detector path only — never touches the audio. Double precision. */
#include "BiquadFilter.h"
#include <juce_core/juce_core.h>

namespace mdd
{
class SidechainFilter
{
public:
    void prepare (double sampleRate);
    void reset();

    // Recomputes coefficients only when something changed.
    // type: 0 HP, 1 LP, 2 BP. q used by BP; slopeDb (6/12/18/24) by HP/LP.
    void setParameters (int type, double freqHz, double q, double slopeDb);

    inline double processSample (double x) noexcept
    {
        for (int i = 0; i < numStages; ++i)
            x = stages[i].processSample (x);
        return x;
    }

private:
    BiquadFilter stages[2];
    int numStages = 1;
    double sr = 44100.0;
    int    curType = -1, curSlope = -1;
    double curFreq = -1.0, curQ = -1.0;
};
} // namespace mdd
