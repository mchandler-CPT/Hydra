#pragma once

#include <array>
#include <utility>

class HydraParallelSaturator
{
public:
    static constexpr int numPartials = 7;

    std::pair<float, float> processSample (const std::array<float, numPartials>& partialSamplesLeft,
                                           const std::array<float, numPartials>& partialSamplesRight,
                                           float velocity) noexcept;

private:
    static float processLowBand (float x, float velocity) noexcept;
    static float processMidBand (float x, float velocity) noexcept;
    static float processHighBand (float x, float velocity) noexcept;
};
