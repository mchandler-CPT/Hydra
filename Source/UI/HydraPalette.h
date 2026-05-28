#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace HydraPalette
{
constexpr juce::uint32 backgroundTop = 0xff0a0908;
constexpr juce::uint32 backgroundBottom = 0xff100e0c;
constexpr juce::uint32 backgroundBase = 0xff0c0a09;
constexpr juce::uint32 backgroundWarmTint = 0xff1a1410;
constexpr juce::uint32 panelRaised = 0xff12100e;
constexpr juce::uint32 panelRaisedEdge = 0xff1a1612;

constexpr juce::uint32 borderMuted = 0xff322e2a;
constexpr juce::uint32 accentGold = 0xffc4a574;
constexpr juce::uint32 accentGoldDim = 0xff9a7d52;
constexpr juce::uint32 accentGoldBright = 0xffdcc498;
constexpr juce::uint32 accentDepthWarm = 0xffe0c090;
constexpr juce::uint32 accentGirthWarm = 0xffd4a868;

constexpr juce::uint32 textMuted = 0xff9a948c;
constexpr juce::uint32 textFooter = 0xff6a6560;
constexpr juce::uint32 textLabel = 0xff8a847c;

constexpr juce::uint32 dialTrack = 0xff2a2622;
constexpr juce::uint32 dialFill = accentGold;
constexpr juce::uint32 dialPointer = 0xfff0e6d2;
constexpr juce::uint32 dialBodyTop = 0xff3a342e;
constexpr juce::uint32 dialBodyBottom = 0xff161412;

constexpr juce::uint32 xyPanelTop = 0xff12100e;
constexpr juce::uint32 xyPanelBottom = 0xff080706;

constexpr juce::uint32 snapOnFill = accentGold;
constexpr juce::uint32 snapOffFill = 0xff1a1816;
constexpr juce::uint32 snapOffBorder = borderMuted;
constexpr juce::uint32 snapOnText = backgroundBase;
constexpr juce::uint32 snapOffText = textMuted;

constexpr juce::uint32 keyboardBackground = 0xff0c0a09;
constexpr juce::uint32 keyboardWhite = 0xff2e2a26;
constexpr juce::uint32 keyboardBlack = 0xff12100e;
constexpr juce::uint32 keyboardSeparator = borderMuted;

inline juce::Colour colour (juce::uint32 argb) noexcept
{
    return juce::Colour (argb);
}

inline juce::Colour macroAccent (float depthNorm, float girthNorm) noexcept
{
    const auto blend = juce::jlimit (0.0f, 1.0f, (depthNorm + girthNorm) * 0.5f);
    return colour (accentGold).interpolatedWith (colour (accentDepthWarm), blend * 0.45f);
}
} // namespace HydraPalette
