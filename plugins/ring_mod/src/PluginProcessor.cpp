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

RingModProcessor::RingModProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameters())
{
    carrierRateMs   = apvts.getRawParameterValue("ratehz");
    mix             = apvts.getRawParameterValue("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout RingModProcessor::createParameters()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratehz", 1 }, "Rate (hz)",
        juce::NormalisableRange<float> (0.1f, 8000.f, 0.1f), 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.f, 1.f, 0.1f), 0.5f));

    return layout;
}

bool RingModProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == layouts.getMainOutputChannelSet();
}

void RingModProcessor::prepareToPlay (double newSampleRate, int)
{
    sampleRate = static_cast<float> (newSampleRate);

    // Init the lfo
    lfo.Init (sampleRate);

    // Reset smoothed values and set ramp time (s)
    sCarrierRateMs  .reset (newSampleRate, 0.03);
    sMix            .reset (newSampleRate, 0.05);

    // Set the smoothed value's targets
    sCarrierRateMs  .setCurrentAndTargetValue (carrierRateMs->load());
    sMix            .setCurrentAndTargetValue (mix->load());
}

void RingModProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples    = buffer.getNumSamples();
    const int numChannels   = buffer.getNumChannels();

    // Update target values
    sCarrierRateMs  .setTargetValue(carrierRateMs->load());
    sMix            .setTargetValue(mix->load());

    std::array<float, 12> mod {}; // NOTE: Must be >= maxLayers
    for (int i = 0; i < numSamples; ++i)
    {
        // Get the next smoothed value per-sample
        const float rawRate    = sCarrierRateMs .getNextValue();
        const float rawMix     = sMix           .getNextValue();

        // Update LFO rates per-sample since rate is smoothed
        // And advance each LFO once per sample
        lfo.SetFrequency(rawRate);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = buffer.getWritePointer(ch);
            //const float sign = (ch == 0) ? 1.f : -1.f; // R channel is mirrored

            const float dry = data[i];
            float wet = 0.f;

            wet = dry * lfo.NextSin();
            data[i] = (dry * (1.f - rawMix)) + (wet * rawMix);
        }
    }
}

juce::AudioProcessorEditor* RingModProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RingModProcessor(); }
