#include "HydraMacroMapper.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <limits>

float HydraMacroMapper::sigmoidalBloom (float depth, float betaN, float alphaN) noexcept
{
    return 1.0f / (1.0f + std::exp (-alphaN * (depth - betaN)));
}

HarmonicTargetPacket HydraMacroMapper::computeTargets (float depth, float girth) const noexcept
{
    const auto X = juce::jlimit (0.0f, 1.0f, depth);
    const auto Y = juce::jlimit (0.0f, 1.0f, girth);
    const auto effectiveGirth = Y * juce::jlimit (0.0f, 1.0f, X * 2.0f);

    HarmonicTargetPacket packet;
    std::array<float, numPartials> rawAmplitudes {};

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonicWeight = static_cast<float> (partialIndex) / static_cast<float> (numPartials - 1);

        rawAmplitudes[index] = sigmoidalBloom (X, beta[index], alpha[index]);
        rawAmplitudes[index] *= 1.0f + Y * harmonicWeight;

        const auto panPosition = 0.5f + (harmonicWeight - 0.5f) * effectiveGirth;
        const auto panAngle = panPosition * juce::MathConstants<float>::halfPi;
        packet.panningPairs[index] = { std::cos (panAngle), std::sin (panAngle) };

        const auto rawMorph = effectiveGirth * (1.0f + 0.5f * static_cast<float> (partialIndex));
        packet.morphStates[index] = juce::jlimit (0.0f, 3.0f, rawMorph);
    }

    auto energySum = 0.0f;
    for (const auto raw : rawAmplitudes)
        energySum += raw * raw;

    if (energySum <= std::numeric_limits<float>::epsilon())
    {
        packet.amplitudes.fill (0.0f);
        packet.morphStates.fill (0.0f);
        return packet;
    }

    const auto normalization = 1.0f / std::sqrt (energySum);
    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        packet.amplitudes[static_cast<size_t> (partialIndex)] = rawAmplitudes[static_cast<size_t> (partialIndex)] * normalization;

    return packet;
}
