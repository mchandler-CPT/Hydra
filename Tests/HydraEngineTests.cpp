#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/HydraEngine.h"
#include "DSP/HydraMacroMapper.h"
#include "DSP/HydraOscillator.h"

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
} // namespace

TEST_CASE ("HydraOscillator Waveshaping Integrity", "[HydraOscillator]")
{
    HydraOscillator oscillator;
    oscillator.setSampleRate (kSampleRate);
    oscillator.setFrequency (440.0);

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
