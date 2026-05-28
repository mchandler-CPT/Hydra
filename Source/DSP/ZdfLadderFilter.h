#pragma once

#include <juce_core/juce_core.h>

inline float clampLowPassCutoffHz (float cutoffHz, double sampleRate) noexcept
{
    const auto maxSafeCutoff = juce::jmin (21000.0f, static_cast<float> (sampleRate) * 0.475f);
    return juce::jmin (cutoffHz, maxSafeCutoff);
}

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
    float lastOutput = 0.0f;
};
