#include "HydraEngine.h"
#include "HydraHarmonySnap.h"

#include <cmath>

float HydraEngine::sanitizeTargetFrequency (float frequencyHz) const noexcept
{
    if (! std::isfinite (frequencyHz))
        return 440.0f;

    return juce::jlimit (8.0f, 20000.0f, frequencyHz);
}

float HydraEngine::midiNoteToFrequency (int midiNoteNumber) const noexcept
{
    if (midiNoteNumber == 69)
        return 440.0f;

    const auto morphValue = scaleMorph;
    const auto tuningDivisor = 12.0f + (morphValue * 12.0f);
    const auto morphedFrequency = 440.0f * std::pow (2.0f,
                                                     (static_cast<float> (midiNoteNumber) - 69.0f) / tuningDivisor);
    return sanitizeTargetFrequency (morphedFrequency);
}

void HydraPartialVoice::configureDelay (double oversampledSampleRate, int partialIndex) noexcept
{
    static constexpr std::array<float, 7> delayTimesVsIndex {
        0.0f, 0.0021f, 0.0044f, 0.0068f, 0.0093f, 0.0119f, 0.0145f
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

    appliedGlideTimeSeconds = juce::jmax (0.003f, glideTimeSeconds);
    smoothedFrequency.reset (sampleRate, static_cast<double> (appliedGlideTimeSeconds));
    smoothedFrequency.setCurrentAndTargetValue (0.0f);

    adsr.setSampleRate (sampleRate);
    filterAdsr.setSampleRate (sampleRate);

    adsr.setParameters (baseEnvelopeParameters);
    filterAdsr.setParameters (baseFilterEnvelopeParameters);

    constexpr auto parameterSmoothingSeconds = 0.02;
    harmonicTiltSmoothed.reset (sampleRate, parameterSmoothingSeconds);
    harmonicInversionSmoothed.reset (sampleRate, parameterSmoothingSeconds);
    harmonicTiltSmoothed.setCurrentAndTargetValue (0.0f);
    harmonicInversionSmoothed.setCurrentAndTargetValue (0.0f);
    harmonicInversionIndex = 0;

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
    lastEnvelopeGain = 0.0f;
    fundamentalFreq = 0.0f;
    samplesSinceNoteOn = 0;

    adsr.reset();
    filterAdsr.reset();
    filterCutoffBuffer.clear();

    smoothedCutoffHz.setCurrentAndTargetValue (20000.0f);
    smoothedFrequency.setCurrentAndTargetValue (0.0f);

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
    const auto clampedHarmony = juce::jlimit (0.0f, 1.0f, harmony);
    const auto effectiveHarmonyForMapper = harmonyQuantize
        ? HydraHarmonySnap::quantizeHarmonyValue (clampedHarmony)
        : clampedHarmony;

    const auto packet = macroMapper.computeTargets (depth, girth, effectiveHarmonyForMapper, harmonicInversionIndex);
    frequencyMultipliers = packet.frequencyMultipliers;
    assignedHarmonicOrders = packet.assignedHarmonicOrders;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        auto& voice = voices[index];

        voice.amplitude.setTargetValue (packet.amplitudes[index]);
        voice.morph.setTargetValue (packet.morphTargets[index]);
        voice.panL.setTargetValue (packet.panningPairs[index].first);
        voice.panR.setTargetValue (packet.panningPairs[index].second);
    }
}

void HydraEngine::updateOscillatorTuning (bool glidePitch) noexcept
{
    const auto baseFrequency = smoothedFrequency.getCurrentValue();

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
    {
        const auto index = static_cast<size_t> (partialIndex);
        const auto harmonicFrequency =
            static_cast<double> (baseFrequency * frequencyMultipliers[index]);

        oscillators[index].setFrequency (harmonicFrequency, glidePitch);
    }
}

