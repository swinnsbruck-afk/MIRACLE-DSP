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

#include "MiracleEngine.h"

#include <cmath>

using namespace params;

namespace
{
constexpr int controlBlock = 16;

// 0..99 -> 1.5 ms .. 30 s, exponential like the Mirage envelope rates
float envTime (float v)
{
    return 0.0015f * std::pow (20000.0f, juce::jlimit (0.0f, 99.0f, v) / 99.0f);
}

float fastTanh (float x)
{
    x = juce::jlimit (-3.0f, 3.0f, x);
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

Envelope::Settings envSettings (const ParamRefs& p, int half, int base, int note, float vel)
{
    Envelope::Settings s;
    const float keyPos = juce::jlimit (0.0f, 1.0f, (float) (note - 36) / 60.0f);

    s.attack  = envTime (p.h (half, base + 0)) * (1.0f - 0.95f * p.h (half, base + 5) / 99.0f * vel);
    s.peak    = p.h (half, base + 1) / 99.0f * (1.0f - p.h (half, base + 6) / 99.0f * (1.0f - vel));
    s.decay   = envTime (p.h (half, base + 2)) * std::exp2 (-(float) (note - 60) / 12.0f * p.h (half, base + 7) / 99.0f);
    s.sustain = s.peak * p.h (half, base + 3) / 99.0f * (1.0f - p.h (half, base + 8) / 99.0f * keyPos);
    return s;
}

float releaseTime (const ParamRefs& p, int half, int base, float releaseVel)
{
    return envTime (p.h (half, base + 4)) * (1.0f - 0.9f * p.h (half, base + 9) / 99.0f * releaseVel);
}
} // namespace

//==============================================================================
void Envelope::start (const Settings& s)
{
    peak = s.peak;
    sustain = s.sustain;
    attackInc = std::max (peak, 1.0e-3f) / std::max (1.0f, s.attack * sampleRate);
    decayCoef = std::exp (-5.0f / std::max (1.0f, s.decay * sampleRate));
    stage = value >= peak ? Decay : Attack;
}

void Envelope::release (float releaseSeconds)
{
    if (stage == Idle)
        return;

    releaseCoef = std::exp (-5.0f / std::max (1.0f, releaseSeconds * sampleRate));
    stage = Release;
}

//==============================================================================
void LadderFilter::set (float cutoffHz, float resonance01, float sampleRate)
{
    cutoffHz = juce::jlimit (20.0f, sampleRate * 0.45f, cutoffHz);
    g = std::tan (juce::MathConstants<float>::pi * cutoffHz / sampleRate);
    G = g / (1.0f + g);
    k = 3.96f * juce::jlimit (0.0f, 1.0f, resonance01);
    gain = 1.0f + 0.25f * k; // the CEM3328 loses bass at high resonance; partially compensate
}

float LadderFilter::process (float x)
{
    const float inv = 1.0f / (1.0f + g);
    const float G2 = G * G, G3 = G2 * G, G4 = G3 * G;
    const float S = (G3 * s[0] + G2 * s[1] + G * s[2] + s[3]) * inv;

    float y = fastTanh ((x * gain - k * S) / (1.0f + k * G4));

    for (auto& st : s)
    {
        const float v = (y - st) * G;
        const float out = v + st;
        st = out + v;
        y = out;
    }

    return y;
}

//==============================================================================
void SampleOsc::setup (const Wavesample* w, const ParamRefs& p, int half)
{
    ws = w;
    active = false;

    if (w == nullptr || w->data.size() < 2)
        return;

    const int n = (int) w->data.size();
    start = juce::jlimit (0, n - 2, (int) ((double) n * p.h (half, hpSmpStart) / 1000.0));
    end   = juce::jlimit (start + 1, n, (int) ((double) n * p.h (half, hpSmpEnd) / 1000.0));

    const int mode = (int) p.h (half, hpLoopMode); // 0 Off, 1 Manual, 2 File
    looping = false;

    if (mode == 2 && w->fileLoopStart >= 0)
    {
        loopStart = juce::jlimit (0, n - 2, w->fileLoopStart);
        loopEnd   = juce::jlimit (loopStart + 1, n, w->fileLoopEnd);
        looping = true;
    }
    else if (mode == 1)
    {
        const double len = end - start;
        loopStart = start + (int) (len * p.h (half, hpLoopStart) / 1000.0);
        loopEnd   = start + (int) (len * p.h (half, hpLoopEnd) / 1000.0);
        looping = loopEnd - loopStart >= 2;
    }

    if (looping)
    {
        end = loopEnd;
        if (start >= loopEnd)
            start = loopStart;
    }

    pos = start;
    clock = 1.0;
    held = 0.0f;
    active = true;
}

void SampleOsc::advance (double step)
{
    pos += step;

    if (looping)
    {
        if (pos >= loopEnd)
            pos = loopStart + std::fmod (pos - loopStart, (double) (loopEnd - loopStart));
    }
    else if (pos >= end)
    {
        active = false;
    }
}

// The Mirage plays a wavesample by varying its DAC clock with pitch. We step a
// virtual DAC clock and fetch a new 8-bit value on each tick (zero-order hold),
// which recreates its characteristic aliasing and quantisation grit.
float SampleOsc::tickLoFi (double dacTicksPerOutput, double sourcePerTick)
{
    clock += dacTicksPerOutput;

    while (clock >= 1.0 && active)
    {
        clock -= 1.0;
        held = std::round (ws->data[(size_t) pos] * 127.0f) / 127.0f;
        advance (sourcePerTick);
    }

    return active ? held : 0.0f;
}

float SampleOsc::tickHiFi (double sourcePerOutput)
{
    const int i = (int) pos;
    const float frac = (float) (pos - i);
    const float s0 = ws->data[(size_t) i];
    float s1 = 0.0f;

    if (i + 1 < end)
        s1 = ws->data[(size_t) i + 1];
    else if (looping)
        s1 = ws->data[(size_t) loopStart];

    advance (sourcePerOutput);
    return s0 + frac * (s1 - s0);
}

//==============================================================================
void MiracleEngine::prepare (double sr)
{
    sampleRate = sr;

    if (defaultBank == nullptr)
        defaultBank = SampleBank::createDefault();

    for (auto& v : voices)
    {
        v.ampEnv.setSampleRate (sr);
        v.filterEnv.setSampleRate (sr);
        v.ampEnv.reset();
        v.filterEnv.reset();
        v.filter.reset();
        v.active = false;
        v.bank = nullptr;
    }

    stackSize = {};
    pitchBend = 0.0f;
    modWheel = 0.0f;
}

void MiracleEngine::setBanks (SampleBank::Ptr lower, SampleBank::Ptr upper)
{
    banks[0] = std::move (lower);
    banks[1] = std::move (upper);
}

void MiracleEngine::allNotesOff()
{
    for (auto& v : voices)
        if (v.active)
            releaseVoice (v, 0.0f);

    stackSize = {};
    sustainPedal = false;
}

const SampleBank* MiracleEngine::bankFor (int half) const
{
    const auto& b = banks[half];
    if (b != nullptr && ! b->samples.empty())
        return b.get();

    return defaultBank.get();
}

int MiracleEngine::halfForNote (int note) const
{
    return note >= (int) p->g (gpSplit) ? 1 : 0;
}

void MiracleEngine::process (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    buffer.clear();

    if (p == nullptr || defaultBank == nullptr)
        return;

    const int numSamples = buffer.getNumSamples();
    float* out = buffer.getWritePointer (0);
    int pos = 0;

    auto renderUntil = [&] (int endPos)
    {
        while (pos < endPos)
        {
            const int chunk = std::min (controlBlock, endPos - pos);
            renderVoices (out + pos, chunk);
            pos += chunk;
        }
    };

    for (const auto meta : midi)
    {
        renderUntil (juce::jlimit (0, numSamples, meta.samplePosition));
        handleMidi (meta.getMessage());
    }

    renderUntil (numSamples);

    const float vol = p->g (gpVolume) / 99.0f;
    const float gain = 0.9f * vol * vol;

    for (int i = 0; i < numSamples; ++i)
        out[i] = fastTanh (out[i] * gain);

    for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
        buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);
}

