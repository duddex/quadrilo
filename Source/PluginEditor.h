#pragma once
#include "PluginProcessor.h"

class QuadriloEditor : public juce::AudioProcessorEditor
{
public:
    explicit QuadriloEditor (QuadriloProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    QuadriloProcessor& processor;

    struct TapUI
    {
        juce::Slider delay, feedback, lowpass, gain, pan;
        juce::ComboBox note;
        std::unique_ptr<SliderAtt> aDelay, aFeedback, aLowpass, aGain, aPan;
        std::unique_ptr<ComboAtt> aNote;
    };

    std::array<TapUI, QuadriloProcessor::numTaps> tapUI;
    juce::Slider mix;
    juce::ToggleButton sync { "Tempo Sync" }, bypass { "Bypass" };
    std::unique_ptr<SliderAtt> aMix;
    std::unique_ptr<ButtonAtt> aSync, aBypass;
    juce::OwnedArray<juce::Label> labels;

    void setupKnob (juce::Slider&, const juce::String& caption);
    void updateSyncVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuadriloEditor)
};
