#pragma once

#include "HydraMacroMapper.h"
#include "HydraOscillator.h"

#include <juce_dsp/juce_dsp.h>
#include <array>

struct HydraPartialVoice
{
    juce::LinearSmoothedValue<float> amplitude;
    juce::LinearSmoothedValue<float> morph;
    juce::LinearSmoothedValue<float> panL;
    juce::LinearSmoothedValue<float> panR;
};

struct AllPassFilter
{
    float x1 { 0.0f };
    float y1 { 0.0f };

    void reset() noexcept
    {
        x1 = 0.0f;
        y1 = 0.0f;
    }

    float processSample (float input, float coefficient) noexcept
    {
        const auto output = (-coefficient * input) + x1 + (coefficient * y1);
        x1 = input;
        y1 = output;
        return output;
    }
};

class HydraEngine
{
public:
    static constexpr int numPartials = HydraMacroMapper::numPartials;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset() noexcept;

    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber) noexcept;

    void setMorph (float morph) noexcept;
    void setDepth (float depth) noexcept;
    void setGirth (float girth) noexcept;

    void renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;

    float getVoiceAmplitude() const noexcept { return voiceAmplitude; }

private:
    static constexpr std::array<int, numPartials> primeNumbers { 2, 3, 5, 7, 11, 13, 17 };
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    static double midiNoteToFrequency (int midiNoteNumber) noexcept;
    void applyMacroTargets() noexcept;
    void resetAllPassChains() noexcept;

    double sampleRate = 44100.0;
    float depth = 0.0f;
    float girth = 0.0f;
    float noteVelocity = 0.0f;
    float voiceAmplitude = 0.0f;
    bool noteIsActive = false;
    bool isKeyHeld = false;

    HydraMacroMapper macroMapper;
    std::array<HydraOscillator, numPartials> oscillators {};
    std::array<HydraPartialVoice, numPartials> voices {};
    std::array<AllPassFilter, 4> allPassChainL {};
    std::array<AllPassFilter, 4> allPassChainR {};
};
