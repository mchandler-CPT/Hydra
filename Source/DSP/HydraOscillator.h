#pragma once

#include <juce_dsp/juce_dsp.h>

class HydraOscillator
{
public:
    void setSampleRate (double newSampleRate) noexcept;
    void setFrequency (double frequencyHz) noexcept;
    void setPhase (double initialPhase) noexcept;

    void advance() noexcept;
    float evaluateSample (float morphState) const noexcept;

    double getCurrentPhase() const noexcept { return currentPhase; }
    double getPhaseIncrement() const noexcept { return phaseIncrement; }
    double getSampleRate() const noexcept { return sampleRate; }

private:
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    static float evaluateSine (double theta) noexcept;
    static float evaluateTriangle (double theta) noexcept;
    static float evaluateSquare (double theta) noexcept;
    static float evaluateSawtooth (double theta) noexcept;

    double currentPhase = 0.0;
    double phaseIncrement = 0.0;
    double sampleRate = 44100.0;
};
