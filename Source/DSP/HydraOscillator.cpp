#include "HydraOscillator.h"

#include <cmath>

namespace
{
constexpr float kMorphMin = 0.0f;
constexpr float kMorphMax = 3.0f;
} // namespace

void HydraOscillator::setSampleRate (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
}

void HydraOscillator::setFrequency (double frequencyHz) noexcept
{
    phaseIncrement = twoPi * frequencyHz / sampleRate;
}

void HydraOscillator::setPhase (double initialPhase) noexcept
{
    currentPhase = initialPhase;
}

void HydraOscillator::advance() noexcept
{
    currentPhase += phaseIncrement;

    if (currentPhase >= twoPi)
        currentPhase -= twoPi;
}

float HydraOscillator::evaluateSine (double theta) noexcept
{
    return std::sin (static_cast<float> (theta));
}

float HydraOscillator::evaluateTriangle (double theta) noexcept
{
    return (2.0f / juce::MathConstants<float>::pi)
         * std::asin (std::sin (static_cast<float> (theta)));
}

float HydraOscillator::evaluateSquare (double theta) noexcept
{
    return evaluateSine (theta) >= 0.0f ? 1.0f : -1.0f;
}

float HydraOscillator::evaluateSawtooth (double theta) noexcept
{
    return static_cast<float> (2.0 * (theta / twoPi) - 1.0);
}

float HydraOscillator::evaluateSample (float morphState) const noexcept
{
    const auto clampedMorph = juce::jlimit (kMorphMin, kMorphMax, morphState);
    const auto sine = evaluateSine (currentPhase);
    const auto triangle = evaluateTriangle (currentPhase);
    const auto square = evaluateSquare (currentPhase);
    const auto saw = evaluateSawtooth (currentPhase);

    if (clampedMorph < 1.0f)
        return juce::jmap (clampedMorph, 0.0f, 1.0f, sine, triangle);

    if (clampedMorph < 2.0f)
    {
        const auto blend = clampedMorph - 1.0f;
        return juce::jmap (blend, 0.0f, 1.0f, triangle, square);
    }

    const auto blend = clampedMorph - 2.0f;
    return juce::jmap (blend, 0.0f, 1.0f, square, saw);
}
