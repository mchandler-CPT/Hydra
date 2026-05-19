#include "HydraEngine.h"

#include <cmath>

double HydraEngine::midiNoteToFrequency (int midiNoteNumber) noexcept
{
    return 440.0 * std::pow (2.0, (midiNoteNumber - 69) / 12.0);
}

void HydraPartialVoice::configureDelay (double oversampledSampleRate, int partialIndex) noexcept
{
    static constexpr std::array<float, 7> delayTimesVsIndex {
        0.0f, 0.0021f, 0.0044f, 0.0068f, 0.0093f, 0.00119f, 0.0145f
    };

    if (partialIndex <= 0)
    {
        delayInSamples = 0;
        return;
    }

    const auto index = static_cast<size_t> (partialIndex);
    delayInSamples = static_cast<int> (delayTimesVsIndex[index] * oversampledSampleRate);
    delayInSamples = juce::jmin (delayInSamples, delayBufferMask);
}

void HydraPartialVoice::clearDelay() noexcept
{
    delayBuffer.fill (0.0f);
    writeIndex = 0;
}

void HydraEngine::clearAllDelayLines() noexcept
{
    for (auto& voice : voices)
        voice.clearDelay();
}

void HydraEngine::prepare (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;

    constexpr double smoothingSeconds = 0.01;

    phaseDisperser.prepare (sampleRate);

    smoothedCutoffHz.reset (sampleRate, smoothingSeconds);
    smoothedCutoffHz.setCurrentAndTargetValue (20000.0f);

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

        voice.configureDelay (sampleRate, partialIndex);
        voice.clearDelay();
    }
}

void HydraEngine::reset() noexcept
{
    noteIsActive = false;
    numNotesInStack = 0;
    noteStack.fill (0);
    isKeyHeld = false;
    noteVelocity = 0.0f;
    voiceAmplitude = 0.0f;
    fundamentalFreq = 0.0f;

    smoothedCutoffHz.setCurrentAndTargetValue (20000.0f);

    phaseDisperser.reset();

    for (auto& voice : voices)
    {
        voice.amplitude.setCurrentAndTargetValue (0.0f);
        voice.morph.setCurrentAndTargetValue (0.0f);
        voice.panL.setCurrentAndTargetValue (1.0f);
        voice.panR.setCurrentAndTargetValue (0.0f);
    }

    clearAllDelayLines();

    for (auto& oscillator : oscillators)
    {
        oscillator.setPhase (0.0);
        oscillator.setFrequency (0.0, false);
    }
}

void HydraEngine::applyMacroTargets() noexcept
{
    const auto packet = macroMapper.computeTargets (depth, girth);
    frequencyMultipliers = packet.frequencyMultipliers;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        auto& voice = voices[index];

        voice.amplitude.setTargetValue (packet.amplitudes[index]);
        voice.morph.setTargetValue (packet.morphTargets[index]);
        voice.panL.setTargetValue (packet.panningPairs[index].first);
        voice.panR.setTargetValue (packet.panningPairs[index].second);
    }

    if (fundamentalFreq > 0.0f)
        updateOscillatorTuning (true);
}

void HydraEngine::updateOscillatorTuning (bool glidePitch) noexcept
{
    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonicFrequency =
            static_cast<double> (fundamentalFreq * frequencyMultipliers[index]);

        oscillators[index].setFrequency (harmonicFrequency, glidePitch);
    }
}

void HydraEngine::retuneOscillatorsForNote (int midiNoteNumber, bool glidePitch) noexcept
{
    const auto fundamentalHz = midiNoteToFrequency (midiNoteNumber);
    fundamentalFreq = static_cast<float> (fundamentalHz);

    if (! glidePitch)
    {
        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            oscillators[index].setPhase (static_cast<double> (juce::MathConstants<float>::twoPi / primes[index]));
        }
    }

    updateOscillatorTuning (glidePitch);
}

