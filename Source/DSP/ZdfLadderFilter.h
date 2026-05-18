#pragma once

class ZdfLadderFilter
{
public:
    void reset() noexcept;

    float processSample (float input, float cutoffHz, float resonance, double sampleRate) noexcept;

private:
    float s1 = 0.0f;
    float s2 = 0.0f;
    float s3 = 0.0f;
    float s4 = 0.0f;
};
