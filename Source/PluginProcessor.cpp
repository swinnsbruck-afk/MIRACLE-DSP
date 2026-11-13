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

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace params;

namespace
{
juce::PropertiesFile::Options settingsOptions()
{
    juce::PropertiesFile::Options o;
    o.applicationName = "MIRACLE DSP";
    o.filenameSuffix = ".settings";
    o.folderName = "MIRACLE DSP";
    o.osxLibrarySubFolder = "Application Support";
    return o;
}

const juce::Identifier stateId ("MiracleDSPState");
const juce::Identifier foldersId ("Folders");
const juce::Identifier programsId ("Programs");
const juce::Identifier programId ("Program");
} // namespace

MiracleAudioProcessor::MiracleAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.registerBasicFormats();

    for (int half = 0; half < 2; ++half)
        for (int i = 0; i < kNumHP; ++i)
            refs.half[(size_t) half][(size_t) i] = apvts.getRawParameterValue (halfId (half, i));

    for (int i = 0; i < kNumGP; ++i)
        refs.global[(size_t) i] = apvts.getRawParameterValue (globalId (i));

    engine.setParams (&refs);

    // New instances start with the folders used last time; a saved host
    // session overrides this in setStateInformation().
    settings = std::make_unique<juce::PropertiesFile> (settingsOptions());
    editPanelOpen = settings->getBoolValue ("editPanelOpen", false);
    for (int half = 0; half < 2; ++half)
    {
        const auto key = juce::String (half == 0 ? "lastLower" : "lastUpper");
        const auto path = settings->getValue (key + "Folder");
        if (path.isNotEmpty() && juce::File (path).isDirectory())
            loadFolder (half, juce::File (path), settings->getValue (key + "Sound"));
    }

    startTimer (1000);
}

MiracleAudioProcessor::~MiracleAudioProcessor()
{
    stopTimer();
}

bool MiracleAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void MiracleAudioProcessor::prepareToPlay (double sampleRate, int)
{
    engine.prepare (sampleRate);
    engineBankVersion = -1;
    keyboardState.reset();
    uiMidi.reset (sampleRate);
}

void MiracleAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;

    const int version = bankVersion.load();
    if (version != engineBankVersion)
    {
        const juce::SpinLock::ScopedTryLockType lock (bankLock);
        if (lock.isLocked())
        {
            engine.setBanks (banks[0], banks[1]);
            engineBankVersion = version;
        }
    }

    uiMidi.removeNextBlockOfMessages (midi, buffer.getNumSamples());
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    engine.process (buffer, midi);
}

void MiracleAudioProcessor::sendUiMidi (juce::MidiMessage message)
{
    message.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    uiMidi.addMessageToQueue (message);
}

void MiracleAudioProcessor::setEditPanelOpen (bool open)
{
    editPanelOpen = open;
    if (settings != nullptr)
    {
        settings->setValue ("editPanelOpen", open);
        settings->saveIfNeeded();
    }
}

//==============================================================================
void MiracleAudioProcessor::loadFolder (int half, const juce::File& folder, const juce::String& soundPath)
{
    auto list = SoundList::scan (folder);
    {
        const juce::SpinLock::ScopedLockType lock (bankLock);
        lists[half] = list;
    }

    selectSound (half, std::max (0, list->indexOf (soundPath)));
}

void MiracleAudioProcessor::loadFolderOrFile (int half, const juce::File& folderOrFile)
{
    // A single file brings its whole folder along, so browsing keeps working
    if (folderOrFile.isDirectory())
        loadFolder (half, folderOrFile);
    else
        loadFolder (half, folderOrFile.getParentDirectory(), folderOrFile.getFileName());
}

void MiracleAudioProcessor::selectSound (int half, int index)
{
    std::shared_ptr<const SoundList> list;
    {
        const juce::SpinLock::ScopedLockType lock (bankLock);
        list = lists[half];
    }

    if (list == nullptr)
        return;

    auto bank = SampleBank::load (list, index, formatManager);

    {
        const juce::SpinLock::ScopedLockType lock (bankLock);
        if (banks[half] != nullptr)
            retiredBanks.add (banks[half]);
        banks[half] = bank;
    }

    ++bankVersion;
    rememberFolders();
}

void MiracleAudioProcessor::stepSound (int half, int delta)
{
    const int n = getNumSounds (half);
    if (n > 0)
        selectSound (half, ((getSoundIndex (half) + delta) % n + n) % n);
}

std::shared_ptr<const SoundList> MiracleAudioProcessor::getSoundList (int half) const
{
    const juce::SpinLock::ScopedLockType lock (bankLock);
    return lists[half];
}

int MiracleAudioProcessor::getNumSounds (int half) const
{
    auto list = getSoundList (half);
    return list != nullptr ? list->files.size() : 0;
}

int MiracleAudioProcessor::getSoundIndex (int half) const
{
    const juce::SpinLock::ScopedLockType lock (bankLock);
    return banks[half] != nullptr ? banks[half]->index : -1;
}

juce::String MiracleAudioProcessor::getSoundPath (int half) const
{
    auto list = getSoundList (half);
    return list != nullptr ? list->relativePath (getSoundIndex (half)) : juce::String();
}

juce::File MiracleAudioProcessor::getFolder (int half) const
{
    const juce::SpinLock::ScopedLockType lock (bankLock);
    return lists[half] != nullptr ? lists[half]->folder : juce::File();
}

