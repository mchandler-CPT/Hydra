#include "HydraOscillator.h"

#include <cmath>

namespace
{
constexpr float kMorphMin = 0.0f;
constexpr float kMorphMax = 3.0f;
constexpr double kPhaseIncrementSmoothingSeconds = 0.003;
} // namespace

void HydraOscillator::prepare (double newSampleRate) noexcept
{
    sampleRate = newSampleRate;
    phaseIncrement.reset (sampleRate, kPhaseIncrementSmoothingSeconds);
}

void HydraOscillator::setFrequency (double frequencyHz, bool glidePitch) noexcept
{
    const auto targetIncrement = (twoPi * frequencyHz) / sampleRate;

    if (glidePitch)
        phaseIncrement.setTargetValue (targetIncrement);
    else
        phaseIncrement.setCurrentAndTargetValue (targetIncrement);
}

void HydraOscillator::setPhase (double initialPhase) noexcept
{
    currentPhase = initialPhase;
}

void HydraOscillator::advance() noexcept
{
    currentPhase += phaseIncrement.getNextValue();

    if (currentPhase >= twoPi)
        currentPhase -= twoPi;
}

float HydraOscillator::evaluateSample (float morphState) const noexcept
{
    const auto morph = juce::jlimit (kMorphMin, kMorphMax, morphState);
    const auto sine = std::sin (static_cast<float> (currentPhase));

    if (morph < 1.0f)
    {
        const auto tri = (2.0f / juce::MathConstants<float>::pi) * std::asin (sine);
        return sine + morph * (tri - sine);
    }

    if (morph < 2.0f)
    {
        const auto tri = (2.0f / juce::MathConstants<float>::pi) * std::asin (sine);
        const auto saw = 2.0f * (static_cast<float> (currentPhase) / juce::MathConstants<float>::twoPi) - 1.0f;
        const auto t = morph - 1.0f;
        return tri + t * (saw - tri);
    }

    const auto saw = 2.0f * (static_cast<float> (currentPhase) / juce::MathConstants<float>::twoPi) - 1.0f;
    const auto crushedSaw = std::round (saw * 4.0f) / 4.0f;
    const auto t = juce::jlimit (0.0f, 1.0f, morph - 2.0f);
    return saw + t * (crushedSaw - saw);
}
