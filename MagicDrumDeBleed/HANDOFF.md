# Magic Drum De-Bleed — Engineering Handoff

Context doc for continuing development in a fresh session. Dense by design.

## 1. What it is
JUCE 8 audio plugin (VST3 + AU + Standalone), CMake, C++17. Removes mic bleed
from close-miked drum tracks via **parallel null-cancellation**. All DSP is
double precision; works at any sample rate. Repo: `JoeDobro93/projects`, path
`MagicDrumDeBleed/`, dev branch `claude/audio-plugin-vst3-au-6tn2wn`.
Namespace for all DSP/UI helpers is `mdd` (NOT `dsp` — collides with `juce::dsp`).

## 2. Signal flow
```
input ─┬─ dry delay (= lookahead samples) ──────────────────────────┐
       ├─ parallel: [copy] ─ comp(gate) ─┬─ preEq ─ EQ ─┬─ EQ-gate blend ─┤
       │                                 └──────────────┘                ▼
       │                                                     out = dry − intensity·parallel
       └─ detector (mono mix, un-delayed, optional SC bandpass) ─ RMS ─ threshold
```
- **Parallel path is polarity-flipped**; the flip is folded into the final sum
  as a subtraction (`out = dry − intensity·parallel`). With comp+EQ bypassed and
  intensity=100%, output nulls to digital silence (built-in null test).
- **Compressor = fixed-reduction gate** (NOT ratio). When detector RMS > threshold,
  a fixed dB reduction is applied to the parallel path (engage is instant within
  the lookahead window via an attack coeff sized to lookahead/5; then hold, then
  exp release to 0 dB). Ducking the parallel path lets the drum transient through
  (it stops cancelling) while steady bleed keeps cancelling.
