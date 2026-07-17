# ==============================================================================
#  Magic Drum De-Bleed — JUCE 8 audio plugin (VST3 / AU)
#
#  Configure:   cmake -B build                        (add -G Xcode on macOS)
#  Build:       cmake --build build --config Release
#
#  JUCE is fetched automatically via FetchContent. To use a local JUCE checkout
#  instead, configure with:  -DMDDB_FETCH_JUCE=OFF -DCMAKE_PREFIX_PATH=/path/to/JUCE/install
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

project(MagicDrumDeBleed VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(MDDB_FETCH_JUCE "Download JUCE automatically with FetchContent" ON)
option(MDDB_COPY_PLUGIN "Copy the built plugin into the system plugin folder after build" OFF)
set(MDDB_JUCE_TAG "8.0.8" CACHE STRING "JUCE git tag to fetch")

if(MDDB_FETCH_JUCE)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${MDDB_JUCE_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

# AU is only built on macOS; JUCE's CMake skips it elsewhere automatically.
juce_add_plugin(MagicDrumDeBleed
    PRODUCT_NAME                "Magic Drum De-Bleed"
    COMPANY_NAME                "MagicDSP"
    BUNDLE_ID                   "com.magicdsp.MagicDrumDeBleed"
    PLUGIN_MANUFACTURER_CODE    Magi
    PLUGIN_CODE                 Mddb
    FORMATS                     VST3 AU Standalone
    VST3_CATEGORIES             Fx Dynamics
    AU_MAIN_TYPE                kAudioUnitType_Effect
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${MDDB_COPY_PLUGIN})

juce_generate_juce_header(MagicDrumDeBleed)

target_sources(MagicDrumDeBleed PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/BiquadFilter.cpp
    Source/DSP/SidechainFilter.cpp
    Source/DSP/CompressorProcessor.cpp
    Source/DSP/EQProcessor.cpp
    Source/DSP/LearnAnalyzer.cpp
    Source/UI/CompressorPanel.cpp
    Source/UI/EQPanel.cpp
    Source/UI/OutputStrip.cpp
    Source/UI/PresetBrowser.cpp
    Source/UI/AdvancedView.cpp
    Source/UI/SimpleView.cpp
    Source/Licensing/LicenseValidator.cpp
    Source/Licensing/NagDialog.cpp)

target_include_directories(MagicDrumDeBleed PRIVATE Source)

target_compile_definitions(MagicDrumDeBleed PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_MODAL_LOOPS_PERMITTED=0)

target_link_libraries(MagicDrumDeBleed
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
