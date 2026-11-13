# MIRACLE DSP – Digital Sample Player, just like 1985

![MIRACLE DSP – Digital Sample Player, just like 1985](Release/MIRACLE%20DSP%201.3.1/Mockups/01_hero_1920x1080.png)

Made by ESS76. 8-bit sample player modelled on the Ensoniq Mirage.

Instrument plugin (VST3, AU, standalone) for macOS, built with JUCE 8. Manuals and documentation are in `Release/MIRACLE DSP 1.3.1/`, in English and German. [Deutsche README](README.md)

Download: https://github.com/swinnsbruck-afk/MIRACLE-DSP/releases

Project page: https://www.76ideen.com/miracle-dsp.html#top

☕ **Enjoying MIRACLE DSP?** I would be happy about a coffee: [☕ Buy me a coffee for €2.90](https://paypal.me/ess76/2.90EUR). Entirely voluntary, the plugin stays free.

## Screenshots

![MIRACLE DSP – interface](Release/MIRACLE%20DSP%201.3.1/Bilder/oberflaeche.png)
*The interface (version 1.3.1)*

![MIRACLE DSP – edit panel](Release/MIRACLE%20DSP%201.3.1/Bilder/oberflaeche_edit.png)
*With the edit panel open: wavesample, oscillators, LFO, filter, envelopes, master*

## Installation

The build copies the plugins automatically to:

- `~/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3`
- `~/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component` (AU, e.g. for Logic)

The standalone app is at `~/Developer/MiracleDSP-build/MiracleDSP_artefacts/Release/Standalone/MIRACLE DSP.app`.

## Building

Requirements: Xcode, CMake (`pip3 install --user cmake`), JUCE 8.0.10 at `~/Developer/JUCE`:

```bash
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE.git ~/Developer/JUCE
```

```bash
export PATH="$HOME/Library/Python/3.9/bin:$PATH"
cmake -S . -B ~/Developer/MiracleDSP-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build ~/Developer/MiracleDSP-build -j8
```

The build directory is deliberately outside iCloud. For Intel Macs, also pass `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`. `Release/build_installer.sh` creates the installer and DMG from a universal build.

## Choosing sounds

1. On the keypad under LOAD, press `Upper` or `Lower` and choose a folder or file, or drag either one onto the plugin. The plugin searches the folder and all subfolders for WAV, AIFF, FLAC, OGG and MP3 (up to 5000 files).
2. Browse with `Snd ▲/▼`, the mouse wheel over the UPP/LOW lines, or the list in the disk tray.
3. Transpose the active half in octaves with `Oct ▲/▼` (−3 … +3).

Each sound plays across the whole half. The root note comes from the file metadata or file name (`Piano_C3.wav`, `Bass F#2.wav`; C3 = MIDI 60), otherwise C3 is used. Each file is normalised and limited to 30 s. Without a folder, a built-in waveform plays.

All controls are in the `EDIT PANEL` (key at the top right). Parameters can also be selected on the keypad: type the number, then `On/▲`, `Off/▼` or value + `Enter`. For the next parameter press `Param` first, for program selection press `Cancel` first.

**Saving settings:** the DAW project stores all parameters, both folders, the selected sound per half and the 4 programs per half. New instances automatically load the most recently used folders and sounds (`~/Library/Application Support/MIRACLE DSP/`).

## Features modelled on the original

| Mirage | MIRACLE DSP |
|---|---|
| 8-bit samples, variable sample rate | 8-bit quantisation and virtual DAC clock 8–33 kHz (71) with aliasing; can be switched off with `8-BIT` (76) |
| Lower/upper keyboard halves | Two independent halves with their own folders, split point (70, red triangle on the keyboard) |
| 4 programs per half | Programs 1–4 per half (`0/Prog` + digit) |
| 2 oscillators per voice, detune, mix, mix velocity, mix mode | 26, 27, 28, 22 (mix mode plays the next sound in the list on osc 2) |
| Curtis 4-pole low-pass (CEM3328), resonance, keyboard tracking | ZDF ladder filter with saturation: 31, 32 (0–40), 33 (0–4) |
| Filter and amp envelopes with velocity and key scaling | each with attack, peak, decay, sustain, release, attack vel, peak vel, decay key, sustain key, release vel (40–59) |
| LFO (vibrato) via mod wheel | 24, 25; the mod wheel raises the depth |
| Loop start/end, sample start/end | 60–64; loop mode Off/Manual/File (File = loop from WAV `smpl` chunk) |
| Mono mode | 23, last-note priority |
| 8 voices | 1–16 voices (74), default 8 |
| Display with parameter numbers | Green 7-segment display shows parameter number and value during use; half, program and sound number when idle |

Additional: pitch bend range (73), master tune (72), master octave (77), sustain pedal, 61-key on-screen keyboard, pitch and mod wheels that follow your controller, drag & drop.

Not implemented: sampling from an audio input, the original's disk format.

## License

Copyright (C) 2026 Stefan Schäfer (ESS76)

MIRACLE DSP is free software under the **GNU Affero General Public License, version 3 or later** (AGPL-3.0-or-later). The full licence text is in [`LICENSE`](LICENSE). You may use, modify and share the plugin, including for a fee; modified versions must be shared under the AGPL with their source code. There is no warranty.

Third-party components (full list and licence texts: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), [`LICENSES/`](LICENSES)):

- [JUCE 8](https://juce.com) – used under the AGPLv3
- Steinberg VST3 SDK 3.7.12 – used under the GPLv3 (the SDK is dual-licensed under the Steinberg VST3 License or the GPLv3)
- Apple AudioUnitSDK – Apache License 2.0
- FLAC, Ogg Vorbis, zlib, libpng, jpeglib, HarfBuzz, SheenBidi, LV2 headers (bundled with JUCE)

---

Not affiliated with Ensoniq or Creative Technology. "Ensoniq" and "Mirage" are trademarks of their respective owners and are used descriptively only.
