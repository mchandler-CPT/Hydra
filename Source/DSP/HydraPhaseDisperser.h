#pragma once

#include <array>

struct AllPassFilter
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

class HydraPhaseDisperser
{
public:
    void prepare (double sampleRate);
    void reset() noexcept;
    void processBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept;

private:
    static constexpr std::array<float, 4> coefficients { 0.42f, -0.15f, 0.68f, -0.33f };

    double sampleRate = 44100.0;
    std::array<AllPassFilter, 4> allPassChainL {};
    std::array<AllPassFilter, 4> allPassChainR {};
};
