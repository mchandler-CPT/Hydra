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
};
