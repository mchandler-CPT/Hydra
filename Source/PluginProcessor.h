#pragma once

#include "DSP/HydraEngine.h"
#include "DSP/ZdfLadderFilter.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>
#include <memory>
#include <vector>

class HydraAudioProcessor : public juce::AudioProcessor
{
public:
    HydraAudioProcessor();
    ~HydraAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    HydraEngine hydraEngine;
    ZdfLadderFilter filterL;
    ZdfLadderFilter filterR;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float>* depthParam { nullptr };
    std::atomic<float>* girthParam { nullptr };
    std::atomic<float>* gainParam { nullptr };
    std::atomic<float>* cutoffParam { nullptr };
    std::atomic<float>* resParam { nullptr };

    double currentSampleRate = 44100.0;
    double oversampledSampleRate = 44100.0;
    std::vector<float> monoRightScratch;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessor)
};
