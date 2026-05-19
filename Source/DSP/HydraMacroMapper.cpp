#include "HydraMacroMapper.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <limits>

HarmonicTargetPacket HydraMacroMapper::computeTargets (float depth, float girth) const noexcept
{
    static constexpr std::array<float, numPartials> beta { 0.00f, 0.15f, 0.30f, 0.45f, 0.60f, 0.75f, 0.90f };
    static constexpr std::array<float, numPartials> alpha { 20.0f, 18.0f, 15.0f, 12.0f, 10.0f, 8.0f, 6.0f };
    static constexpr auto centerGain = juce::MathConstants<float>::sqrt2 * 0.5f;

    const auto X = juce::jlimit (0.0f, 1.0f, depth);
    const auto Y = juce::jlimit (0.0f, 1.0f, girth);
    const auto effectiveGirth = Y * juce::jlimit (0.0f, 1.0f, X * 2.0f);

    HarmonicTargetPacket packet;
    std::array<float, numPartials> rawAmplitudes {};

    const auto thetaG = effectiveGirth * 0.08f;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto n = partialIndex;
        const auto index = static_cast<size_t> (partialIndex);

        rawAmplitudes[index] = 1.0f / (1.0f + std::exp (-alpha[index] * (X - beta[index])));

        if (n == 0)
        {
            packet.panningPairs[index] = { centerGain, centerGain };
            packet.morphTargets[index] = 0.0f;
            packet.frequencyMultipliers[index] = 1.0f;
        }
        else if (n == 1 || n == 3 || n == 5)
        {
            const auto harmonicIndex = n + 1;
            const auto sign = (harmonicIndex % 2 == 0) ? -1.0f : 1.0f;
            const auto angle = (static_cast<float> (harmonicIndex - 1) * juce::MathConstants<float>::pi / 12.0f) + thetaG;
            const auto panOffset = effectiveGirth * sign * std::sin (angle);
            const auto clampedPan = juce::jlimit (-1.0f, 1.0f, panOffset);
            const auto normalizedPan = (clampedPan + 1.0f) * 0.5f;

            packet.panningPairs[index].first =
                std::cos (normalizedPan * juce::MathConstants<float>::pi * 0.5f);
            packet.panningPairs[index].second =
                std::sin (normalizedPan * juce::MathConstants<float>::pi * 0.5f);

            packet.morphTargets[index] = 2.0f;
            packet.frequencyMultipliers[index] = static_cast<float> (n + 1);
        }
        else
        {
            const auto standardHarmonic = static_cast<float> (n + 1);
            const auto targetDetune = (n == 2) ? 1.98f : ((n == 4) ? 5.039f : 8.00f);
            packet.frequencyMultipliers[index] =
                standardHarmonic + effectiveGirth * (targetDetune - standardHarmonic);
            packet.morphTargets[index] = 2.0f + effectiveGirth;

            const auto spread = effectiveGirth;
            const auto goLeft = (n == 2 || n == 6);
            const auto normalizedPan = goLeft ? 0.5f * (1.0f - spread)
                                              : 0.5f + 0.5f * spread;

            packet.panningPairs[index].first =
                std::cos (normalizedPan * juce::MathConstants<float>::pi * 0.5f);
            packet.panningPairs[index].second =
                std::sin (normalizedPan * juce::MathConstants<float>::pi * 0.5f);
        }
    }

    auto energySum = 0.0f;
    for (const auto amplitude : rawAmplitudes)
        energySum += amplitude * amplitude;

    const auto totalEnergy = std::sqrt (energySum);

    if (totalEnergy > 0.0f)
    {
        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
            packet.amplitudes[static_cast<size_t> (partialIndex)] =
                rawAmplitudes[static_cast<size_t> (partialIndex)] / totalEnergy;
    }
    else
    {
        packet.amplitudes.fill (0.0f);
        packet.morphTargets.fill (0.0f);

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
            packet.frequencyMultipliers[static_cast<size_t> (partialIndex)] =
                static_cast<float> (partialIndex + 1);
    }

    return packet;
}
