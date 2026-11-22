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

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter ("note" + n)))
            t.note.addItemList (choice->choices, 1);
        addChildComponent (t.note);
        auto* noteLabel = labels.add (new juce::Label ({}, "Note"));
        noteLabel->setJustificationType (juce::Justification::centred);
        noteLabel->attachToComponent (&t.note, false);
        t.aNote = std::make_unique<ComboAtt> (s, "note" + n, t.note);
    }
    setupKnob (mix, "Mix");
    aMix = std::make_unique<SliderAtt> (s, "mix", mix);
    addAndMakeVisible (sync);
    addAndMakeVisible (bypass);
    aSync = std::make_unique<ButtonAtt> (s, "sync", sync);
    aBypass = std::make_unique<ButtonAtt> (s, "bypass", bypass);
    sync.onStateChange = [this] { updateSyncVisibility(); };
    updateSyncVisibility();
    setSize (620, 420);
}

void QuadriloEditor::updateSyncVisibility()
{
    const bool synced = sync.getToggleState();
    for (auto& t : tapUI)
    {
        t.delay.setVisible (! synced);
        t.note.setVisible (synced);
    }
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
        auto delayArea = row.removeFromLeft (w).withTrimmedTop (16).reduced (2);
        t.delay.setBounds (delayArea);
        t.note.setBounds (delayArea.withSizeKeepingCentre (delayArea.getWidth(), 26));
        for (auto* k : { &t.feedback, &t.lowpass, &t.gain, &t.pan })
            k->setBounds (row.removeFromLeft (w).withTrimmedTop (16).reduced (2));
    }
}
