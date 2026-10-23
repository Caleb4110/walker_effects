#pragma once
#include <array>
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
    std::array<walker::delay_line::DelayLine, 2> delayLines;
    walker::lfo::Lfo lfo;

    float sampleRate    = 44100.0f;
    float baseDelayMs   = 15.0f;
    float depthMs       = 3.0f;
    float lfoRateHz     = 0.8f;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChorusProcessor)
};