void HydraEngine::retuneOscillatorsForNote (int midiNoteNumber, bool glidePitch) noexcept
{
    activeMidiNoteNumber = midiNoteNumber;
    const auto targetFreq = midiNoteToFrequency (midiNoteNumber);
    fundamentalFreq = targetFreq;

    if (glidePitch)
        smoothedFrequency.setTargetValue (targetFreq);
    else
        smoothedFrequency.setCurrentAndTargetValue (targetFreq);
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
    const auto isNoteAlreadyPlaying = adsr.isActive();
    const auto activeNote = noteStack[static_cast<size_t> (numNotesInStack - 1)];
    activeMidiNoteNumber = activeNote;
    const auto targetFreq = midiNoteToFrequency (activeNote);

    noteVelocity = clampedVelocity;
    isKeyHeld = true;
    noteIsActive = true;
    fundamentalFreq = targetFreq;

    adsr.noteOn();
    filterAdsr.noteOn();

    if (! isNoteAlreadyPlaying)
    {
        samplesSinceNoteOn = 0;
        smoothedFrequency.setCurrentAndTargetValue (targetFreq);

        for (auto& oscillator : oscillators)
            oscillator.setPhase (0.0);
    }
    else
    {
        smoothedFrequency.setTargetValue (targetFreq);
    }

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
    adsr.noteOff();
    filterAdsr.noteOff();
}

void HydraEngine::setEnvelopeParameters (float attack, float decay, float sustain, float release) noexcept
{
    baseEnvelopeParameters.attack = juce::jmax (0.001f, attack);
    baseEnvelopeParameters.decay = juce::jmax (0.001f, decay);
    baseEnvelopeParameters.sustain = juce::jlimit (0.0f, 1.0f, sustain);
    baseEnvelopeParameters.release = juce::jmax (0.001f, release);
    baseAttackSeconds = baseEnvelopeParameters.attack;
    adsr.setParameters (baseEnvelopeParameters);
}

void HydraEngine::setEnvWarp (float newEnvWarp) noexcept
{
    envWarp = juce::jlimit (-1.0f, 1.0f, newEnvWarp);
}

void HydraEngine::setEgrAmount (float newEgrAmount) noexcept
{
    egrAmount = juce::jlimit (-1.0f, 1.0f, newEgrAmount);
}

void HydraEngine::setGlideTime (float newGlideTimeSeconds) noexcept
{
    glideTimeSeconds = juce::jlimit (0.0f, 2.0f, newGlideTimeSeconds);
}

void HydraEngine::setScaleMorph (float newScaleMorph) noexcept
{
    scaleMorph = juce::jlimit (0.0f, 1.0f, newScaleMorph);
}

int HydraEngine::harmonicInversionIndexFromParameter (float harmonicInversion) noexcept
{
    return juce::jlimit (0, 2, juce::roundToInt (harmonicInversion));
}

void HydraEngine::setHarmonicTiltTarget (float harmonicTilt) noexcept
{
    harmonicTiltSmoothed.setTargetValue (juce::jlimit (-1.0f, 1.0f, harmonicTilt));
}

void HydraEngine::setHarmonicInversionIndexTarget (int harmonicInversionIndex) noexcept
{
    harmonicInversionSmoothed.setTargetValue (static_cast<float> (juce::jlimit (0, 2, harmonicInversionIndex)));
}

float HydraEngine::computeHarmonicTiltGain (float harmonicMultiplier, float tilt) noexcept
{
    constexpr auto tiltCurve = 0.5f;
    constexpr auto harmonicSix = 6.0f;
    constexpr auto harmonicSeven = 7.0f;

    if (tilt < 0.0f)
        return std::exp (tilt * (harmonicMultiplier - 1.0f) * tiltCurve);

    if (tilt > 0.0f)
    {
        auto gain = std::exp (tilt * (harmonicMultiplier - 1.0f) * tiltCurve);

        if (std::abs (harmonicMultiplier - harmonicSix) < 0.01f)
            gain *= (1.0f - tilt * 0.25f);

        if (std::abs (harmonicMultiplier - harmonicSeven) < 0.01f)
            gain *= (1.0f - tilt * 0.75f);

        return gain;
    }

    return 1.0f;
}

void HydraEngine::setKbTrack (float newKbTrack) noexcept
{
    kbTrack = juce::jlimit (0.0f, 1.0f, newKbTrack);
}

