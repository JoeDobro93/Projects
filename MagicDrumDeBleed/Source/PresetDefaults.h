#pragma once

/*
    PresetDefaults.h — every factory preset value lives here as a named
    constant. Edit a number, recompile, and the factory preset changes.

    IMPORTANT: Threshold is deliberately NOT part of any preset. Applying a
    preset never touches the user's threshold setting.

    Anything a preset does not specify falls back to the parameter defaults:
      - Notch bands 2–5: off (freq/Q/gain at their parameter defaults)
      - LPF: 20 kHz, off
      - Sidechain filter: enabled
      - All notch shapes: Bell
      - HPF slope / LPF slope: 12 dB/oct
      - Monitoring: Normal, bypasses off
*/

namespace presets
{

struct NotchSetting
{
    bool  enabled;
    float freqHz;
    float q;
    float gainDb;
};

struct FactoryPreset
{
    const char*  name;

    // Sidechain detector
    float scFreqHz;      // bandpass centre
    float scQ;
    bool  scEnabled;

    // Compressor (fixed-reduction gate)
    float rmsWindowMs;
    float holdMs;
    float releaseMs;
    float reductionDb;   // fixed gain reduction applied when engaged
    int   lookaheadMs;

    // EQ (parallel path)
    bool  hpfEnabled;
    float hpfFreqHz;
    NotchSetting notch1; // factory presets only ever use notch band 1

    // Output
    float intensityPercent;
};

//                            name         scFreq  scQ  scOn   rms   hold  rel   reduct  look  hpfOn hpfFreq  { n1On, n1Freq, n1Q, n1Gain }  intensity
static constexpr FactoryPreset kDefault   { "Default",   200.0f, 1.5f, true, 10.0f, 20.0f, 100.0f, -24.0f, 5,  true, 800.0f, { false, 200.0f, 4.0f, -24.0f },  100.0f };
static constexpr FactoryPreset kKick      { "Kick",       65.0f, 1.0f, true, 10.0f, 40.0f, 200.0f, -48.0f, 5,  true, 120.0f, { false, 200.0f, 4.0f, -24.0f },  100.0f };
static constexpr FactoryPreset kSnare     { "Snare",     200.0f, 1.5f, true,  5.0f, 20.0f, 100.0f, -24.0f, 5,  true, 800.0f, { true,  200.0f, 6.0f, -18.0f },   85.0f };
static constexpr FactoryPreset kHiTom     { "Hi Tom",    350.0f, 1.5f, true,  8.0f, 25.0f, 120.0f, -30.0f, 5,  true, 600.0f, { false, 200.0f, 4.0f, -24.0f },   90.0f };
static constexpr FactoryPreset kMidTom    { "Mid Tom",   220.0f, 1.5f, true, 15.0f, 30.0f, 150.0f, -30.0f, 5,  true, 400.0f, { false, 200.0f, 4.0f, -24.0f },   90.0f };
static constexpr FactoryPreset kFloorTom  { "Floor Tom", 100.0f, 1.2f, true, 25.0f, 40.0f, 200.0f, -36.0f, 5,  true, 250.0f, { false, 200.0f, 4.0f, -24.0f },   95.0f };

static constexpr FactoryPreset kFactoryPresets[] = { kDefault, kKick, kSnare, kHiTom, kMidTom, kFloorTom };
static constexpr int kNumFactoryPresets = (int) (sizeof (kFactoryPresets) / sizeof (kFactoryPresets[0]));

} // namespace presets
