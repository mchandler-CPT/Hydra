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
    struct HighBandAllPassStage
    {
        float x1 { 0.0f };
        float y1 { 0.0f };

        void reset() noexcept
        {
            x1 = 0.0f;
            y1 = 0.0f;
        }

        float processSample (float input, float coefficient) noexcept
        {
            const auto output = (-coefficient * input) + x1 + (coefficient * y1);
            x1 = input;
            y1 = output;
            return output;
        }
    };

    static float processLowBand (float x, float velocity) noexcept;
    static float processMidBand (float x, float velocity) noexcept;
    static float processHighBandBaseline (float x) noexcept;
    float processHighBandGlossExcited (float x, float girthMacro, HighBandAllPassStage* glossChain) noexcept;
    float processHighBand (float x, float girthMacro, HighBandAllPassStage* glossChain) noexcept;

    static constexpr std::array<float, 3> highGlossCoefficients { 0.91f, -0.77f, 0.84f };

    float lastMidOutputL = 0.0f;
    float lastMidOutputR = 0.0f;
    float lastHighOutputL = 0.0f;
    float lastHighOutputR = 0.0f;
    std::array<HighBandAllPassStage, 3> highGlossChainL {};
    std::array<HighBandAllPassStage, 3> highGlossChainR {};
};
