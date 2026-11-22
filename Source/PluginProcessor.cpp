// Quadrilo - four-tap stereo delay
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Reports exactly 0 or 1 from getValue(). The state tree stores the snapped value,
// so an unsnapped value (such as 0.53 set by a host or by pluginval) would
// otherwise never be reset when the state is restored.
class SnappingBoolParameter : public juce::AudioParameterBool
{
public:
    using juce::AudioParameterBool::AudioParameterBool;

    float getValue() const override { return get() ? 1.0f : 0.0f; }
};
} // namespace

QuadriloProcessor::QuadriloProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout QuadriloProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    StringArray noteNames;
    for (auto* n : quadrilo::noteNames)
        noteNames.add (n);

    p.push_back (std::make_unique<SnappingBoolParameter> (ParameterID { "sync", 1 }, "Tempo Sync", false));
    p.push_back (std::make_unique<SnappingBoolParameter> (ParameterID { "bypass", 1 }, "Bypass", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                         NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    for (int i = 0; i < numTaps; ++i)
    {
        const auto n = String (i + 1);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "delay" + n, 1 }, "Delay " + n,
                         NormalisableRange<float> (1.0f, quadrilo::maxDelaySeconds * 1000.0f, 0.1f, 0.4f),
                         250.0f * (i + 1), AudioParameterFloatAttributes().withLabel ("ms")));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "note" + n, 1 }, "Note " + n, noteNames, 8));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "feedback" + n, 1 }, "Feedback " + n,
                         NormalisableRange<float> (0.0f, 0.99f), 0.5f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "damp" + n, 1 }, "Damp " + n,
                         NormalisableRange<float> (0.0f, 0.99f), 0.0f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "volume" + n, 1 }, "Volume " + n,
                         NormalisableRange<float> (0.0f, 1.0f), 0.5f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pan" + n, 1 }, "Pan " + n,
                         NormalisableRange<float> (0.0f, 1.0f), 0.5f));
    }
    return { p.begin(), p.end() };
}

void QuadriloProcessor::prepareToPlay (double sr, int)
{
    engine.prepare (sr);
    needsSnap = true;
}

bool QuadriloProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void QuadriloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    const bool bypass = apvts.getRawParameterValue ("bypass")->load() > 0.5f;

    for (int i = 0; i < numTaps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto value = [&] (const juce::String& id) { return apvts.getRawParameterValue (id + n)->load(); };

        quadrilo::TapParams t;
        t.delaySeconds = sync ? quadrilo::syncDelaySeconds ((int) value ("note"), bpm)
                              : value ("delay") * 0.001f;
        t.feedback = value ("feedback");
        t.damp = value ("damp");
        t.volume = value ("volume");
        t.pan = value ("pan");
        engine.setTap (i, t);
    }

    engine.setMix (bypass ? 0.0f : apvts.getRawParameterValue ("mix")->load());
    if (needsSnap)
    {
        engine.snap();
        needsSnap = false;
    }

    engine.process (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
}

juce::AudioProcessorEditor* QuadriloProcessor::createEditor()
{
    return new QuadriloEditor (*this);
}

void QuadriloProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void QuadriloProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new QuadriloProcessor();
}
