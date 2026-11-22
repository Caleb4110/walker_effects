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
        juce::ParameterID { "depthms", 1 }, "Depth (ms)",
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

    // Initialse L & R delay lines
    for (size_t i = 0; i < delayLines.size(); i++)
    {
        for (auto& dl : delayLines[i])
        {
            dl.Init (sampleRate, 1.0f);
        }
    }

    // Init the lfos - spread phases equally and randomise rate
    for (size_t j = 0; j < lfos.size(); ++j)
    {
        lfos[j].Init (sampleRate);
        lfos[j].SetPhase ((float) j / (float) lfos.size()); // Evenly spaced phases (1/n, 2/n, 3/n, ...)
        lfoDetunes[j] = 1.f + 0.15f * random_f (-1.f, 1.f); // ±15% rate
    }

    // Reset smoothed values and set ramp time (s)
    sBaseDelayMs.reset (newSampleRate, 0.08);
    sDepthMs    .reset (newSampleRate, 0.05);
    sLfoRateMs  .reset (newSampleRate, 0.03);
    sMix        .reset (newSampleRate, 0.05);

    // Set the smoothed value's targets
    sBaseDelayMs.setCurrentAndTargetValue (baseDelayMs->load());
    sDepthMs    .setCurrentAndTargetValue (depthMs->load());
    sLfoRateMs  .setCurrentAndTargetValue (lfoRateHz->load());
    sMix        .setCurrentAndTargetValue (mix->load());
}

void ChorusProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples    = buffer.getNumSamples();
    const int numChannels   = buffer.getNumChannels();

    // Update target values
    sBaseDelayMs.setTargetValue(baseDelayMs->load());
    sDepthMs    .setTargetValue(depthMs->load());
    sLfoRateMs  .setTargetValue(lfoRateHz->load());
    sMix        .setTargetValue(mix->load());


    const int maxLayers     = (int) juce::jmin(lfos.size(), delayLines[0].size());
    const int layers        = juce::jlimit(1, maxLayers, juce::roundToInt(rawLayers->load()));


    std::array<float, 16> mod {}; // NOTE: Must be >= maxLayers
    for (int i = 0; i < numSamples; ++i)
    {
        // Get the next smoothed value per-sample
        const float baseDelay  = sBaseDelayMs.getNextValue() * 0.001f * sampleRate; // In samples
        const float depth      = sDepthMs    .getNextValue() * 0.001f * sampleRate; // In samples
        const float rawRate    = sLfoRateMs  .getNextValue();
        const float rawMix     = sMix        .getNextValue();

        // Update LFO rates per-sample since rate is smoothed
        // And advance each LFO once per sample
        for (int j = 0; j < layers; ++j)
        {
            lfos[(size_t)j].SetFrequency (rawRate * lfoDetunes[(size_t)j]);
            mod[(size_t)j] = lfos[(size_t)j].NextSin();
        }

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
            data[i] = (dry * (1.f - rawMix)) + (allWet * rawMix);
        }
    }
}

juce::AudioProcessorEditor* ChorusProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ChorusProcessor(); }
