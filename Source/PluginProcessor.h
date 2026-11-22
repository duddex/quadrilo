// Quadrilo - four-tap stereo delay
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "dsp/QuadDelay.h"

class QuadriloProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numTaps = quadrilo::numTaps;

    QuadriloProcessor();
    ~QuadriloProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Quadrilo"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 20.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    quadrilo::QuadDelay engine;
    bool needsSnap = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuadriloProcessor)
};
