#pragma once

#include <array>
#include <utility>

class HydraParallelSaturator
{
public:
    static constexpr int numPartials = 7;
    static constexpr float maxStereoLeakFactor = 0.08f;

    void reset() noexcept;

    std::pair<float, float> processSample (const std::array<float, numPartials>& partialSamplesLeft,
                                           const std::array<float, numPartials>& partialSamplesRight,
                                           float velocity,
                                           float depth,
                                           float girth) noexcept;

private:
    static float processLowBand (float x, float velocity) noexcept;
    static float processMidBand (float x, float velocity) noexcept;
    static float processHighBand (float x, float velocity) noexcept;

    float lastMidOutputL = 0.0f;
    float lastMidOutputR = 0.0f;
    float lastHighOutputL = 0.0f;
    float lastHighOutputR = 0.0f;
};
