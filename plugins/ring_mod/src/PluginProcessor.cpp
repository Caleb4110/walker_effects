#include "PluginProcessor.h"
#include "juce_audio_processors_headless/juce_audio_processors_headless.h"
#include "juce_core/juce_core.h"
#include "juce_dsp/juce_dsp.h"

RingModProcessor::RingModProcessor()
    : AudioProcessor (BusesProperties()
          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createParameters())
{
    carrierRateHz   = apvts.getRawParameterValue("ratehz");
    mix             = apvts.getRawParameterValue("mix");
}

juce::AudioProcessorValueTreeState::ParameterLayout RingModProcessor::createParameters()
{
    juce::NormalisableRange<float> rateRange (0.1f, 8000.f, 0.f);
    rateRange.setSkewForCentre(400.f);

    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratehz", 1 }, "Rate (hz)",
        rateRange, 1.f));

    layout.add(std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.f, 1.f, 0.f), 0.5f));

    return layout;
}

bool RingModProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == layouts.getMainOutputChannelSet();
}

void RingModProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();

    const double osRate = newSampleRate * oversampler.getOversamplingFactor();

    // Init the lfo
    lfo.Init (osRate);

    // Reset smoothed values and set ramp time (s)
    sCarrierRateHz  .reset (osRate, 0.05);
    sMix            .reset (osRate, 0.05);

    // Set the smoothed value's targets
    sCarrierRateHz  .setCurrentAndTargetValue (carrierRateHz->load());
    sMix            .setCurrentAndTargetValue (mix->load());

    setLatencySamples ((int) oversampler.getLatencyInSamples());
}

void RingModProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Update target values
    sCarrierRateHz  .setTargetValue(carrierRateHz->load());
    sMix            .setTargetValue(mix->load());

    juce::dsp::AudioBlock<float> hostBlock (buffer);
    auto upsampledBlock = oversampler.processSamplesUp (hostBlock); 

    const int numSamples    = (int) upsampledBlock.getNumSamples();
    const int numChannels   = (int) upsampledBlock.getNumChannels();

    for (int i = 0; i < numSamples; ++i)
    {
        // Get the next smoothed value per-sample
        const float rawRate    = sCarrierRateHz .getNextValue();
        const float rawMix     = sMix           .getNextValue();

        // Update LFO rates per-sample since rate is smoothed
        lfo.SetFrequency(rawRate);
        const float carrier = lfo.NextSin(); // Advance phase once per sample

        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* data = upsampledBlock.getChannelPointer(ch);
            const float dry = data[i];
            const float wet = dry * carrier;
            data[i] = (dry * (1.f - rawMix)) + (wet * rawMix);
        }
    }

    oversampler.processSamplesDown(hostBlock);
}

juce::AudioProcessorEditor* RingModProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new RingModProcessor(); }
