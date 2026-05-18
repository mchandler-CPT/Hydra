#pragma once

#include <array>
#include <utility>

class HydraParallelSaturator
{
public:
    static constexpr int numPartials = 7;

    std::pair<float, float> processSample (const std::array<float, numPartials>& partialSamplesLeft,
                                           const std::array<float, numPartials>& partialSamplesRight) noexcept;

private:
    static float processLowBand (float x) noexcept;
    static float processMidBand (float x) noexcept;
    static float processHighBand (float x) noexcept;
};
