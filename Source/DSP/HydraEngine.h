#pragma once

#include "HydraMacroMapper.h"
#include "HydraOscillator.h"
#include "HydraParallelSaturator.h"
#include "HydraPhaseDisperser.h"

#include <juce_dsp/juce_dsp.h>
#include <array>

struct HydraPartialVoice
{
    static constexpr int delayBufferSize = 16384;
    static constexpr int delayBufferMask = delayBufferSize - 1;

    juce::LinearSmoothedValue<float> amplitude;
    juce::LinearSmoothedValue<float> morph;
    juce::LinearSmoothedValue<float> panL;
    juce::LinearSmoothedValue<float> panR;

    std::array<float, delayBufferSize> delayBuffer {};
    int writeIndex = 0;
    int delayInSamples = 0;

    void configureDelay (double oversampledSampleRate, int partialIndex) noexcept;
    void clearDelay() noexcept;

    inline float processDelaySample (float inputSample) noexcept
    {
        if (delayInSamples <= 0)
            return inputSample;

        delayBuffer[static_cast<size_t> (writeIndex)] = inputSample;

        const auto readIndex = (writeIndex - delayInSamples + delayBufferSize) & delayBufferMask;
        const auto delayedOutput = delayBuffer[static_cast<size_t> (readIndex)];

        writeIndex = (writeIndex + 1) & delayBufferMask;

        return delayedOutput;
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

    void setDepth (float depth) noexcept;
    void setGirth (float girth) noexcept;

    void renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;

    float getVoiceAmplitude() const noexcept { return voiceAmplitude; }

private:
    static constexpr std::array<int, numPartials> primeNumbers { 2, 3, 5, 7, 11, 13, 17 };
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    static double midiNoteToFrequency (int midiNoteNumber) noexcept;
    void applyMacroTargets() noexcept;
    void retuneOscillatorsForNote (int midiNoteNumber, bool glidePitch) noexcept;
    void clearAllDelayLines() noexcept;

    double sampleRate = 44100.0;
    float depth = 0.0f;
    float girth = 0.0f;
    float noteVelocity = 0.0f;
    float voiceAmplitude = 0.0f;
    bool noteIsActive = false;

    std::array<int, 16> noteStack {};
    int numNotesInStack = 0;
    bool isKeyHeld = false;

    HydraMacroMapper macroMapper;
    HydraParallelSaturator saturator;
    HydraPhaseDisperser phaseDisperser;
    std::array<HydraOscillator, numPartials> oscillators {};
    std::array<HydraPartialVoice, numPartials> voices {};
};