- **EQ notches** cut freqs *out of the parallel path* → those freqs are **preserved**
  in the output (they don't get cancelled). Used to keep drum body/ring.
- **EQ Gate (Post EQ Gate)**: per-sample 0..1 envelope from the SAME detector
  (threshold/lookahead/RMS/SC). env=1 → hear the EQ'd parallel (drum sounding);
  env=0 → parallel blends to the raw pre-EQ inverted copy → full cancellation.
  Attack is INSTANT (env=1 the moment RMS>threshold, mirrors GR); only hold/release
  differ from the main gate. Purpose: notches preserve decay during hits, but would
  rumble between hits — the gate cancels fully when nothing plays.
- Latency = lookahead samples, reported via `setLatencySamples()` on change.
- Learn: FFT captures input, skips first ~5 ms, finds dominant bin below Learn
  Ceiling, sets SC Frequency.

## 3. File map (`Source/`)
- `PluginProcessor.h/.cpp` — params (APVTS), path split/sum, latency, state, meter
  feeds, solo, learn. `ParamIDs` namespace holds all IDs.
- `PluginEditor.h/.cpp` — `StyledLookAndFeel` (knob draw, themed colours,
  `createSliderTextBox` for tall value boxes) + editor: hosts AdvancedView/SimpleView,
  **locked aspect ratio + uniform scale transform**, view switching, theme toggle.
- `DSP/BiquadFilter.h/.cpp` — RBJ biquads (peaking/bandpass/LP/HP/1st-order),
  `makeBlendedNotch` (finite-depth flat-bottom band-reject, single biquad),
  `magnitudeAt` (for UI curve), `mdd::exactlyEqual`.
- `DSP/CompressorProcessor.h/.cpp` — gate + RMS detector + lookahead delay
  (`MonoDelay`) + eq-gate envelope. `process(buf, detector, n, applyGain, eqGateEnvOut)`.
- `DSP/SidechainFilter.h/.cpp` — detector bandpass.
- `DSP/EQProcessor.h/.cpp` — 7-band IIR. `computeCoefficients(params,kind,sr,out[2])`
  is static & shared with UI curve. kind: 0=HPF,1=LPF,2=notch.
- `DSP/LearnAnalyzer.h/.cpp` — FFT dominant-freq (spinlock, audio-thread safe).
- `UI/UIHelpers.h` — `LabelledKnob`, `SlideSwitch` (Input/Output), `CurveIconButton`
  (notch shape icons).
- `UI/CompressorPanel.h/.cpp` — top panel + `InputLevelMeter` (draggable threshold)
  & `GainReductionMeter` (both reused elsewhere; caption is a ctor arg).
- `UI/EQPanel.h/.cpp` — spectrum (`SpectrumDisplay`), band handles (`BandOverlay`),
  band strip, Post EQ Gate cluster.
- `UI/OutputStrip.h/.cpp` — Preview Processed Signal, Simple, Theme buttons.
- `UI/AdvancedView.h/.cpp` — full UI + right column (vertical Intensity + OUT meter).
- `UI/SimpleView.h/.cpp` — IN, GR, Intensity(vert), OUT, Advanced button.
- `UI/PresetBrowser.h/.cpp` — factory + user presets (threshold excluded).
- `ThemeColors.h` — ALL colours, dark+light `Palette`. `bandColours[7]`: 0=HPF,1=LPF,2-6=N1-5.
- `PresetDefaults.h` — factory preset table.
- `Licensing/` — donationware nag, compile-gated by `ENABLE_DONATION_NAG` (0 default).

## 4. Parameters (APVTS ID → range / default / unit)
Value text carries units (e.g. "-24.0 dB", "1.20 kHz", "5 ms", "100.0 %").
Freq params are log-skewed; `logHzRange(min,max)`.

**Transient Detection (gate):**
| ID | range | default | unit |
|---|---|---|---|
| threshold | -60..0 | -20 | dB |
| reduction | -96..0 | **-96** | dB |
| lookahead | 1..20 (int) | 5 | ms |
| rmsWindow | 1..100 | 10 | ms |
| hold | 0..500 | 20 | ms |
| release | 5..1000 | 100 | ms |

**Sidechain detector:**
| scEnable | bool | true | (Filter On) |
| scFreq | 30..2000 | 200 | Hz |
| scQ | 0.3..12 | 1.5 | |
| learnCeiling | 200..2000 | 1000 | Hz (no knob; hidden, DAW-list only) |

**EQ — HPF/LPF:**
| hpfOn | bool | true | |
| hpfFreq | 20..15000 | 800 | Hz |
| hpfSlope | choice {6 dB/oct,12 dB/oct} | 1 (=12) | |
| lpfOn | bool | false | |
| lpfFreq | 50..20000 | 20000 | Hz |
| lpfSlope | choice | 1 (=12) | |

**EQ — Notch bands i=1..5 (IDs notch{N}On/Freq/Q/Gain/Shape):**
| notchOn | bool | false | |
| notchFreq | 20..20000 | 100/200/400/800/1600 (per band) | Hz |
| notchQ | 0.5..30 | 4.0 | |
| notchGain | -48..0 | -24 | dB |
| notchShape | choice {Bell,Flat,Band} | 0 (Bell) | |

- **Bell** = RBJ peaking (rounded, wide skirts).
- **Flat** = `makeBlendedNotch(freq, q*0.6, gain)` — flat-bottom band-reject, 1 biquad.
- **Band** = 2 straddled blended notches (freq/s & freq·s, s=2^(bwOct·0.10), q·0.8,
  gain·0.56 each) — CONTAINED: steep walls, ~0 dB one octave out. Returns 2 stages.

**Post EQ Gate:**
| eqGateOn | bool | true | (UI shows inverted "Bypass") |
| eqGateHold | 0..2000 | 0 | ms |
| eqGateRelease | 5..5000 | 1000 | ms |

**Output/monitor:**
| intensity | 0..100 | 100 | % |
| monitorMode | choice {Normal,Sidechain,Processing,Delta} | 0 | |
| compBypass | bool | false | |
| eqBypass | bool | false | |

`monitorMode`: Normal=final out; Sidechain=solo detector (SC "Preview"); Processing=
solo parallel un-flipped, dry muted (bottom "Preview Processed Signal"); Delta exists
in enum but has no button. Only one preview active; both preview buttons toggle back
to Normal.

## 5. Factory presets (`PresetDefaults.h`)
Threshold NEVER in presets. Unlisted params → param defaults (EQ gate: on/0/1000).
All presets reduction = **-96**. Fields: scFreq, scQ, scEnabled, rms, hold, release,
reduction, lookahead, hpfOn, hpfFreq, notch1{on,freq,q,gain}, intensity.
- Default: 200,1.5,on,10,20,100,-96,5,on,800,{off},100%
- Kick: 65,1.0,on,10,40,200,-96,5,on,120,{off},100%
- Snare: 200,1.5,on,5,20,100,-96,5,on,800,{on,200,6.0,-18},85%
- Hi Tom: 350,1.5,on,8,25,120,-96,5,on,600,{off},90%
- Mid Tom: 220,1.5,on,15,30,150,-96,5,on,400,{off},90%
- Floor Tom: 100,1.2,on,25,40,200,-96,5,on,250,{off},95%

## 6. UI layout
**Scaling model (critical):** each view lays out at a FIXED logical size; the editor
sets `view.setBounds(0,0,logicalW,logicalH)` + `setTransform(scale)` where scale =
min(w/lw, h/lh). Aspect ratio locked via `ComponentBoundsConstrainer` fixed ratio;
min size = logical (kMinScale=1.0), max 2.2×. So proportions are identical at every
size — NEVER add per-element min-size clamps; just use plain arithmetic at logical size.
Each view remembers its own size (advWidth/advHeight, simpleWidth/simpleHeight in state).
Theme (themeDark) + view (simpleView) also in APVTS state.

**AdvancedView (logical 1100×760):**
- Header 40px: title "MAGIC DRUM DE-BLEED" left; PresetBrowser right 380px
  (combo + Save + Delete).
- Right column 116px full height: far-right OUT meter 46px; left of it vertical
  Intensity slider + "Intensity" label on top (100% at top).
- Bottom strip 54px (OutputStrip): "Preview Processed Signal" toggle centered;
  right side "Simple" then "Light/Dark" theme button.
- Remaining split: CompressorPanel (top 42%), EQPanel (rest).

**CompressorPanel ("TRANSIENT DETECTION"):**
- Title row top-left.
- Left: IN meter (48px, draggable threshold line + peak hold) then GR meter (48px).
- Middle: 5 equal knob columns, 3×2 style — row1: Threshold, Reduction, Lookahead;
  row2: RMS, Hold, Release. All knobs identical cell size (this equality matters).
- Right of a divider = SIDECHAIN: SC Freq, SC Q knobs (top); Learn + Preview buttons
  (below knobs); "Filter On" (scEnable) bottom-left of sidechain area.
- Bottom-left of whole panel: "Bypass" (compBypass).

**EQPanel ("EQ - PROCESSED SIGNAL"):**
- Title row: title left; right→ SlideSwitch "Input|Output" (spectrum tap pre/post EQ),
  "Accumulate", "Freeze".
- Middle: spectrum + BandOverlay (draggable handles). Axis **20 Hz–20 kHz log**
  (20k at right edge), log-paper gridlines, "cuts here are preserved in the output" hint.
  Overlay draws combined EQ curve from real coefficients. Drag handle = freq (X) & gain
  (Y, notches); mouse-wheel = Q. Handle glyph: circle=bell/HPF/LPF, rounded-sq=Flat,
  triangle=Band.
- Bottom controls (124px):
  - Left 320px: band selector row [HPF LPF N1 N2 N3 N4 N5]; under each a colored
    enable toggle + "S" solo button; "Bypass" (eqBypass) bottom-left.
  - Selected-band strip: Freq knob (+ Gain, Q for notches); shape selector — for
    HPF/LPF two stacked highlighted slope buttons (6/12 dB/oct), for notches three
    CurveIconButtons (Bell/Flat/Band icons).
  - "POST EQ GATE" cluster: label + "Bypass" (inverted eqGateOn) in a header row,
    then Hold + Release knobs.
  - Far right: EQG meter (eq-gate reduction, 0 = fully engaged).

**SimpleView (logical 360×400):** header title; row left→right: IN meter, GR meter,
vertical Intensity (label top, 100% top), OUT meter; "Advanced View" button bottom.

**Meters:** InputLevelMeter caption "IN"/"OUT"; shows level vs -60..0 dB scale, peak-hold
line; if constructed with a threshold param it draws a draggable threshold line + grab
tab (IN only). GainReductionMeter caption "GR"/"EQG"; 0..-60 dB top-down.
Feeds: `getDetectorRmsDb()` (post-SC, threshold-comparable), `getGainReductionDb()`,
`getEqGateReductionDb()`, `getOutputPeakDb()`.

## 7. Gotchas / conventions
- Choice-param highlight: read `param->convertFrom0to1(param->getValue())`, NOT the
  APVTS raw atomic (its listener can lag one click).
- `EQProcessor::computeCoefficients` is the single source for both audio & UI curve.
- Solo (`setSoloBand`) is UI-only transient state, cleared on editor close; auditions
  a bandpass at band freq/Q, gain NOT applied, dry muted, exclusive.
- Band solo & previews route through `processInternal`'s monitor switch.
- Adding a param: add ID in `ParamIDs`, create in `createParameterLayout`, cache raw
  ptr in ctor, read in `updateParametersForBlock`.

## 8. Build / CI / verify
- Local (Linux, VST3+Standalone): `cmake -B build -DCMAKE_BUILD_TYPE=Release &&
  cmake --build build -j`. JUCE 8.0.8 auto-fetched. Deps: alsa/x11/freetype/fontconfig.
- CI `.github/workflows/build-plugin.yml`: Windows VST3+Standalone; macOS universal
  AU+VST3+Standalone, **ad-hoc codesigned**, ditto-zipped. Artifacts per run.
- macOS "damaged" error = Gatekeeper quarantine: `xattr -dr com.apple.quarantine "…"`
  (+ ad-hoc `codesign --force --deep -s -` if needed). Not notarized (no Dev ID).
- Verify offscreen without a DAW: build VST3, compile a small harness that links the
  `SharedCode.a` via the VST3 `link.txt` recipe (strip `-shared`), call
  `createPluginFilter()`, drive `processBlock`, or render editor via
  `createComponentSnapshot` under `xvfb-run`. Scratchpad has `test_host.cpp`
  (runtime asserts: null, latency, gate, solo, eq-gate) + `snapshot.cpp`.
