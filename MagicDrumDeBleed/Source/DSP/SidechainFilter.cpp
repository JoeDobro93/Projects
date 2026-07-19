#include "SidechainFilter.h"

namespace mdd
{

void SidechainFilter::prepare (double sampleRate)
{
    sr = sampleRate;
    curType = -1; curSlope = -1; curFreq = -1.0; curQ = -1.0;   // force refresh
    reset();
}

void SidechainFilter::reset()
{
    for (auto& s : stages)
        s.reset();
}

void SidechainFilter::setParameters (int type, double freqHz, double q, double slopeDb)
{
    const int slope = juce::jlimit (6, 24, (int) std::lround (slopeDb / 6.0) * 6);
    if (type == curType && slope == curSlope
        && exactlyEqual (freqHz, curFreq) && exactlyEqual (q, curQ))
        return;

    const bool structural = type != curType || slope != curSlope;
    curType = type; curSlope = slope; curFreq = freqHz; curQ = q;

    if (type == 2)                       // bandpass (constant 0 dB peak)
    {
        numStages = 1;
        stages[0].setCoefficients (BiquadFilter::makeBandpass (sr, freqHz, q));
    }
    else
    {
        const bool hp = type == 0;
        auto first  = [&] { return hp ? BiquadFilter::makeFirstOrderHighpass (sr, freqHz)
                                      : BiquadFilter::makeFirstOrderLowpass (sr, freqHz); };
        auto second = [&] (double sq) { return hp ? BiquadFilter::makeHighpass (sr, freqHz, sq)
                                                  : BiquadFilter::makeLowpass (sr, freqHz, sq); };
        switch (slope)                   // Butterworth-aligned cascades
        {
            case 6:  numStages = 1; stages[0].setCoefficients (first()); break;
            case 12: numStages = 1; stages[0].setCoefficients (second (0.70710678)); break;
            case 18: numStages = 2; stages[0].setCoefficients (first());
                                    stages[1].setCoefficients (second (1.0)); break;
            default: numStages = 2; stages[0].setCoefficients (second (0.54119610));
                                    stages[1].setCoefficients (second (1.30656296)); break;
        }
    }

    if (structural)
        reset();
}

} // namespace mdd
