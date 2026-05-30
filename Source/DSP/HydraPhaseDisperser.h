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
    static constexpr std::array<float, 4> baseCoefficients { 0.42f, -0.15f, 0.68f, -0.33f };
    static constexpr float coefficientModulationDepth = 0.015f;
    static constexpr std::array<float, 4> modulationRatesHz { 0.08f, 0.093f, 0.107f, 0.12f };
    static constexpr std::array<float, 4> modulationStartPhasesRadians {
        0.0f,
        1.5707963f,
        3.1415927f,
        4.7123890f
    };

    double sampleRate = 44100.0;
    std::array<float, 4> modulationPhaseRadians {};
    std::array<float, 4> modulationPhaseIncrement {};
    std::array<AllPassFilter, 4> allPassChainL {};
    std::array<AllPassFilter, 4> allPassChainR {};
};