void HydraEngine::setFilterOverload (float newFilterOverload) noexcept
{
    filterOverload = juce::jlimit (0.0f, 1.0f, newFilterOverload);
}

float HydraEngine::applyFilterOverloadSample (float sample, float overloadKnob) noexcept
{
    const auto clampedOverload = juce::jlimit (0.0f, 1.0f, overloadKnob);

    if (clampedOverload <= 0.0f)
        return sample;

    const auto driveFactor = 1.0f + (clampedOverload * 4.0f);
    const auto drivenSample = sample * driveFactor;
    const auto saturatedSample = std::tanh (drivenSample);
    const auto compensationFactor = 1.0f / std::sqrt (driveFactor);

    return saturatedSample * compensationFactor;
}

void HydraEngine::applyFilterOverload (float* leftChannel, float* rightChannel, int numSamples) const noexcept
{
    if (leftChannel == nullptr || numSamples <= 0 || filterOverload <= 0.0f)
        return;

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        leftChannel[sampleIndex] = applyFilterOverloadSample (leftChannel[sampleIndex], filterOverload);

        if (rightChannel != nullptr)
            rightChannel[sampleIndex] = applyFilterOverloadSample (rightChannel[sampleIndex], filterOverload);
    }
}

void HydraEngine::updateFrequencyGlideSmoothing() noexcept
{
    const auto targetSmoothTime = juce::jmax (0.003f, glideTimeSeconds);

    if (std::abs (targetSmoothTime - appliedGlideTimeSeconds) <= 1.0e-6f)
        return;

    smoothedFrequency.reset (sampleRate, static_cast<double> (targetSmoothTime));
    appliedGlideTimeSeconds = targetSmoothTime;
}

