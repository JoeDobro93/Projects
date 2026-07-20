#pragma once

/*
    BiquadFilter — double-precision transposed direct form II biquad with
    RBJ (Audio EQ Cookbook) coefficient designs. Used for every filter in
    the plugin: sidechain bandpass, EQ notches/shelves and HPF/LPF.

    All maths is double precision so high-Q notches (Q up to 30) remain
    stable at any sample rate.
*/

#include <cmath>

namespace mdd
{

// Intentional exact comparison, used to detect changes in cached parameter
// values (both sides always originate from the same parameter read).
inline bool exactlyEqual (double a, double b) noexcept   { return ! (a < b) && ! (b < a); }

class BiquadFilter
{
public:
    struct Coeffs
    {
        // Normalised (a0 == 1)
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };

    void setCoefficients (const Coeffs& c) noexcept   { coeffs = c; }
    const Coeffs& getCoefficients() const noexcept    { return coeffs; }

    void reset() noexcept                             { z1 = z2 = 0.0; }

    inline double processSample (double x) noexcept
    {
        const double y = coeffs.b0 * x + z1;
        z1 = coeffs.b1 * x - coeffs.a1 * y + z2;
        z2 = coeffs.b2 * x - coeffs.a2 * y;
        return y;
    }

    // ---- RBJ cookbook designs (all frequencies clamped to a safe range) ----
    static Coeffs makePeaking          (double sampleRate, double freq, double q, double gainDb);
    static Coeffs makeBandpass         (double sampleRate, double freq, double q);            // constant 0 dB peak gain
    static Coeffs makeLowpass          (double sampleRate, double freq, double q);
    static Coeffs makeHighpass         (double sampleRate, double freq, double q);
    static Coeffs makeFirstOrderLowpass  (double sampleRate, double freq);
    static Coeffs makeFirstOrderHighpass (double sampleRate, double freq);
    static Coeffs makeHighShelf          (double sampleRate, double freq, double gainDb);   // S = 1

    // Finite-depth flat-bottom band-reject: a dry/notch crossfade folded into
    // one biquad. Floor sits exactly at gainDb, walls are steeper than a bell.
    static Coeffs makeBlendedNotch (double sampleRate, double freq, double q, double gainDb);

    // |H(e^jw)| at the given frequency — used by the UI to draw the EQ curve
    // from the exact same coefficients the audio path runs.
    static double magnitudeAt (const Coeffs& c, double freq, double sampleRate);

    // Complex H(e^jw): needed for |1 − H| (the "kept" transfer curve), where
    // phase matters. re/im out-params to keep <complex> out of this header.
    static void responseAt (const Coeffs& c, double freq, double sampleRate,
                            double& re, double& im);

private:
    static double clampFreq (double sampleRate, double freq) noexcept;

    Coeffs coeffs;
    double z1 = 0.0, z2 = 0.0;
};

} // namespace mdd
