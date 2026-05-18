#include "HydraPhaseDisperser.h"

void HydraPhaseDisperser::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    reset();
}

void HydraPhaseDisperser::reset() noexcept
{
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

        for (size_t stage = 0; stage < coefficients.size(); ++stage)
        {
            leftSample = allPassChainL[stage].processSample (leftSample, coefficients[stage]);
            rightSample = allPassChainR[stage].processSample (rightSample, coefficients[stage]);
        }

        leftChannel[sampleIndex] = leftSample;
        rightChannel[sampleIndex] = rightSample;
    }
}