SampleBank::Ptr MiracleAudioProcessor::getBank (int half) const
{
    const juce::SpinLock::ScopedLockType lock (bankLock);
    return banks[half];
}

void MiracleAudioProcessor::rememberFolders()
{
    if (settings == nullptr)
        return;

    for (int half = 0; half < 2; ++half)
    {
        const auto folder = getFolder (half);
        if (folder != juce::File())
        {
            const auto key = juce::String (half == 0 ? "lastLower" : "lastUpper");
            settings->setValue (key + "Folder", folder.getFullPathName());
            settings->setValue (key + "Sound", getSoundPath (half));
        }
    }

    settings->saveIfNeeded();
}

void MiracleAudioProcessor::timerCallback()
{
    // Free banks the audio thread no longer references
    for (int i = retiredBanks.size(); --i >= 0;)
        if (retiredBanks.getObjectPointerUnchecked (i)->getReferenceCount() == 1)
            retiredBanks.remove (i);
}

//==============================================================================
void MiracleAudioProcessor::storeProgram (int half, int program)
{
    juce::ValueTree t (programId);
    for (int i = 0; i < kNumHP; ++i)
        if (auto* param = apvts.getParameter (halfId (half, i)))
            t.setProperty (halfDefs[i].id, param->getValue(), nullptr);

    programs[half][program] = t;
}

void MiracleAudioProcessor::loadProgram (int half, int program)
{
    const auto& t = programs[half][program];
    if (! t.isValid())
        return;

    for (int i = 0; i < kNumHP; ++i)
        if (auto* param = apvts.getParameter (halfId (half, i)))
            if (t.hasProperty (halfDefs[i].id))
                param->setValueNotifyingHost ((float) t[halfDefs[i].id]);
}

void MiracleAudioProcessor::selectHalfProgram (int half, int program)
{
    program = juce::jlimit (0, numPrograms - 1, program);
    if (program == currentProgram[half])
        return;

    storeProgram (half, currentProgram[half]);

    // An empty slot starts as a copy of the current sound
    if (! programs[half][program].isValid())
        storeProgram (half, program);

    currentProgram[half] = program;
    loadProgram (half, program);
}

//==============================================================================
void MiracleAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    for (int half = 0; half < 2; ++half)
        storeProgram (half, currentProgram[half]);

    juce::ValueTree state (stateId);
    state.setProperty ("version", 1, nullptr);
    state.setProperty ("editPanelOpen", editPanelOpen, nullptr);
    state.appendChild (apvts.copyState(), nullptr);

    juce::ValueTree folders (foldersId);
    folders.setProperty ("lower", getFolder (0).getFullPathName(), nullptr);
    folders.setProperty ("upper", getFolder (1).getFullPathName(), nullptr);
    folders.setProperty ("lowerSound", getSoundPath (0), nullptr);
    folders.setProperty ("upperSound", getSoundPath (1), nullptr);
    state.appendChild (folders, nullptr);

    juce::ValueTree progs (programsId);
    progs.setProperty ("currentLower", currentProgram[0], nullptr);
    progs.setProperty ("currentUpper", currentProgram[1], nullptr);
    for (int half = 0; half < 2; ++half)
        for (int p = 0; p < numPrograms; ++p)
            if (programs[half][p].isValid())
            {
                auto t = programs[half][p].createCopy();
                t.setProperty ("half", half, nullptr);
                t.setProperty ("slot", p, nullptr);
                progs.appendChild (t, nullptr);
            }
    state.appendChild (progs, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MiracleAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr)
        return;

    const auto state = juce::ValueTree::fromXml (*xml);
    if (! state.hasType (stateId))
        return;

    editPanelOpen = state.getProperty ("editPanelOpen", editPanelOpen);

    const auto paramState = state.getChildWithName (apvts.state.getType());
    if (paramState.isValid())
        apvts.replaceState (paramState);

    const auto progs = state.getChildWithName (programsId);
    for (auto& halfPrograms : programs)
        for (auto& t : halfPrograms)
            t = {};

    currentProgram[0] = juce::jlimit (0, numPrograms - 1, (int) progs.getProperty ("currentLower", 0));
    currentProgram[1] = juce::jlimit (0, numPrograms - 1, (int) progs.getProperty ("currentUpper", 0));

    for (const auto& t : progs)
    {
        const int half = t["half"];
        const int slot = t["slot"];
        if (half >= 0 && half < 2 && slot >= 0 && slot < numPrograms)
            programs[half][slot] = t.createCopy();
    }

    const auto folders = state.getChildWithName (foldersId);
    for (int half = 0; half < 2; ++half)
    {
        const juce::File folder (folders.getProperty (half == 0 ? "lower" : "upper").toString());
        const auto sound = folders.getProperty (half == 0 ? "lowerSound" : "upperSound").toString();
        if (folder.getFullPathName().isEmpty())
            continue;

        if (folder != getFolder (half))
            loadFolder (half, folder, sound);
        else if (auto list = getSoundList (half); list != nullptr && list->indexOf (sound) >= 0)
            selectSound (half, list->indexOf (sound));
    }
}

//==============================================================================
juce::AudioProcessorEditor* MiracleAudioProcessor::createEditor()
{
    return new MiracleAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MiracleAudioProcessor();
}
