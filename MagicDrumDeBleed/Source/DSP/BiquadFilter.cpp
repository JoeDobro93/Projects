#include "BiquadFilter.h"
#include <algorithm>

namespace mdd
{

static constexpr double kPi = 3.141592653589793238462643383279502884;

double BiquadFilter::clampFreq (double sampleRate, double freq) noexcept
{
    // Keep well below Nyquist so tan/sin designs stay finite and stable.
    return std::clamp (freq, 10.0, sampleRate * 0.49);
}

BiquadFilter::Coeffs BiquadFilter::makePeaking (double sampleRate, double freq, double q, double gainDb)
{
    freq = clampFreq (sampleRate, freq);
    q    = std::clamp (q, 0.05, 100.0);

    const double A     = std::pow (10.0, gainDb / 40.0);
    const double w0    = 2.0 * kPi * freq / sampleRate;
    const double cosw  = std::cos (w0);
    const double sinw  = std::sin (w0);
    const double alpha = sinw / (2.0 * q);

    const double a0 = 1.0 + alpha / A;
    Coeffs c;
    c.b0 = (1.0 + alpha * A) / a0;
    c.b1 = (-2.0 * cosw)     / a0;
    c.b2 = (1.0 - alpha * A) / a0;
    c.a1 = (-2.0 * cosw)     / a0;
    c.a2 = (1.0 - alpha / A) / a0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeBandpass (double sampleRate, double freq, double q)
{
    freq = clampFreq (sampleRate, freq);
    q    = std::clamp (q, 0.05, 100.0);

    const double w0    = 2.0 * kPi * freq / sampleRate;
    const double cosw  = std::cos (w0);
    const double sinw  = std::sin (w0);
    const double alpha = sinw / (2.0 * q);

    const double a0 = 1.0 + alpha;
    Coeffs c;
    c.b0 =  alpha        / a0;
    c.b1 =  0.0;
    c.b2 = -alpha        / a0;
    c.a1 = (-2.0 * cosw) / a0;
    c.a2 = (1.0 - alpha) / a0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeLowpass (double sampleRate, double freq, double q)
{
    freq = clampFreq (sampleRate, freq);
    q    = std::clamp (q, 0.05, 100.0);

    const double w0    = 2.0 * kPi * freq / sampleRate;
    const double cosw  = std::cos (w0);
    const double sinw  = std::sin (w0);
    const double alpha = sinw / (2.0 * q);

    const double a0 = 1.0 + alpha;
    Coeffs c;
    c.b0 = ((1.0 - cosw) * 0.5) / a0;
    c.b1 =  (1.0 - cosw)        / a0;
    c.b2 = ((1.0 - cosw) * 0.5) / a0;
    c.a1 = (-2.0 * cosw)        / a0;
    c.a2 = (1.0 - alpha)        / a0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeHighpass (double sampleRate, double freq, double q)
{
    freq = clampFreq (sampleRate, freq);
    q    = std::clamp (q, 0.05, 100.0);

    const double w0    = 2.0 * kPi * freq / sampleRate;
    const double cosw  = std::cos (w0);
    const double sinw  = std::sin (w0);
    const double alpha = sinw / (2.0 * q);

    const double a0 = 1.0 + alpha;
    Coeffs c;
    c.b0 =  ((1.0 + cosw) * 0.5) / a0;
    c.b1 = -((1.0 + cosw))       / a0;
    c.b2 =  ((1.0 + cosw) * 0.5) / a0;
    c.a1 =  (-2.0 * cosw)        / a0;
    c.a2 =  (1.0 - alpha)        / a0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeFirstOrderLowpass (double sampleRate, double freq)
{
    freq = clampFreq (sampleRate, freq);
    const double w  = std::tan (kPi * freq / sampleRate);
    const double a0 = 1.0 + w;

    Coeffs c;
    c.b0 = w / a0;
    c.b1 = w / a0;
    c.b2 = 0.0;
    c.a1 = (w - 1.0) / a0;
    c.a2 = 0.0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeFirstOrderHighpass (double sampleRate, double freq)
{
    freq = clampFreq (sampleRate, freq);
    const double w  = std::tan (kPi * freq / sampleRate);
    const double a0 = 1.0 + w;

    Coeffs c;
    c.b0 =  1.0 / a0;
    c.b1 = -1.0 / a0;
    c.b2 =  0.0;
    c.a1 = (w - 1.0) / a0;
    c.a2 = 0.0;
    return c;
}

} // namespace mdd
