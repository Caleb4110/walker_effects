#include "PluginProcessor.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include <random>

float random_f(float min, float max) {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_real_distribution<float> dist(min, max);
    return dist(gen);
}

ChorusProcessor::ChorusProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameters())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout ChorusProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratehz", 1 }, "Rate (hz)",
        juce::NormalisableRange<float> (0.3f, 1.5f, 0.1f), 0.3f));

    layout.add(std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.f, 1.f, 0.1f), 0.5f));

    layout.add(std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "layers", 1 }, "Layers", 1, 5, 1));

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

    for (auto& dl : delayLines[0])
    {
        dl.Init (sampleRate, 1.0f);
    }

    for (auto& dl : delayLines[1])
    {
        dl.Init (sampleRate, 1.0f);
    }

    for (size_t j = 0; j < lfos.size(); ++j)
    {
        lfos[j].Init (sampleRate);
        const float detune = 1.f + 0.15f * random_f (-1.f, 1.f);   // ±15% rate
        lfos[j].SetFrequency (lfoRateHz * detune);
        lfos[j].SetPhase ((float) j / (float) lfos.size());        // 0, 1/3, 2/3
    }
}

void ChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    const float baseDelay = baseDelayMs * 0.001f * sampleRate;  // samples
    const float depth     = depthMs     * 0.001f * sampleRate;  // samples

    for (size_t i = 0; i < numSamples; ++i)
    {
        // Advance each LFO once per sample
        std::array<float, 7> mod;
        for (size_t j = 0; j < lfos.size(); ++j)
            mod[j] = lfos[j].NextSin();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            const float sign = (ch == 0) ? 1.f : -1.f; // R channel is mirrored

            const float dry = data[i];
            float allWet = 0.f;

            for (size_t j = 0; j < delayLines[ch].size(); j++)
            {
                delayLines[ch][j].SetDelay(baseDelay + depth * sign * mod[j]);
                allWet += delayLines[ch][j].Process(dry);
            }
            allWet /= (float)delayLines[ch].size();
            data[i] = (dry * (1.0f - mix)) + (allWet * mix);
        }
    }
}

juce::AudioProcessorEditor* ChorusProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusProcessor(); }
