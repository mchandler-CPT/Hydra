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
    const auto clampedDepth = juce::jlimit (0.0f, 1.0f, depth);
    const auto clampedGirth = juce::jlimit (0.0f, 1.0f, girth);

    HarmonicTargetPacket packet;
    std::array<float, numPartials> rawAmplitudes {};

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonicWeight = static_cast<float> (partialIndex) / static_cast<float> (numPartials - 1);

        rawAmplitudes[index] = sigmoidalBloom (clampedDepth, beta[index], alpha[index]);
        rawAmplitudes[index] *= 1.0f + clampedGirth * harmonicWeight;

        const auto panPosition = 0.5f + (harmonicWeight - 0.5f) * clampedGirth;
        const auto panAngle = panPosition * juce::MathConstants<float>::halfPi;
        packet.panningPairs[index] = { std::cos (panAngle), std::sin (panAngle) };
    }

    auto energySum = 0.0f;
    for (const auto raw : rawAmplitudes)
        energySum += raw * raw;

    if (energySum <= std::numeric_limits<float>::epsilon())
    {
        packet.amplitudes.fill (0.0f);
        return packet;
    }

    const auto normalization = 1.0f / std::sqrt (energySum);
    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        packet.amplitudes[static_cast<size_t> (partialIndex)] = rawAmplitudes[static_cast<size_t> (partialIndex)] * normalization;

    return packet;
}
