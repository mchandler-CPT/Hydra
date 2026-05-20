#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/HydraEngine.h"
#include "DSP/HydraMacroMapper.h"
#include "DSP/HydraOscillator.h"
#include "DSP/HydraParallelSaturator.h"
#include "DSP/HydraPhaseDisperser.h"
#include "DSP/ZdfLadderFilter.h"

#include <cmath>
#include <vector>

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
constexpr double kTwoPi = juce::MathConstants<double>::twoPi;
HydraEngine makePreparedEngine()
{
    HydraEngine engine;
    engine.prepare (kSampleRate, kBlockSize);
    return engine;
}

float renderPeak (HydraEngine& engine, int numSamples)
{
    std::vector<float> left (static_cast<size_t> (numSamples), 0.0f);
    std::vector<float> right (static_cast<size_t> (numSamples), 0.0f);
    engine.renderBlock (left.data(), right.data(), numSamples);

    auto peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = std::max (peak, std::max (std::abs (left[static_cast<size_t> (i)]),
                                         std::abs (right[static_cast<size_t> (i)])));
    return peak;
}

float sumSquaredAmplitudes (const HarmonicTargetPacket& packet)
{
    auto energy = 0.0f;
    for (const auto amplitude : packet.amplitudes)
        energy += amplitude * amplitude;
    return energy;
}

float computeRms (const std::vector<float>& buffer, int startIndex, int numSamples)
{
    double sumSquares = 0.0;
    for (int i = 0; i < numSamples; ++i)
    {
        const auto sample = static_cast<double> (buffer[static_cast<size_t> (startIndex + i)]);
        sumSquares += sample * sample;
    }
    return static_cast<float> (std::sqrt (sumSquares / static_cast<double> (numSamples)));
}

float computePeak (const std::vector<float>& buffer, int startIndex, int numSamples)
{
    auto peak = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        peak = std::max (peak, std::abs (buffer[static_cast<size_t> (startIndex + i)]));
    return peak;
}

bool bufferContainsNonFinite (const std::vector<float>& buffer)
{
    for (const auto sample : buffer)
        if (! std::isfinite (sample))
            return true;
    return false;
}
} // namespace

