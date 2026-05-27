#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SnapToggleButton : public juce::ToggleButton
{
public:
    SnapToggleButton();

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;

private:
    static constexpr juce::uint32 snapOnFill = 0xffc4a574;
    static constexpr juce::uint32 snapOffFill = 0xff1a1816;
    static constexpr juce::uint32 snapOffBorder = 0xff3a3530;
    static constexpr juce::uint32 snapOnText = 0xff161412;
    static constexpr juce::uint32 snapOffText = 0xff9a948c;
};
