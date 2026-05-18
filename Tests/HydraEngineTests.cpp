#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "DSP/HydraEngine.h"

#include <array>
#include <cmath>
#include <vector>

namespace
{
constexpr double kSampleRate = 44100.0;
constexpr int kBlockSize = 512;
constexpr int kPrimeNumbers[HydraEngine::numPartials] { 2, 3, 5, 7, 11, 13, 17 };

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

float renderChannelEnergy (HydraEngine& engine, int numSamples, bool leftChannel)
{
    std::vector<float> left (static_cast<size_t> (numSamples), 0.0f);
    std::vector<float> right (static_cast<size_t> (numSamples), 0.0f);
    engine.renderBlock (left.data(), right.data(), numSamples);

    const auto* channel = leftChannel ? left.data() : right.data();
    double sumSquares = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto sample = static_cast<double> (channel[i]);
        sumSquares += sample * sample;
    }

    return static_cast<float> (std::sqrt (sumSquares / static_cast<double> (numSamples)));
}
} // namespace

TEST_CASE ("HydraEngine exposes seven harmonic partials", "[HydraEngine]")
{
    REQUIRE (HydraEngine::numPartials == 7);
}

TEST_CASE ("HydraEngine maps MIDI note 69 to 440 Hz", "[HydraEngine][pitch]")
{
    REQUIRE (HydraEngineTestAccess::midiNoteToHz (69) == Catch::Approx (440.0).margin (0.001));
    REQUIRE (HydraEngineTestAccess::midiNoteToHz (60) == Catch::Approx (261.626).margin (0.001));
}

TEST_CASE ("HydraEngine noteOn assigns prime phase offsets and harmonic increments", "[HydraEngine][noteOn]")
{
    auto engine = makePreparedEngine();
    constexpr int midiNote = 69;
    engine.noteOn (midiNote, 1.0f);

    const auto fundamentalHz = HydraEngineTestAccess::midiNoteToHz (midiNote);
    const auto twoPi = juce::MathConstants<double>::twoPi;

    for (int partialIndex = 0; partialIndex < HydraEngine::numPartials; ++partialIndex)
    {
        const auto& head = HydraEngineTestAccess::getHead (engine, partialIndex);
        const auto harmonic = static_cast<double> (partialIndex + 1);
        const auto expectedIncrement = twoPi * fundamentalHz * harmonic / kSampleRate;
        const auto expectedPhase = twoPi / static_cast<double> (kPrimeNumbers[partialIndex]);

        REQUIRE (head.phaseIncrement == Catch::Approx (expectedIncrement).margin (1.0e-9));
        REQUIRE (head.phase == Catch::Approx (expectedPhase).margin (1.0e-9));
    }
}

TEST_CASE ("HydraEngine morph endpoints match analytic waveforms", "[HydraEngine][morph]")
{
    const auto theta = juce::MathConstants<double>::halfPi;
    const auto sine = std::sin (static_cast<float> (theta));
    const auto triangle = (2.0f / juce::MathConstants<float>::pi)
                        * std::asin (std::sin (static_cast<float> (theta)));
    const auto saw = -0.5f;

    REQUIRE (HydraEngineTestAccess::evaluatePartialAt (theta, 0.0f) == Catch::Approx (sine).margin (1.0e-5f));
    REQUIRE (HydraEngineTestAccess::evaluatePartialAt (theta, 1.0f) == Catch::Approx (triangle).margin (1.0e-5f));
    REQUIRE (HydraEngineTestAccess::evaluatePartialAt (theta, 2.0f) == Catch::Approx (saw).margin (1.0e-5f));

    const auto crushed = HydraEngineTestAccess::evaluatePartialAt (theta, 3.0f);
    REQUIRE (crushed == Catch::Approx (std::floor (saw * 3.0f) / 3.0f).margin (1.0e-5f));
    REQUIRE (crushed != Catch::Approx (saw).margin (1.0e-5f));
}

