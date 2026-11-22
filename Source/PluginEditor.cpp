// Quadrilo - four-tap stereo delay
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginEditor.h"

namespace
{
constexpr int kWidth = 640, kHeight = 450;
constexpr float colW = 120.0f, colX0 = 70.0f;      // four columns aligned with the four tape heads
constexpr float loopTop = 45.0f, loopBottom = 220.0f;
constexpr float bottomPanelY = 262.0f;

const juce::Colour tapeColour (0xffb5835a), tapeHighlight (0xffdcb894);
const juce::Colour amber (0xffffb35c);

float columnCentre (int i) { return colX0 + colW * ((float) i + 0.5f); }

juce::String panText (double v)
{
    const int pct = (int) std::round (std::abs (v - 0.5) * 200.0);
    if (pct == 0) return "C";
    return (v < 0.5 ? "L " : "R ") + juce::String (pct);
}

double panFromText (const juce::String& t)
{
    const auto s = t.trim().toUpperCase();
    if (s.startsWith ("C")) return 0.5;
    const double n = juce::jlimit (0.0, 100.0, s.substring (1).getDoubleValue());
    return s.startsWith ("L") ? 0.5 - n / 200.0 : 0.5 + n / 200.0;
}

juce::String dbText (double v)
{
    return v <= 0.0001 ? juce::String ("-inf dB") : juce::String (20.0 * std::log10 (v), 2) + " dB";
}

double dbFromText (const juce::String& t)
{
    if (t.containsIgnoreCase ("inf")) return 0.0;
    return juce::jlimit (0.0, 1.0, std::pow (10.0, t.getDoubleValue() / 20.0));
}

void styleText (juce::Slider& s)
{
    s.setColour (juce::Slider::textBoxTextColourId, amber);
    s.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0xff17120f));
    s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff6b4a30));
}
}

TapeLookAndFeel::TapeLookAndFeel()
{
    setColour (juce::Label::textColourId, juce::Colour (0xffe8d8c4));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff17120f));
    setColour (juce::ComboBox::textColourId, amber);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6b4a30));
    setColour (juce::ComboBox::arrowColourId, amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff17120f));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe8d8c4));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff6b4a30));
}

juce::Font TapeLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (12.5f, juce::Font::bold));
}

juce::Label* TapeLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (juce::Font (juce::FontOptions (12.0f)));
    return l;
}

void TapeLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f - 3.0f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    g.setColour (juce::Colour (0xff0e0b09));
    g.fillEllipse (c.x - r - 3, c.y - r - 3, 2 * (r + 3), 2 * (r + 3));

    juce::Path arc;
    arc.addCentredArc (c.x, c.y, r + 1.5f, r + 1.5f, 0.0f, startAngle, angle, true);
    g.setColour (amber.withAlpha (0.85f));
    g.strokePath (arc, juce::PathStrokeType (2.5f));

    juce::ColourGradient body (juce::Colour (0xffe0a070), c.x - r * 0.4f, c.y - r * 0.5f,
                               juce::Colour (0xff6e3a1c), c.x + r * 0.6f, c.y + r * 0.7f, true);
    g.setGradientFill (body);
    g.fillEllipse (c.x - r + 3, c.y - r + 3, 2 * r - 6, 2 * r - 6);

    g.setColour (juce::Colour (0xff2a1a10));
    g.drawEllipse (c.x - r + 3, c.y - r + 3, 2 * r - 6, 2 * r - 6, 1.2f);

    juce::Path p;
    p.addRoundedRectangle (-1.8f, -(r - 5.0f), 3.6f, r * 0.5f, 1.5f);
    g.setColour (juce::Colour (0xfffff3e0));
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void TapeLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) w * 0.5f;
    g.setColour (juce::Colour (0xff0e0b09));
    g.fillRoundedRectangle (cx - 4.0f, (float) y, 8.0f, (float) h, 4.0f);
    g.setColour (amber.withAlpha (0.8f));
    g.fillRoundedRectangle (cx - 2.0f, pos, 4.0f, (float) (y + h) - pos, 2.0f);

    juce::Rectangle<float> thumb (cx - 15.0f, pos - 8.0f, 30.0f, 16.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe0a070), thumb.getX(), thumb.getY(),
                                             juce::Colour (0xff6e3a1c), thumb.getX(), thumb.getBottom(), false));
    g.fillRoundedRectangle (thumb, 3.0f);
    g.setColour (juce::Colour (0xfffff3e0));
    g.drawLine (thumb.getX() + 4.0f, pos, thumb.getRight() - 4.0f, pos, 1.5f);
}

void TapeLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool highlighted, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? juce::Colour (0xffc0301c) : juce::Colour (0xff3a1a14));
    g.fillRoundedRectangle (r, 4.0f);
    if (on)
    {
        g.setColour (juce::Colour (0x40ff6040));
        g.drawRoundedRectangle (r.expanded (1.5f), 5.0f, 2.0f);
    }
    g.setColour (highlighted ? juce::Colour (0xffffffff) : juce::Colour (0xffe8d8c4));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText (b.getButtonText(), r, juce::Justification::centred);
}

QuadriloEditor::QuadriloEditor (QuadriloProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&laf);
    auto& s = p.apvts;

    for (int i = 0; i < QuadriloProcessor::numTaps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto& t = tapUI[(size_t) i];

        setupKnob (t.delay, "Delay");
        setupKnob (t.volume, "Volume");
        setupKnob (t.damp, "Damp");
        setupKnob (t.pan, "Pan");
        setupKnob (t.feedback, "Feedback");

        t.aDelay    = std::make_unique<SliderAtt> (s, "delay" + n, t.delay);
        t.aVolume   = std::make_unique<SliderAtt> (s, "volume" + n, t.volume);
        t.aDamp     = std::make_unique<SliderAtt> (s, "damp" + n, t.damp);
        t.aPan      = std::make_unique<SliderAtt> (s, "pan" + n, t.pan);
        t.aFeedback = std::make_unique<SliderAtt> (s, "feedback" + n, t.feedback);

        t.volume.textFromValueFunction = dbText;
        t.volume.valueFromTextFunction = dbFromText;
        t.pan.textFromValueFunction = panText;
        t.pan.valueFromTextFunction = panFromText;
        t.delay.textFromValueFunction = [] (double v) { return juce::String (v, 1) + " ms"; };
        t.delay.valueFromTextFunction = [] (const juce::String& txt) { return txt.getDoubleValue(); };
        for (auto* pc : { &t.feedback, &t.damp })
        {
            pc->textFromValueFunction = [] (double v) { return juce::String (v * 100.0, 1) + " %"; };
            pc->valueFromTextFunction = [] (const juce::String& txt) { return txt.getDoubleValue() / 100.0; };
        }
        for (auto* k : { &t.delay, &t.feedback, &t.damp, &t.volume, &t.pan })
            k->updateText();

        if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (s.getParameter ("note" + n)))
            t.note.addItemList (choice->choices, 1);
        addChildComponent (t.note);
        auto* noteLabel = labels.add (new juce::Label ({}, "Note"));
        noteLabel->setJustificationType (juce::Justification::centred);
        noteLabel->attachToComponent (&t.note, false);
        t.aNote = std::make_unique<ComboAtt> (s, "note" + n, t.note);
    }

    mix.setSliderStyle (juce::Slider::LinearVertical);
    mix.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (mix);
    aMix = std::make_unique<SliderAtt> (s, "mix", mix);

    addAndMakeVisible (sync);
    addAndMakeVisible (bypass);
    aSync = std::make_unique<ButtonAtt> (s, "sync", sync);
    aBypass = std::make_unique<ButtonAtt> (s, "bypass", bypass);
    sync.onStateChange = [this] { updateSyncVisibility(); };
    updateSyncVisibility();

    setSize (kWidth, kHeight);
}

QuadriloEditor::~QuadriloEditor()
{
    setLookAndFeel (nullptr);
}

void QuadriloEditor::setupKnob (juce::Slider& k, const juce::String& caption)
{
    k.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 56, 17);
    k.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
    styleText (k);
    addAndMakeVisible (k);
    auto* l = labels.add (new juce::Label ({}, caption));
    l->setJustificationType (juce::Justification::centred);
    l->attachToComponent (&k, false);
    addAndMakeVisible (l);
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

