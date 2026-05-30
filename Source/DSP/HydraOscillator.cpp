#include "HydraOscillator.h"

#include <cmath>

namespace
{
constexpr float kMorphMin = 0.0f;
constexpr float kMorphMax = 3.0f;
constexpr double kPhaseIncrementSmoothingSeconds = 0.003;
constexpr double kTableTwoPi = juce::MathConstants<double>::twoPi;
} // namespace

SharedWavetables::SharedWavetables()
{
    for (int sampleIndex = 0; sampleIndex <= kWavetableSize; ++sampleIndex)
    {
        const auto phase = (kTableTwoPi * static_cast<double> (sampleIndex))
                         / static_cast<double> (kWavetableSize);
        const auto phaseFloat = static_cast<float> (phase);
        const auto sine = std::sin (phase);
        const auto saw = 2.0f * (phaseFloat / juce::MathConstants<float>::twoPi) - 1.0f;

        sineTable[static_cast<size_t> (sampleIndex)] = static_cast<float> (sine);
        triangleTable[static_cast<size_t> (sampleIndex)] =
            (2.0f / juce::MathConstants<float>::pi) * std::asin (static_cast<float> (sine));
        sawtoothTable[static_cast<size_t> (sampleIndex)] = saw;
        crushedTable[static_cast<size_t> (sampleIndex)] = std::round (saw * 4.0f) / 4.0f;
    }

    sineTable[static_cast<size_t> (kWavetableSize)] = sineTable[0];
    triangleTable[static_cast<size_t> (kWavetableSize)] = triangleTable[0];
    sawtoothTable[static_cast<size_t> (kWavetableSize)] = sawtoothTable[0];
    crushedTable[static_cast<size_t> (kWavetableSize)] = crushedTable[0];
}

const SharedWavetables& HydraOscillator::getSharedTables()
{
    static const SharedWavetables tables;
    return tables;
}

float HydraOscillator::interpolateTable (const std::array<float, kWavetableSize + 1>& table,
                                         int indexBase,
                                         float fraction) noexcept
{
    const auto wrappedIndex = ((indexBase % kWavetableSize) + kWavetableSize) % kWavetableSize;
    const auto baseIndex = static_cast<size_t> (wrappedIndex);
    const auto clampedFraction = juce::jlimit (0.0f, 1.0f, fraction);
    return table[baseIndex] + clampedFraction * (table[baseIndex + 1] - table[baseIndex]);
}

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

void HydraOscillator::noteOn() noexcept
{
    static constexpr double maxInitialPhaseRadians = 0.05 * twoPi;
    currentPhase = static_cast<double> (juce::Random::getSystemRandom().nextFloat())
                 * maxInitialPhaseRadians;
}

void HydraOscillator::setPhase (double initialPhase) noexcept
{
    currentPhase = initialPhase;
}

void HydraOscillator::advance() noexcept
{
    currentPhase += phaseIncrement.getNextValue();

    while (currentPhase >= twoPi)
        currentPhase -= twoPi;

    while (currentPhase < 0.0)
        currentPhase += twoPi;
}

float HydraOscillator::evaluateSample (float morphState) const noexcept
{
    const auto& tables = getSharedTables();
    const auto morph = juce::jlimit (kMorphMin, kMorphMax, morphState);

    auto wrappedPhase = currentPhase;
    while (wrappedPhase >= twoPi)
        wrappedPhase -= twoPi;
    while (wrappedPhase < 0.0)
        wrappedPhase += twoPi;

    const auto continuousIndex = static_cast<float> (wrappedPhase / twoPi)
                               * static_cast<float> (kWavetableSize);
    const auto clampedIndex = juce::jlimit (0.0f,
                                            static_cast<float> (kWavetableSize - 1),
                                            continuousIndex);
    const auto indexBase = static_cast<int> (clampedIndex);
    const auto fraction = clampedIndex - static_cast<float> (indexBase);

    const auto sine = interpolateTable (tables.sineTable, indexBase, fraction);
    const auto triangle = interpolateTable (tables.triangleTable, indexBase, fraction);
    const auto saw = interpolateTable (tables.sawtoothTable, indexBase, fraction);
    const auto crushed = interpolateTable (tables.crushedTable, indexBase, fraction);

    if (morph < 1.0f)
        return sine + morph * (triangle - sine);

    if (morph < 2.0f)
    {
        const auto t = morph - 1.0f;
        return triangle + t * (saw - triangle);
    }

    const auto t = juce::jlimit (0.0f, 1.0f, morph - 2.0f);
    return saw + t * (crushed - saw);
}
