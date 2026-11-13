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

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <atomic>

#include "Params.h"
#include "SampleBank.h"

// Raw parameter pointers for fast reads on the audio thread
struct ParamRefs
{
    std::array<std::array<std::atomic<float>*, params::kNumHP>, 2> half {};
    std::array<std::atomic<float>*, params::kNumGP> global {};

    float h (int halfIndex, int p) const { return half[(size_t) halfIndex][(size_t) p]->load (std::memory_order_relaxed); }
    float g (int p) const                { return global[(size_t) p]->load (std::memory_order_relaxed); }
};

// Mirage-style envelope: Attack -> Peak, Decay -> Sustain, Release
class Envelope
{
public:
    struct Settings { float attack, peak, decay, sustain; };

    void setSampleRate (double sr) { sampleRate = (float) sr; }
    void start (const Settings& s);
    void release (float releaseSeconds);
    void reset() { stage = Idle; value = 0.0f; }
    bool isActive() const { return stage != Idle; }
    float getValue() const { return value; }

    float next()
    {
        switch (stage)
        {
            case Attack:
                value += attackInc;
                if (value >= peak) { value = peak; stage = Decay; }
                break;
            case Decay:
                value = sustain + (value - sustain) * decayCoef;
                if (std::abs (value - sustain) < 1.0e-4f) { value = sustain; stage = Sustain; }
                break;
            case Sustain:
                break;
            case Release:
                value *= releaseCoef;
                if (value < 1.0e-4f) { value = 0.0f; stage = Idle; }
                break;
            case Idle:
            default:
                break;
        }
        return value;
    }

private:
    enum Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage = Idle;
    float sampleRate = 44100.0f;
    float value = 0.0f, peak = 1.0f, sustain = 1.0f;
    float attackInc = 1.0f, decayCoef = 0.0f, releaseCoef = 0.0f;
};

// 4-pole resonant low-pass in the spirit of the Curtis CEM3328 used in the Mirage
// (zero-delay-feedback ladder with soft saturation in the feedback path)
class LadderFilter
{
public:
    void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }
    void set (float cutoffHz, float resonance01, float sampleRate);
    float process (float x);

private:
    float s[4] {};
    float g = 0.5f, G = 0.33f, k = 0.0f, gain = 1.0f;
};

// One sample playback oscillator with Mirage-style sample clock emulation
struct SampleOsc
{
    const Wavesample* ws = nullptr;
    bool active = false;
    double pos = 0.0;
    double clock = 1.0;
    float held = 0.0f;
    int start = 0, end = 0, loopStart = 0, loopEnd = 0;
    bool looping = false;

    void setup (const Wavesample* w, const ParamRefs& p, int half);
    float tickLoFi (double dacTicksPerOutput, double sourcePerTick);
    float tickHiFi (double sourcePerOutput);

private:
    void advance (double step);
};

struct Voice
{
    bool active = false;
    bool keyDown = false;
    bool sustained = false;
    int half = 0;
    int note = 60;
    float velocity = 1.0f;
    juce::uint32 age = 0;

    SampleBank::Ptr bank;
    SampleOsc osc[2];
    Envelope ampEnv, filterEnv;
    LadderFilter filter;
    float oscMix = 0.0f;
    int controlCounter = 0;
};

class MiracleEngine
{
public:
    static constexpr int maxVoices = 16;

    void prepare (double sampleRate);
    void setParams (const ParamRefs* refs) { p = refs; }
    void setBanks (SampleBank::Ptr lower, SampleBank::Ptr upper);
    void allNotesOff();
    void process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi);

    // Wheel positions for the editor, -1..1 and 0..1
    float getPitchBend() const { return pitchBend.load (std::memory_order_relaxed); }
    float getModWheel() const  { return modWheel.load (std::memory_order_relaxed); }

private:
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float velocity);
    void noteOff (int note, float releaseVelocity);
    void startVoice (Voice& v, int half, int note, float velocity, bool legato);
    void releaseVoice (Voice& v, float releaseVelocity);
    Voice* findVoiceToUse (int half);
    void renderVoices (float* out, int numSamples);
    const SampleBank* bankFor (int half) const;
    int halfForNote (int note) const;

    const ParamRefs* p = nullptr;
    double sampleRate = 44100.0;
    std::array<Voice, maxVoices> voices;
    SampleBank::Ptr banks[2];
    SampleBank::Ptr defaultBank;
    juce::uint32 ageCounter = 0;

    std::atomic<float> pitchBend { 0.0f };   // -1..1
    std::atomic<float> modWheel { 0.0f };    // 0..1
    bool sustainPedal = false;
    double lfoPhase[2] {};

    // mono mode note stacks
    std::array<std::array<int, 16>, 2> noteStack {};
    std::array<int, 2> stackSize {};
};
