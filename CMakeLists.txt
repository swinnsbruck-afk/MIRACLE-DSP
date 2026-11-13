# MIRACLE DSP – Copyright (C) 2026 Stefan Schäfer (ESS76)
# SPDX-License-Identifier: AGPL-3.0-or-later

cmake_minimum_required(VERSION 3.22)

project(MiracleDSP VERSION 1.3.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE checkout location (override with -DJUCE_PATH=...)
set(JUCE_PATH "$ENV{HOME}/Developer/JUCE" CACHE PATH "Path to the JUCE framework")
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

juce_add_plugin(MiracleDSP
    COMPANY_NAME "ESS76"
    BUNDLE_ID com.ess76.miracledsp
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Ess7
    PLUGIN_CODE Mdsp
    FORMATS AU VST3 Standalone
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Sampler
    PRODUCT_NAME "MIRACLE DSP")

target_sources(MiracleDSP PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MiracleEngine.cpp
    Source/SampleBank.cpp)

target_compile_definitions(MiracleDSP PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(MiracleDSP
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
