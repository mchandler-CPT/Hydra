#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

struct HydraHead
{
    double phase = 0.0;
    double phaseIncrement = 0.0;
    juce::LinearSmoothedValue<float> amplitude;
    juce::LinearSmoothedValue<float> morph;
    juce::LinearSmoothedValue<float> panL;
    juce::LinearSmoothedValue<float> panR;
};

class HydraEngine
{
public:
    static constexpr int numPartials = 7;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset() noexcept;

    void noteOn (int midiNoteNumber, float velocity);
    void noteOff (int midiNoteNumber) noexcept;

    void setMorph (float morph) noexcept;
    void setPan (float pan) noexcept;

    void renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;

private:
    static constexpr std::array<int, numPartials> primeNumbers { 2, 3, 5, 7, 11, 13, 17 };
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    static double midiNoteToFrequency (int midiNoteNumber) noexcept;
    static float evaluatePartial (double theta, float morph) noexcept;

    void setFundamentalFrequency (double fundamentalHz) noexcept;

    double sampleRate = 44100.0;
    std::array<HydraHead, numPartials> heads {};
};
