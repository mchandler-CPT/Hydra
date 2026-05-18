#include "HydraEngine.h"

#include <cmath>

double HydraEngine::midiNoteToFrequency (int midiNoteNumber) noexcept
{
    return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
}

void HydraEngine::prepare (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;

    constexpr double smoothingSeconds = 0.01;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        auto& oscillator = oscillators[static_cast<size_t> (partialIndex)];
        auto& voice = voices[static_cast<size_t> (partialIndex)];

        oscillator.prepare (sampleRate);

        voice.amplitude.reset (sampleRate, smoothingSeconds);
        voice.morph.reset (sampleRate, smoothingSeconds);
        voice.panL.reset (sampleRate, smoothingSeconds);
        voice.panR.reset (sampleRate, smoothingSeconds);

        voice.amplitude.setCurrentAndTargetValue (0.0f);
        voice.morph.setCurrentAndTargetValue (0.0f);
        voice.panL.setCurrentAndTargetValue (1.0f);
        voice.panR.setCurrentAndTargetValue (0.0f);
    }
}

void HydraEngine::reset() noexcept
{
    noteIsActive = false;
    isKeyHeld = false;
    noteVelocity = 0.0f;
    voiceAmplitude = 0.0f;

    for (auto& voice : voices)
    {
        voice.amplitude.setCurrentAndTargetValue (0.0f);
        voice.morph.setCurrentAndTargetValue (0.0f);
        voice.panL.setCurrentAndTargetValue (1.0f);
        voice.panR.setCurrentAndTargetValue (0.0f);
    }

    for (auto& oscillator : oscillators)
    {
        oscillator.setPhase (0.0);
        oscillator.setFrequency (0.0, false);
    }
}

void HydraEngine::applyMacroTargets() noexcept
{
    const auto packet = macroMapper.computeTargets (depth, girth);

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        auto& voice = voices[index];

        voice.amplitude.setTargetValue (packet.amplitudes[index]);
        voice.panL.setTargetValue (packet.panningPairs[index].first);
        voice.panR.setTargetValue (packet.panningPairs[index].second);
    }
}

void HydraEngine::noteOn (int midiNoteNumber, float velocity)
{
    const auto fundamentalHz = midiNoteToFrequency (midiNoteNumber);
    const auto clampedVelocity = juce::jlimit (0.0f, 1.0f, velocity);
    const auto isLegatoTransition = (voiceAmplitude > 0.0f);

    noteVelocity = clampedVelocity;
    voiceAmplitude = clampedVelocity;
    isKeyHeld = true;
    noteIsActive = true;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonic = static_cast<double> (partialIndex + 1);
        const auto harmonicFrequency = fundamentalHz * harmonic;

        if (isLegatoTransition)
        {
            oscillators[index].setFrequency (harmonicFrequency, true);
        }
        else
        {
            oscillators[index].setPhase (twoPi / static_cast<double> (primeNumbers[index]));
            oscillators[index].setFrequency (harmonicFrequency, false);
        }
    }

    applyMacroTargets();
}

void HydraEngine::noteOff (int /*midiNoteNumber*/) noexcept
{
    isKeyHeld = false;
}

void HydraEngine::setMorph (float morph) noexcept
{
    const auto clampedMorph = juce::jlimit (0.0f, 3.0f, morph);

    for (auto& voice : voices)
        voice.morph.setTargetValue (clampedMorph);
}

void HydraEngine::setDepth (float newDepth) noexcept
{
    depth = juce::jlimit (0.0f, 1.0f, newDepth);
    applyMacroTargets();
}

void HydraEngine::setGirth (float newGirth) noexcept
{
    girth = juce::jlimit (0.0f, 1.0f, newGirth);
    applyMacroTargets();
}

void HydraEngine::renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept
{
    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        if (! isKeyHeld)
        {
            voiceAmplitude *= 0.9995f;

            if (voiceAmplitude < 0.001f)
                voiceAmplitude = 0.0f;
        }

        if (voiceAmplitude == 0.0f)
        {
            leftChannel[sampleIndex] = 0.0f;
            rightChannel[sampleIndex] = 0.0f;
            continue;
        }

        auto leftSample = 0.0f;
        auto rightSample = 0.0f;

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            auto& oscillator = oscillators[index];
            auto& voice = voices[index];

            const auto amplitude = voice.amplitude.getNextValue();
            const auto morph = voice.morph.getNextValue();
            const auto panL = voice.panL.getNextValue();
            const auto panR = voice.panR.getNextValue();

            const auto partialSample = oscillator.evaluateSample (morph) * amplitude;
            leftSample += partialSample * panL;
            rightSample += partialSample * panR;

            oscillator.advance();
        }

        leftChannel[sampleIndex] = leftSample * voiceAmplitude;
        rightChannel[sampleIndex] = rightSample * voiceAmplitude;
    }

    if (voiceAmplitude == 0.0f)
    {
        noteIsActive = false;
        noteVelocity = 0.0f;

        for (auto& voice : voices)
            voice.amplitude.setCurrentAndTargetValue (0.0f);
    }
}
