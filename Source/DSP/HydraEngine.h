#pragma once

#include "HydraMacroMapper.h"
#include "HydraOscillator.h"
#include "HydraParallelSaturator.h"
#include "HydraPhaseDisperser.h"

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

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
    void setHarmonyQuantize (bool harmonyQuantize) noexcept;
    void setFilterCutoff (float cutoffHz) noexcept;
    void setEnvelopeParameters (float attack, float decay, float sustain, float release) noexcept;
    void setFilterEnvelopeParameters (float attack, float decay, float sustain, float release) noexcept;
    void setEgrAmount (float newEgrAmount) noexcept;
    void setEnvWarp (float envWarp) noexcept;
    void setGlideTime (float glideTimeSeconds) noexcept;
    void setScaleMorph (float scaleMorph) noexcept;
    void setHarmonicTiltTarget (float harmonicTilt) noexcept;
    void setHarmonicInversionIndexTarget (int harmonicInversionIndex) noexcept;
    void setKbTrack (float kbTrack) noexcept;
    void setFilterOverload (float filterOverload) noexcept;

    void renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;
    void applyFilterOverload (float* leftChannel, float* rightChannel, int numSamples) const noexcept;
    const float* getFilterCutoffBuffer() const noexcept { return filterCutoffBuffer.empty() ? nullptr : filterCutoffBuffer.data(); }

    float getVoiceAmplitude() const noexcept { return lastEnvelopeGain; }

private:
    static constexpr std::array<float, numPartials> primes { 2.0f, 3.0f, 5.0f, 7.0f, 11.0f, 13.0f, 17.0f };
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    float midiNoteToFrequency (int midiNoteNumber) const noexcept;
    float sanitizeTargetFrequency (float frequencyHz) const noexcept;
    void applyMacroTargets() noexcept;
    void updateOscillatorTuning (bool glidePitch) noexcept;
    void retuneOscillatorsForNote (int midiNoteNumber, bool glidePitch) noexcept;
    void clearAllDelayLines() noexcept;
    void updateFrequencyGlideSmoothing() noexcept;

    static float computeHarmonicTiltGain (float harmonicMultiplier, float tilt) noexcept;
    static int harmonicInversionIndexFromParameter (float harmonicInversion) noexcept;
    static float trianglePitchDriftMultiplier (float lfoPhase01) noexcept;
    static void advancePitchDriftLfo (float& lfoPhase01, float rateHz, double sr) noexcept;

    static constexpr float spectralDampingS = 0.0008f;
    static constexpr float maxPitchDriftCentsMultiplier = 0.001f;
    static constexpr std::array<float, numPartials> pitchDriftRatesHz {
        0.052f, 0.067f, 0.083f, 0.097f, 0.112f, 0.131f, 0.149f
    };

    double sampleRate = 44100.0;
    float noteOnSafetySampleCount = 44.1f;
    float fundamentalFreq = 0.0f;
    std::array<float, numPartials> pitchDriftLfoPhase {};
    juce::LinearSmoothedValue<float> smoothedCutoffHz;
    juce::LinearSmoothedValue<float> smoothedFrequency;
    float depth = 0.0f;
    float girth = 0.0f;
    float harmony = 0.0f;
    bool harmonyQuantize = false;
    float envWarp = 0.0f;
    float egrAmount = 0.0f;
    float glideTimeSeconds = 0.05f;
    float appliedGlideTimeSeconds = 0.003f;
    float scaleMorph = 0.0f;
    int harmonicInversionIndex = 0;
    juce::LinearSmoothedValue<float> harmonicTiltSmoothed;
    juce::LinearSmoothedValue<float> harmonicInversionSmoothed;
    float kbTrack = 0.0f;
    float filterOverload = 0.0f;
    int activeMidiNoteNumber = 69;

    static float applyFilterOverloadSample (float sample, float overloadKnob) noexcept;
    juce::ADSR::Parameters baseEnvelopeParameters { 0.1f, 0.3f, 0.8f, 0.5f };
    float baseAttackSeconds = 0.1f;
    juce::ADSR::Parameters baseFilterEnvelopeParameters { 0.1f, 0.3f, 0.7f, 0.5f };
    std::array<float, numPartials> frequencyMultipliers { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
    std::array<float, numPartials> assignedHarmonicOrders { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
    float noteVelocity = 0.0f;
    float lastEnvelopeGain = 0.0f;
    bool noteIsActive = false;
    int64_t samplesSinceNoteOn { 0 };

    juce::ADSR adsr;
    juce::ADSR filterAdsr;
    std::vector<float> filterCutoffBuffer;

    std::array<int, 16> noteStack {};
    int numNotesInStack = 0;
    bool isKeyHeld = false;

    HydraMacroMapper macroMapper;
    HydraParallelSaturator saturator;
    HydraPhaseDisperser phaseDisperser;
    std::array<HydraOscillator, numPartials> oscillators {};
    std::array<HydraPartialVoice, numPartials> voices {};
};
