#pragma once

#include <array>

#include <juce_dsp/juce_dsp.h>

struct SharedWavetables
{
    static constexpr int kWavetableSize = 4096;

    SharedWavetables();

    std::array<float, kWavetableSize + 1> sineTable {};
    std::array<float, kWavetableSize + 1> triangleTable {};
    std::array<float, kWavetableSize + 1> sawtoothTable {};
    std::array<float, kWavetableSize + 1> crushedTable {};
};

class HydraOscillator
{
public:
    static constexpr int kWavetableSize = SharedWavetables::kWavetableSize;

    static const SharedWavetables& getSharedTables();

    void prepare (double sampleRate) noexcept;
    void noteOn() noexcept;
    void setFrequency (double frequencyHz, bool glidePitch) noexcept;
    void setPhase (double initialPhase) noexcept;

    void advance() noexcept;
    float evaluateSample (float morphState, double phaseModulationOffset) const noexcept;

    double getCurrentPhase() const noexcept { return currentPhase; }
    double getPhaseIncrement() const noexcept { return phaseIncrement.getTargetValue(); }
    double getSampleRate() const noexcept { return sampleRate; }

private:
    static float interpolateTable (const std::array<float, kWavetableSize + 1>& table,
                                   int indexBase,
                                   float fraction) noexcept;

    static constexpr double twoPi = juce::MathConstants<double>::twoPi;

    double currentPhase = 0.0;
    juce::LinearSmoothedValue<double> phaseIncrement { 0.0 };
    double sampleRate = 44100.0;
};
