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

#include <juce_audio_processors/juce_audio_processors.h>

// Parameter catalogue. Every parameter carries a Mirage-style two-digit
// number that is shown in the LED display, like on the original front panel.
// Values use the Mirage 0..99 convention wherever the original did.

namespace params
{
enum Type { tInt, tBool, tChoice };

struct Def
{
    const char* id;
    const char* name;
    const char* shortName;
    int number;
    int minV, maxV, def;
    Type type;
    const char* choices;
};

// Per-half ("Lower" / "Upper") parameters
enum HP
{
    hpMixMode, hpMono,
    hpLfoSpeed, hpLfoDepth,
    hpDetune, hpOscMix, hpMixVel,
    hpCoarse, hpFine, hpOctave, hpLevel,
    hpCutoff, hpReso, hpKeyTrack,
    hpFAtt, hpFPeak, hpFDec, hpFSus, hpFRel, hpFAttVel, hpFPeakVel, hpFDecKey, hpFSusKey, hpFRelVel,
    hpAAtt, hpAPeak, hpADec, hpASus, hpARel, hpAAttVel, hpAPeakVel, hpADecKey, hpASusKey, hpARelVel,
    hpLoopMode, hpLoopStart, hpLoopEnd, hpSmpStart, hpSmpEnd,
    kNumHP
};

inline const Def halfDefs[kNumHP] = {
    { "mixmode",   "Mix Mode",           "MIX MODE",22, 0, 1,    0,    tBool,   nullptr },
    { "mono",      "Mono Mode",          "MONO",    23, 0, 1,    0,    tBool,   nullptr },
    { "lfospeed",  "LFO Speed",          "SPEED",   24, 0, 99,   70,   tInt,    nullptr },
    { "lfodepth",  "LFO Depth",          "DEPTH",   25, 0, 99,   0,    tInt,    nullptr },
    { "detune",    "Osc Detune",         "DETUNE",  26, 0, 99,   0,    tInt,    nullptr },
    { "oscmix",    "Osc Mix",            "MIX",     27, 0, 99,   0,    tInt,    nullptr },
    { "mixvel",    "Mix Velocity",       "MIX VEL", 28, 0, 99,   0,    tInt,    nullptr },
    { "coarse",    "Coarse Tune",        "COARSE",  29, -24, 24, 0,    tInt,    nullptr },
    { "fine",      "Fine Tune",          "FINE",    30, -50, 50, 0,    tInt,    nullptr },
    { "octave",    "Octave",             "OCTAVE",  34, -3, 3,   0,    tInt,    nullptr },
    { "level",     "Level",              "LEVEL",   39, 0, 99,   80,   tInt,    nullptr },
    { "cutoff",    "Filter Cutoff",      "CUTOFF",  31, 0, 99,   70,   tInt,    nullptr },
    { "reso",      "Filter Resonance",   "RESO",    32, 0, 40,   0,    tInt,    nullptr },
    { "keytrack",  "Filter Kbd Track",   "KEYTRK",  33, 0, 4,    2,    tInt,    nullptr },
    { "f_att",     "Filter Attack",      "ATT",     40, 0, 99,   0,    tInt,    nullptr },
    { "f_peak",    "Filter Peak",        "PEAK",    41, 0, 99,   0,    tInt,    nullptr },
    { "f_dec",     "Filter Decay",       "DEC",     42, 0, 99,   40,   tInt,    nullptr },
    { "f_sus",     "Filter Sustain",     "SUS",     43, 0, 99,   0,    tInt,    nullptr },
    { "f_rel",     "Filter Release",     "REL",     44, 0, 99,   30,   tInt,    nullptr },
    { "f_attvel",  "Filter Attack Vel",  "ATT VEL", 45, 0, 99,   0,    tInt,    nullptr },
    { "f_peakvel", "Filter Peak Vel",    "PK VEL",  46, 0, 99,   0,    tInt,    nullptr },
    { "f_deckey",  "Filter Decay Key",   "DEC KEY", 47, 0, 99,   0,    tInt,    nullptr },
    { "f_suskey",  "Filter Sustain Key", "SUS KEY", 48, 0, 99,   0,    tInt,    nullptr },
    { "f_relvel",  "Filter Release Vel", "REL VEL", 49, 0, 99,   0,    tInt,    nullptr },
    { "a_att",     "Amp Attack",         "ATT",     50, 0, 99,   0,    tInt,    nullptr },
    { "a_peak",    "Amp Peak",           "PEAK",    51, 0, 99,   99,   tInt,    nullptr },
    { "a_dec",     "Amp Decay",          "DEC",     52, 0, 99,   40,   tInt,    nullptr },
    { "a_sus",     "Amp Sustain",        "SUS",     53, 0, 99,   99,   tInt,    nullptr },
    { "a_rel",     "Amp Release",        "REL",     54, 0, 99,   30,   tInt,    nullptr },
    { "a_attvel",  "Amp Attack Vel",     "ATT VEL", 55, 0, 99,   0,    tInt,    nullptr },
    { "a_peakvel", "Amp Peak Vel",       "PK VEL",  56, 0, 99,   40,   tInt,    nullptr },
    { "a_deckey",  "Amp Decay Key",      "DEC KEY", 57, 0, 99,   0,    tInt,    nullptr },
    { "a_suskey",  "Amp Sustain Key",    "SUS KEY", 58, 0, 99,   0,    tInt,    nullptr },
    { "a_relvel",  "Amp Release Vel",    "REL VEL", 59, 0, 99,   0,    tInt,    nullptr },
    { "loopmode",  "Loop Mode",          "LOOP",    60, 0, 2,    2,    tChoice, "Off|Manual|File" },
    { "loopstart", "Loop Start",         "LP START",61, 0, 1000, 500,  tInt,    nullptr },
    { "loopend",   "Loop End",           "LP END",  62, 0, 1000, 1000, tInt,    nullptr },
    { "smpstart",  "Sample Start",       "START",   63, 0, 1000, 0,    tInt,    nullptr },
    { "smpend",    "Sample End",         "END",     64, 0, 1000, 1000, tInt,    nullptr },
};

// Global parameters
enum GP { gpSplit, gpRate, gpTune, gpBend, gpVoices, gpVolume, gpLoFi, gpOctave, kNumGP };

inline const Def globalDefs[kNumGP] = {
    { "split",  "Split Point",       "SPLIT",  70, 0, 127, 60, tInt,  nullptr },
    { "srate",  "Sample Rate (kHz)", "RATE",   71, 8, 33,  29, tInt,  nullptr },
    { "tune",   "Master Tune",       "TUNE",   72, -50, 50, 0, tInt,  nullptr },
    { "bend",   "Bend Range",        "BEND",   73, 0, 12,  2,  tInt,  nullptr },
    { "voices", "Voices",            "VOICES", 74, 1, 16,  8,  tInt,  nullptr },
    { "volume", "Master Volume",     "VOLUME", 75, 0, 99,  80, tInt,  nullptr },
    { "lofi",   "8-Bit Mode",        "8-BIT",  76, 0, 1,   1,  tBool, nullptr },
    { "octave", "Master Octave",     "OCTAVE", 77, -3, 3,  0,  tInt,  nullptr },
};

inline juce::String halfPrefix (int half)          { return half == 0 ? "L_" : "U_"; }
inline juce::String halfId (int half, int index)   { return halfPrefix (half) + halfDefs[index].id; }
inline juce::String globalId (int index)           { return globalDefs[index].id; }

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    auto add = [&layout] (const juce::String& id, const juce::String& name, const Def& d)
    {
        const juce::ParameterID pid { id, 1 };
        if (d.type == tBool)
            layout.add (std::make_unique<juce::AudioParameterBool> (pid, name, d.def != 0));
        else if (d.type == tChoice)
            layout.add (std::make_unique<juce::AudioParameterChoice> (pid, name, juce::StringArray::fromTokens (d.choices, "|", ""), d.def));
        else
            layout.add (std::make_unique<juce::AudioParameterInt> (pid, name, d.minV, d.maxV, d.def));
    };

    for (int i = 0; i < kNumGP; ++i)
        add (globalId (i), globalDefs[i].name, globalDefs[i]);

    for (int half = 0; half < 2; ++half)
        for (int i = 0; i < kNumHP; ++i)
            add (halfId (half, i), juce::String (half == 0 ? "Lower " : "Upper ") + halfDefs[i].name, halfDefs[i]);

    return layout;
}
} // namespace params
