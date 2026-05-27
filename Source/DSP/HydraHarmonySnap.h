#pragma once

#include <array>
#include <cmath>

namespace HydraHarmonySnap
{
inline constexpr int numSteps = 13;

/** Physical slider coordinates aligned to the native inverse-log harmony timeline. */
inline constexpr std::array<float, numSteps> steppedHarmonyPositions {
    0.0000f, // recipe 0 -> 1 crossfade segment
    0.0833f, // recipe 0 -> 1 crossfade segment
    0.1667f, // recipe 0 -> 1 crossfade segment
    0.2500f, // recipe 0 -> 1 crossfade segment
    0.3333f, // recipe 1 -> 2 crossfade segment
    0.4167f, // recipe 1 -> 2 crossfade segment
    0.5000f, // recipe 1 -> 2 crossfade segment
    0.5833f, // recipe 2 -> 3 crossfade segment
    0.6667f, // recipe 2 -> 3 crossfade segment
    0.7500f, // recipe 2 -> 3 crossfade segment
    0.8333f, // recipe 3 -> 4 crossfade segment
    0.9167f, // recipe 3 -> 4 crossfade segment
    1.0000f  // recipe 3 -> 4 crossfade segment
};

inline int stepIndexForHarmony (float rawHarmony) noexcept
{
    const auto clampedHarmony = rawHarmony < 0.0f ? 0.0f : (rawHarmony > 1.0f ? 1.0f : rawHarmony);
    int nearestIndex = 0;
    auto nearestDistance = std::abs (steppedHarmonyPositions[0] - clampedHarmony);

    for (int index = 1; index < numSteps; ++index)
    {
        const auto distance = std::abs (steppedHarmonyPositions[static_cast<size_t> (index)] - clampedHarmony);

        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestIndex = index;
        }
    }

    return nearestIndex;
}

inline float quantizeHarmonyValue (float rawHarmony) noexcept
{
    return steppedHarmonyPositions[static_cast<size_t> (stepIndexForHarmony (rawHarmony))];
}

inline int stepIndexForQuantizedHarmony (float quantizedHarmony) noexcept
{
    return stepIndexForHarmony (quantizedHarmony);
}

} // namespace HydraHarmonySnap
