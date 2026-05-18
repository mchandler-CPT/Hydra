#include "ZdfLadderFilter.h"

#include <juce_dsp/juce_dsp.h>

#include <cmath>

void ZdfLadderFilter::reset() noexcept
{
    s1 = 0.0f;
    s2 = 0.0f;
    s3 = 0.0f;
    s4 = 0.0f;
    lastOutput = 0.0f;
}

float ZdfLadderFilter::processSample (float input, float cutoffHz, float resonance, double sampleRate) noexcept
{
    const auto clampedResonance = juce::jlimit (0.0f, 4.0f, resonance);

    const auto modulatedCutoff = cutoffHz + (lastOutput * clampedResonance * 600.0f);
    const auto clampedCutoff = juce::jlimit (20.0f, 0.40f * static_cast<float> (sampleRate), modulatedCutoff);

    const auto g = std::tan (juce::MathConstants<float>::pi * clampedCutoff / static_cast<float> (sampleRate));
    const auto h = g / (1.0f + g);

    const auto compensation = 1.0f + 0.5f * clampedResonance;
    const auto compensatedInput = input * compensation;

    const auto baseFeedback = (h * h * h * s1) + (h * h * s2) + (h * s3) + s4;
    const auto denominator = 1.0f + (clampedResonance * h * h * h * h);
    const auto u = (compensatedInput - (clampedResonance * std::tanh (baseFeedback))) / denominator;

    const auto v1 = (u - s1) * h;
    const auto y1 = v1 + s1;
    s1 = y1 + v1;

    const auto v2 = (y1 - s2) * h;
    const auto y2 = v2 + s2;
    s2 = y2 + v2;

    const auto v3 = (y2 - s3) * h;
    const auto y3 = v3 + s3;
    s3 = y3 + v3;

    const auto v4 = (y3 - s4) * h;
    const auto y4 = v4 + s4;
    s4 = y4 + v4;

    lastOutput = y4;
    return y4;
}
