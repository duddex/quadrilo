#include "PluginEditor.h"

QuadriloEditor::QuadriloEditor (QuadriloProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto& s = p.apvts;
    for (int i = 0; i < QuadriloProcessor::numTaps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto& t = tapUI[(size_t) i];
        setupKnob (t.delay, "Delay");
        setupKnob (t.feedback, "Feedback");
        setupKnob (t.lowpass, "LowPass");
        setupKnob (t.gain, "Gain");
        setupKnob (t.pan, "Pan");
        t.aDelay    = std::make_unique<SliderAtt> (s, "delay" + n, t.delay);
        t.aFeedback = std::make_unique<SliderAtt> (s, "feedback" + n, t.feedback);
        t.aLowpass  = std::make_unique<SliderAtt> (s, "lowpass" + n, t.lowpass);
        t.aGain     = std::make_unique<SliderAtt> (s, "gain" + n, t.gain);
        t.aPan      = std::make_unique<SliderAtt> (s, "pan" + n, t.pan);
    }
    setupKnob (mix, "Mix");
    aMix = std::make_unique<SliderAtt> (s, "mix", mix);
    addAndMakeVisible (sync);
    addAndMakeVisible (bypass);
    aSync = std::make_unique<ButtonAtt> (s, "sync", sync);
    aBypass = std::make_unique<ButtonAtt> (s, "bypass", bypass);
    setSize (620, 420);
}

void QuadriloEditor::setupKnob (juce::Slider& k, const juce::String& caption)
{
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    addAndMakeVisible (k);
    auto* l = labels.add (new juce::Label ({}, caption));
    l->setJustificationType (juce::Justification::centred);
    l->attachToComponent (&k, false);
    addAndMakeVisible (l);
}

void QuadriloEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e2227));
    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawText ("Quadrilo", 10, 6, 200, 24, juce::Justification::left);
    g.setFont (14.0f);
    for (int i = 0; i < QuadriloProcessor::numTaps; ++i)
        g.drawText ("Tap " + juce::String (i + 1), 10, 70 + i * 80, 50, 20, juce::Justification::left);
}

void QuadriloEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    auto top = area.removeFromTop (30);
    bypass.setBounds (top.removeFromRight (90));
    sync.setBounds (top.removeFromRight (110));
    mix.setBounds (top.removeFromRight (70).withHeight (30));

    area.removeFromTop (10);
    const int rowH = 80;
    for (auto& t : tapUI)
    {
        auto row = area.removeFromTop (rowH);
        row.removeFromLeft (50);
        const int w = row.getWidth() / 5;
        for (auto* k : { &t.delay, &t.feedback, &t.lowpass, &t.gain, &t.pan })
            k->setBounds (row.removeFromLeft (w).withTrimmedTop (16).reduced (2));
    }
}
