#include "BiquadFilter.h"
#include <algorithm>
#include <complex>

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

BiquadFilter::Coeffs BiquadFilter::makeHighShelf (double sampleRate, double freq, double gainDb)
{
    freq = clampFreq (sampleRate, freq);

    const double A    = std::pow (10.0, gainDb / 40.0);
    const double w0   = 2.0 * kPi * freq / sampleRate;
    const double cosw = std::cos (w0);
    const double sinw = std::sin (w0);
    const double alpha = 0.5 * sinw * std::sqrt (2.0);   // shelf slope S = 1
    const double k    = 2.0 * std::sqrt (A) * alpha;

    const double a0 = (A + 1.0) - (A - 1.0) * cosw + k;
    Coeffs c;
    c.b0 =  A * ((A + 1.0) + (A - 1.0) * cosw + k) / a0;
    c.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * cosw) / a0;
    c.b2 =  A * ((A + 1.0) + (A - 1.0) * cosw - k) / a0;
    c.a1 =  2.0 * ((A - 1.0) - (A + 1.0) * cosw) / a0;
    c.a2 = ((A + 1.0) - (A - 1.0) * cosw - k) / a0;
    return c;
}

BiquadFilter::Coeffs BiquadFilter::makeBlendedNotch (double sampleRate, double freq, double q, double gainDb)
{
    freq = clampFreq (sampleRate, freq);
    q    = std::clamp (q, 0.05, 100.0);

    const double w0    = 2.0 * kPi * freq / sampleRate;
    const double cosw  = std::cos (w0);
    const double sinw  = std::sin (w0);
    const double alpha = sinw / (2.0 * q);
    const double a0    = 1.0 + alpha;

    // RBJ notch, normalised
    const double nb0 = 1.0 / a0;
    const double nb1 = (-2.0 * cosw) / a0;
    const double nb2 = 1.0 / a0;
    const double a1  = (-2.0 * cosw) / a0;
    const double a2  = (1.0 - alpha) / a0;

    // H = (1-w)·1 + w·N  over the shared denominator. The identity's
    // numerator equals the denominator polynomial, so the sum stays a biquad.
    const double w = 1.0 - std::pow (10.0, gainDb / 20.0);

    Coeffs c;
    c.a1 = a1;
    c.a2 = a2;
    c.b0 = (1.0 - w)       + w * nb0;
    c.b1 = (1.0 - w) * a1  + w * nb1;
    c.b2 = (1.0 - w) * a2  + w * nb2;
    return c;
}

void BiquadFilter::responseAt (const Coeffs& c, double freq, double sampleRate,
                               double& re, double& im)
{
    const std::complex<double> z  = std::polar (1.0, -2.0 * kPi * freq / sampleRate);
    const std::complex<double> z2 = z * z;
    const auto h = (c.b0 + c.b1 * z + c.b2 * z2) / (1.0 + c.a1 * z + c.a2 * z2);
    re = h.real();
    im = h.imag();
}

double BiquadFilter::magnitudeAt (const Coeffs& c, double freq, double sampleRate)
{
    const std::complex<double> z  = std::polar (1.0, -2.0 * kPi * freq / sampleRate);
    const std::complex<double> z2 = z * z;
    const auto num = c.b0 + c.b1 * z + c.b2 * z2;
    const auto den = 1.0 + c.a1 * z + c.a2 * z2;
    return std::abs (num / den);
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