void MiracleEngine::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
        noteOn (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isNoteOff())
        noteOff (m.getNoteNumber(), m.getFloatVelocity());
    else if (m.isPitchWheel())
        pitchBend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isControllerOfType (1))
        modWheel = (float) m.getControllerValue() / 127.0f;
    else if (m.isSustainPedalOn())
        sustainPedal = true;
    else if (m.isSustainPedalOff())
    {
        sustainPedal = false;
        for (auto& v : voices)
            if (v.active && v.sustained)
                releaseVoice (v, 0.0f);
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff())
        allNotesOff();
}

Voice* MiracleEngine::findVoiceToUse (int half)
{
    juce::ignoreUnused (half);
    const int limit = juce::jlimit (1, maxVoices, (int) p->g (gpVoices));

    Voice* oldestReleased = nullptr;
    Voice* oldest = nullptr;

    for (int i = 0; i < limit; ++i)
    {
        auto& v = voices[(size_t) i];
        if (! v.active)
            return &v;

        if (! v.keyDown && ! v.sustained && (oldestReleased == nullptr || v.age < oldestReleased->age))
            oldestReleased = &v;

        if (oldest == nullptr || v.age < oldest->age)
            oldest = &v;
    }

    return oldestReleased != nullptr ? oldestReleased : oldest;
}