TEST_CASE ("HydraOscillator Waveshaping Integrity", "[HydraOscillator]")
{
    const auto& tables = HydraOscillator::getSharedTables();

    REQUIRE (tables.sineTable[static_cast<size_t> (HydraOscillator::kWavetableSize)]
             == Catch::Approx (tables.sineTable[0]).margin (1.0e-7f));
    REQUIRE (tables.crushedTable[static_cast<size_t> (HydraOscillator::kWavetableSize)]
             == Catch::Approx (tables.crushedTable[0]).margin (1.0e-7f));

    HydraOscillator oscillator;
    oscillator.prepare (kSampleRate);
    oscillator.setFrequency (440.0, false);

    const auto theta = juce::MathConstants<double>::halfPi;
    oscillator.setPhase (theta);

    const auto sine = std::sin (static_cast<float> (theta));
    const auto triangle = (2.0f / juce::MathConstants<float>::pi)
                        * std::asin (std::sin (static_cast<float> (theta)));

    REQUIRE (oscillator.evaluateSample (0.0f) == Catch::Approx (sine).margin (2.0e-4f));
    REQUIRE (oscillator.evaluateSample (1.0f) == Catch::Approx (triangle).margin (2.0e-4f));

    const auto sineTriangleBlend = oscillator.evaluateSample (0.5f);
    REQUIRE (sineTriangleBlend == Catch::Approx (0.5f * (sine + triangle)).margin (2.0e-4f));

    const auto saw = 2.0f * (static_cast<float> (theta) / juce::MathConstants<float>::twoPi) - 1.0f;
    const auto crushedSaw = std::round (saw * 4.0f) / 4.0f;

    REQUIRE (oscillator.evaluateSample (2.0f) == Catch::Approx (saw).margin (2.0e-4f));
    REQUIRE (oscillator.evaluateSample (3.0f) == Catch::Approx (crushedSaw).margin (2.0e-4f));

    const auto triangleSawBlend = oscillator.evaluateSample (1.5f);
    REQUIRE (triangleSawBlend == Catch::Approx (0.5f * (triangle + saw)).margin (2.0e-4f));

    const auto sawCrushBlend = oscillator.evaluateSample (2.5f);
    REQUIRE (sawCrushBlend == Catch::Approx (0.5f * (saw + crushedSaw)).margin (2.0e-4f));

    oscillator.setPhase (0.0);
    for (int frame = 0; frame < 100'000; ++frame)
    {
        oscillator.advance();
        REQUIRE (oscillator.getCurrentPhase() >= 0.0);
        REQUIRE (oscillator.getCurrentPhase() < kTwoPi);
    }

    const auto expectedIncrement = kTwoPi * 440.0 / kSampleRate;
    REQUIRE (oscillator.getPhaseIncrement() == Catch::Approx (expectedIncrement).margin (1.0e-9));
}

TEST_CASE ("HydraOscillator legato preserves phase continuity", "[HydraOscillator]")
{
    HydraOscillator oscillator;
    oscillator.prepare (kSampleRate);
    oscillator.setFrequency (440.0, false);
    oscillator.setPhase (1.25);

    const auto phaseBeforeLegato = oscillator.getCurrentPhase();

    for (int frame = 0; frame < 512; ++frame)
        oscillator.advance();

    oscillator.setFrequency (523.25, true);

    for (int frame = 0; frame < 512; ++frame)
        oscillator.advance();

    REQUIRE (oscillator.getCurrentPhase() >= 0.0);
    REQUIRE (oscillator.getCurrentPhase() < kTwoPi);
    REQUIRE (oscillator.getCurrentPhase() != Catch::Approx (phaseBeforeLegato).margin (1.0e-9));
    REQUIRE (oscillator.getCurrentPhase() != Catch::Approx (0.0).margin (1.0e-9));
    REQUIRE (oscillator.getPhaseIncrement() == Catch::Approx (kTwoPi * 523.25 / kSampleRate).margin (1.0e-4));
}

TEST_CASE ("HydraPhaseDisperser Energy Conservation", "[HydraPhaseDisperser]")
{
    HydraPhaseDisperser disperser;
    disperser.prepare (kSampleRate);

    constexpr int totalSamples = 1000;
    constexpr int settleSamples = 128;
    std::vector<float> left (static_cast<size_t> (totalSamples), 0.0f);
    std::vector<float> right (static_cast<size_t> (totalSamples), 0.0f);

    for (int i = 0; i < totalSamples; ++i)
    {
        const auto phase = static_cast<float> (i) * 0.13f;
        left[static_cast<size_t> (i)] = std::sin (phase);
        right[static_cast<size_t> (i)] = std::cos (phase * 0.97f);
    }

    const auto inputRmsL = computeRms (left, settleSamples, totalSamples - settleSamples);
    const auto inputRmsR = computeRms (right, settleSamples, totalSamples - settleSamples);
    const auto inputPeakL = computePeak (left, settleSamples, totalSamples - settleSamples);
    const auto inputPeakR = computePeak (right, settleSamples, totalSamples - settleSamples);

    disperser.processBlock (left.data(), right.data(), totalSamples);

    const auto outputRmsL = computeRms (left, settleSamples, totalSamples - settleSamples);
    const auto outputRmsR = computeRms (right, settleSamples, totalSamples - settleSamples);
    const auto outputPeakL = computePeak (left, settleSamples, totalSamples - settleSamples);
    const auto outputPeakR = computePeak (right, settleSamples, totalSamples - settleSamples);

    constexpr float energyTolerance = 0.002f;

    REQUIRE ((outputRmsL / inputRmsL) == Catch::Approx (1.0f).margin (energyTolerance));
    REQUIRE ((outputRmsR / inputRmsR) == Catch::Approx (1.0f).margin (energyTolerance));
    REQUIRE ((outputPeakL / inputPeakL) == Catch::Approx (1.0f).margin (energyTolerance));
    REQUIRE ((outputPeakR / inputPeakR) == Catch::Approx (1.0f).margin (energyTolerance));
}

TEST_CASE ("ZdfLadderFilter Non-Linear Integration Stability", "[ZdfLadderFilter]")
{
    ZdfLadderFilter filter;
    filter.reset();

    constexpr int numFrames = 200'000;
    auto peak = 0.0f;

    for (int frame = 0; frame < numFrames; ++frame)
    {
        const auto sweepPhase = static_cast<float> (frame) / static_cast<float> (numFrames - 1);
        const auto cutoffHz = 80.0f + sweepPhase * 12000.0f;
        const auto resonance = 1.0f + sweepPhase * 3.0f;
        const auto drive = std::sin (static_cast<float> (frame) * 0.013f) * 2.5f;

        const auto output = filter.processSample (drive, cutoffHz, resonance, kSampleRate);

        REQUIRE (std::isfinite (output));
        REQUIRE (std::abs (output) < 8.0f);

        peak = std::max (peak, std::abs (output));
    }

    REQUIRE (peak > 0.01f);
}

TEST_CASE ("HydraParallelSaturator Boundary Verification", "[HydraParallelSaturator]")
{
    HydraParallelSaturator saturator;

    std::array<float, HydraParallelSaturator::numPartials> zeroLeft {};
    std::array<float, HydraParallelSaturator::numPartials> zeroRight {};

    const auto silent = saturator.processSample (zeroLeft, zeroRight);
    REQUIRE (silent.first == Catch::Approx (0.0f).margin (1.0e-6f));
    REQUIRE (silent.second == Catch::Approx (0.0f).margin (1.0e-6f));

    std::array<float, HydraParallelSaturator::numPartials> extremeLeft {};
    std::array<float, HydraParallelSaturator::numPartials> extremeRight {};
    extremeLeft[2] = 100.0f;
    extremeLeft[3] = -80.0f;
    extremeLeft[4] = 60.0f;
    extremeRight[2] = 100.0f;
    extremeRight[3] = -80.0f;
    extremeRight[4] = 60.0f;

    const auto midSumL = extremeLeft[2] + extremeLeft[3] + extremeLeft[4];
    const auto expectedMidL = std::tanh (midSumL * 1.4f);
    const auto driven = saturator.processSample (extremeLeft, extremeRight);

    REQUIRE (driven.first == Catch::Approx (expectedMidL).margin (1.0e-5f));
    REQUIRE (driven.second == Catch::Approx (expectedMidL).margin (1.0e-5f));
    REQUIRE (std::abs (driven.first) <= 1.0f);
    REQUIRE (std::abs (driven.second) <= 1.0f);

    std::array<float, HydraParallelSaturator::numPartials> highOnlyLeft {};
    std::array<float, HydraParallelSaturator::numPartials> highOnlyRight {};
    highOnlyLeft[5] = 2.0f;
    highOnlyLeft[6] = 2.0f;

    const auto clipped = saturator.processSample (highOnlyLeft, highOnlyRight);
    REQUIRE (clipped.first == Catch::Approx (0.7f).margin (1.0e-5f));
    REQUIRE (clipped.second == Catch::Approx (0.0f).margin (1.0e-6f));

    std::array<float, HydraParallelSaturator::numPartials> highOnlyNegativeLeft {};
    std::array<float, HydraParallelSaturator::numPartials> highOnlyNegativeRight {};
    highOnlyNegativeLeft[5] = -2.0f;
    highOnlyNegativeLeft[6] = -2.0f;

    const auto negativeClipped = saturator.processSample (highOnlyNegativeLeft, highOnlyNegativeRight);
    REQUIRE (negativeClipped.first == Catch::Approx (-0.58f).margin (1.0e-5f));
    REQUIRE (negativeClipped.second == Catch::Approx (0.0f).margin (1.0e-6f));

    SECTION ("Symmetry break proves even-harmonic MGTR asymmetry")
    {
        constexpr float kDrive = 0.5f;

        std::array<float, HydraParallelSaturator::numPartials> positiveDrive {};
        std::array<float, HydraParallelSaturator::numPartials> negativeDrive {};
        positiveDrive.fill (kDrive);
        negativeDrive.fill (-kDrive);

        const auto posOutput = saturator.processSample (positiveDrive, zeroRight);
        const auto negOutput = saturator.processSample (negativeDrive, zeroRight);

        REQUIRE (posOutput.first != -negOutput.first);
    }
}

TEST_CASE ("HydraMacroMapper Energy Conservation", "[HydraMacroMapper]")
{
    HydraMacroMapper mapper;

    SECTION ("RSS energy governor conserves unit power")
    {
        const std::array<std::pair<float, float>, 4> macroVariations {
            std::pair<float, float> { 0.0f, 0.0f },
            { 0.0f, 1.0f },
            { 0.5f, 0.5f },
            { 1.0f, 1.0f }
        };

        for (const auto [depth, girth] : macroVariations)
        {
            const auto packet = mapper.computeTargets (depth, girth, 0.0f);
            REQUIRE (sumSquaredAmplitudes (packet) == Catch::Approx (1.0f).margin (0.0001f));
        }
    }

    SECTION ("Depth zero yields logistic fundamental dominance")
    {
        const auto packet = mapper.computeTargets (0.0f, 0.5f, 0.0f);

        REQUIRE (packet.amplitudes[0] > packet.amplitudes[1]);
        REQUIRE (packet.amplitudes[0] > packet.amplitudes[6]);
        REQUIRE (packet.amplitudes[6] == Catch::Approx (0.0f).margin (1.0e-6f));

        for (int partialIndex = 1; partialIndex < HydraMacroMapper::numPartials; ++partialIndex)
            REQUIRE (packet.amplitudes[static_cast<size_t> (partialIndex)] == Catch::Approx (0.0f).margin (1.0e-6f));
    }

    SECTION ("Hyperbolic panning anchors fundamental and conserves equal power")
    {
        const std::array<std::pair<float, float>, 2> girthVariations {
            std::pair<float, float> { 1.0f, 0.0f },
            { 1.0f, 1.0f }
        };

        for (const auto [depth, girth] : girthVariations)
        {
            const auto packet = mapper.computeTargets (depth, girth, 0.0f);

            REQUIRE (packet.panningPairs[0].first == Catch::Approx (packet.panningPairs[0].second).margin (1.0e-5f));
            REQUIRE (packet.panningPairs[0].first == Catch::Approx (0.707f).margin (1.0e-5f));

            for (int partialIndex = 0; partialIndex < HydraMacroMapper::numPartials; ++partialIndex)
            {
                const auto& [panL, panR] = packet.panningPairs[static_cast<size_t> (partialIndex)];
                REQUIRE ((panL * panL + panR * panR) == Catch::Approx (1.0f).margin (1.0e-3f));
            }
        }
    }

    SECTION ("Full-scale girth hyperbolically fans overtones outward")
    {
        const auto packet = mapper.computeTargets (1.0f, 1.0f, 0.0f);

        const auto stereoSpread = [] (const std::pair<float, float>& pan)
        {
            return std::abs (pan.first - pan.second);
        };

        REQUIRE (packet.panningPairs[1].first > packet.panningPairs[1].second);
        REQUIRE (packet.panningPairs[2].first > packet.panningPairs[2].second);
        REQUIRE (packet.panningPairs[4].first < packet.panningPairs[4].second);
        REQUIRE (stereoSpread (packet.panningPairs[6]) > stereoSpread (packet.panningPairs[1]));
    }

    SECTION ("Depth gates girth-driven morph and detune to pure sines")
    {
        const auto silentDepth = mapper.computeTargets (0.0f, 1.0f, 0.0f);

        for (const auto morph : silentDepth.morphTargets)
            REQUIRE (morph == Catch::Approx (0.0f).margin (1.0e-6f));

        REQUIRE (silentDepth.frequencyMultipliers[2] == Catch::Approx (3.0f).margin (1.0e-6f));
        REQUIRE (silentDepth.frequencyMultipliers[4] == Catch::Approx (5.0f).margin (1.0e-6f));
        REQUIRE (silentDepth.frequencyMultipliers[6] == Catch::Approx (7.0f).margin (1.0e-6f));
    }

    SECTION ("Full-scale XY assigns tri-group morph roles")
    {
        const auto fullScale = mapper.computeTargets (1.0f, 1.0f, 0.0f);

        REQUIRE (fullScale.morphTargets[0] == Catch::Approx (0.0f).margin (0.001f));
        REQUIRE (fullScale.morphTargets[1] == Catch::Approx (2.0f).margin (0.001f));
        REQUIRE (fullScale.morphTargets[3] == Catch::Approx (2.0f).margin (0.001f));
        REQUIRE (fullScale.morphTargets[5] == Catch::Approx (2.0f).margin (0.001f));
        REQUIRE (fullScale.morphTargets[2] == Catch::Approx (3.0f).margin (0.001f));
        REQUIRE (fullScale.morphTargets[6] == Catch::Approx (3.0f).margin (0.001f));
    }

    SECTION ("Tri-group frequency multipliers use harmony scaffold and micro-cents detune")
    {
        const auto fullScale = mapper.computeTargets (1.0f, 1.0f, 0.0f);

        REQUIRE (fullScale.frequencyMultipliers[0] == Catch::Approx (1.0f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[1] == Catch::Approx (2.0f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[3] == Catch::Approx (4.0f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[5] == Catch::Approx (6.0f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[2] == Catch::Approx (3.015f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[4] == Catch::Approx (4.982f).margin (0.001f));
        REQUIRE (fullScale.frequencyMultipliers[6] == Catch::Approx (7.045f).margin (0.001f));
    }
}

TEST_CASE ("HydraEngine Temporal Staggering Verification", "[HydraEngine]")
{
    HydraPartialVoice fundamentalVoice;
    HydraPartialVoice highestPartialVoice;

    fundamentalVoice.configureDelay (kSampleRate, 0);
    highestPartialVoice.configureDelay (kSampleRate, 6);

    REQUIRE (fundamentalVoice.delayInSamples == 0);

    const auto expectedHighestDelay = static_cast<int> (0.0145f * static_cast<float> (kSampleRate));
    REQUIRE (highestPartialVoice.delayInSamples == expectedHighestDelay);
    REQUIRE (highestPartialVoice.delayInSamples > 0);

    REQUIRE (fundamentalVoice.processDelaySample (1.0f) == Catch::Approx (1.0f).margin (1.0e-6f));

    HydraPartialVoice delayLine;
    delayLine.configureDelay (kSampleRate, 0);
    delayLine.delayInSamples = 4;
    delayLine.clearDelay();

    delayLine.processDelaySample (1.0f);
    delayLine.processDelaySample (2.0f);
    delayLine.processDelaySample (3.0f);
    delayLine.processDelaySample (4.0f);
    REQUIRE (delayLine.processDelaySample (5.0f) == Catch::Approx (1.0f).margin (1.0e-6f));
}

TEST_CASE ("HydraEngine A-note rendering stays finite", "[HydraEngine]")
{
    auto engine = makePreparedEngine();
    engine.setDepth (1.0f);
    engine.setGirth (0.0f);
    engine.setScaleMorph (1.0f);

    const std::array<int, 4> aNotes { 45, 57, 69, 81 };

    for (const auto midiNote : aNotes)
    {
        engine.reset();
        engine.setDepth (1.0f);
        engine.noteOn (midiNote, 1.0f);

        std::vector<float> left (static_cast<size_t> (kSampleRate * 0.25), 0.0f);
        std::vector<float> right (static_cast<size_t> (kSampleRate * 0.25), 0.0f);
        engine.renderBlock (left.data(), right.data(), static_cast<int> (left.size()));

        REQUIRE_FALSE (bufferContainsNonFinite (left));
        REQUIRE_FALSE (bufferContainsNonFinite (right));
        REQUIRE (computePeak (left, 0, static_cast<int> (left.size())) < 2.0f);
        REQUIRE (computePeak (right, 0, static_cast<int> (right.size())) < 2.0f);
    }
}

TEST_CASE ("HydraEngine Voice Lifecycle", "[HydraEngine]")
{
    auto engine = makePreparedEngine();

    SECTION ("Silent before noteOn")
    {
        REQUIRE (renderPeak (engine, kBlockSize) == Catch::Approx (0.0f).margin (1.0e-6f));
    }

    SECTION ("noteOn with macro depth produces audible output")
    {
        engine.setDepth (1.0f);
        engine.setGirth (0.0f);
        engine.noteOn (69, 1.0f);

        REQUIRE (renderPeak (engine, static_cast<int> (kSampleRate * 0.05)) > 0.05f);
    }

    SECTION ("noteOff ramps output toward silence")
    {
        engine.reset();
        engine.setDepth (1.0f);
        engine.noteOn (69, 1.0f);
        renderPeak (engine, static_cast<int> (kSampleRate * 0.02));

        engine.noteOff (69);
        renderPeak (engine, static_cast<int> (kSampleRate * 1.0));

        REQUIRE (engine.getVoiceAmplitude() == Catch::Approx (0.0f).margin (1.0e-6f));
        REQUIRE (renderPeak (engine, static_cast<int> (kSampleRate * 0.02)) < 0.01f);
    }

    SECTION ("setGirth spreads stereo energy across partials")
    {
        engine.setDepth (1.0f);
        engine.setGirth (1.0f);
        engine.noteOn (69, 1.0f);

        std::vector<float> left (static_cast<size_t> (kSampleRate * 0.05), 0.0f);
        std::vector<float> right (static_cast<size_t> (kSampleRate * 0.05), 0.0f);
        engine.renderBlock (left.data(), right.data(), static_cast<int> (left.size()));

        auto leftEnergy = 0.0;
        auto rightEnergy = 0.0;
        for (size_t i = 0; i < left.size(); ++i)
        {
            leftEnergy += static_cast<double> (left[i]) * static_cast<double> (left[i]);
            rightEnergy += static_cast<double> (right[i]) * static_cast<double> (right[i]);
        }

        REQUIRE (leftEnergy > 0.0);
        REQUIRE (rightEnergy > 0.0);
    }

    SECTION ("reset silences output")
    {
        engine.setDepth (1.0f);
        engine.noteOn (69, 1.0f);
        engine.reset();
        REQUIRE (renderPeak (engine, kBlockSize) == Catch::Approx (0.0f).margin (1.0e-6f));
    }
}
