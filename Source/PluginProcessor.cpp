#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
// Delay-time note values used when Tempo Sync is on (in quarter-note beats).
const float syncBeats[] = { 0.25f, 0.5f, 0.75f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f };
const juce::StringArray syncNames { "1/16", "1/8", "1/8.", "1/4", "1/4.", "1/2", "1/2.", "1/1" };
constexpr float maxDelaySeconds = 2.0f;
}

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

    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "sync", 1 }, "Tempo Sync", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 }, "Bypass", false));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                         NormalisableRange<float> (0.0f, 1.0f), 0.5f));

    for (int i = 0; i < numTaps; ++i)
    {
        const auto n = String (i + 1);
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "delay" + n, 1 }, "Delay " + n,
                         NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.4f), 250.0f * (i + 1), AudioParameterFloatAttributes().withLabel ("ms")));
        p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "note" + n, 1 }, "Note " + n, syncNames, 3));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "feedback" + n, 1 }, "Feedback " + n,
                         NormalisableRange<float> (0.0f, 0.95f), 0.35f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "lowpass" + n, 1 }, "LowPass " + n,
                         NormalisableRange<float> (0.0f, 0.99f), 0.3f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "gain" + n, 1 }, "Gain " + n,
                         NormalisableRange<float> (0.0f, 1.0f), 0.0f));
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "pan" + n, 1 }, "Pan " + n,
                         NormalisableRange<float> (0.0f, 1.0f), (float) i / (numTaps - 1)));
    }
    return { p.begin(), p.end() };
}

void QuadriloProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    const int size = (int) (sr * maxDelaySeconds) + 4;
    for (auto& t : taps)
    {
        t.buffer.assign ((size_t) size, 0.0f);
        t.writePos = 0;
        t.lowpassState = 0.0f;
        for (auto* s : { &t.feedback, &t.gain, &t.pan, &t.delaySamples })
            s->reset (sr, 0.05);
    }
    mix.reset (sr, 0.05);
    mix.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load());
}

bool QuadriloProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void QuadriloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    if (apvts.getRawParameterValue ("bypass")->load() > 0.5f)
        return;

    const int numSamples = buffer.getNumSamples();
    auto* l = buffer.getWritePointer (0);
    auto* r = buffer.getWritePointer (1);

    double bpm = 120.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                bpm = *b;

    const bool sync = apvts.getRawParameterValue ("sync")->load() > 0.5f;
    mix.setTargetValue (apvts.getRawParameterValue ("mix")->load());

    for (int i = 0; i < numTaps; ++i)
    {
        const auto n = juce::String (i + 1);
        auto& t = taps[(size_t) i];
        float seconds;
        if (sync)
        {
            const int idx = juce::jlimit (0, 7, (int) apvts.getRawParameterValue ("note" + n)->load());
            seconds = syncBeats[idx] * 60.0f / (float) bpm;
        }
        else
            seconds = apvts.getRawParameterValue ("delay" + n)->load() * 0.001f;

        seconds = juce::jlimit (0.001f, maxDelaySeconds - 0.01f, seconds);
        t.delaySamples.setTargetValue (seconds * (float) sampleRate);
        t.feedback.setTargetValue (apvts.getRawParameterValue ("feedback" + n)->load());
        t.gain.setTargetValue (apvts.getRawParameterValue ("gain" + n)->load());
        t.pan.setTargetValue (apvts.getRawParameterValue ("pan" + n)->load());
    }

    std::array<float, numTaps> lowpass;
    for (int i = 0; i < numTaps; ++i)
        lowpass[(size_t) i] = apvts.getRawParameterValue ("lowpass" + juce::String (i + 1))->load();

    for (int s = 0; s < numSamples; ++s)
    {
        const float inL = l[s], inR = r[s];
        const float wet = mix.getNextValue() * 0.5f;
        float outL = inL, outR = inR;

        for (int i = 0; i < numTaps; ++i)
        {
            auto& t = taps[(size_t) i];
            const int size = (int) t.buffer.size();
            const float d = t.delaySamples.getNextValue();
            const float fb = t.feedback.getNextValue();
            const float g = t.gain.getNextValue();
            const float p = t.pan.getNextValue();

            float readPos = (float) t.writePos - d;
            if (readPos < 0.0f) readPos += (float) size;
            const int i0 = (int) readPos;
            const int i1 = (i0 + 1) % size;
            const float frac = readPos - (float) i0;
            const float delayed = t.buffer[(size_t) i0] * (1.0f - frac) + t.buffer[(size_t) i1] * frac;

            const float a = lowpass[(size_t) i];
            t.lowpassState = delayed * (1.0f - a) + a * t.lowpassState;
            const float y = t.lowpassState;

            t.buffer[(size_t) t.writePos] = 0.5f * (inL + inR) + y * fb;
            t.writePos = (t.writePos + 1) % size;

            outL += g * inL + y * wet * (1.0f - p);
            outR += g * inR + y * wet * p;
        }
        l[s] = outL;
        r[s] = outR;
    }
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
