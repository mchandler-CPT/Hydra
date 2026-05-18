#include "HydraMacroMapper.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>
#include <limits>

HarmonicTargetPacket HydraMacroMapper::computeTargets (float depth, float girth) const noexcept
{
    static constexpr std::array<float, numPartials> beta { 0.00f, 0.15f, 0.30f, 0.45f, 0.60f, 0.75f, 0.90f };
    static constexpr std::array<float, numPartials> alpha { 20.0f, 18.0f, 15.0f, 12.0f, 10.0f, 8.0f, 6.0f };

    const auto X = juce::jlimit (0.0f, 1.0f, depth);
    const auto Y = juce::jlimit (0.0f, 1.0f, girth);
    const auto effectiveGirth = Y * juce::jlimit (0.0f, 1.0f, X * 2.0f);

    HarmonicTargetPacket packet;
    std::array<float, numPartials> rawAmplitudes {};

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonicWeight = static_cast<float> (partialIndex) / static_cast<float> (numPartials - 1);

        rawAmplitudes[index] = 1.0f / (1.0f + std::exp (-alpha[index] * (X - beta[index])));

        const auto panPosition = 0.5f + (harmonicWeight - 0.5f) * effectiveGirth;
        const auto panAngle = panPosition * juce::MathConstants<float>::halfPi;
        packet.panningPairs[index] = { std::cos (panAngle), std::sin (panAngle) };

        const auto targetMorph = (effectiveGirth * 2.0f)
                               + (effectiveGirth * 1.0f * (static_cast<float> (partialIndex) / 6.0f));
        packet.morphStates[index] = juce::jlimit (0.0f, 3.0f, targetMorph);
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
        packet.morphStates.fill (0.0f);
    }

    return packet;
}
