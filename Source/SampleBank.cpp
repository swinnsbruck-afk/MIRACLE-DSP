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

#include "SampleBank.h"

#include <algorithm>
#include <cmath>

std::shared_ptr<const SoundList> SoundList::scan (const juce::File& folder)
{
    auto list = std::make_shared<SoundList>();
    list->folder = folder;

    if (! folder.isDirectory())
        return list;

    for (const auto& entry : juce::RangedDirectoryIterator (folder, true, "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3",
                                                            juce::File::findFiles))
    {
        const auto& f = entry.getFile();
        if (f.getFileName().startsWithChar ('.')) // macOS resource forks like "._name.wav"
            continue;

        list->files.add (f);
        if (list->files.size() >= maxFiles)
            break;
    }

    std::sort (list->files.begin(), list->files.end(), [&folder] (const juce::File& a, const juce::File& b)
    {
        return a.getRelativePathFrom (folder).compareNatural (b.getRelativePathFrom (folder)) < 0;
    });

    return list;
}

juce::String SoundList::relativePath (int i) const
{
    return juce::isPositiveAndBelow (i, files.size()) ? files[i].getRelativePathFrom (folder) : juce::String();
}

juce::String SoundList::displayName (int i) const
{
    const auto rel = relativePath (i);
    return rel.upToLastOccurrenceOf (".", false, false).replaceCharacter ('\\', '/');
}

int SoundList::indexOf (const juce::String& rel) const
{
    for (int i = 0; i < files.size(); ++i)
        if (relativePath (i) == rel)
            return i;

    return -1;
}

//==============================================================================
SampleBank::Ptr SampleBank::load (std::shared_ptr<const SoundList> list, int index, juce::AudioFormatManager& formats)
{
    Ptr bank = new SampleBank();
    bank->list = list;

    if (list == nullptr || list->files.isEmpty())
        return bank;

    const int n = list->files.size();
    bank->index = juce::jlimit (0, n - 1, index);

    Wavesample main;
    if (loadFile (list->files[bank->index], formats, main))
    {
        main.name = list->displayName (bank->index);
        bank->samples.push_back (std::move (main));

        Wavesample next;
        const int nextIndex = (bank->index + 1) % n;
        if (n > 1 && loadFile (list->files[nextIndex], formats, next))
        {
            next.name = list->displayName (nextIndex);
            bank->samples.push_back (std::move (next));
        }
    }

    return bank;
}

bool SampleBank::loadFile (const juce::File& file, juce::AudioFormatManager& formats, Wavesample& ws)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));
    if (reader == nullptr || reader->lengthInSamples <= 0 || reader->sampleRate <= 0)
        return false;

    const auto length = (int) std::min<juce::int64> (reader->lengthInSamples,
                                                     (juce::int64) (reader->sampleRate * maxSeconds));
    const int channels = (int) juce::jlimit (1u, 2u, reader->numChannels);

    juce::AudioBuffer<float> buffer (channels, length);
    reader->read (&buffer, 0, length, 0, true, channels > 1);

    ws.name = file.getFileNameWithoutExtension();
    ws.fileRate = reader->sampleRate;
    ws.data.resize ((size_t) length);

    for (int i = 0; i < length; ++i)
    {
        float s = 0.0f;
        for (int c = 0; c < channels; ++c)
            s += buffer.getSample (c, i);
        ws.data[(size_t) i] = s / (float) channels;
    }

    // Normalise like setting the Mirage input level for full 8-bit range
    float peak = 0.0f;
    for (auto s : ws.data)
        peak = std::max (peak, std::abs (s));
    if (peak > 1.0e-6f)
        for (auto& s : ws.data)
            s /= peak;

    const auto& meta = reader->metadataValues;

    if (meta.containsKey ("MidiUnityNote"))
    {
        const int unity = meta["MidiUnityNote"].getIntValue();
        if (unity > 0 && unity < 128)
        {
            ws.rootNote = unity;
            ws.rootKnown = true;
        }
    }

    if (! ws.rootKnown)
    {
        const int parsed = parseNoteFromName (ws.name);
        if (parsed >= 0)
        {
            ws.rootNote = parsed;
            ws.rootKnown = true;
        }
    }

    if (meta.getValue ("NumSampleLoops", "0").getIntValue() > 0)
    {
        const int ls = meta["Loop0Start"].getIntValue();
        const int le = meta["Loop0End"].getIntValue();
        if (le > ls && le <= length)
        {
            ws.fileLoopStart = ls;
            ws.fileLoopEnd = le;
        }
    }

    return true;
}

SampleBank::Ptr SampleBank::createDefault()
{
    Ptr bank = new SampleBank();

    // One cycle of a bright saw/pulse blend, 128 points, played back so that
    // MIDI note 60 sounds at its nominal pitch.
    Wavesample ws;
    ws.name = "INIT WAVE";
    const int cycle = 128;
    const int cycles = 64;
    ws.fileRate = cycle * 261.6256;
    ws.rootNote = 60;
    ws.rootKnown = true;
    ws.data.resize ((size_t) (cycle * cycles));

    for (int i = 0; i < cycle * cycles; ++i)
    {
        const float ph = (float) (i % cycle) / (float) cycle;
        const float saw = 2.0f * ph - 1.0f;
        const float pulse = ph < 0.3f ? 1.0f : -1.0f;
        ws.data[(size_t) i] = 0.6f * saw + 0.4f * pulse;
    }

    ws.fileLoopStart = cycle * (cycles - 1);
    ws.fileLoopEnd = cycle * cycles;
    bank->samples.push_back (std::move (ws));
    return bank;
}

int SampleBank::parseNoteFromName (const juce::String& name)
{
    static const int semis[] = { 9, 11, 0, 2, 4, 5, 7 }; // A B C D E F G
    const auto s = name.toUpperCase();
    const int len = s.length();

    // Scan from the end: the note name is usually the last token
    for (int i = len - 1; i >= 0; --i)
    {
        const auto c = s[i];
        if (c < 'A' || c > 'G')
            continue;

        // must start a token
        if (i > 0 && juce::CharacterFunctions::isLetter (s[i - 1]))
            continue;

        int pos = i + 1;
        int semi = semis[c - 'A'];

        if (pos < len && (s[pos] == '#' || s[pos] == 'S'))
        {
            ++semi;
            ++pos;
        }
        else if (pos < len && s[pos] == 'B')
        {
            --semi;
            ++pos;
        }

        bool negative = false;
        if (pos < len && s[pos] == '-')
        {
            negative = true;
            ++pos;
        }

        if (pos >= len || ! juce::CharacterFunctions::isDigit (s[pos]))
            continue;

        const int octave = (s[pos] - '0') * (negative ? -1 : 1);
        ++pos;

        // must end the token
        if (pos < len && juce::CharacterFunctions::isLetterOrDigit (s[pos]))
            continue;

        const int note = (octave + 2) * 12 + semi; // C3 = 60
        if (note >= 0 && note < 128)
            return note;
    }

    return -1;
}
