#include "HydraMacroMapper.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <limits>

HarmonicTargetPacket HydraMacroMapper::computeTargets (float depth, float girth, float harmony) const noexcept
{
    static constexpr std::array<float, numPartials> beta { 0.00f, 0.15f, 0.30f, 0.45f, 0.60f, 0.75f, 0.90f };
    static constexpr std::array<float, numPartials> alpha { 20.0f, 18.0f, 15.0f, 12.0f, 10.0f, 8.0f, 6.0f };
    static constexpr auto centerGain = 0.707f;

    static constexpr float harmonicRecipes[5][7] = {
        { 1.000f, 2.000f, 3.000f, 4.000f, 5.000f, 6.000f, 7.000f },
        { 1.000f, 2.000f, 3.000f, 4.000f, 6.000f, 8.000f, 12.00f },
        { 1.000f, 2.000f, 2.400f, 3.000f, 4.000f, 4.800f, 6.000f },
        { 1.000f, 3.000f, 5.000f, 7.000f, 9.000f, 11.00f, 13.00f },
        { 1.000f, 2.000f, 4.000f, 5.039f, 6.000f, 8.000f, 16.00f }
    };

    const auto X = juce::jlimit (0.0f, 1.0f, depth);
    const auto rawGirth = juce::jlimit (0.0f, 1.0f, girth);
    const auto Y = rawGirth * juce::jlimit (0.0f, 1.0f, X * 2.0f);

    const auto harmonyClamped = juce::jlimit (0.0f, 1.0f, harmony);
    const auto scaledIndex = harmonyClamped * 4.0f;
    auto baseIdx = static_cast<int> (scaledIndex);
    auto frac = scaledIndex - static_cast<float> (baseIdx);

    if (baseIdx >= 4)
    {
        baseIdx = 3;
        frac = 1.0f;
    }

    HarmonicTargetPacket packet;
    std::array<float, numPartials> rawAmplitudes {};

    for (int n = 0; n < numPartials; ++n)
    {
        const auto index = static_cast<size_t> (n);

        rawAmplitudes[index] = 1.0f / (1.0f + std::exp (-alpha[index] * (X - beta[index])));

        const auto baseHarmonic = harmonicRecipes[baseIdx][n]
                                + frac * (harmonicRecipes[baseIdx + 1][n] - harmonicRecipes[baseIdx][n]);

        if (n == 0)
        {
            packet.panningPairs[index] = { centerGain, centerGain };
            packet.morphTargets[index] = 0.0f;
            packet.frequencyMultipliers[index] = 1.0f;
        }
        else if (n == 1 || n == 3 || n == 5)
        {
            packet.morphTargets[index] = Y * 2.0f;
            packet.frequencyMultipliers[index] = baseHarmonic;

            const auto harmonicIndex = n + 1;
            const auto sign = (harmonicIndex % 2 == 0) ? -1.0f : 1.0f;
            const auto thetaG = Y * 0.08f;
            const auto angle = (static_cast<float> (harmonicIndex - 1) * juce::MathConstants<float>::pi / 12.0f)
                             + thetaG;
            const auto panOffset = juce::jlimit (-1.0f, 1.0f, Y * sign * std::sin (angle));
            const auto normalizedPan = (panOffset + 1.0f) * 0.5f;

            packet.panningPairs[index].first =
                std::cos (normalizedPan * juce::MathConstants<float>::pi * 0.5f);
            packet.panningPairs[index].second =
                std::sin (normalizedPan * juce::MathConstants<float>::pi * 0.5f);
        }
        else if (n == 2 || n == 4 || n == 6)
        {
            packet.morphTargets[index] = Y * 3.0f;

            auto microOffset = 0.0f;

            if (n == 2)
                microOffset = Y * 0.015f;
            else if (n == 4)
                microOffset = Y * -0.018f;
            else if (n == 6)
                microOffset = Y * 0.045f;

            packet.frequencyMultipliers[index] = baseHarmonic + microOffset;

            const auto leftEdge = (n == 2 || n == 6) ? 1.0f : 0.0f;
            const auto rightEdge = (n == 4) ? 1.0f : 0.0f;

            packet.panningPairs[index].first = centerGain + Y * (leftEdge - centerGain);
            packet.panningPairs[index].second = centerGain + Y * (rightEdge - centerGain);
        }

        if (X < 0.02f && n > 0)
        {
            rawAmplitudes[index] = 0.0f;
            packet.frequencyMultipliers[index] = static_cast<float> (n + 1);
            packet.morphTargets[index] = 0.0f;
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
