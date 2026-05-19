#pragma once

#include <array>
#include <utility>

struct HarmonicTargetPacket
{
    std::array<float, 7> amplitudes {};
    std::array<std::pair<float, float>, 7> panningPairs {};
    std::array<float, 7> morphTargets { 0.0f };
    std::array<float, 7> frequencyMultipliers { 1.0f };
};

class HydraMacroMapper
{
public:
    static constexpr int numPartials = 7;

    HarmonicTargetPacket computeTargets (float depth, float girth, float harmony) const noexcept;
};
