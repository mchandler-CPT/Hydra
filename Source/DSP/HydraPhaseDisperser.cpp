#include "HydraPhaseDisperser.h"

#include <cmath>

namespace
{
constexpr float kTwoPi = 6.2831853f;
} // namespace

void HydraPhaseDisperser::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    const auto sampleRateFloat = static_cast<float> (sampleRate);

    for (size_t stage = 0; stage < modulationPhaseIncrement.size(); ++stage)
    {
        modulationPhaseIncrement[stage] =
            kTwoPi * modulationRatesHz[stage] / sampleRateFloat;
        modulationPhaseRadians[stage] = modulationStartPhasesRadians[stage];
    }

    reset();
}

void HydraPhaseDisperser::reset() noexcept
{
    for (size_t stage = 0; stage < modulationPhaseRadians.size(); ++stage)
        modulationPhaseRadians[stage] = modulationStartPhasesRadians[stage];

    for (auto& stage : allPassChainL)
        stage.reset();

    for (auto& stage : allPassChainR)
        stage.reset();
}

void HydraPhaseDisperser::processBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept
{
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        auto leftSample = leftChannel[sampleIndex];
        auto rightSample = rightChannel[sampleIndex];

        for (size_t stage = 0; stage < baseCoefficients.size(); ++stage)
        {
            auto& lfoPhase = modulationPhaseRadians[stage];
            lfoPhase += modulationPhaseIncrement[stage];

            if (lfoPhase >= kTwoPi)
                lfoPhase -= kTwoPi;

            const auto dynamicCoefficient =
                baseCoefficients[stage]
                + (std::sin (lfoPhase) * coefficientModulationDepth);

            leftSample = allPassChainL[stage].processSample (leftSample, dynamicCoefficient);
            rightSample = allPassChainR[stage].processSample (rightSample, dynamicCoefficient);
        }

        leftChannel[sampleIndex] = leftSample;
        rightChannel[sampleIndex] = rightSample;
    }
}
