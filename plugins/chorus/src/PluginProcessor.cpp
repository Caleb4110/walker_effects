#include "PluginProcessor.h"

ChorusProcessor::ChorusProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ChorusProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Chorus",
        juce::NormalisableRange<float> (-60.f, 12.f, 0.1f), 0.f));
    return layout;
}

bool ChorusProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == layouts.getMainOutputChannelSet();
}

void ChorusProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = static_cast<float> (newSampleRate);

    for (auto& dl : delayLines)
        dl.Init (sampleRate, 0.1f);

    lfo.Init (sampleRate);
    lfo.SetFrequency (lfoRateHz);
}

void ChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const float baseDelay = baseDelayMs * 0.001f * sampleRate;  // samples
    const float depth     = depthMs     * 0.001f * sampleRate;  // samples

    for (int i = 0; i < numSamples; ++i)
    {
        const float mod   = lfo.Next();             // -1..1
        const float delay = baseDelay + depth * mod;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);

            const float dry = data[i];
            delayLines[ch].SetDelay(delay);
            const float wet = delayLines[ch].Process(data[i]);
            data[i] = dry * (1.0f - 0.5f) + wet * 0.5;
        }
    }
}

juce::AudioProcessorEditor* ChorusProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusProcessor(); }
