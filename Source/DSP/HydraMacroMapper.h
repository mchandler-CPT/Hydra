#pragma once

#include <array>
#include <utility>

struct HarmonicTargetPacket
{
    std::array<float, 7> amplitudes {};
    std::array<std::pair<float, float>, 7> panningPairs {};
    std::array<float, 7> morphTargets { 0.0f };
    std::array<float, 7> frequencyMultipliers { 1.0f };
    std::array<float, 7> assignedHarmonicOrders { 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f };
};

class HydraMacroMapper
{
public:
    static constexpr int numPartials = 7;
    static constexpr int numHarmonicInversionModes = 6;

    HarmonicTargetPacket computeTargets (float depth, float girth, float harmony) const noexcept
    {
        return computeTargets (depth, girth, harmony, 0);
    }

    HarmonicTargetPacket computeTargets (float depth,
                                         float girth,
                                         float harmony,
                                         int harmonicInversionIndex) const noexcept;
};
