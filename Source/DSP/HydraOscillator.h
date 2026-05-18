#pragma once

#include <juce_dsp/juce_dsp.h>

class HydraOscillator
{
public:
    void prepare (double sampleRate) noexcept;
    void setFrequency (double frequencyHz, bool glidePitch) noexcept;
    void setPhase (double initialPhase) noexcept;

    void advance() noexcept;
    float evaluateSample (float morphState) const noexcept;

    double getCurrentPhase() const noexcept { return currentPhase; }
    double getPhaseIncrement() const noexcept { return phaseIncrement.getTargetValue(); }
    double getSampleRate() const noexcept { return sampleRate; }

private:
    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    double currentPhase = 0.0;
    juce::LinearSmoothedValue<double> phaseIncrement { 0.0 };
    double sampleRate = 44100.0;
};