void HydraEngine::noteOn (int midiNoteNumber, float velocity)
{
    auto noteAlreadyHeld = false;

    for (int stackIndex = 0; stackIndex < numNotesInStack; ++stackIndex)
    {
        if (noteStack[static_cast<size_t> (stackIndex)] == midiNoteNumber)
        {
            noteAlreadyHeld = true;
            break;
        }
    }

    if (! noteAlreadyHeld && numNotesInStack < static_cast<int> (noteStack.size()))
    {
        noteStack[static_cast<size_t> (numNotesInStack)] = midiNoteNumber;
        ++numNotesInStack;
    }

    const auto clampedVelocity = juce::jlimit (0.0f, 1.0f, velocity);
    const auto isLegatoTransition = (voiceAmplitude > 0.0f);
    const auto activeNote = noteStack[static_cast<size_t> (numNotesInStack - 1)];

    retuneOscillatorsForNote (activeNote, isLegatoTransition);

    noteVelocity = clampedVelocity;
    voiceAmplitude = clampedVelocity;
    isKeyHeld = true;
    noteIsActive = true;

    applyMacroTargets();
}

void HydraEngine::noteOff (int midiNoteNumber) noexcept
{
    for (int stackIndex = 0; stackIndex < numNotesInStack; ++stackIndex)
    {
        if (noteStack[static_cast<size_t> (stackIndex)] != midiNoteNumber)
            continue;

        for (int shiftIndex = stackIndex; shiftIndex < numNotesInStack - 1; ++shiftIndex)
            noteStack[static_cast<size_t> (shiftIndex)] = noteStack[static_cast<size_t> (shiftIndex + 1)];

        --numNotesInStack;
        break;
    }

    if (numNotesInStack > 0)
    {
        const auto underlyingNote = noteStack[static_cast<size_t> (numNotesInStack - 1)];
        retuneOscillatorsForNote (underlyingNote, true);
        isKeyHeld = true;
        return;
    }

    isKeyHeld = false;
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

void HydraEngine::setFilterCutoff (float cutoffHz) noexcept
{
    smoothedCutoffHz.setTargetValue (juce::jlimit (20.0f, 20000.0f, cutoffHz));
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

        const auto fc = smoothedCutoffHz.getNextValue();

        std::array<float, HydraParallelSaturator::numPartials> partialSamplesLeft {};
        std::array<float, HydraParallelSaturator::numPartials> partialSamplesRight {};

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            auto& oscillator = oscillators[index];
            auto& voice = voices[index];

            const auto amplitude = voice.amplitude.getNextValue();
            const auto morph = voice.morph.getNextValue();
            const auto panL = voice.panL.getNextValue();
            const auto panR = voice.panR.getNextValue();

            const auto fn = fundamentalFreq * frequencyMultipliers[index];
            const auto damping = (fn <= fc) ? 1.0f : std::exp (-(fn - fc) * spectralDampingS);

            const auto oscillated = oscillator.evaluateSample (morph);
            const auto delayed = voice.processDelaySample (oscillated);
            const auto partialSample = delayed * amplitude * damping;
            partialSamplesLeft[index] = partialSample * panL;
            partialSamplesRight[index] = partialSample * panR;

            oscillator.advance();
        }

        const auto saturated = saturator.processSample (partialSamplesLeft, partialSamplesRight);

        leftChannel[sampleIndex] = saturated.first * voiceAmplitude;
        rightChannel[sampleIndex] = saturated.second * voiceAmplitude;
    }

    phaseDisperser.processBlock (leftChannel, rightChannel, numSamples);

    if (voiceAmplitude == 0.0f)
    {
        noteIsActive = false;
        noteVelocity = 0.0f;
        numNotesInStack = 0;
        noteStack.fill (0);
        isKeyHeld = false;

        for (auto& voice : voices)
        {
            voice.amplitude.setCurrentAndTargetValue (0.0f);
            voice.morph.setCurrentAndTargetValue (0.0f);
        }

        clearAllDelayLines();
        phaseDisperser.reset();
    }
}
