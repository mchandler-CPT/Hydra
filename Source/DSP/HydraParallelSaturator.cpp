#include "HydraParallelSaturator.h"

#include <cmath>

namespace
{
float cubicSoftClip (float x) noexcept
{
    return x - (x * x * x / 3.0f);
}

float blendTowardAsymmetry (float symmetricValue, float asymmetricValue, float velocity) noexcept
{
    return symmetricValue + velocity * (asymmetricValue - symmetricValue);
}
} // namespace

float HydraParallelSaturator::processLowBand (float x, float velocity) noexcept
{
    if (x >= 0.0f)
        return cubicSoftClip (x);

    const auto negScale = blendTowardAsymmetry (1.0f, 1.08f, velocity);
    return cubicSoftClip (x * negScale);
}

float HydraParallelSaturator::processMidBand (float x, float velocity) noexcept
{
    static constexpr float symmetricDrive = 1.4f;
    static constexpr float asymmetricNegDrive = 1.22f;
    static constexpr float asymmetricNegPullback = 0.92f;

    if (x >= 0.0f)
        return std::tanh (x * symmetricDrive);

    const auto negDrive = blendTowardAsymmetry (symmetricDrive, asymmetricNegDrive, velocity);
    const auto negPullback = blendTowardAsymmetry (1.0f, asymmetricNegPullback, velocity);
    return std::tanh (x * negDrive) * negPullback;
}

float HydraParallelSaturator::processHighBand (float x, float velocity) noexcept
{
    static constexpr float symmetricPosLimit = 0.70f;
    static constexpr float symmetricNegLimit = -0.70f;
    static constexpr float asymmetricNegLimit = -0.58f;

    if (x >= 0.0f)
        return std::min (x, symmetricPosLimit);

    const auto negLimit = blendTowardAsymmetry (symmetricNegLimit, asymmetricNegLimit, velocity);
    return std::max (x, negLimit);
}

std::pair<float, float> HydraParallelSaturator::processSample (
    const std::array<float, numPartials>& partialSamplesLeft,
    const std::array<float, numPartials>& partialSamplesRight,
    float velocity) noexcept
{
    const auto clampedVelocity = std::fmin (1.0f, std::fmax (0.0f, velocity));

    const auto lowBandL = partialSamplesLeft[0] + partialSamplesLeft[1];
    const auto lowBandR = partialSamplesRight[0] + partialSamplesRight[1];

    const auto midBandL = partialSamplesLeft[2] + partialSamplesLeft[3] + partialSamplesLeft[4];
    const auto midBandR = partialSamplesRight[2] + partialSamplesRight[3] + partialSamplesRight[4];

    const auto highBandL = partialSamplesLeft[5] + partialSamplesLeft[6];
    const auto highBandR = partialSamplesRight[5] + partialSamplesRight[6];

    const auto saturatedLowL = processLowBand (lowBandL, clampedVelocity);
    const auto saturatedLowR = processLowBand (lowBandR, clampedVelocity);
    const auto saturatedMidL = processMidBand (midBandL, clampedVelocity);
    const auto saturatedMidR = processMidBand (midBandR, clampedVelocity);
    const auto saturatedHighL = processHighBand (highBandL, clampedVelocity);
    const auto saturatedHighR = processHighBand (highBandR, clampedVelocity);

    return { saturatedLowL + saturatedMidL + saturatedHighL,
             saturatedLowR + saturatedMidR + saturatedHighR };
}
