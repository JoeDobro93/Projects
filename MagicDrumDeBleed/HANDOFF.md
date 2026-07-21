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
> **v2.3.4:** Shelf edges Ozone-tuned: corners at ±(bw/2 + 0.12 oct),
> shelf Q 0.9 fading →0.707 as |gp| grows (no ripple when deep):
> reference-case plateau 1.66 oct ≈ Ozone's 1.65, flank intrusion
> ~0.45 oct/side, visible plateau from Q≈1. `makeHighShelf` gained a
> shelfQ param. DrumButtons: label under icon; all three share one
> measured width (max of icon width & GlyphArrangement label width
> + padding), centred as a group; row 58 tall (Simple 380×654 min 570).
> **v2.3.5:** Shelf edge Q 0.9→**1.05** (same drive-fade): plateau visible
> at Q1 for 1–5 dB cuts (full plateau 1.1 oct at −2 dB, Q1); overshoot
> ≤0.21 dB. Simple view rows align to ONE content rect (= knob-row
> width): meters spread edge-to-edge across it, preset buttons equal
> size justified first-left/last-right to the same edges.
> **v2.3.6 (shelf flat-top + far-field):** Band Shelf rebuilt around tail
> cancellation: corner Q is no longer a drive-fade but
> `min(0.90, 0.80 + 0.08·max(0,1−bw) + 0.02·max(0,−gp−6))` — each shelf's
> corner lobe cancels its own S=1 tail, so the response returns to 0 dB
> ~1 oct past the edges (visible spurious cut ≤0.1 dB for q≤1 through
> −24 dB vs up to 1.7 dB before; any leftover is a small boost the
> cut-axis canvas clips). Corners at ±(bw/2+**0.16**) oct. **kMaxStages
> 5→6**: stage 5 = shallow wide "flattener" bell (Q from 0.8·bw, gain
> −sag·1/(1−w) with w = the bell's own shoulder/centre ratio at ±0.20·bw,
> clamp ±6.5 — sized by cgscan worst case 6.07), stage 6 = centre
> blendedNotch; both can coexist so capped depths keep exact centres AND
> flat shoulders. Drive iteration (14 its, tol 0.04) converges the
> *post-correction shoulder level* `ctr − sag + w·cg` onto the plateau —
> honest even when the clamp saturates. Measured (plugcheck.cpp): ripple
> ≤0.33 dB through −20 for q≤1 (was 0.44 at −8), centres exact at depth,
> Q1 plateau widths 1.12/0.98/0.84 oct at −2/−3/−5 (up ~40%); Ozone ref
> case flat −4.43 through ±0.6 oct. Simple view: 1 px `pal->line` divider
> under the preset row (member `divider`, spans the content rect), gap
> 10→22; 380×**666** min 330×**582** (snapshot size-check + render sizes
> now read the SimpleView constants).
> **v2.3.7 (Butterworth corners + Default preset):** the user-visible
> corner ripple ("middle pushed down, sides pushed the other way") was the
> v2.3.6 resonant corner Q — its tail-cancel lobes reach +0.85 dB just
> past the flanks at deep/wide settings, in a zone no metric covered
> (plugcheck now has a "boost" column, edge−0.2..edge+1.2 oct). Fix per
> the higher-order-shelving literature (Holters/Zölzer): each corner is a
> **4th-order maximally-flat shelf** — two biquads at the same corner
> freq, Qs 0.54119610/1.30656296, half the drive each (out[0..3] all
> distinct now); the corner-Q formula is deleted. Monotone flanks: boost
> ≤0.13 dB worst-case (0.00 for q≤0.5), far residue ≤0.03 at −8 dB and
> ≤0.16 through −20 dB, plateaus wider still (1.16/1.04/0.92 oct at Q1
> −2/−3/−5). Remaining deep-setting (−24 and below) far residue
> ~0.3–0.7 dB is the *monotone* skirt of the band-Q remainder notch —
> narrowing it (q·1.4..1.8) contains it but shrinks the max-depth cut
> width, so band-Q is deliberately kept (small Q must still reach real
> notches). Simple view: 4th preset button **Default** (crossed
> drumsticks icon, leftmost; presetBtns[4], DrumButton::def; row is
> equal-width buttons justified at cx0 + i·(contentW−bw)/3).
> **v2.3.8 (QoL round):** (1) Skewed parameter ranges lost their step
> intervals — step+skew makes a dead zone at the range bottom (first step
> ≈ 19% of knob travel on band Q, 11% Smoothing/Tail-hold, 9% Hold, ~5%
> freq knobs) that read as a sticky "snap" at the minimum; ranges are now
> continuous, scQ/notchQ gained 2-decimal stringFromValue (interval no
> longer implies display rounding). (2) Tail gate = labelled
> `ui::MiniSwitch` (widget gained an optional label: pill left, text
> right) centred UNDER Tail hold/fade; CheckToggle deleted. (3) Bypass
> dulling via setAlpha 0.45 wired into the existing attachments:
> compBypass dims Threshold/Smoothing + the three gate knobs; scEnable
> off dims Focus/Width/type + TRIGGER FILTER caption; eqBypass dims all
> band controls/captions/shape/tail group (TailStage::updateDim with
> eqByp/tgOn); tail-gate-off dims Tail hold/fade. (4) History speed
> slider is vertical (MiniSlider vertical mode) right of the history
> display, rotated SPEED label (speedLabelArea). (5) Simple view opens
> with the same 46px toolbar strip as Advanced (panel fill, hairline,
> 16pt title left, theme btn right); 380×692 min 330×608. (6) Default
> icon: tapered stick shafts + small elongated acorn tips.
> **v2.3.9:** Knob drag ~1.5–2× faster (tiers 0.5/0.85/1.0 at |dy|≤2/≤6,
> sens 220, shift 1200); value editor opens with the exact value at 4
> decimals (display keeps 1 — scQ/notchQ stringFromValue now 1 dec).
> HintBar: registers a Desktop global mouse listener; while any control
> is held/dragged the hint locks to the PRESSED component
> (SafePointer `pressed`, e.eventComponent), and hover hints only show
> when the editor's peer isFocused() and the pointer is inside the
> window. Drumsticks thickened (grip 2.1·lw, shaft 1.4·lw, bead
> 0.26×0.15·L) so the tips survive small sizes.
> **v2.3.10:** (1) Skewed ranges → GENUINE log mappings (custom
> convertFrom/To0to1 lambdas — constant relative resolution, so the old
> dead zones now change value everywhere): logHzRange = pure log;
> `logRange(min, centre, max)` two-segment log keeps the old 12-o'clock
> centres for rmsWindow(1|10|100), hold(5|40|200), release(5|60|200),
> scQ(0.3|1.9|12), notchQ(0.1|1|40), eqGateRelease(5|800|5000);
> eqGateHold keeps its power skew (min is 0). Q displays back to 2 dec
> (user-specified; everything else 0–1 dec). (2) Knob value editor
> commits on ANY outside click: `Knob::ClickAway` global mouse listener
> fires edit->onFocusLost when the click isn't in the editor (needed
> since controls don't take keyboard focus — was staying open until
> Enter, which also looked like "knobs showing 4 decimals");
> Knob::mouseDown also commits first; Knob gained a dtor that
> unregisters. (3) RightRail: AMOUNT caption + % value are fader-column
> width (amountX/amountY), caption top level with the meter tops, value
> bottom level with meter bottoms; meters span the full row. (4) MON
> fader moved BETWEEN the SCALE column and the canvas, full canvas
> height (window minimums unchanged). (5) Drumsticks: single tapered
> polygon per shaft (half-widths 1.15→0.55·lw) + round butt — no more
> mid-shaft step.
> **v2.3.11:** (1) Hold/Release (gate) + Tail hold/fade are whole-ms:
> `logRange(..., wholeNumbers)` adds a round-to-int snapToLegalValue
> lambda; Knob::mouseDrag and applyTyped now route through
> range.snapToLegalValue (pure clamp for everything else); new `msInt`
> attributes (0-dec display); eqGateHold range is logRange(1|150|2000)
> (min 0→1 ms — log needs a positive floor; 1 ms ≈ none). Smoothing
> keeps its decimal. (2) TailCanvas: quarter-height gridlines replaced
> by whole-dB lines + "-N dB" labels (step 3 dB on Fine, 6 dB on
> Med/Wide), following dispMax so the Scale is always readable.
> (3) Drumsticks composite inside g.beginTransparencyLayer(0.85) with
> opaque fills — overlapping shaft/butt/bead no longer double-blend
> into a brighter spot.
> **v2.3.12 (v1.0 candidate):** Knob drags 30% faster again (base sens
> 220→170 px full-travel, shift 1200→920). Canvas: left dB labels sit ON
> their gridlines (panel2 patch under the text); right edge gains
> monitor-signal-axis ticks + labels at 0/−12/−24/−36/−48 (dyy mapping,
> kTopDb +9..kBotDb −54) — ticks only, not gridlines, since it's the
> spectra's axis, not the cut axis.
> **v2.3.13:** Knob base sens 170→136 px (shift 740). Right-edge signal
> ticks now mark TRUE input dB tracking the MON fader — y = (kTopDb −
> (v + monGainDb))/(span)·h for v = +36..−84 step 12, drawn only inside
> [22px, h−6px] — and alternate dry-blue 0x4aa8e0 / kept-orange 0xe07e2a
> (0.75 alpha; even multiples of 12 blue) so they read as the spectra's
> axis; positive values show a + sign.
> **v2.3.14:** JUCE trap fixed: `String(v, 0)` does NOT round — it prints
> the raw float, so continuous values ≥10 showed as e.g.
> "40.7002143859863 ms" (the real cause of the "4 decimals" reports; the
> `ms` lambda and `fmtHz` now round explicitly via roundToInt). Ring
> level range is continuous (0.1 interval dropped; displays still 1 dec).
> MON fader moved to the RIGHT of the canvas (beside the signal ticks):
> [scale 56][5][canvas][4][mon 26]. History speed remapped: rate =
> 10·9^(2v−1) Hz → default position (0.5) = 10 Hz (the old slowest),
> range 1.1–90 Hz; ctor timer starts at 10 Hz.
>
> **v2.4 (contrast gate + detector controls, 2026-07):** three new params.
> (1) **Selectivity** (`contrast`, 0–24 dB, default 24 = Off, knob
> REVERSED so stricter = fuller arc, in the TRIGGER FILTER group, dims
> with scEnable): opening additionally requires
> `broadPkDb − fastBandDb ≤ contrast` where broadPk is a fast follower on
> the UNFILTERED sidechain (detectorRaw — same τ as the band's fast
> follower) run through a peak-hold (instant attack, 12 ms release,
> broadPkRelCoeff). The peak-hold is load-bearing: fast followers ripple
> several dB on low-frequency content and a downward ripple of the
> reference must not blink the veto off mid-bleed (first attempt without
> it leaked — caught by the new tests). Veto gates ALL opening paths
> (fast, slow, knee) and never closes an open gate. Off-frequency bleed
> is rejected by ratio regardless of absolute level; on-band ghosts pass
> untouched. (2) **Attack** (`attack`, logRange 0.2|1.7|10 ms, default
> 1.7 ≈ the old derived value): the fast opening detector's τ, no longer
> derived from Smoothing. (3) **Hysteresis** (`hysteresis`, 0–24 dB,
> default 8 = the old constant, replaces kHysteresisDb): close-below
> depth. CompressorProcessor::process gained a detectorBroad arg
> (nullptr → falls back to detector); getCurrentFastDetectorDb/
> getFastDetectorDb exposed. UI: Attack+Hysteresis knobs in a second
> centred row under the gate knobs (GateStage 172→208 tall, adv kMinH
> 750→786, snapshot check reads AdvancedView constants); history plots
> the fast detector as a bright thin trace (Sample gained `fast`) plus a
> dimmer short-dash line at threshold−hysteresis (the close level);
> test_host now has 20 tests (contrast off/vetoed/ghost-passes).
> **v2.4.1 (off-band reference + onset latch):** hard off-drum hits leaked
> through the veto late in the hit (sympathetic snare buzz + 2nd
> harmonics grow IN-band nonlinearly with hit strength, collapsing the
> ratio 10-20 ms after onset). Two changes: (1) the reference is now
> OFF-BAND-ONLY energy by subtraction (`offSq = max(broadSq − bandSq,
> broadSq·0.001)`, same-τ followers, no extra filters; floor = −30 dB
> "purity" cap) — the target's own energy no longer inflates the
> reference, so on-band ratios sit ~−30 instead of ~0 and the usable
> margin widens hugely; (2) the veto decision LATCHES per event
> (`vetoLatch`): latch on when excess > contrast while offDb >
> threshold−6 (events are classified AT ONSET, where toms are maximally
> distinguishable — fundamental instant, buzz late), release when the
> band convincingly takes over (excess < contrast−6, so a real hit
> landing on top of a ringing tom still opens) or the event fades
> (offDb < threshold−12). On-band onsets briefly false-latch while the
> band filter builds up, then the release condition clears within ~1 ms
> — no added latency beyond the filter's own. History gains the ORANGE
> off-band trace (Sample.off, getOffbandDb) — the by-eye tuning tool:
> target drum lifts the bright trace above orange, other drums the
> reverse; set Selectivity between. 22 tests (+hard-tom-late-buzz stays
> vetoed, ghost-after-tom opens).
> **v2.5 (MIDI trigger + gate cleanup, 2026-07):** the manual-repair path.
> (1) `attack` param and knob REMOVED (user: confusing vs gate-attack, and
> marginal) — fast detector τ back to clamp(rmsWindow/6, 0.5–3 ms). The
> bright fast trace in the history stays. (2) **MIDI input enabled**
> (CMake NEEDS_MIDI_INPUT, acceptsMidi true): `midiTrigger` bool param
> (default on, LightToggle "MIDI" in the gate grid). buildForceMask()
> turns the block's notes into a per-sample mask (note counting; CC123/
> all-sound-off clears; both float+double processBlock paths);
> CompressorProcessor::process gained `forceOpen` — a forced sample sets
> gateOpen+hold exactly like a detector crossing, bypassing threshold AND
> the Selectivity veto; note length sustains, then normal hold/release.
> History chips tint accent-blue while MIDI-forced (Sample.forced,
> getMidiForced). Workflow: draw notes in a MIDI item on the same track
> at missed hits — the DAW is the tempo-aware editor. (3) Gate knob grid:
> row1 Lookahead+Hysteresis+MIDI toggle, row2 Hold+Release beneath.
> (4) History speed: rate = 30·9^v Hz (bottom 30 = old middle, default 90
> = old top, top 270 for per-hit detail). 24 tests (+MIDI opens,
> closes after note-off).
> **v2.5.1:** (1) Stuck-gate fix: held MIDI notes are a per-note SET
> (heldKeys[128]+heldCount) instead of a counter — muting the MIDI track
> swallows note-offs, and a counter drifts up when the same note is
> re-played (1→2→1 = stuck, the user's "sometimes"); a set self-heals on
> re-trigger. Also clears on transport stop and backwards ppq jumps
> (getPlayHead()->getPosition(), wasPlaying/lastPpq members; null-safe
> for hosts without a playhead). Regression test 25 covers the
> lost-note-off scenario. (2) MIDI toggle moved to TRIGGER's SENSITIVITY
> column, centred under Threshold/Smoothing (sens knobs now
> removeFromTop(84)); dims with compBypass. (3) Band selector LOWS→"LP",
> HIGHS→"HP" — deliberately INVERTED from the internal parallel-path
> filter kinds (band 0 = HPF on the cancellation copy = keeps lows =
> LP of the KEPT signal); canvas empty-state text updated.
> **v2.5.2 (history polish):** gate knob column 212→140 (history stretches
> into the old MIDI-toggle space). Trace LED toggles above the chart:
> Trigger (bright fast) / Smoothed (blue) / Off-band (orange) —
> showTrig/showSmooth/showOff, not persisted. New `ui::SnowButton`
> (icon-only snowflake toggle, panel2 backing): freezes the history
> (skips hist.push) and REPLACES the TAIL header Freeze text button,
> overlaying each display's top-right corner (TailCanvas legend kx
> 110→136 to clear it). History speed top half now 90·4^(2v−1) → max
> 360 Hz (bottom/middle unchanged 30/90). Gate/tail backgrounds use the
> REAL envelopes: Sample gained o01/t01; tail drawn under at alpha
> 0.16·t01, gate on top at 0.24·o01 (accent when MIDI-forced) — release
> fades the green to uncover the tail. LevelMeter value text shows the
> PEAK-HOLD line's value (pk), hold 1.0→1.6 s.
> **v1.0 (logo + release):** toolbar title text replaced by a vector logo
> in BOTH views — `ui::drawLogo(g)` in Widgets.cpp, drawn at x=sc(12) in
> the sc(46) strip, coordinates in 46-unit mockup space × ui::scale.
> Elements: accent-blue snare (same shell/hoops/lugs construction as the
> Simple-view preset icon, dim lugs), txt-colour drumstick striking down
> from upper right (tapered polygon + butt circle + rotated acorn bead,
> bead hovering just OFF the rim — user-requested), two gold 4-point
> sparkles at the impact, wordmark "MAGIC " (accent) + "DRUM GATE" (txt)
> via AttributedString/TextLayout (kerning 0.06), and an underline the
> width of the text: ColourGradient open→(0.55)tail→tail-alpha-0.
> User iterated across 3 concept sheets (scratchpad logos*.py renders);
> approved mockup = scratchpad/logo_final.png. CMake VERSION was already
> 1.0.0; tagged v1.0.0. Simple-view min-width check: logo ends ~x219,
> themeBtn starts x262 at 330 wide — no overlap.
>
> **v2.5.4 (preset tuning):** Kick/Snare/Toms factory values retuned per
> drum ADSR, assuming Learn re-centres scFreq/K1 (PresetDefaults.h has
> the full rationale block). Kick {Width Q 1.9, hold 50, rel 60, K1 Q 2.5
> ring 12, tail 180/250} — wide K1 for the downward pitch glide, wider
> trigger because a 60 Hz narrow BP rings ~15 ms. Snare {Q 3.5, hold 10,
> rel 20, K1 Q 8 ring 16, tail 200/600} — narrower trigger vs toms, K1
> survives detune, fade matches wire decay. Toms {Q 2.4, hold 25, rel 50,
> K1 Q 2.0 ring 14, tail 400/1000} — old 2-oct focus reached into kick +
> snare fundamentals; 100 ms tail fade was chopping the ring. Default
> unchanged. Header-only change; 25/25 tests.
>
> **v2.5.3 (audit round):** (1) applyFactoryPreset now PRESERVES the
> per-mic calibration params — threshold, contrast (Selectivity),
> hysteresis, midiTrigger — via a `preserved` StringArray; preset hints
> updated to say so. (2) **`reduction` parameter REMOVED entirely**
> (pre-1.0 last chance): the gate always ducks the parallel copy by
> kReductionDb = −96 (anon-namespace constant in PluginProcessor.cpp);
> this also makes the /−96 open01 normalisation in gateState/history
> exact by construction. Old sessions carrying the param load fine
> (unknown XML params are ignored). (3) Dead code removed:
> spectrumPostEq atomic/setter/cached (never read) and
> removedPeakDb/getRemovedPeakDb (per-block magnitude scan feeding a
> meter deleted in v2.3). (4) LOWS/HIGHS→LP/HP comment rot fixed in
> BandIds.h, TailCanvas.h, ThemeColors.h, PresetDefaults.h.
> test_host: burst-passthrough expectation ~0.47→~0.50 and GR ~−24→~−96
> (reduction is fixed now); text-format test re-pointed to threshold.
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