void MiracleEngine::noteOn (int note, float velocity)
{
    const int half = halfForNote (note);

    if (p->h (half, hpMono) > 0.5f)
    {
        auto& stack = noteStack[(size_t) half];
        auto& size = stackSize[(size_t) half];

        for (int i = 0; i < size; ++i)
            if (stack[(size_t) i] == note)
            {
                std::copy (stack.begin() + i + 1, stack.begin() + size, stack.begin() + i);
                --size;
                break;
            }

        if (size == (int) stack.size())
        {
            std::copy (stack.begin() + 1, stack.end(), stack.begin());
            --size;
        }

        stack[(size_t) size++] = note;

        for (auto& v : voices)
            if (v.active && v.half == half && v.keyDown)
            {
                startVoice (v, half, note, velocity, true);
                return;
            }
    }

    if (auto* v = findVoiceToUse (half))
        startVoice (*v, half, note, velocity, false);
}

void MiracleEngine::noteOff (int note, float releaseVelocity)
{
    for (int half = 0; half < 2; ++half)
    {
        auto& stack = noteStack[(size_t) half];
        auto& size = stackSize[(size_t) half];
        for (int i = 0; i < size; ++i)
            if (stack[(size_t) i] == note)
            {
                std::copy (stack.begin() + i + 1, stack.begin() + size, stack.begin() + i);
                --size;
                break;
            }
    }

    for (auto& v : voices)
    {
        if (! v.active || ! v.keyDown || v.note != note)
            continue;

        const int half = v.half;
        if (p->h (half, hpMono) > 0.5f && stackSize[(size_t) half] > 0)
        {
            // last-note priority: fall back to the previous held key
            startVoice (v, half, noteStack[(size_t) half][(size_t) stackSize[(size_t) half] - 1], v.velocity, true);
        }
        else if (sustainPedal)
        {
            v.keyDown = false;
            v.sustained = true;
        }
        else
        {
            releaseVoice (v, releaseVelocity);
        }
    }
}

void MiracleEngine::startVoice (Voice& v, int half, int note, float velocity, bool legato)
{
    const auto* bank = bankFor (half);
    const int n = (int) bank->samples.size();

    // Mix Mode: oscillator 2 plays the next sound of the folder, like the Mirage's alternate wavesample
    const int idx = 0;
    const int idx2 = p->h (half, hpMixMode) > 0.5f && n > 1 ? 1 : 0;

    v.bank = const_cast<SampleBank*> (bank);
    v.osc[0].setup (&bank->samples[(size_t) idx], *p, half);
    v.osc[1].setup (&bank->samples[(size_t) idx2], *p, half);

    v.half = half;
    v.note = note;
    v.velocity = velocity;
    v.active = true;
    v.keyDown = true;
    v.sustained = false;
    v.age = ++ageCounter;
    v.controlCounter = 0;

    v.oscMix = juce::jlimit (0.0f, 1.0f, p->h (half, hpOscMix) / 99.0f
                                           + p->h (half, hpMixVel) / 99.0f * (velocity - 0.5f));

    if (! legato)
    {
        v.filter.reset();
        v.ampEnv.reset();
        v.filterEnv.reset();
    }

    v.ampEnv.start (envSettings (*p, half, hpAAtt, note, velocity));
    v.filterEnv.start (envSettings (*p, half, hpFAtt, note, velocity));
}

