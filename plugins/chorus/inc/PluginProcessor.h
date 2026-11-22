#pragma once
#include "juce_audio_basics/juce_audio_basics.h"
#include <array>
#include <atomic>
#include <juce_audio_processors/juce_audio_processors.h>
#include <walker/delay_line/DelayLine.h>
#include <walker/lfo/Lfo.h>

class ChorusProcessor : public juce::AudioProcessor
{
public:
    ChorusProcessor();

    void prepareToPlay (double newSampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}

    juce::AudioProcessorValueTreeState apvts;

private:
    std::array<std::array<walker::delay_line::DelayLine, 5>, 2>     delayLines; // 3 delay lines for each channel
    std::array<walker::lfo::Lfo, 5>                                 lfos; // 5 individual lfos for each pair of delay lines
    std::array<float, 5>                                            lfoDetunes;

    float sampleRate    = 44100.0f;

    // APVTS values
    std::atomic<float>* baseDelayMs = nullptr; // 20.0f initial
    std::atomic<float>* depthMs     = nullptr; // 4.0f initial
    std::atomic<float>* lfoRateHz   = nullptr; // 0.5f initial
    std::atomic<float>* rawLayers   = nullptr; // 1    initial
    std::atomic<float>* mix         = nullptr; // 0.5f initial

    // Smoothed values - These are what we use in code to avoid pops
    //                   and crackles when changing values
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>           sBaseDelayMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>           sDepthMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative>   sLfoRateMs;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear>           sMix;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusProcessor)
};
