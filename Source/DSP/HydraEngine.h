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
    void setHarmony (float harmony) noexcept;
    void setFilterCutoff (float cutoffHz) noexcept;
    void setEnvelopeParameters (float attack, float decay, float sustain, float release) noexcept;
    void setEnvWarp (float envWarp) noexcept;

    void renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;

    float getVoiceAmplitude() const noexcept { return lastEnvelopeGain; }

private:
    static constexpr std::array<float, numPartials> primes { 2.0f, 3.0f, 5.0f, 7.0f, 11.0f, 13.0f, 17.0f };
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    static double midiNoteToFrequency (int midiNoteNumber) noexcept;
    void applyMacroTargets() noexcept;
    void updateOscillatorTuning (bool glidePitch) noexcept;
    void retuneOscillatorsForNote (int midiNoteNumber, bool glidePitch) noexcept;
    void clearAllDelayLines() noexcept;

    static constexpr float spectralDampingS = 0.0008f;

    double sampleRate = 44100.0;
    float fundamentalFreq = 0.0f;
    juce::LinearSmoothedValue<float> smoothedCutoffHz;
    juce::LinearSmoothedValue<float> smoothedFrequency;
    float depth = 0.0f;
    float girth = 0.0f;
    float harmony = 0.0f;
    float envWarp = 0.0f;
    float baseAttackSeconds = 0.1f;
    std::array<float, numPartials> frequencyMultipliers { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
    float noteVelocity = 0.0f;
    float lastEnvelopeGain = 0.0f;
    bool noteIsActive = false;
    int64_t samplesSinceNoteOn { 0 };

    juce::ADSR adsr;

    std::array<int, 16> noteStack {};
    int numNotesInStack = 0;
    bool isKeyHeld = false;

    HydraMacroMapper macroMapper;
    HydraParallelSaturator saturator;
    HydraPhaseDisperser phaseDisperser;
    std::array<HydraOscillator, numPartials> oscillators {};
    std::array<HydraPartialVoice, numPartials> voices {};
};