void QuadriloEditor::paint (juce::Graphics& g)
{
    const auto w = (float) getWidth();

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a322c), 0, 0, juce::Colour (0xff17130f), 0, bottomPanelY, false));
    g.fillRect (0.0f, 0.0f, w, bottomPanelY);

    // tape loop: rounded rectangle wrapped around four rollers
    const juce::Rectangle<float> loop (50.0f, loopTop, 520.0f, loopBottom - loopTop);
    const float cr = 36.0f;
    juce::Path tape;
    tape.addRoundedRectangle (loop.getX(), loop.getY(), loop.getWidth(), loop.getHeight(), cr);

    g.setColour (juce::Colour (0x66000000));
    g.strokePath (tape, juce::PathStrokeType (9.0f), juce::AffineTransform::translation (2.0f, 4.0f));
    g.setColour (tapeColour);
    g.strokePath (tape, juce::PathStrokeType (7.0f));
    g.setColour (tapeHighlight.withAlpha (0.6f));
    g.strokePath (tape, juce::PathStrokeType (1.5f));

    const juce::Point<float> rollerCentres[] = {
        { loop.getX() + cr, loop.getY() + cr }, { loop.getRight() - cr, loop.getY() + cr } };   // top corners only
    for (auto c : rollerCentres)
    {
        const float r = cr - 7.0f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb8b2aa), c.x - r * 0.5f, c.y - r * 0.5f,
                                                 juce::Colour (0xff4a4640), c.x + r, c.y + r, true));
        g.fillEllipse (c.x - r, c.y - r, 2 * r, 2 * r);
        g.setColour (juce::Colour (0xff201d1a));
        g.fillEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);
        for (int k = 0; k < 3; ++k)
        {
            const float a = juce::MathConstants<float>::twoPi * (float) k / 3.0f + 0.4f;
            g.drawLine (c.x + std::cos (a) * 6.0f, c.y + std::sin (a) * 6.0f,
                        c.x + std::cos (a) * (r - 4.0f), c.y + std::sin (a) * (r - 4.0f), 2.0f);
        }
    }

    // tape heads on the bottom run, one per tap
    for (int i = 0; i < QuadriloProcessor::numTaps; ++i)
    {
        const juce::Rectangle<float> head (columnCentre (i) - 38.0f, loopBottom - 3.0f, 76.0f, 34.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6660), head.getX(), head.getY(),
                                                 juce::Colour (0xff25221f), head.getX(), head.getBottom(), false));
        g.fillRoundedRectangle (head, 5.0f);
        g.setColour (juce::Colour (0xff0e0c0a));
        g.drawRoundedRectangle (head, 5.0f, 1.2f);
        g.setColour (juce::Colour (0xff0e0c0a));
        g.fillRect (head.getCentreX() - 22.0f, head.getY() + 6.0f, 44.0f, 3.0f);
        g.setColour (amber.withAlpha (0.9f));
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
        g.drawText (juce::String (i + 1), head.withTrimmedTop (12.0f), juce::Justification::centred);
    }

    g.setColour (juce::Colour (0x55e8d8c4));
    g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::italic)));
    g.drawText ("Quadrilo", juce::Rectangle<float> (colX0, loopTop + 14.0f, 4 * colW, 36.0f), juce::Justification::centred);

    // wet / dry labels next to the mix fader
    g.setColour (juce::Colour (0xffe8d8c4));
    g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    g.drawText ("Wet", juce::Rectangle<float> (w - 70.0f, 36.0f, 50.0f, 16.0f), juce::Justification::centred);
    g.drawText ("Dry", juce::Rectangle<float> (w - 70.0f, 216.0f, 50.0f, 16.0f), juce::Justification::centred);

    // lower panel
    g.setColour (juce::Colour (0xff1b1c22));
    g.fillRect (0.0f, bottomPanelY, w, (float) getHeight() - bottomPanelY);
    g.setColour (juce::Colour (0xff6b4a30));
    g.fillRect (0.0f, bottomPanelY, w, 2.0f);
    g.setColour (juce::Colour (0xff2c2e36));
    for (int i = 0; i <= QuadriloProcessor::numTaps; ++i)
        g.fillRect (colX0 + colW * (float) i - 1.0f, bottomPanelY + 10.0f, 2.0f, (float) getHeight() - bottomPanelY - 20.0f);
}

void QuadriloEditor::resized()
{
    const float w = (float) getWidth();
    sync.setBounds (14, 14, 64, 24);
    bypass.setBounds ((int) w - 14 - 76, 8, 76, 24);
    mix.setBounds ((int) w - 70, 54, 50, 160);

    for (int i = 0; i < QuadriloProcessor::numTaps; ++i)
    {
        auto& t = tapUI[(size_t) i];
        const int cx = (int) columnCentre (i);

        const juce::Rectangle<int> delayArea (cx - 36, 116, 72, 84);   // label sits just above
        t.delay.setBounds (delayArea);
        t.note.setBounds (juce::Rectangle<int> (cx - 40, 138, 80, 26));

        const int cellW = (int) (colW / 2.0f), cellH = 82;
        const int x0 = (int) (colX0 + colW * (float) i);
        const int y0 = (int) bottomPanelY + 10;
        auto cell = [&] (int col, int row) { return juce::Rectangle<int> (x0 + col * cellW, y0 + row * cellH, cellW, cellH).withTrimmedTop (16).reduced (2, 0); };
        t.volume.setBounds   (cell (0, 0));
        t.damp.setBounds     (cell (1, 0));
        t.pan.setBounds      (cell (0, 1));
        t.feedback.setBounds (cell (1, 1));
    }
}
