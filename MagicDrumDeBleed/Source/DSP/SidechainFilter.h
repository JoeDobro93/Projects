#pragma once

/*
    SidechainFilter — a mono double-precision bandpass biquad used on the
    compressor's detector signal only. It never touches the audio path.
*/

#include "BiquadFilter.h"

namespace mdd
{

class SidechainFilter
{
public:
    void prepare (double sampleRate);
    void reset();

    // Recomputes coefficients only when a value actually changed.
    void setParameters (double freqHz, double q);

    inline double processSample (double x) noexcept   { return filter.processSample (x); }

private:
    BiquadFilter filter;
    double sr = 44100.0;
    double currentFreq = -1.0, currentQ = -1.0;
};

} // namespace mdd
