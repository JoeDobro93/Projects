#include "SidechainFilter.h"

namespace mdd
{

void SidechainFilter::prepare (double sampleRate)
{
    sr = sampleRate;
    currentFreq = -1.0; // force coefficient refresh
    currentQ    = -1.0;
    filter.reset();
}

void SidechainFilter::reset()
{
    filter.reset();
}

void SidechainFilter::setParameters (double freqHz, double q)
{
    if (freqHz == currentFreq && q == currentQ)
        return;

    currentFreq = freqHz;
    currentQ    = q;
    filter.setCoefficients (BiquadFilter::makeBandpass (sr, freqHz, q));
}

} // namespace mdd
