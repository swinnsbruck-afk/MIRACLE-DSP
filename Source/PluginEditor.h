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

#include <juce_audio_utils/juce_audio_utils.h>

#include "PluginProcessor.h"

// 1980s keyboard look: slate chassis, flat keypad, mini faders, silkscreen labels
class MiracleLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MiracleLookAndFeel();

    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float minPos, float maxPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return 7; }
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    void drawScrollbar (juce::Graphics&, juce::ScrollBar&, int x, int y, int width, int height, bool vertical,
                        int thumbStart, int thumbSize, bool over, bool down) override;
};

// Green backlit 7-segment display
class LedDisplay : public juce::Component
{
public:
    void show (const juce::String& left, const juce::String& right, const juce::String& caption);
    void paint (juce::Graphics&) override;

private:
    void drawDigits (juce::Graphics&, juce::Rectangle<float> area, const juce::String& text, int digits);

    juce::String left { "L1" }, right, caption { "MIRACLE DSP" };
};

class WaveformView : public juce::Component
{
public:
    explicit WaveformView (MiracleAudioProcessor& p) : audioProcessor (p) {}
    void setHalf (int h) { half = h; repaint(); }
    void paint (juce::Graphics&) override;

private:
    MiracleAudioProcessor& audioProcessor;
    int half = 0;
};

// Flat rectangular panel key
class KeyCap : public juce::Button
{
public:
    KeyCap (const juce::String& name, juce::Colour colour) : juce::Button (name), cap (colour) {}
    void setLit (bool shouldBeLit);
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    juce::Colour cap;
    bool lit = false;
};

// Status line of one keyboard half inside the disk tray
class SoundSlot : public juce::Component,
                  public juce::SettableTooltipClient
{
public:
    SoundSlot (MiracleAudioProcessor& p, int half);

    std::function<void()> onSelect;

    void setActive (bool a) { active = a; repaint(); }
    void refresh();
    void syncOctave();
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

private:
    int getOctave() const;

    MiracleAudioProcessor& audioProcessor;
    const int half;
    bool active = false;
    int shownOctave = 0;
    juce::String soundName, counter;
};

// Pitch and mod wheel. The pitch wheel springs back to the centre.
class ControlWheel : public juce::Component
{
public:
    explicit ControlWheel (bool springLoaded) : spring (springLoaded) {}

    std::function<void (float)> onChange;

    // Follows the hardware wheel of a connected controller
    void setPosition (float newValue);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    const bool spring;
    float value = 0.0f;
    float dragStart = 0.0f;
};

class RealisticKeyboard : public juce::MidiKeyboardComponent
{
public:
    RealisticKeyboard (juce::MidiKeyboardState& s) : MidiKeyboardComponent (s, horizontalKeyboard) {}
    int splitNote = 60;

protected:
    void drawWhiteNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour lineColour, juce::Colour textColour) override;
    void drawBlackNote (int note, juce::Graphics&, juce::Rectangle<float> area, bool isDown, bool isOver,
                        juce::Colour noteFillColour) override;
};

class MiracleAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    public juce::FileDragAndDropTarget,
                                    private juce::Timer,
                                    private juce::ListBoxModel
{
public:
    explicit MiracleAudioProcessorEditor (MiracleAudioProcessor&);
    ~MiracleAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    void setEditPanelVisible (bool open);

private:
    struct Fader
    {
        juce::Slider slider;
        juce::Label label;
        int index = 0;
        bool global = false;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Toggle
    {
        juce::ToggleButton button;
        int index = 0;
        bool global = false;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> attachment;
    };

    struct Section
    {
        juce::String title;
        juce::Rectangle<int> bounds;
    };

    enum class PadMode { idle, param, value, program };

    struct ParamEntry
    {
        int number = 0, index = 0;
        bool global = false;
    };

    struct PanelKey
    {
        std::unique_ptr<KeyCap> cap;
        juce::String label;
        int column = 0, row = 0;
    };

    // ListBoxModel
    int getNumRows() override;
    void paintListBoxItem (int row, juce::Graphics&, int width, int height, bool selected) override;
    void selectedRowsChanged (int lastRowSelected) override;

    void timerCallback() override;
    KeyCap& addKey (const juce::String& label, juce::Colour colour, int column, int row, std::function<void()> action);
    void buildKeypad();
    Fader& addFader (int index, bool global);
    Toggle& addToggle (int index, bool global);
    void selectHalf (int half);
    void attachHalf();
    void refreshSounds();
    void chooseFolder (int half);
    void showParam (int index, bool global, double value);
    void showIdle();
    const params::Def& defFor (int index, bool global) const;
    void renderBackground();

    // Keypad workflow: type a parameter number, then change its value
    void pressDigit (int digit);
    void pressParam();
    void pressValue();
    void pressStep (int delta);
    void pressProgram();
    void pressEnter();
    void pressCancel();
    void showSelected();
    float currentValue (const ParamEntry&) const;
    void setValue (const ParamEntry&, int value);
    void shiftOctave (int delta);

    MiracleAudioProcessor& audioProcessor;
    MiracleLookAndFeel laf;

    int currentHalf = 0;
    int lastBankVersion = -1;
    juce::int64 lastTouch = 0;
    bool updatingList = false;
    bool editOpen = false;
    juce::Image background;
    juce::Rectangle<int> driveSlot;
    juce::String folderText;

    std::vector<ParamEntry> entries;
    int selectedEntry = -1;
    PadMode padMode = PadMode::idle;
    juce::String padDigits;

    LedDisplay led;
    std::vector<PanelKey> keys;
    KeyCap* lowerKey = nullptr;
    KeyCap* upperKey = nullptr;
    KeyCap* programKeys[MiracleAudioProcessor::numPrograms] {};
    KeyCap editKey;

    SoundSlot upperSlot, lowerSlot;
    juce::ListBox soundList;
    ControlWheel pitchWheel { true }, modWheel { false };

    juce::Slider volumeFader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> volumeAttachment;

    juce::ComboBox loopModeBox;
    juce::Label loopLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> loopModeAttachment;
    WaveformView waveform;
    RealisticKeyboard keyboard;
    juce::TooltipWindow tooltipWindow { this, 700 };

    std::vector<std::unique_ptr<Fader>> faders;
    std::vector<std::unique_ptr<Toggle>> toggles;
    std::vector<Section> sections;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiracleAudioProcessorEditor)
};
