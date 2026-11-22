#pragma once
#include "PluginProcessor.h"

class TapeLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TapeLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float minPos, float maxPos, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool highlighted, bool down) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

class QuadriloEditor : public juce::AudioProcessorEditor
{
public:
    explicit QuadriloEditor (QuadriloProcessor&);
    ~QuadriloEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAtt = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAtt = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    struct TapUI
    {
        juce::Slider delay, feedback, damp, volume, pan;
        juce::ComboBox note;
        std::unique_ptr<SliderAtt> aDelay, aFeedback, aDamp, aVolume, aPan;
        std::unique_ptr<ComboAtt> aNote;
    };

    QuadriloProcessor& processor;
    TapeLookAndFeel laf;
    std::array<TapUI, QuadriloProcessor::numTaps> tapUI;
    juce::Slider mix;
    juce::ToggleButton sync { "SYNC" }, bypass { "BYPASS" };
    std::unique_ptr<SliderAtt> aMix;
    std::unique_ptr<ButtonAtt> aSync, aBypass;
    juce::OwnedArray<juce::Label> labels;

    void setupKnob (juce::Slider&, const juce::String& caption);
    void updateSyncVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (QuadriloEditor)
};
