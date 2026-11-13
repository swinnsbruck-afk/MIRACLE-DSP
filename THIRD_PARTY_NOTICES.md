# Third-party notices

MIRACLE DSP – Copyright (C) 2026 Stefan Schäfer (ESS76) – is licensed under the
GNU Affero General Public License v3.0 or later (see [`LICENSE`](LICENSE)).

The plugin binaries (AU, VST3, standalone app) are built with the JUCE framework
8.0.10 and contain the following third-party components. Their licence texts are
in the [`LICENSES`](LICENSES) folder and are shipped with every release.

| Component | Used for | Licence | Licence text |
|---|---|---|---|
| JUCE 8.0.10 (Raw Material Software Limited) | Plugin framework, GUI, audio | AGPLv3 (dual-licensed with the commercial JUCE licence) | [`LICENSE`](LICENSE), [`LICENSES/JUCE.md`](LICENSES/JUCE.md) |
| Steinberg VST3 SDK 3.7.12 | VST3 plugin format | GPLv3 (dual-licensed with the Steinberg VST3 License) | [`LICENSES/GPL-3.0.txt`](LICENSES/GPL-3.0.txt), [`LICENSES/VST3_SDK.txt`](LICENSES/VST3_SDK.txt) |
| Apple AudioUnitSDK | Audio Unit plugin format | Apache License 2.0 | [`LICENSES/AudioUnitSDK-Apache-2.0.txt`](LICENSES/AudioUnitSDK-Apache-2.0.txt) |
| FLAC | Reading FLAC files | BSD | [`LICENSES/FLAC.txt`](LICENSES/FLAC.txt) |
| Ogg Vorbis | Reading OGG files | BSD | [`LICENSES/OggVorbis.txt`](LICENSES/OggVorbis.txt) |
| zlib | Compression | zlib licence | [`LICENSES/zlib.txt`](LICENSES/zlib.txt) |
| libpng | PNG images | libpng licence | [`LICENSES/libpng.txt`](LICENSES/libpng.txt) |
| jpeglib (Independent JPEG Group) | JPEG images | IJG licence | [`LICENSES/jpeglib-IJG.txt`](LICENSES/jpeglib-IJG.txt) |
| HarfBuzz | Text shaping | Old MIT licence | [`LICENSES/HarfBuzz.txt`](LICENSES/HarfBuzz.txt) |
| SheenBidi | Bidirectional text | Apache License 2.0 | [`LICENSES/SheenBidi-Apache-2.0.txt`](LICENSES/SheenBidi-Apache-2.0.txt) |
| LV2 headers | Part of the JUCE plugin hosting code | ISC licence | [`LICENSES/LV2-ISC.txt`](LICENSES/LV2-ISC.txt) |

This software is based in part on the work of the Independent JPEG Group.

JUCE is not included in this repository. Building from source requires JUCE
8.0.10 from https://github.com/juce-framework/JUCE (tag `8.0.10`), which brings
the components above with it.

## Trademarks

VST is a registered trademark of Steinberg Media Technologies GmbH. Audio Units
is a trademark of Apple Inc. "Ensoniq" and "Mirage" are trademarks of their
respective owners and are used descriptively only. MIRACLE DSP is not
affiliated with or endorsed by Ensoniq, Creative Technology, Steinberg or Apple.
