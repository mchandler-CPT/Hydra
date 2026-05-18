#pragma once

#include <array>
#include <cmath>
#include <utility>

#include <juce_dsp/juce_dsp.h>

class HydraParallelSaturator
{
public:
    static constexpr int numPartials = 7;

    std::pair<float, float> processSample (const std::array<float, numPartials>& partialSamplesLeft,
                                           const std::array<float, numPartials>& partialSamplesRight) noexcept
    {
        auto lowBandL = partialSamplesLeft[0] + partialSamplesLeft[1];
        auto lowBandR = partialSamplesRight[0] + partialSamplesRight[1];

        auto midBandL = partialSamplesLeft[2] + partialSamplesLeft[3] + partialSamplesLeft[4];
        auto midBandR = partialSamplesRight[2] + partialSamplesRight[3] + partialSamplesRight[4];

        auto highBandL = partialSamplesLeft[5] + partialSamplesLeft[6];
        auto highBandR = partialSamplesRight[5] + partialSamplesRight[6];

        lowBandL -= (lowBandL * lowBandL * lowBandL / 3.0f);
        lowBandR -= (lowBandR * lowBandR * lowBandR / 3.0f);

        midBandL = std::tanh (midBandL * 1.4f);
        midBandR = std::tanh (midBandR * 1.4f);

        highBandL = juce::jlimit (-0.7f, 0.7f, highBandL);
        highBandR = juce::jlimit (-0.7f, 0.7f, highBandR);

        return { lowBandL + midBandL + highBandL, lowBandR + midBandR + highBandR };
    }
};
