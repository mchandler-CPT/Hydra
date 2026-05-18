#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/HydraEngine.h"
#include "DSP/HydraMacroMapper.h"
#include "DSP/HydraOscillator.h"
#include "DSP/HydraParallelSaturator.h"
#include "DSP/HydraPhaseDisperser.h"

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
} // namespace

TEST_CASE ("HydraOscillator Waveshaping Integrity", "[HydraOscillator]")
{
    HydraOscillator oscillator;
    oscillator.prepare (kSampleRate);
    oscillator.setFrequency (440.0, false);

    const auto theta = juce::MathConstants<double>::halfPi;
    oscillator.setPhase (theta);

    const auto sine = std::sin (static_cast<float> (theta));
    const auto triangle = (2.0f / juce::MathConstants<float>::pi)
                        * std::asin (std::sin (static_cast<float> (theta)));

    REQUIRE (oscillator.evaluateSample (0.0f) == Catch::Approx (sine).margin (1.0e-5f));
    REQUIRE (oscillator.evaluateSample (1.0f) == Catch::Approx (triangle).margin (1.0e-5f));

    const auto sineTriangleBlend = oscillator.evaluateSample (0.5f);
    REQUIRE (sineTriangleBlend == Catch::Approx (0.5f * (sine + triangle)).margin (1.0e-5f));

    const auto square = 1.0f;
    const auto saw = -0.5f;

    REQUIRE (oscillator.evaluateSample (2.0f) == Catch::Approx (square).margin (1.0e-5f));
    REQUIRE (oscillator.evaluateSample (3.0f) == Catch::Approx (saw).margin (1.0e-5f));

    const auto triangleSquareBlend = oscillator.evaluateSample (1.5f);
    REQUIRE (triangleSquareBlend == Catch::Approx (0.5f * (triangle + square)).margin (1.0e-5f));

    const auto squareSawBlend = oscillator.evaluateSample (2.5f);
    REQUIRE (squareSawBlend == Catch::Approx (0.5f * (square + saw)).margin (1.0e-5f));

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
}

TEST_CASE ("HydraMacroMapper Energy Conservation", "[HydraMacroMapper]")
{
    HydraMacroMapper mapper;
    const std::array<std::pair<float, float>, 3> macroVariations {
        std::pair<float, float> { 0.0f, 0.0f },
        { 0.5f, 0.5f },
        { 1.0f, 1.0f }
    };

    for (const auto [depth, girth] : macroVariations)
    {
        const auto packet = mapper.computeTargets (depth, girth);
        REQUIRE (sumSquaredAmplitudes (packet) == Catch::Approx (1.0f).margin (0.0001f));

        for (const auto& [panL, panR] : packet.panningPairs)
            REQUIRE ((panL * panL + panR * panR) == Catch::Approx (1.0f).margin (1.0e-5f));
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
        engine.setMorph (0.0f);
        engine.noteOn (69, 1.0f);

        REQUIRE (renderPeak (engine, static_cast<int> (kSampleRate * 0.05)) > 0.05f);
    }

    SECTION ("noteOff ramps output toward silence")
    {
        engine.reset();
        engine.setDepth (1.0f);
        engine.setMorph (0.0f);
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
        engine.setMorph (0.0f);
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
