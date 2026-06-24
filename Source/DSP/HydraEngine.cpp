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
        maxDelayInSamples = 0;
        return;
    }

    const auto index = static_cast<size_t> (partialIndex);
    maxDelayInSamples = static_cast<int> (delayTimesVsIndex[index] * oversampledSampleRate);
    maxDelayInSamples = juce::jmin (maxDelayInSamples, delayBufferMask);
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

float HydraEngine::trianglePitchDriftMultiplier (float lfoPhase01) noexcept
{
    const auto wrappedPhase = lfoPhase01 - std::floor (lfoPhase01);
    const auto triangle = wrappedPhase < 0.5f
                              ? (4.0f * wrappedPhase - 1.0f)
                              : (3.0f - 4.0f * wrappedPhase);

    return 1.0f + triangle * maxPitchDriftCentsMultiplier;
}

void HydraEngine::advancePitchDriftLfo (float& lfoPhase01, float rateHz, double sr) noexcept
{
    lfoPhase01 += rateHz / static_cast<float> (sr);

    if (lfoPhase01 >= 1.0f)
        lfoPhase01 -= 1.0f;
}

void HydraEngine::prepare (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    noteOnSafetySampleCount = static_cast<float> (sampleRate * 0.001);
    maxSafeCutoffHz = juce::jmin (21000.0f, static_cast<float> (sampleRate) * 0.475f);
    delayModulationPhaseIncrement =
        delayModulationTwoPi * delayModulationRateHz / static_cast<float> (sampleRate);
    delayModulationPhase = 0.0f;

    for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        pitchDriftLfoPhase[static_cast<size_t> (partialIndex)] =
            static_cast<float> (partialIndex) / static_cast<float> (numPartials);

    constexpr double smoothingSeconds = 0.01;

    phaseDisperser.prepare (sampleRate);

    smoothedCutoffHz.reset (sampleRate, smoothingSeconds);
    smoothedCutoffHz.setCurrentAndTargetValue (20000.0f);

    smoothedVelocity.reset (sampleRate, 0.015);
    smoothedVelocity.setCurrentAndTargetValue (0.0f);

    appliedGlideTimeSeconds = juce::jmax (0.003f, glideTimeSeconds);
    smoothedFrequency.reset (sampleRate, static_cast<double> (appliedGlideTimeSeconds));
    smoothedFrequency.setCurrentAndTargetValue (0.0f);

    adsr.setSampleRate (sampleRate);
    filterAdsr.setSampleRate (sampleRate);

    adsr.setParameters (baseEnvelopeParameters);
    filterAdsr.setParameters (baseFilterEnvelopeParameters);

    constexpr auto parameterSmoothingSeconds = 0.02;
    smoothedDepth.reset (sampleRate, parameterSmoothingSeconds);
    smoothedDepth.setCurrentAndTargetValue (depth);
    smoothedGirth.reset (sampleRate, parameterSmoothingSeconds);
    smoothedGirth.setCurrentAndTargetValue (girth);
    smoothedFilterOverload.reset (sampleRate, parameterSmoothingSeconds);
    smoothedFilterOverload.setCurrentAndTargetValue (filterOverload);
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
    smoothedVelocity.setCurrentAndTargetValue (0.0f);
    lastEnvelopeGain = 0.0f;
    fundamentalFreq = 0.0f;
    samplesSinceNoteOn = 0;
    delayModulationPhase = 0.0f;
    lastFilterOutputL = 0.0;
    lastFilterOutputR = 0.0;
    filterL.reset();
    filterR.reset();

    adsr.reset();
    filterAdsr.reset();
    filterCutoffBuffer.clear();
    filterCutoffBufferR.clear();
    smoothedDepth.setCurrentAndTargetValue (0.0f);
    smoothedGirth.setCurrentAndTargetValue (0.0f);
    smoothedFilterOverload.setCurrentAndTargetValue (0.0f);
    saturator.reset();

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

    const auto packet = macroMapper.computeTargets (depth,
                                                    girth,
                                                    effectiveHarmonyForMapper,
                                                    harmonicInversionIndex);
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
    const auto isVoiceGenuinelySilent = ! adsr.isActive();
    const auto activeNote = noteStack[static_cast<size_t> (numNotesInStack - 1)];
    activeMidiNoteNumber = activeNote;
    const auto targetFreq = midiNoteToFrequency (activeNote);

    isKeyHeld = true;
    noteIsActive = true;
    fundamentalFreq = targetFreq;

    adsr.noteOn();
    filterAdsr.noteOn();

    if (isVoiceGenuinelySilent)
    {
        samplesSinceNoteOn = 0;
        smoothedVelocity.setCurrentAndTargetValue (clampedVelocity);
        smoothedFrequency.setCurrentAndTargetValue (targetFreq);

        for (auto& oscillator : oscillators)
            oscillator.noteOn();
    }
    else
    {
        smoothedVelocity.setTargetValue (clampedVelocity);
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
    constexpr auto maxInversionIndex = HydraMacroMapper::numHarmonicInversionModes - 1;

    if (harmonicInversion <= static_cast<float> (maxInversionIndex) + 0.5f)
        return juce::jlimit (0, maxInversionIndex, juce::roundToInt (harmonicInversion));

    return juce::jlimit (0,
                         maxInversionIndex,
                         juce::roundToInt (harmonicInversion * static_cast<float> (maxInversionIndex)));
}

void HydraEngine::setHarmonicTiltTarget (float harmonicTilt) noexcept
{
    harmonicTiltSmoothed.setTargetValue (juce::jlimit (-1.0f, 1.0f, harmonicTilt));
}

void HydraEngine::setHarmonicInversionIndexTarget (int harmonicInversionIndex) noexcept
{
    harmonicInversionSmoothed.setTargetValue (
        static_cast<float> (juce::jlimit (0, HydraMacroMapper::numHarmonicInversionModes - 1, harmonicInversionIndex)));
}

float HydraEngine::computeHarmonicTiltGain (float harmonicMultiplier, float tilt) noexcept
{
    constexpr auto negativeTiltCurve = 0.38f;
    constexpr auto harmonicSix = 6.0f;
    constexpr auto harmonicSeven = 7.0f;

    if (tilt < 0.0f)
        return std::exp (tilt * (harmonicMultiplier - 1.0f) * negativeTiltCurve);

    if (tilt > 0.0f)
    {
        const auto harmonicOrder = harmonicMultiplier;
        const auto isFundamental = std::abs (harmonicOrder - 1.0f) < 0.01f;

        float tiltWeight;

        if (isFundamental)
        {
            tiltWeight = juce::jmax (0.05f, 1.0f - tilt * 0.90f);
        }
        else if (harmonicOrder <= 3.01f)
        {
            const auto lowHarmonicBias = (4.0f - harmonicOrder) / 3.0f;
            tiltWeight = 1.0f - tilt * 0.55f * lowHarmonicBias;
        }
        else
        {
            tiltWeight = 1.0f + tilt * std::pow (harmonicOrder - 1.0f, 0.60f) * 0.22f;
        }

        if (std::abs (harmonicOrder - harmonicSix) < 0.01f)
            tiltWeight *= (1.0f - tilt * 0.25f);

        if (std::abs (harmonicOrder - harmonicSeven) < 0.01f)
            tiltWeight *= (1.0f - tilt * 0.65f);

        return juce::jmax (0.01f, tiltWeight);
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
    smoothedFilterOverload.setTargetValue (filterOverload);
}

void HydraEngine::setFilterResonance (float resonance) noexcept
{
    filterResonance = juce::jlimit (0.0f, 4.0f, resonance);
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
    smoothedDepth.setTargetValue (depth);
    applyMacroTargets();
}

void HydraEngine::setGirth (float newGirth) noexcept
{
    girth = juce::jlimit (0.0f, 1.0f, newGirth);
    smoothedGirth.setTargetValue (girth);
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
    smoothedCutoffHz.setTargetValue (juce::jlimit (20.0f, 21000.0f, cutoffHz));
}

namespace
{
float westCoastWaveFolder (float input, float gain) noexcept
{
    auto x = input * gain;

    while (x > 1.0f || x < -1.0f)
    {
        if (x > 1.0f)
            x = 2.0f - x;
        else if (x < -1.0f)
            x = -2.0f - x;
    }

    return x;
}
} // namespace

void HydraEngine::renderBlock (float* leftChannel, float* rightChannel, int numSamples) noexcept
{
    if (static_cast<int> (filterCutoffBuffer.size()) < numSamples)
    {
        filterCutoffBuffer.resize (static_cast<size_t> (numSamples), 20000.0f);
        filterCutoffBufferR.resize (static_cast<size_t> (numSamples), 20000.0f);
    }

    updateFrequencyGlideSmoothing();

    applyMacroTargets();

    const auto currentResonance = filterResonance;
    auto cdcDepth = 0.0f;

    if (currentResonance > 3.5f)
        cdcDepth = juce::jlimit (0.0f, 1.0f, (currentResonance - 3.5f) / 0.5f);

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
        const auto currentVelocity = smoothedVelocity.getNextValue();
        lastEnvelopeGain = envelopeGain * currentVelocity;

        if (! adsr.isActive())
        {
            (void) smoothedDepth.getNextValue();
            (void) smoothedGirth.getNextValue();
            (void) smoothedFilterOverload.getNextValue();
            const auto idleCutoff = smoothedCutoffHz.getCurrentValue();
            filterCutoffBuffer[static_cast<size_t> (sampleIndex)] = idleCutoff;
            filterCutoffBufferR[static_cast<size_t> (sampleIndex)] = idleCutoff;
            leftChannel[sampleIndex] = 0.0f;
            rightChannel[sampleIndex] = 0.0f;
            lastFilterOutputL = 0.0;
            lastFilterOutputR = 0.0;
            continue;
        }

        const auto baselineCutoff = smoothedCutoffHz.getNextValue();
        const auto currentBaseFreq = smoothedFrequency.getNextValue();
        const auto octavesFromAnchor = (currentBaseFreq > 0.0f)
                                         ? std::log2 (currentBaseFreq / 440.0f)
                                         : 0.0f;
        const auto trackedCutoffFloor =
            baselineCutoff * std::pow (2.0f, kbTrack * octavesFromAnchor);
        const auto dynamicCutoff =
            trackedCutoffFloor + (filterEnvAmt * egrAmount * 3500.0f);
        const auto clampedCutoff = juce::jlimit (20.0f, 21000.0f, dynamicCutoff);
        const auto currentDepth = smoothedDepth.getNextValue();
        const auto currentGirth = smoothedGirth.getNextValue();
        const auto stereoOffset = 1.0f + (currentGirth * 0.025f);
        const auto cutoffL = juce::jlimit (20.0f, maxSafeCutoffHz, clampedCutoff * stereoOffset);
        const auto cutoffR = juce::jlimit (20.0f, maxSafeCutoffHz, clampedCutoff * (2.0f - stereoOffset));
        filterCutoffBuffer[static_cast<size_t> (sampleIndex)] = cutoffL;
        filterCutoffBufferR[static_cast<size_t> (sampleIndex)] = cutoffR;
        const auto cutoffHz = clampedCutoff;

        delayModulationPhase += delayModulationPhaseIncrement;

        if (delayModulationPhase >= delayModulationTwoPi)
            delayModulationPhase -= delayModulationTwoPi;

        const auto delaySwingL = static_cast<int> (currentGirth * delayMicroSwingMaxSamples
                                                   * std::sin (delayModulationPhase));
        const auto delaySwingR = static_cast<int> (currentGirth * delayMicroSwingMaxSamples
                                                   * std::sin (delayModulationPhase + delayModulationPi));

        std::array<float, HydraParallelSaturator::numPartials> partialSamplesLeft {};
        std::array<float, HydraParallelSaturator::numPartials> partialSamplesRight {};
        std::array<float, numPartials> baseAmplitudes {};
        std::array<float, numPartials> tiltWeights {};

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            baseAmplitudes[index] = voices[index].amplitude.getNextValue();
            tiltWeights[index] =
                computeHarmonicTiltGain (assignedHarmonicOrders[index], harmonicTilt);
        }

        auto energyBeforeTilt = 0.0f;
        auto energyAfterTilt = 0.0f;

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            const auto baseAmplitude = baseAmplitudes[index];
            const auto tiltedAmplitude = baseAmplitude * tiltWeights[index];
            energyBeforeTilt += baseAmplitude * baseAmplitude;
            energyAfterTilt += tiltedAmplitude * tiltedAmplitude;
        }

        const auto tiltEnergyNorm = energyAfterTilt > 1.0e-12f
                                      ? std::sqrt (energyBeforeTilt / energyAfterTilt)
                                      : 1.0f;

        const auto ov = juce::jlimit (0.0f, 1.0f, smoothedFilterOverload.getNextValue());
        const auto foldingDrive = 1.0f + (std::pow (ov, 3.0f) * 2.5f);
        const auto folderWetMix = std::pow (ov, 2.0f);

        const auto safeLastFilterL = std::isfinite (lastFilterOutputL) ? lastFilterOutputL : 0.0;
        const auto safeLastFilterR = std::isfinite (lastFilterOutputR) ? lastFilterOutputR : 0.0;
        const auto cdcOffsetL = safeLastFilterL * static_cast<double> (cdcDepth) * 0.45;
        const auto cdcOffsetR = safeLastFilterR * static_cast<double> (cdcDepth) * 0.45;

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            auto& oscillator = oscillators[index];
            auto& voice = voices[index];

            const auto amplitude = baseAmplitudes[index] * tiltWeights[index] * tiltEnergyNorm;
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

            auto& pitchDriftPhase = pitchDriftLfoPhase[index];
            advancePitchDriftLfo (pitchDriftPhase, pitchDriftRatesHz[index], sampleRate);
            const auto pitchDriftMultiplier = trianglePitchDriftMultiplier (pitchDriftPhase);
            const auto fn = sanitizeTargetFrequency (currentBaseFreq * frequencyMultipliers[index]
                                                       * pitchDriftMultiplier);
            const auto harmonicFrequency = static_cast<double> (fn);
            oscillator.setFrequency (harmonicFrequency, false);

            const auto damping = (fn <= cutoffHz) ? 1.0f : std::exp (-(fn - cutoffHz) * spectralDampingS);

            float oscillatedL = 0.0f;
            float oscillatedR = 0.0f;

            if (cdcDepth > 0.0f)
            {
                oscillatedL = oscillator.evaluateSample (morph, cdcOffsetL);
                oscillatedR = oscillator.evaluateSample (morph, cdcOffsetR);
            }
            else
            {
                const auto oscillated = oscillator.evaluateSample (morph, 0.0);
                oscillatedL = oscillated;
                oscillatedR = oscillated;
            }

            const auto gain = amplitude * phaseIn * damping;
            float delayedL = oscillatedL;
            float delayedR = oscillatedR;

            if (currentGirth > 0.0f && voice.maxDelayInSamples > 0)
            {
                const auto baseDelaySamples = static_cast<int> (
                    static_cast<float> (voice.maxDelayInSamples) * currentGirth);
                const auto effectiveDelayL = juce::jlimit (0,
                                                           HydraPartialVoice::delayBufferMask,
                                                           baseDelaySamples + delaySwingL);
                const auto effectiveDelayR = juce::jlimit (0,
                                                           HydraPartialVoice::delayBufferMask,
                                                           baseDelaySamples + delaySwingR);
                voice.processDelaySampleStereo (oscillatedL,
                                                effectiveDelayL,
                                                effectiveDelayR,
                                                delayedL,
                                                delayedR);
            }

            partialSamplesLeft[index] = delayedL * gain * panL;
            partialSamplesRight[index] = delayedR * gain * panR;

            oscillator.advance();
        }

        auto summedPartialL = 0.0f;
        auto summedPartialR = 0.0f;

        for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
        {
            const auto index = static_cast<size_t> (partialIndex);
            summedPartialL += partialSamplesLeft[index];
            summedPartialR += partialSamplesRight[index];
        }

        const auto dryPartialL = summedPartialL;
        const auto dryPartialR = summedPartialR;
        const auto wetPartialL = westCoastWaveFolder (summedPartialL, foldingDrive);
        const auto wetPartialR = westCoastWaveFolder (summedPartialR, foldingDrive);
        const auto blendedPartialL = dryPartialL + folderWetMix * (wetPartialL - dryPartialL);
        const auto blendedPartialR = dryPartialR + folderWetMix * (wetPartialR - dryPartialR);

        if (std::abs (summedPartialL) > 1.0e-12f)
        {
            const auto foldScaleL = blendedPartialL / summedPartialL;

            for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
                partialSamplesLeft[static_cast<size_t> (partialIndex)] *= foldScaleL;
        }

        if (std::abs (summedPartialR) > 1.0e-12f)
        {
            const auto foldScaleR = blendedPartialR / summedPartialR;

            for (int partialIndex = 0; partialIndex < numPartials; ++partialIndex)
                partialSamplesRight[static_cast<size_t> (partialIndex)] *= foldScaleR;
        }

        const auto saturated = saturator.processSample (partialSamplesLeft,
                                                        partialSamplesRight,
                                                        currentVelocity,
                                                        currentDepth,
                                                        currentGirth);

        auto outputL = saturated.first * lastEnvelopeGain;
        auto outputR = saturated.second * lastEnvelopeGain;

        if (samplesSinceNoteOn < static_cast<int64_t> (noteOnSafetySampleCount))
        {
            const auto hardRamp = static_cast<float> (samplesSinceNoteOn) / noteOnSafetySampleCount;
            outputL *= hardRamp;
            outputR *= hardRamp;
        }

        outputL = applyFilterOverloadSample (outputL, filterOverload);
        outputR = applyFilterOverloadSample (outputR, filterOverload);

        const auto warpedCutoffL = clampLowPassCutoffHz (cutoffL, sampleRate);
        const auto warpedCutoffR = clampLowPassCutoffHz (cutoffR, sampleRate);
        const auto filteredL = filterL.processSample (outputL,
                                                      warpedCutoffL,
                                                      currentResonance,
                                                      sampleRate);
        const auto filteredR = filterR.processSample (outputR,
                                                      warpedCutoffR,
                                                      currentResonance,
                                                      sampleRate);

        leftChannel[sampleIndex] = filteredL;
        rightChannel[sampleIndex] = filteredR;
        lastFilterOutputL = std::isfinite (filteredL) ? static_cast<double> (filteredL) : 0.0;
        lastFilterOutputR = std::isfinite (filteredR) ? static_cast<double> (filteredR) : 0.0;

        ++samplesSinceNoteOn;
    }

    phaseDisperser.processBlock (leftChannel, rightChannel, numSamples);

    if (! adsr.isActive())
    {
        noteIsActive = false;
        smoothedVelocity.setCurrentAndTargetValue (0.0f);
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
        saturator.reset();
        filterL.reset();
        filterR.reset();
        lastFilterOutputL = 0.0;
        lastFilterOutputR = 0.0;
        filterAdsr.reset();
    }
}
