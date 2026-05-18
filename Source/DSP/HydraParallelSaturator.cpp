#include "HydraParallelSaturator.h"

#include <cmath>

namespace
{
float cubicSoftClip (float x) noexcept
{
    return x - (x * x * x / 3.0f);
}
} // namespace

float HydraParallelSaturator::processLowBand (float x) noexcept
{
    if (x >= 0.0f)
        return cubicSoftClip (x);

    const auto scaled = x * 1.08f;
    return cubicSoftClip (scaled);
}

float HydraParallelSaturator::processMidBand (float x) noexcept
{
    if (x >= 0.0f)
        return std::tanh (x * 1.4f);

    return std::tanh (x * 1.22f) * 0.92f;
}

float HydraParallelSaturator::processHighBand (float x) noexcept
{
    if (x >= 0.0f)
        return std::min (x, 0.70f);

    return std::max (x, -0.58f);
}

std::pair<float, float> HydraParallelSaturator::processSample (
    const std::array<float, numPartials>& partialSamplesLeft,
    const std::array<float, numPartials>& partialSamplesRight) noexcept
{
    const auto lowBandL = partialSamplesLeft[0] + partialSamplesLeft[1];
    const auto lowBandR = partialSamplesRight[0] + partialSamplesRight[1];

    const auto midBandL = partialSamplesLeft[2] + partialSamplesLeft[3] + partialSamplesLeft[4];
    const auto midBandR = partialSamplesRight[2] + partialSamplesRight[3] + partialSamplesRight[4];

    const auto highBandL = partialSamplesLeft[5] + partialSamplesLeft[6];
    const auto highBandR = partialSamplesRight[5] + partialSamplesRight[6];

    const auto saturatedLowL = processLowBand (lowBandL);
    const auto saturatedLowR = processLowBand (lowBandR);
    const auto saturatedMidL = processMidBand (midBandL);
    const auto saturatedMidR = processMidBand (midBandR);
    const auto saturatedHighL = processHighBand (highBandL);
    const auto saturatedHighR = processHighBand (highBandR);

    return { saturatedLowL + saturatedMidL + saturatedHighL,
             saturatedLowR + saturatedMidR + saturatedHighR };
}