void MiracleEngine::releaseVoice (Voice& v, float releaseVelocity)
{
    v.keyDown = false;
    v.sustained = false;
    v.ampEnv.release (releaseTime (*p, v.half, hpAAtt, releaseVelocity));
    v.filterEnv.release (releaseTime (*p, v.half, hpFAtt, releaseVelocity));
}

void MiracleEngine::renderVoices (float* out, int numSamples)
{
    const auto sr = (float) sampleRate;
    const double dacRate = p->g (gpRate) * 1000.0;
    const bool lofi = p->g (gpLoFi) > 0.5f;
    const float globalSemis = pitchBend.load (std::memory_order_relaxed) * p->g (gpBend) + 12.0f * p->g (gpOctave) + p->g (gpTune) / 100.0f;

    float lfoSemis[2];
    for (int half = 0; half < 2; ++half)
    {
        const float hz = 0.1f * std::pow (200.0f, p->h (half, hpLfoSpeed) / 99.0f);
        auto& ph = lfoPhase[half];
        ph += (double) hz * numSamples / sampleRate;
        ph -= std::floor (ph);
        const float tri = 4.0f * std::abs ((float) ph - 0.5f) - 1.0f;
        const float depth = std::min (99.0f, p->h (half, hpLfoDepth) + modWheel.load (std::memory_order_relaxed) * 99.0f) / 99.0f;
        lfoSemis[half] = tri * depth * depth * 3.0f;
    }

    for (auto& v : voices)
    {
        if (! v.active)
            continue;

        const int half = v.half;
        const float baseSemis = (float) v.note + 12.0f * p->h (half, hpOctave) + p->h (half, hpCoarse) + p->h (half, hpFine) / 100.0f
                              + globalSemis + lfoSemis[half];

        double dacTicks[2] {}, srcPerTick[2] {}, srcStep[2] {};
        bool use[2] { v.oscMix < 1.0f && v.osc[0].active, v.oscMix > 0.0f && v.osc[1].active };

        for (int o = 0; o < 2; ++o)
        {
            if (! use[o])
                continue;

            const auto* ws = v.osc[o].ws;
            const float semis = baseSemis - (float) ws->rootNote + (o == 1 ? p->h (half, hpDetune) / 99.0f * 0.5f : 0.0f);
            const double ratio = std::exp2 (semis / 12.0);
            dacTicks[o]   = dacRate * ratio / sampleRate;
            srcPerTick[o] = ws->fileRate / dacRate;
            srcStep[o]    = ratio * ws->fileRate / sampleRate;
        }

        const float cutoffOct = p->h (half, hpCutoff) / 99.0f * 9.5f
                              + p->h (half, hpKeyTrack) / 4.0f * (float) (v.note - 60) / 12.0f
                              + v.filterEnv.getValue() * 9.5f;
        v.filter.set (30.0f * std::exp2 (cutoffOct), p->h (half, hpReso) / 40.0f, sr);

        const float level = p->h (half, hpLevel) / 99.0f;
        const float mix = v.oscMix;

        for (int i = 0; i < numSamples; ++i)
        {
            float s = 0.0f;

            if (use[0] && v.osc[0].active)
                s += (1.0f - mix) * (lofi ? v.osc[0].tickLoFi (dacTicks[0], srcPerTick[0])
                                          : v.osc[0].tickHiFi (srcStep[0]));
            if (use[1] && v.osc[1].active)
                s += mix * (lofi ? v.osc[1].tickLoFi (dacTicks[1], srcPerTick[1])
                                 : v.osc[1].tickHiFi (srcStep[1]));

            v.filterEnv.next();
            const float amp = v.ampEnv.next();
            out[i] += v.filter.process (s) * amp * level;
        }

        const bool soundLeft = (use[0] && v.osc[0].active) || (use[1] && v.osc[1].active);
        if (! v.ampEnv.isActive() || ! soundLeft)
        {
            v.active = false;
            v.bank = nullptr;
        }
    }
}