void HydraEngine::setFilterEnvelopeParameters (float attack, float decay, float sustain, float release) noexcept
{
    baseFilterEnvelopeParameters.attack = juce::jmax (0.001f, attack);
    baseFilterEnvelopeParameters.decay = juce::jmax (0.001f, decay);
    baseFilterEnvelopeParameters.sustain = juce::jlimit (0.0f, 1.0f, sustain);
    baseFilterEnvelopeParameters.release = juce::jmax (0.001f, release);
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

void HydraEngine::setHarmony (float newHarmony) noexcept
{
    harmony = juce::jlimit (0.0f, 1.0f, newHarmony);
    applyMacroTargets();
}

void HydraEngine::setHarmonyQuantize (bool newHarmonyQuantize) noexcept
{
    harmonyQuantize = newHarmonyQuantize;
    applyMacroTargets();
}

void HydraEngine::setFilterCutoff (float cutoffHz) noexcept
{
    smoothedCutoffHz.setTargetValue (juce::jlimit (20.0f, 20000.0f, cutoffHz));
}

void HydraEngine::renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept
{
    if (static_cast<int> (filterCutoffBuffer.size()) < numSamples)
        filterCutoffBuffer.resize (static_cast<size_t> (numSamples), 20000.0f);

    updateFrequencyGlideSmoothing();

    applyMacroTargets();

    for (int sampleIndex = 0; sampleIndex < numSamples; ++sampleIndex)
    {
        const auto nextInversionIndex =
            harmonicInversionIndexFromParameter (harmonicInversionSmoothed.getNextValue());

        if (nextInversionIndex != harmonicInversionIndex)
        {
            harmonicInversionIndex = nextInversionIndex;
            applyMacroTargets();
        }

        const auto harmonicTilt = harmonicTiltSmoothed.getNextValue();
        juce::ADSR::Parameters warpedVolumeEnvelope = baseEnvelopeParameters;
        warpedVolumeEnvelope.attack = juce::jmax (0.001f, baseEnvelopeParameters.attack * (1.0f + envWarp));
        adsr.setParameters (warpedVolumeEnvelope);

        juce::ADSR::Parameters warpedFilterEnvelope = baseFilterEnvelopeParameters;
        warpedFilterEnvelope.attack = juce::jmax (0.001f, baseFilterEnvelopeParameters.attack * (1.0f + envWarp));
        filterAdsr.setParameters (warpedFilterEnvelope);

        const auto envelopeGain = adsr.getNextSample();
        const auto filterEnvAmt = filterAdsr.getNextSample();
        lastEnvelopeGain = envelopeGain * noteVelocity;

        if (! adsr.isActive())
        {
            filterCutoffBuffer[static_cast<size_t> (sampleIndex)] = smoothedCutoffHz.getCurrentValue();
            leftChannel[sampleIndex] = 0.0f;
            rightChannel[sampleIndex] = 0.0f;
            continue;
        }

        const auto baselineCutoff = smoothedCutoffHz.getNextValue();
        const auto morphValue = scaleMorph;
        const auto tuningDivisor = 12.0f + (morphValue * 12.0f);
        const auto octavesFromAnchor =
            (static_cast<float> (activeMidiNoteNumber) - 69.0f) / tuningDivisor;
        const auto trackedCutoffFloor =
            baselineCutoff * std::pow (2.0f, kbTrack * octavesFromAnchor);
        const auto dynamicCutoff =
            trackedCutoffFloor + (filterEnvAmt * egrAmount * 3500.0f);
        const auto cutoffHz = juce::jlimit (20.0f, 20000.0f, dynamicCutoff);
        filterCutoffBuffer[static_cast<size_t> (sampleIndex)] = cutoffHz;
        const auto currentBaseFreq = smoothedFrequency.getNextValue();

        std::array<float, HydraParallelSaturator::numPartials> partialSamplesLeft {};
        std::array<float, HydraParallelSaturator::numPartials> partialSamplesRight {};

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            auto& oscillator = oscillators[index];
            auto& voice = voices[index];

            const auto baseAmplitude = voice.amplitude.getNextValue();
            const auto tiltGain =
                computeHarmonicTiltGain (assignedHarmonicOrders[index], harmonicTilt);
            const auto amplitude = baseAmplitude * tiltGain;
            const auto morph = voice.morph.getNextValue();
            const auto panL = voice.panL.getNextValue();
            const auto panR = voice.panR.getNextValue();

            float partialAttackSeconds = baseAttackSeconds;
            const auto currentEnvWarp = envWarp;

            if (currentEnvWarp >= 0.0f)
            {
                partialAttackSeconds += (currentEnvWarp * 0.05f * static_cast<float> (partialIndex));
            }
            else
            {
                const auto warpIntensity = std::abs (currentEnvWarp);
                partialAttackSeconds += (warpIntensity * 0.05f * static_cast<float> ((numPartials - 1) - partialIndex));
            }

            const auto partialAttackSamples = partialAttackSeconds * static_cast<float> (sampleRate);

            auto phaseIn = 1.0f;

            if (partialAttackSamples > 0.0f)
            {
                phaseIn = juce::jlimit (0.0f,
                                        1.0f,
                                        static_cast<float> (samplesSinceNoteOn) / partialAttackSamples);
            }

            const auto fn = sanitizeTargetFrequency (currentBaseFreq * frequencyMultipliers[index]);
            const auto harmonicFrequency = static_cast<double> (fn);
            oscillator.setFrequency (harmonicFrequency, false);

            const auto damping = (fn <= cutoffHz) ? 1.0f : std::exp (-(fn - cutoffHz) * spectralDampingS);

            const auto oscillated = oscillator.evaluateSample (morph);
            const auto delayed = voice.processDelaySample (oscillated);
            const auto partialSample = delayed * amplitude * phaseIn * damping;
            partialSamplesLeft[index] = partialSample * panL;
            partialSamplesRight[index] = partialSample * panR;

            oscillator.advance();
        }

        const auto saturated = saturator.processSample (partialSamplesLeft, partialSamplesRight);

        leftChannel[sampleIndex] = saturated.first * lastEnvelopeGain;
        rightChannel[sampleIndex] = saturated.second * lastEnvelopeGain;

        ++samplesSinceNoteOn;
    }

    phaseDisperser.processBlock (leftChannel, rightChannel, numSamples);

    if (! adsr.isActive())
    {
        noteIsActive = false;
        noteVelocity = 0.0f;
        samplesSinceNoteOn = 0;
        smoothedFrequency.setCurrentAndTargetValue (0.0f);
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
        filterAdsr.reset();
    }
}
