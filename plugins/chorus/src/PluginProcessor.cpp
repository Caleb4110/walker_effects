#include "PluginProcessor.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include <array>
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
    baseDelayMs = apvts.getRawParameterValue("delayms");
    depthMs     = apvts.getRawParameterValue("depthms");
    lfoRateHz   = apvts.getRawParameterValue("ratehz");
    rawLayers   = apvts.getRawParameterValue("layers"); // is an int, but raw is float
    mix         = apvts.getRawParameterValue("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout ChorusProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayms", 1 }, "Delay (ms)",
        juce::NormalisableRange<float> (10.f, 30.f, 0.1f), 15.f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "depthms", 1 }, "Depth",
        juce::NormalisableRange<float> (1.f, 5.f, 0.1f), 2.f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratehz", 1 }, "Rate (hz)",
        juce::NormalisableRange<float> (0.1f, 3.f, 0.1f), 1.f));

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
        lfos[j].SetPhase ((float) j / (float) lfos.size());        // 0, 1/3, 2/3
        lfoDetunes[j] = 1.f + 0.15f * random_f (-1.f, 1.f);   // ±15% rate
    }
}

void ChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples    = buffer.getNumSamples();
    const int numChannels   = buffer.getNumChannels();

    const float baseDelay   = baseDelayMs->load() * 0.001f * sampleRate; // in samples
    const float depth       = depthMs->load()     * 0.001f * sampleRate; // in samples
    const float mixVal      = mix->load();

    const int maxLayers     = (int) juce::jmin(lfos.size(), delayLines[0].size());
    const int layers        = juce::jlimit(1, maxLayers, juce::roundToInt(rawLayers->load()));

    // Update LFO rates
    for (int j = 0; j < layers; ++j)
    {
        lfos[(size_t)j].SetFrequency (lfoRateHz->load() * lfoDetunes[(size_t)j]);
    }

    std::array<float, 16> mod {}; // NOTE: Must be >= maxLayers
    for (int i = 0; i < numSamples; ++i)
    {
        // Advance each LFO once per sample
        for (int j = 0; j < layers; ++j)
            mod[(size_t)j] = lfos[(size_t)j].NextSin();

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            const float sign = (ch == 0) ? 1.f : -1.f; // R channel is mirrored

            const float dry = data[i];
            float allWet = 0.f;

            for (int j = 0; j < layers; j++)
            {
                auto& dl = delayLines[(size_t)ch][(size_t)j];
                dl.SetDelay(baseDelay + depth * sign * mod[(size_t)j]);
                allWet += dl.Process(dry);
            }

            allWet /= (float) layers; // Normalise to number of layers
            data[i] = (dry * (1.f - mixVal)) + (allWet * mixVal);
        }
    }
}

juce::AudioProcessorEditor* ChorusProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusProcessor(); }
