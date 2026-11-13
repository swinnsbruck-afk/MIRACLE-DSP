# MIRACLE DSP – Digital Sample Player, just like 1985

Hersteller: ESS76. 8-Bit-Sample-Player nach dem Vorbild des Ensoniq Mirage.

Instrument-Plugin (VST3, AU, Standalone) für macOS, gebaut mit JUCE 8. Anleitung und Dokumentation liegen in `Release/MIRACLE DSP 1.3.1/`, auf Deutsch und Englisch. [English README](README.en.md)

Download: https://github.com/swinnsbruck-afk/MIRACLE-DSP/releases

☕ **Gefällt dir MIRACLE DSP?** Dann freue ich mich über einen Kaffee: [paypal.me/ess76](https://paypal.me/ess76). Freiwillig, das Plugin bleibt kostenlos.

## Installation

Der Build kopiert die Plugins automatisch nach:

- `~/Library/Audio/Plug-Ins/VST3/MIRACLE DSP.vst3`
- `~/Library/Audio/Plug-Ins/Components/MIRACLE DSP.component` (AU, z. B. für Logic)

Die Standalone-App liegt unter `~/Developer/MiracleDSP-build/MiracleDSP_artefacts/Release/Standalone/MIRACLE DSP.app`.

## Neu bauen

Voraussetzungen: Xcode, CMake (`pip3 install --user cmake`), JUCE 8.0.10 unter `~/Developer/JUCE`:

```bash
git clone --depth 1 --branch 8.0.10 https://github.com/juce-framework/JUCE.git ~/Developer/JUCE
```

```bash
export PATH="$HOME/Library/Python/3.9/bin:$PATH"
cmake -S . -B ~/Developer/MiracleDSP-build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build ~/Developer/MiracleDSP-build -j8
```

Das Build-Verzeichnis liegt bewusst außerhalb von iCloud. Für Intel-Macs zusätzlich `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` angeben.

## Sounds auswählen

1. Im Keypad unter LOAD `Upper` oder `Lower` drücken und einen Ordner oder eine Datei wählen, oder beides aufs Plugin ziehen. Das Plugin durchsucht den Ordner samt allen Unterordnern nach WAV, AIFF, FLAC, OGG und MP3 (bis 5000 Dateien).
2. Mit `Snd ▲/▼`, dem Mausrad über den Zeilen UPP/LOW oder in der Liste im Disk-Fach blättern.
3. Mit `Oct ▲/▼` die aktive Hälfte in Oktaven transponieren (−3 … +3).

Jeder Sound liegt auf der ganzen Hälfte. Der Grundton kommt aus den Datei-Metadaten oder dem Dateinamen (`Piano_C3.wav`, `Bass F#2.wav`; C3 = MIDI 60), sonst gilt C3. Jede Datei wird normalisiert und auf 30 s begrenzt. Ohne Ordner spielt eine eingebaute Wellenform.

Alle Regler liegen im `EDIT PANEL` (Taste oben rechts). Parameter lassen sich auch über das Keypad wählen: Nummer tippen, dann `On/▲`, `Off/▼` oder Wert + `Enter`. Für den nächsten Parameter erst `Param`, für die Programmwahl erst `Cancel`.

**Einstellungen speichern:** Das DAW-Projekt speichert alle Parameter, beide Ordner, den gewählten Sound pro Hälfte und die 4 Programme pro Hälfte. Neue Instanzen laden automatisch die zuletzt benutzten Ordner und Sounds (`~/Library/Application Support/MIRACLE DSP/`).

## Funktionen nach dem Vorbild des Originals

| Mirage | MIRACLE DSP |
|---|---|
| 8-Bit-Samples, variable Sample-Rate | 8-Bit-Quantisierung und virtueller DAC-Takt 8–33 kHz (71) mit Aliasing; abschaltbar per `8-BIT` (76) |
| Lower/Upper-Keyboardhälften | Zwei unabhängige Hälften mit eigenem Ordner, Split-Punkt (70, rotes Dreieck auf der Tastatur) |
| 4 Programme pro Hälfte | Programme 1–4 pro Hälfte (`0/Prog` + Ziffer) |
| 2 Oszillatoren pro Stimme, Detune, Mix, Mix-Velocity, Mix-Mode | 26, 27, 28, 22 (Mix-Mode spielt auf Osc 2 den nächsten Sound der Liste) |
| Curtis-4-Pol-Tiefpass (CEM3328), Resonanz, Keyboard-Tracking | ZDF-Ladder-Filter mit Sättigung: 31, 32 (0–40), 33 (0–4) |
| Filter- und Amp-Hüllkurve mit Velocity- und Key-Scaling | je Attack, Peak, Decay, Sustain, Release, Attack-Vel, Peak-Vel, Decay-Key, Sustain-Key, Release-Vel (40–59) |
| LFO (Vibrato) per Modrad | 24, 25; Modrad erhöht die Tiefe |
| Loop-Start/-Ende, Sample-Start/-Ende | 60–64; Loop-Modus Off/Manual/File (File = Loop aus WAV-`smpl`-Chunk) |
| Mono-Modus | 23, Last-Note-Priority |
| 8 Stimmen | 1–16 Stimmen (74), Standard 8 |
| Display mit Parameternummern | Grünes 7-Segment-Display zeigt beim Bedienen Parameternummer und Wert, im Ruhezustand Hälfte, Programm und Sound-Nummer |

Zusätzlich: Pitchbend-Bereich (73), Master-Tune (72), Master-Oktave (77), Sustain-Pedal, 61-Tasten-Bildschirmtastatur, Pitch- und Mod-Rad.

Nicht umgesetzt: Sampling über den Audioeingang, Diskettenformat des Originals.

## Lizenz

Copyright (C) 2026 Stefan Schäfer (ESS76)

MIRACLE DSP ist freie Software unter der **GNU Affero General Public License, Version 3 oder später** (AGPL-3.0-or-later). Den vollständigen Lizenztext enthält die Datei [`LICENSE`](LICENSE). Du darfst das Plugin nutzen, verändern und weitergeben, auch gegen Geld; veränderte Fassungen müssen wieder unter der AGPL und mit Quellcode weitergegeben werden. Es gibt keine Gewährleistung.

Verwendete Komponenten (vollständige Liste und Lizenztexte: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md), [`LICENSES/`](LICENSES)):

- [JUCE 8](https://juce.com) – genutzt unter der AGPLv3
- VST3-SDK 3.7.12 von Steinberg – verwendet unter der GPLv3 (das SDK steht wahlweise unter der Steinberg-VST3-Lizenz oder der GPLv3)
- AudioUnitSDK von Apple – Apache License 2.0
- FLAC, Ogg Vorbis, zlib, libpng, jpeglib, HarfBuzz, SheenBidi, LV2-Header (kommen mit JUCE)

---

Nicht mit Ensoniq oder Creative Technology verbunden. „Ensoniq“ und „Mirage“ sind Marken ihrer jeweiligen Inhaber und werden nur beschreibend verwendet.
