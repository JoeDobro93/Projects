# Magic Drum Gate — Engineering Handoff

> **v2.3 UI/QoL round (2026-07):** threshold default −40. **Ring level is now
> 0–20 units** (param `notch*Gain` range 0..20, default 9.8; internal cut =
> `mdd::ringToGainDb(u) = −2.4·u` — converted at every BandParams build site,
> incl. TailCanvas::chainH and the solo path). Canvas gold curve/handles use a
> **cut-depth axis: 0 at the bottom line → dispMax at top** with a SCALE
> LightSelector (Fine/Med/Wide = 12/18/24 dB, persisted in state prop
> "tailScale"); spectra keep the ±9/−54 axis; drag above the canvas reaches
> ring max; internals mirror from the top. HPF/LPF slopes now
> {6,12,24,36,48} dB/oct (EQProcessor::kMaxStages=4, Butterworth Q tables).
> **Solo = kept ring**: out = dry − band(dry) via the band's real stages
> (soloStages[4][2]), bypassing gate/tail/Amount. New widgets:
> `ui::LightSelector` (LED + icon radio; filter/shape icons), `ui::MiniSwitch`
> (band enables, above the band buttons; SOLO full-width below),
> `ui::MiniSlider` (Gate history speed, 10–90 Hz timer). Knob drags are
> velocity-sensitive (slow = 0.3×, base 300 px, shift 1600). Tail stage:
> eqBypass Bypass button in header; bottom controls in three anchored groups
> (bands left / band settings centred / tail gate+meter right, proportional
> shrink fallback). Trigger: LightSelector for HP/LP/BP; stack order
> Enabled/Link/**Learn** (bottom, `eqids::LearnButton` — accent when idle,
> green while listening, 3 s auto-stop). Rail 190 wide: fader + OUT/GATE side
> by side (AdvancedView 1200×830 min 940×700). Simple view 380×620 (min
> 330×540): Resonance (=Focus) with Learn beneath, Reso Amt (=K1 ring),
> Tail hold, and Kick/Snare/Toms factory-preset buttons. Factory presets
> rebuilt (Default/Kick/Snare/Toms, PresetDefaults.h; applied via
> `eqids::applyFactoryPreset`, threshold never touched).
> **v2.3.1:** Enabled/Link are `ui::LightToggle` (LED-in-button); Bandpass
> icon = bell shape; gold curve grid adds 7 clustered samples per enabled
> band (`buildGoldCurve`) + handles eval `hDbAt(f0)` exactly — no tip
> jitter on narrow-Q sweeps. Simple view 380×636 (min 552): + Tail fade,
> Kick/Snare/Toms are `DrumButton`s with vector line-art icons. Light
> palette contrast raised (panel2/line/knob/knobEdge/btn darker).
> **v2.3.2:** LightToggle is chrome-less (LED + text only). Drum icons:
> vertical kick pedal, straight side views (no tilted head/wires), tom legs.
> Prop-Q reference 18→12 dB: equal to Bell at ring 5, NARROWER above
> (0.89 vs 1.39 oct at ring 9.8, Q1), broader only when shallow. Views
> open at their MINIMUM (editor `applyViewSize`; adv min 940×750 so the
> MON fader never collapses); per-view sizes remembered, saves guarded
> (≥300px) against mid-construction/switch contamination — verified by
> 4 size checks in snapshot.cpp. Simple view has a theme toggle
> (shared `themeToggle` lambda passed to both views).
> **v2.3.3 (Ozone-calibrated shapes):** Prop-Q ref 18→12→**3 dB**, clamp
> [0.4,4] — verified vs Ozone screenshots (906 Hz/−4.4/Q0.5): PropQ now
> narrower than Bell at all but very shallow cuts. **Band Shelf = two
> cascaded ±gp S=1 shelf pairs** (edges f0·2^(±bw/2), gp iterated to a
> plateau of max(g, −min(20, 14·bw)), bounded gp ≥ −10·bw against smear)
> **+ centre blendedNotch for the remainder** — kMaxStages now **5**;
> centre depth exact at all settings, plateau genuinely flat, leak
> ≤ ~−2.6 dB at 1 oct out. Kick pedal base tangent to shell bottom; tom
> legs straight down the shell sides w/ outward foot. Simple view preset
> row moved to the TOP (accidental-click safety); AMOUNT caption follows
> the fader (amountY).
>
> **v2.2 gate detection (2026-07):** CompressorProcessor detection reworked so
> marginal hits (ghost notes, LF kicks) neither click nor cut short — no new
> UI. (1) OPEN on fast RMS (one-pole, τ=clamp(rmsWindow/6, 0.5–3 ms)) OR slow
> windowed RMS crossing T. (2) CLOSE on slow RMS only with 8 dB hysteresis,
> but the zone only sustains a FALLING level (slowDb trails its 6 ms lag by
> >0.15 dB ≈ 25 dB/s) — a decaying hit rings through the zone, bleed parked
> in it releases normally. (3) Soft knee: within 6 dB below T, open01 =
> knee01(fast)−knee01(slow), gated by fast leading slow by >4 dB — pre-opens
> during attack rises only; zero for steady bleed (incl. LF detector ripple).
> targetDb = reduction·open01. eqGate env now triggers off the shared
> gateOpen state. Constants in CompressorProcessor.cpp anon namespace.
> test_host: 17 tests incl. ghost open-time, open latency <4.5 ms, knee
> no-leak, re-close onto in-zone bleed. NOTE: test signals need a ~12 ms
> peak plateau — a from-birth decaying tone never crosses T after RMS
> smoothing (the knee blip it gets instead is by design).
>
> **v2.1 refinements (2026-07):** stage-title explanations removed. Knob value
> text is click-to-type (`ui::Knob` inline TextEditor, "2k"=2000; choice knobs
> snap to nearest choice's leading number). Gate Hold/Release ranges 5–200 ms.
> Trigger filter: button stack **Enabled / Learn / Link to K1** (left) · Focus+
> Width knobs · HP/LP/BP segment (right); Listen removed (rail "Trigger signal"
> covers it). `linkK1` param mirrors Focus↔K1 freq (processor is an APVTS
> Listener, reentry guard `linkSyncing`; enabling snaps K1:=Focus; Learn with
> link on enables K1 at gain −10.69 dB = ring −3 dB if K1 was off).
> K shapes now **Bell / Proportional Q (Qeff=q·clamp((|g|/18)^0.7,.1,3)) /
> Band Shelf (= blendedNotch, exact floor)**; Width knob is a plain **Q** knob
> (0.1–40, default 1, not reversed). LOWS/HIGHS slope segment replaced by a
> stepped Slope knob in the Q cell. Gold canvas curve = **internals flipped**
> (y = h − dyy(hDb)); handles all ride it; ring-drag maps cursor straight to
> cut depth. dry/kept legend chips toggle their layers; `ui::MonitorFader`
> (±24 dB, display-only) left of canvas. REDUCTION meter → **GATE meter**
> (`ui::GateMeter`, state via `TailCanvas::gateState`, green=open fill,
> tail-colour fading=tail). Tail columns shrink proportionally at narrow
> widths (TAIL meter never clips). Simple view 380×520 (min 330×450) adds
> Focus / Tail hold / Learn (shared `eqids::handleLearnClick`).
> **Defaults:** hold 7 / release 5 ms; scQ 2.871 (0.5 oct); linkK1 ON; only
> K1 enabled (200 Hz, Q 1, gain −23.5 = ring −0.6 dB, shape Prop Q); hpfOn
> now default OFF; eqGateHold 120 / eqGateRelease 100 ms. Tail gate toggle
> lives above the Tail hold/fade knobs (group header). TailStage starts on
> K1. `setStateInformation` wraps replaceState in `linkSyncing` so the link
> can't cross-write freqs while params load in undefined order.
>
> **v2 UI overhaul (2026-07):** product renamed **Magic Drum Gate**. UI rebuilt
> around a three-state model (CLOSED/OPEN/TAIL) per UIOVERHAULSPEC.md + mockup.
> Key deltas vs the text below: panels are now stages **1·TRIGGER / 2·GATE /
> 3·TAIL** (`UI/Stages.*`, `UI/TailCanvas.*`, `UI/Widgets.*`, right rail, hover
> hint bar); old CompressorPanel/EQPanel/OutputStrip/UIHelpers are gone. New
> params `scType` (HP/LP/BP, default BP) + `scSlope` (6-24 dB/oct) drive a
> multi-mode SidechainFilter. `compBypass` now forces the gate OPEN (dry passes
> untouched) instead of cancelling. `reduction` + monitorMode "Delta" kept in
> APVTS but hidden. Tail display draws **|1−H|** (what is KEPT, complex math via
> `BiquadFilter::responseAt`), axis +9..−54 dB, dry-input spectrum (blue) +
> derived kept layer (orange); the old pre/post Input|Output switch was removed
> (orange is computed, not tapped). GR feed inverted in UI: open01=grDb/−96.
> Notch relabels: N→K "keep bands", gain shown as Ring level
> ringDb=20·log10(1−10^(gain/20)), Q shown in octaves reversed. Shapes renamed
> Rounded/Flat/Focused. eqGate = "Tail gate/hold/fade" (positive toggle).

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
**Scaling model (v2):** aspect ratio UNLOCKED, no transform. `resized()` lays out
from `getLocalBounds()`; regions stretch. A separate global `ui::scale` =
clamp(min(w/1160,h/830),1,2.2) affects legibility only (fonts, knob/meter/button
sizes via `ui::sc()`). Advanced default 1160×830 min 900×700; Simple 380×430 min
330×360. Global `ui::pal` palette pointer; theme change rebuilds both views.
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