TEST_CASE ("HydraEngine morph segments interpolate between adjacent shapes", "[HydraEngine][morph]")
{
    const auto theta = juce::MathConstants<double>::halfPi;
    const auto sine = HydraEngineTestAccess::evaluatePartialAt (theta, 0.0f);
    const auto triangle = HydraEngineTestAccess::evaluatePartialAt (theta, 1.0f);
    const auto saw = HydraEngineTestAccess::evaluatePartialAt (theta, 2.0f);

    const auto sineTriangleBlend = HydraEngineTestAccess::evaluatePartialAt (theta, 0.5f);
    const auto triangleSawBlend = HydraEngineTestAccess::evaluatePartialAt (theta, 1.5f);

    REQUIRE (sineTriangleBlend == Catch::Approx (0.5f * (sine + triangle)).margin (1.0e-5f));
    REQUIRE (triangleSawBlend == Catch::Approx (0.5f * (triangle + saw)).margin (1.0e-5f));
}

TEST_CASE ("HydraEngine clamps morph values to 0..3", "[HydraEngine][morph]")
{
    const auto theta = 0.75;
    const auto atMin = HydraEngineTestAccess::evaluatePartialAt (theta, 0.0f);
    const auto belowMin = HydraEngineTestAccess::evaluatePartialAt (theta, -4.0f);
    const auto atMax = HydraEngineTestAccess::evaluatePartialAt (theta, 3.0f);
    const auto aboveMax = HydraEngineTestAccess::evaluatePartialAt (theta, 9.0f);

    REQUIRE (belowMin == Catch::Approx (atMin).margin (1.0e-6f));
    REQUIRE (aboveMax == Catch::Approx (atMax).margin (1.0e-6f));
}

TEST_CASE ("HydraEngine is silent before noteOn", "[HydraEngine][render]")
{
    auto engine = makePreparedEngine();
    const auto peak = renderPeak (engine, kBlockSize);
    REQUIRE (peak == Catch::Approx (0.0f).margin (1.0e-6f));
}

TEST_CASE ("HydraEngine renders audible output after noteOn", "[HydraEngine][render]")
{
    auto engine = makePreparedEngine();
    engine.noteOn (69, 1.0f);
    engine.setMorph (0.0f);

    const auto peak = renderPeak (engine, static_cast<int> (kSampleRate * 0.05));
    REQUIRE (peak > 0.1f);
}

TEST_CASE ("HydraEngine noteOff ramps output toward silence", "[HydraEngine][render]")
{
    auto engine = makePreparedEngine();
    engine.noteOn (69, 1.0f);
    engine.setMorph (0.0f);

    renderPeak (engine, static_cast<int> (kSampleRate * 0.02));
    engine.noteOff (69);

    // Burn through the amplitude ramp before measuring the sustained tail.
    renderPeak (engine, static_cast<int> (kSampleRate * 0.05));
    const auto peakAfterRelease = renderPeak (engine, static_cast<int> (kSampleRate * 0.02));
    REQUIRE (peakAfterRelease < 0.01f);
}

TEST_CASE ("HydraEngine setPan routes energy to the requested stereo side", "[HydraEngine][pan]")
{
    auto engine = makePreparedEngine();
    engine.noteOn (69, 1.0f);
    engine.setMorph (0.0f);

    constexpr int renderLength = static_cast<int> (kSampleRate * 0.05);

    engine.setPan (-1.0f);
    const auto hardLeft = renderChannelEnergy (engine, renderLength, true);
    const auto hardLeftOpposite = renderChannelEnergy (engine, renderLength, false);

    engine.noteOn (69, 1.0f);
    engine.setPan (1.0f);
    const auto hardRight = renderChannelEnergy (engine, renderLength, false);
    const auto hardRightOpposite = renderChannelEnergy (engine, renderLength, true);

    REQUIRE (hardLeft > hardLeftOpposite * 4.0f);
    REQUIRE (hardRight > hardRightOpposite * 4.0f);
}

TEST_CASE ("HydraEngine reset silences all partials", "[HydraEngine][reset]")
{
    auto engine = makePreparedEngine();
    engine.noteOn (69, 1.0f);
    engine.reset();

    const auto peak = renderPeak (engine, kBlockSize);
    REQUIRE (peak == Catch::Approx (0.0f).margin (1.0e-6f));
}
