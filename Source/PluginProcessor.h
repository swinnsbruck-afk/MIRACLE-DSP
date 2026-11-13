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
#include <juce_audio_utils/juce_audio_utils.h>

#include "MiracleEngine.h"
#include "Params.h"
#include "SampleBank.h"

class MiracleAudioProcessor : public juce::AudioProcessor,
                             private juce::Timer
{
public:
    static constexpr int numPrograms = 4;

    MiracleAudioProcessor();
    ~MiracleAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    // Host programs are not used; the programs live per keyboard half
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Sample folders (one per keyboard half, subfolders included) and the
    // selected sound of each half
    void loadFolder (int half, const juce::File& folder, const juce::String& soundPath = {});
    void loadFolderOrFile (int half, const juce::File& folderOrFile);
    void selectSound (int half, int index);
    void stepSound (int half, int delta);
    std::shared_ptr<const SoundList> getSoundList (int half) const;
    int getNumSounds (int half) const;
    int getSoundIndex (int half) const;
    juce::String getSoundPath (int half) const;
    juce::File getFolder (int half) const;
    SampleBank::Ptr getBank (int half) const;
    int getBankVersion() const { return bankVersion.load(); }

    // Programs: 4 parameter snapshots per keyboard half
    void selectHalfProgram (int half, int program);
    int getHalfProgram (int half) const { return currentProgram[half]; }

    // Pitch and mod wheels on the panel, they also show what the controller does
    void sendUiMidi (juce::MidiMessage message);
    float getPitchBend() const { return engine.getPitchBend(); }
    float getModWheel() const  { return engine.getModWheel(); }

    // Fold-out edit panel, remembered with the project
    bool isEditPanelOpen() const { return editPanelOpen; }
    void setEditPanelOpen (bool open);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

private:
    void timerCallback() override;
    void storeProgram (int half, int program);
    void loadProgram (int half, int program);
    void rememberFolders();

    juce::AudioFormatManager formatManager;
    juce::MidiMessageCollector uiMidi;
    bool editPanelOpen = false;
    MiracleEngine engine;
    ParamRefs refs;

    juce::SpinLock bankLock;
    std::shared_ptr<const SoundList> lists[2];
    SampleBank::Ptr banks[2];
    juce::ReferenceCountedArray<SampleBank> retiredBanks;
    std::atomic<int> bankVersion { 0 };
    int engineBankVersion = -1;

    juce::ValueTree programs[2][numPrograms];
    int currentProgram[2] { 0, 0 };

    std::unique_ptr<juce::PropertiesFile> settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MiracleAudioProcessor)
};
