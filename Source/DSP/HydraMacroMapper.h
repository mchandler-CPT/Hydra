#pragma once

#include <array>
#include <utility>

struct HarmonicTargetPacket
{
    std::array<float, 7> amplitudes {};
    std::array<std::pair<float, float>, 7> panningPairs {};
    std::array<float, 7> morphStates {};
};

class HydraMacroMapper
{
public:
    static constexpr int numPartials = 7;

    HarmonicTargetPacket computeTargets (float depth, float girth) const noexcept;

private:
    static constexpr std::array<float, numPartials> beta { 0.00f, 0.15f, 0.30f, 0.45f, 0.60f, 0.75f, 0.90f };
    static constexpr std::array<float, numPartials> alpha { 20.0f, 18.0f, 15.0f, 12.0f, 10.0f, 8.0f, 6.0f };

    static float sigmoidalBloom (float depth, float betaN, float alphaN) noexcept;
};
