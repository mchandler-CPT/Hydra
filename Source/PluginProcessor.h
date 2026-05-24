#pragma once

#include "DSP/HydraEngine.h"
#include "DSP/ZdfLadderFilter.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <atomic>
#include <memory>
#include <utility>
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
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getApvts() { return apvts; }
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }

    static constexpr int kFifoSize = 1024;

    void pushVisualSample (float left, float right) noexcept
    {
        const auto writePos = fifoWriteIndex.load();
        visualFifoL[static_cast<size_t> (writePos)] = left;
        visualFifoR[static_cast<size_t> (writePos)] = right;
        fifoWriteIndex.store ((writePos + 1) % kFifoSize);
    }

    std::pair<float, float> popVisualSample (int readIndex) const noexcept
    {
        const auto index = ((readIndex % kFifoSize) + kFifoSize) % kFifoSize;
        return { visualFifoL[static_cast<size_t> (index)], visualFifoR[static_cast<size_t> (index)] };
    }

    int getLatestWriteIndex() const noexcept { return fifoWriteIndex.load(); }

    juce::MidiKeyboardState keyboardState;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    HydraEngine hydraEngine;
    ZdfLadderFilter filterL;
    ZdfLadderFilter filterR;
    juce::dsp::StateVariableTPTFilter<float> mHpFilterL;
    juce::dsp::StateVariableTPTFilter<float> mHpFilterR;
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float>* depthParam { nullptr };
    std::atomic<float>* girthParam { nullptr };
    std::atomic<float>* harmonyParam { nullptr };
    std::atomic<float>* gainParam { nullptr };
    std::atomic<float>* cutoffParam { nullptr };
    std::atomic<float>* resParam { nullptr };
    std::atomic<float>* attackParam { nullptr };
    std::atomic<float>* decayParam { nullptr };
    std::atomic<float>* sustainParam { nullptr };
    std::atomic<float>* releaseParam { nullptr };
    std::atomic<float>* filterAttackParam { nullptr };
    std::atomic<float>* filterDecayParam { nullptr };
    std::atomic<float>* filterSustainParam { nullptr };
    std::atomic<float>* filterReleaseParam { nullptr };
    std::atomic<float>* egrAmountParam { nullptr };
    std::atomic<float>* envWarpParam { nullptr };
    std::atomic<float>* glideTimeParam { nullptr };
    std::atomic<float>* harmonicTiltParam { nullptr };
    std::atomic<float>* harmonicInversionParam { nullptr };
    std::atomic<float>* hpCutoffParam { nullptr };
    std::atomic<float>* kbTrackParam { nullptr };
    std::atomic<float>* filterOverloadParam { nullptr };
    std::atomic<float>* harmonyQuantizeParam { nullptr };

    double currentSampleRate = 44100.0;
    double oversampledSampleRate = 44100.0;
    std::vector<float> monoRightScratch;

    std::array<float, kFifoSize> visualFifoL {};
    std::array<float, kFifoSize> visualFifoR {};
    std::atomic<int> fifoWriteIndex { 0 };

    juce::LinearSmoothedValue<float> mSmoothedHpCutoff;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessor)
};
