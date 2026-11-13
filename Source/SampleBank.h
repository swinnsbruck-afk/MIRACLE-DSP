/*
    MIRACLE DSP – Digital Sample Player
    Copyright (C) 2026 Stefan Schäfer (ESS76)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <memory>
#include <vector>

// One playable sample, the equivalent of a Mirage "wavesample".
struct Wavesample
{
    juce::String name;
    std::vector<float> data;        // mono, peak-normalised
    double fileRate = 44100.0;
    int rootNote = 60;
    bool rootKnown = false;         // root came from file metadata or file name
    int fileLoopStart = -1;         // loop points stored in the file (smpl chunk), -1 = none
    int fileLoopEnd = -1;
};

// All audio files found in a folder and its subfolders. Shared between banks,
// so browsing through the sounds never rescans the disk.
struct SoundList
{
    juce::File folder;
    juce::Array<juce::File> files;

    static constexpr int maxFiles = 5000;

    static std::shared_ptr<const SoundList> scan (const juce::File& folder);
    juce::String displayName (int index) const;
    int indexOf (const juce::String& relativePath) const;
    juce::String relativePath (int index) const;
};

// The sound currently selected for one keyboard half. samples[0] is the
// selected file, samples[1] the next file in the list (played by oscillator 2
// in Mix Mode). Reference counted so the audio thread can keep using a bank
// while a new one is swapped in; the processor deletes retired banks on the
// message thread only.
class SampleBank : public juce::ReferenceCountedObject
{
public:
    using Ptr = juce::ReferenceCountedObjectPtr<SampleBank>;

    std::shared_ptr<const SoundList> list;
    int index = -1;
    std::vector<Wavesample> samples;

    static constexpr double maxSeconds = 30.0;

    static Ptr load (std::shared_ptr<const SoundList> list, int index, juce::AudioFormatManager& formats);

    // Built-in 8-bit waveform so the plugin makes sound before a folder is assigned.
    static Ptr createDefault();

    static bool loadFile (const juce::File& file, juce::AudioFormatManager& formats, Wavesample& out);

    // Parses names like "Piano_C3", "bass-F#2", "Str Eb4" (C3 = MIDI 60).
    static int parseNoteFromName (const juce::String& name);
};
