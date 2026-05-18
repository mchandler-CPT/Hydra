#include "HydraEngine.h"

#include <cmath>

namespace
{
constexpr float kMorphMin = 0.0f;
constexpr float kMorphMax = 3.0f;

float evaluateSine (double theta) noexcept
{
    return std::sin (static_cast<float> (theta));
}

float evaluateTriangle (double theta) noexcept
{
    return (2.0f / juce::MathConstants<float>::pi)
         * std::asin (std::sin (static_cast<float> (theta)));
}

float evaluateSawtooth (double theta) noexcept
{
    auto wrapped = std::fmod (theta, juce::MathConstants<double>::twoPi);
    if (wrapped < 0.0)
        wrapped += juce::MathConstants<double>::twoPi;

    return static_cast<float> (2.0 * (wrapped / juce::MathConstants<double>::twoPi) - 1.0);
}

float evaluateBitCrush (float saw, float morph) noexcept
{
    const auto bits = juce::jmap (morph, 2.0f, 3.0f, 16.0f, 3.0f);
    return std::floor (saw * bits) / bits;
}
} // namespace

double HydraEngine::midiNoteToFrequency (int midiNoteNumber) noexcept
{
    return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
}

float HydraEngine::evaluatePartial (double theta, float morph) noexcept
{
    const auto clampedMorph = juce::jlimit (kMorphMin, kMorphMax, morph);
    const auto sine = evaluateSine (theta);
    const auto triangle = evaluateTriangle (theta);
    const auto saw = evaluateSawtooth (theta);

    if (clampedMorph < 1.0f)
        return juce::jmap (clampedMorph, 0.0f, 1.0f, sine, triangle);

    if (clampedMorph < 2.0f)
    {
        const auto blend = clampedMorph - 1.0f;
        return juce::jmap (blend, 0.0f, 1.0f, triangle, saw);
    }

    const auto blend = clampedMorph - 2.0f;
    const auto crushed = evaluateBitCrush (saw, clampedMorph);
    return juce::jmap (blend, 0.0f, 1.0f, saw, crushed);
}

void HydraEngine::prepare (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;

    constexpr double smoothingSeconds = 0.01;

    for (auto& head : heads)
    {
        head.amplitude.reset (sampleRate, smoothingSeconds);
        head.morph.reset (sampleRate, smoothingSeconds);
        head.panL.reset (sampleRate, smoothingSeconds);
        head.panR.reset (sampleRate, smoothingSeconds);

        head.amplitude.setCurrentAndTargetValue (0.0f);
        head.morph.setCurrentAndTargetValue (0.0f);
        head.panL.setCurrentAndTargetValue (1.0f);
        head.panR.setCurrentAndTargetValue (1.0f);

        head.phase = 0.0;
        head.phaseIncrement = 0.0;
    }
}

void HydraEngine::reset() noexcept
{
    for (auto& head : heads)
    {
        head.amplitude.setCurrentAndTargetValue (0.0f);
        head.morph.setCurrentAndTargetValue (0.0f);
        head.phase = 0.0;
        head.phaseIncrement = 0.0;
    }
}

void HydraEngine::setFundamentalFrequency (double fundamentalHz) noexcept
{
    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto harmonic = static_cast<double> (partialIndex + 1);
        heads[static_cast<size_t> (partialIndex)].phaseIncrement
            = twoPi * fundamentalHz * harmonic / sampleRate;
    }
}

void HydraEngine::noteOn (int midiNoteNumber, float velocity)
{
    const auto fundamentalHz = midiNoteToFrequency (midiNoteNumber);
    setFundamentalFrequency (fundamentalHz);

    const auto clampedVelocity = juce::jlimit (0.0f, 1.0f, velocity);

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        auto& head = heads[static_cast<size_t> (partialIndex)];
        head.phase = twoPi / static_cast<double> (primeNumbers[static_cast<size_t> (partialIndex)]);
        head.amplitude.setTargetValue (clampedVelocity);
    }
}

void HydraEngine::noteOff (int /*midiNoteNumber*/) noexcept
{
    for (auto& head : heads)
        head.amplitude.setTargetValue (0.0f);
}

void HydraEngine::setMorph (float morph) noexcept
{
    const auto clampedMorph = juce::jlimit (kMorphMin, kMorphMax, morph);

    for (auto& head : heads)
        head.morph.setTargetValue (clampedMorph);
}

void HydraEngine::setPan (float pan) noexcept
{
    const auto clampedPan = juce::jlimit (-1.0f, 1.0f, pan);
    const auto angle = (clampedPan + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    const auto leftGain = std::cos (angle);
    const auto rightGain = std::sin (angle);

    for (auto& head : heads)
    {
        head.panL.setTargetValue (leftGain);
        head.panR.setTargetValue (rightGain);
    }
}

void HydraEngine::renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept
{
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        auto leftSample = 0.0f;
        auto rightSample = 0.0f;

        for (auto& head : heads)
        {
            const auto amplitude = head.amplitude.getNextValue();
            const auto morph = head.morph.getNextValue();
            const auto panL = head.panL.getNextValue();
            const auto panR = head.panR.getNextValue();

            const auto partialSample = evaluatePartial (head.phase, morph) * amplitude;
            leftSample += partialSample * panL;
            rightSample += partialSample * panR;

            head.phase += head.phaseIncrement;
            if (head.phase >= twoPi)
                head.phase -= twoPi;
        }

        leftChannel[sampleIndex] = leftSample;
        rightChannel[sampleIndex] = rightSample;
    }
}
