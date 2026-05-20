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
    if (! std::isfinite (input))
        input = 0.0f;

    if (! std::isfinite (cutoffHz))
        cutoffHz = 20000.0f;

    const auto clampedResonance = juce::jlimit (0.0f, 4.0f, resonance);

    const auto maxSwing = std::min (600.0f, cutoffHz * 0.75f);
    const auto modulatedCutoff = cutoffHz + (lastOutput * clampedResonance * maxSwing);
    const auto clampedCutoff = juce::jlimit (20.0f, 0.40f * static_cast<float> (sampleRate), modulatedCutoff);

    const auto g = std::tan (juce::MathConstants<float>::pi * clampedCutoff / static_cast<float> (sampleRate));
    const auto h = g / (1.0f + g);

    const auto compensation = 1.0f + 0.5f * clampedResonance;
    const auto compensatedInput = input * compensation;

    const auto baseFeedback = (h * h * h * s1) + (h * h * s2) + (h * s3) + s4;
    const auto denominator = 1.0f + (clampedResonance * h * h * h * h);
    const auto u = (compensatedInput - (clampedResonance * std::tanh (baseFeedback))) / denominator;

    const auto v1 = std::tanh (u - s1);
    const auto y1 = s1 + g * v1;
    s1 = y1 + g * v1;

    const auto v2 = std::tanh (y1 - s2);
    const auto y2 = s2 + g * v2;
    s2 = y2 + g * v2;

    const auto v3 = std::tanh (y2 - s3);
    const auto y3 = s3 + g * v3;
    s3 = y3 + g * v3;

    const auto v4 = std::tanh (y3 - s4);
    const auto y4 = s4 + g * v4;
    s4 = y4 + g * v4;

    lastOutput = std::isfinite (y4) ? y4 : 0.0f;
    return lastOutput;
}
