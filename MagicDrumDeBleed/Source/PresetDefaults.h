#pragma once

/*
    PresetDefaults.h — every factory preset value lives here as a named
    constant. Edit a number, recompile, and the factory preset changes.

    IMPORTANT: Threshold is deliberately NOT part of any preset. Applying a
    preset never touches the user's threshold setting.

    Anything a preset does not specify falls back to the parameter defaults
    (Bandpass trigger filter, Link to K1 on, K2–K5 / LOWS / HIGHS off,
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
};

//                          name       scFreq  scQ    look hold  rel   k1On  k1Freq k1Q   ring  tHold tFade
static constexpr FactoryPreset kDefault { "Default", 200.0f, 2.871f, 5,  7.0f,  5.0f, true,  200.0f,  1.0f,  9.8f, 120.0f,  100.0f };
static constexpr FactoryPreset kKick    { "Kick",     70.0f, 2.871f, 5, 40.0f, 50.0f, true,   70.0f, 10.0f, 10.0f, 120.0f,  100.0f };
static constexpr FactoryPreset kSnare   { "Snare",   220.0f, 2.871f, 5,  7.0f, 15.0f, true,  220.0f, 20.0f, 20.0f, 200.0f, 1000.0f };
static constexpr FactoryPreset kToms    { "Toms",    150.0f, 0.667f, 5,  7.0f,  5.0f, true,  150.0f,  1.0f, 10.0f, 300.0f,  100.0f };   // 2.0 oct focus

static constexpr FactoryPreset kFactoryPresets[] = { kDefault, kKick, kSnare, kToms };
static constexpr int kNumFactoryPresets = (int) (sizeof (kFactoryPresets) / sizeof (kFactoryPresets[0]));

} // namespace presets
