cmake_minimum_required(VERSION 3.22)
project(Quadrilo VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# JUCE is fetched at configure time. Bump GIT_TAG to update the framework.
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Quadrilo
    COMPANY_NAME "duddex"
    COMPANY_WEBSITE "https://github.com/duddex"
    PLUGIN_MANUFACTURER_CODE Ddex
    PLUGIN_CODE Qdlo
    FORMATS VST3 Standalone
    PRODUCT_NAME "Quadrilo"
    DESCRIPTION "Four-tap stereo delay with per-tap feedback, low-pass and pan"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Delay
    NEEDS_WEB_BROWSER FALSE
    NEEDS_CURL FALSE)

target_sources(Quadrilo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/dsp/QuadDelay.h)

target_compile_definitions(Quadrilo PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Quadrilo PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

enable_testing()

add_executable(QuadriloDspTests tests/DspTests.cpp)
target_include_directories(QuadriloDspTests PRIVATE Source)
target_compile_features(QuadriloDspTests PRIVATE cxx_std_20)
add_test(NAME QuadriloDspTests COMMAND QuadriloDspTests)