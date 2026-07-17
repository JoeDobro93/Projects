# Magic Drum De-Bleed

A JUCE 8 audio plugin (**VST3 + AU**) that removes microphone bleed from close-miked
drum tracks using **parallel null-cancellation** — not gating, not expansion.

## How it works

```
input ──┬───────────────► dry delay (= lookahead) ─────────────┐
        │                                                      ▼
        ├─► polarity flip ▸ compressor ▸ EQ ▸ intensity ───► SUM ──► output
        │
        └─► detector (un-delayed, optional bandpass) ─► compressor sidechain
```

The input splits into a **dry path** (delayed by exactly the lookahead) and a
**parallel path** (polarity-flipped, then compressed and EQ'd). Summed together,
the inverted parallel path cancels the dry path to silence. The compressor —
a *fixed-reduction gate*, not a ratio compressor — ducks the parallel path
whenever the target drum hits, so the drum passes through while everything
else (the bleed) keeps cancelling. EQ notches remove frequencies *from the
parallel path*, which means those frequencies are **preserved** in the output —
use them to keep the drum's body and ring from being cancelled during decay.

All DSP runs in double precision at any sample rate. Latency (= lookahead) is
reported to the host via `setLatencySamples()`.

---

## Building

### Requirements

- **CMake ≥ 3.22** and **git** (JUCE 8.0.8 is downloaded automatically on first configure)
- **Windows:** Visual Studio 2022 with the *Desktop development with C++* workload
- **macOS:** Xcode 14 or newer
- **Linux (optional, VST3 only):** `libasound2-dev libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev pkg-config`

### Windows (Visual Studio 2022)

```bat
cd MagicDrumDeBleed
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

Result: `build\MagicDrumDeBleed_artefacts\Release\VST3\Magic Drum De-Bleed.vst3`
→ copy it to `C:\Program Files\Common Files\VST3\`.

(You can also open `build\MagicDrumDeBleed.sln` in Visual Studio and build there.)

### macOS (Xcode) — VST3 + AU

```bash
cd MagicDrumDeBleed
cmake -B build -G Xcode
cmake --build build --config Release
```

Results under `build/MagicDrumDeBleed_artefacts/Release/`:

| Format | Install to |
|---|---|
| `AU/Magic Drum De-Bleed.component` | `~/Library/Audio/Plug-Ins/Components/` |
| `VST3/Magic Drum De-Bleed.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |

After installing the AU, run `auval -a` (or just restart Logic/your DAW) so the
AU cache picks it up. For local use an ad-hoc signature is fine:
`codesign --force --deep -s - "~/Library/Audio/Plug-Ins/Components/Magic Drum De-Bleed.component"`.

### Linux (VST3 + Standalone)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

### Handy CMake options

| Option | Default | Meaning |
|---|---|---|
| `-DMDDB_COPY_PLUGIN=ON` | OFF | Auto-copy the plugin into your user plugin folder after each build |
| `-DMDDB_JUCE_TAG=8.x.y` | 8.0.8 | JUCE version to fetch |
| `-DMDDB_FETCH_JUCE=OFF` | ON | Use a local JUCE (`-DCMAKE_PREFIX_PATH=/path/to/juce-install`) instead of downloading |

A `Standalone` build is also produced — useful for quick testing without a DAW.

> **JUCE licensing note:** JUCE 8 is dual-licensed (AGPLv3 / commercial). The
> default build shows the JUCE splash screen on the editor; that's intentional —
> only disable it (`JUCE_DISPLAY_SPLASH_SCREEN=0`) if your use complies with the
> AGPLv3 or you hold a JUCE licence.

---

## Using the plugin

1. Insert on the bleeding drum track (e.g. the snare mic). Start with the
   matching **factory preset** (Kick / Snare / Toms).
2. Click **Learn**, play or loop a section where the target drum hits, click
   **Stop** — the sidechain bandpass snaps to the drum's dominant frequency
   (the search is capped by the `learnCeiling` parameter, 1 kHz by default,
   adjustable from your DAW's parameter list). Use the **Preview** button in
   the Sidechain section to hear exactly what the detector hears.
3. Lower **Threshold** until the **GR meter** fires on the target drum only —
   the **IN meter** beside it shows the detector level the threshold is
   compared against. **Drag the threshold line right on that meter** to set
   Threshold by ear; a lingering peak line marks recent peaks. Raise **RMS**
   to ignore short spikes of bleed; raise **Hold**/**Release** to cover the
   drum's decay.
4. Toggle **Preview Processed Signal** (bottom strip) to hear exactly what is
   being subtracted from the track. If you hear drum body or ring in there,
   enable the spectrum **Accumulate** mode, watch the resonant frequencies
   build up, and drop **notch bands** on them: flip a band's coloured switch
   under its selector, drag its handle (mouse-wheel adjusts Q), pick a curve
   shape (**Bell / Flat / Notch** icons), and use its **S** button to solo a
   bandpass around it while centring it on the resonance (band gain is
   intentionally not applied while soloing; solo is exclusive — soloing
   another band moves it). The **Input/Output** switch selects whether the
   spectrum is tapped before or after the notches, and the white curve shows
   the combined response of every enabled band. **Freeze** holds the display
   while you fine-tune. Notched frequencies are *kept* in the output. The
   **EQ Gate** (on by default; Gate toggle + Hold/Release knobs and the EQG
   meter in the EQ section) blends the EQ out using the same detector: hits
   engage the EQ fully so the drum decays naturally, and between hits the
   path returns to full cancellation so the notches cannot rumble.
5. Turn the preview off again; use **Intensity** to dial the cancellation
   from 0 % (off) to 100 % (full).

**Views:** the **Simple** button (bottom strip) collapses the UI to just the
two meters — with the draggable threshold line — and a vertical Intensity
slider, for quick level setting; **Advanced View** returns to the full editor.
Each view remembers its own size. The window keeps a fixed aspect ratio and
scales as one piece, so it looks identical at any size — drag a corner to
resize.

**Monitoring** — normal output is the default and needs no button. **Preview**
(Sidechain section) solos the detector signal; **Preview Processed Signal**
(bottom strip) solos the parallel path — compressor + EQ, no polarity flip,
dry muted — i.e. exactly the signal that gets subtracted. Each section has its
own **Bypass** toggle in its title row. With both stages bypassed and
Intensity at 100 %, the output nulls to silence — a built-in null test.

**Presets:** the browser saves/loads/deletes user presets (XML files in
`<user-app-data>/MagicDrumDeBleed/Presets`, e.g. `~/Library/Application Support`
on macOS, `%APPDATA%` on Windows). **Threshold is never stored or changed by
presets** — it stays where you set it. Factory presets live in
`Source/PresetDefaults.h` as named constants: edit and recompile.

---

## Project layout

```
MagicDrumDeBleed/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h/.cpp        # path splitting, summing, latency, state
│   ├── PluginEditor.h/.cpp           # layout, resizing, LookAndFeel, theming
│   ├── DSP/
│   │   ├── CompressorProcessor.h/.cpp  # fixed-reduction gate, RMS detector, lookahead
│   │   ├── SidechainFilter.h/.cpp      # detector bandpass
│   │   ├── EQProcessor.h/.cpp          # 7-band double-precision IIR EQ
│   │   ├── BiquadFilter.h/.cpp         # RBJ biquads (double precision)
│   │   └── LearnAnalyzer.h/.cpp        # FFT dominant-frequency detection
│   ├── UI/
│   │   ├── CompressorPanel.h/.cpp      # compressor + sidechain controls, GR meter
│   │   ├── EQPanel.h/.cpp              # spectrum (accumulate/freeze), band handles
│   │   ├── OutputStrip.h/.cpp          # intensity, monitor modes, bypasses, theme
│   │   ├── PresetBrowser.h/.cpp        # factory/user presets
│   │   └── UIHelpers.h                 # shared labelled-knob widget
│   ├── Licensing/
│   │   ├── LicenseConfig.h             # ENABLE_DONATION_NAG flag + docs (0 by default)
│   │   ├── LicenseValidator.h/.cpp     # key generation/validation, PropertiesFile storage
│   │   └── NagDialog.h/.cpp            # popup UI (compiled only when flag = 1)
│   ├── ThemeColors.h                   # ALL colours for dark + light themes
│   └── PresetDefaults.h                # ALL factory preset values
```

- **Themes:** every colour lives in `ThemeColors.h`. The theme button swaps the
  palette only; the window size and theme choice are remembered in the plugin state.
- **Donationware scaffold:** fully disabled by default. Flip
  `ENABLE_DONATION_NAG` to `1` in `Source/Licensing/LicenseConfig.h` and rebuild
  to enable the (non-blocking, never feature-gating) registration popup. Key
  format, generation utility and instructions are documented in that same file.
- **Plugin identity:** change `COMPANY_NAME`, `BUNDLE_ID`, `PLUGIN_MANUFACTURER_CODE`
  and `PLUGIN_CODE` in `CMakeLists.txt` before distributing builds.
