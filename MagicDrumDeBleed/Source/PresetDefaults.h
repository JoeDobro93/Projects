#pragma once

/*
    PresetDefaults.h — every factory preset value lives here as a named
    constant. Edit a number, recompile, and the factory preset changes.

    IMPORTANT: Threshold is deliberately NOT part of any preset. Applying a
    preset never touches the user's threshold setting.

    Anything a preset does not specify falls back to the parameter defaults
    (Bandpass trigger filter, Link to K1 on, K2–K5 / LP / HP off,
    Amount 100 %, bypasses off).
*/

namespace presets
{

struct FactoryPreset
{
    const char* name;

    // Trigger filter
    float scFreqHz;
    float scQ;           // width: q = 1 / (2·sinh(ln2·bwOct/2))

    // Gate
    int   lookaheadMs;
    float holdMs;
    float releaseMs;

    // K1 keep band (the only band factory presets use)
    bool  k1On;
    float k1FreqHz;
    float k1Q;
    float k1Ring;        // ring level units 0..20

    // Tail gate
    float tailHoldMs;
    float tailFadeMs;

    // Compressor mode (Ratio and the mode itself are deliberately NOT
    // per-preset: preserved user calibration, like Threshold/Selectivity.)
    float compAttackMs;
    float compReleaseMs;
};

/*  Tuning notes (assumes Learn re-centres scFreq / K1 freq on the drum):
    - Kick: fundamental pitch-glides downward as it decays, so K1 stays wide
      (Q 2.5 ≈ 26 Hz at 65 Hz); trigger Width opens to 0.75 oct because a
      narrow bandpass at 60 Hz rings ~15 ms and delays detection.
    - Snare: Width narrows to 0.41 oct to shave tom overlap (filter ring is
      negligible at 200 Hz); K1 Q 8 survives mid-session detune; tail fade
      600 ms matches wire decay without holding hat bleed open a full second.
    - Toms: Width 0.6 oct (the old 2-oct focus predates Learn and reached
      into kick and snare fundamentals); long hold/release/tail because toms
      sustain longest — 100 ms tail fade chopped the ring being preserved. */
//                          name       scFreq  scQ    look hold  rel   k1On  k1Freq k1Q   ring  tHold tFade   cAtk  cRel
static constexpr FactoryPreset kDefault { "Default", 200.0f, 2.871f, 5,  7.0f,  5.0f, true,  200.0f,  1.0f,  9.8f, 120.0f,  100.0f, 0.1f, 10.0f };
static constexpr FactoryPreset kKick    { "Kick",     70.0f, 1.9f,   5, 50.0f, 60.0f, true,   70.0f,  2.5f, 12.0f, 180.0f,  250.0f, 0.1f, 10.0f };   // 0.75 oct focus
static constexpr FactoryPreset kSnare   { "Snare",   220.0f, 3.5f,   5, 10.0f, 20.0f, true,  220.0f,  8.0f, 16.0f, 200.0f,  600.0f, 0.1f, 10.0f };   // 0.41 oct focus
static constexpr FactoryPreset kToms    { "Toms",    150.0f, 2.4f,   5, 25.0f, 50.0f, true,  150.0f,  2.0f, 14.0f, 400.0f, 1000.0f, 0.1f, 10.0f };   // 0.60 oct focus

static constexpr FactoryPreset kFactoryPresets[] = { kDefault, kKick, kSnare, kToms };
static constexpr int kNumFactoryPresets = (int) (sizeof (kFactoryPresets) / sizeof (kFactoryPresets[0]));

} // namespace presets
