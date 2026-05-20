#pragma once

#include <array>

namespace HydraHarmonySnap
{
inline constexpr int numSteps = 5;

inline constexpr std::array<float, numSteps> positions { 0.0f, 0.3f, 0.6f, 0.8f, 1.0f };

inline float quantizeHarmonyValue (float rawHarmony) noexcept
{
    const auto clampedHarmony = rawHarmony < 0.0f ? 0.0f : (rawHarmony > 1.0f ? 1.0f : rawHarmony);

    if (clampedHarmony < 0.15f)
        return 0.0f;

    if (clampedHarmony < 0.45f)
        return 0.3f;

    if (clampedHarmony < 0.70f)
        return 0.6f;

    if (clampedHarmony < 0.90f)
        return 0.8f;

    return 1.0f;
}

inline int stepIndexForQuantizedHarmony (float quantizedHarmony) noexcept
{
    if (quantizedHarmony < 0.1f)
        return 0;

    if (quantizedHarmony < 0.5f)
        return 1;

    if (quantizedHarmony < 0.7f)
        return 2;

    if (quantizedHarmony < 0.95f)
        return 3;

    return 4;
}

inline int stepIndexForHarmony (float rawHarmony) noexcept
{
    return stepIndexForQuantizedHarmony (quantizeHarmonyValue (rawHarmony));
}
} // namespace HydraHarmonySnap
