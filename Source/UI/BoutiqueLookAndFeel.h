#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class BoutiqueLookAndFeel : public juce::LookAndFeel_V4
{
public:
    BoutiqueLookAndFeel();

    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    juce::Font getLabelFont (juce::Label& label) override;

private:
    static constexpr juce::uint32 backgroundCharcoal = 0xff161412;
    static constexpr juce::uint32 dialTrackColour = 0xff2a2622;
    static constexpr juce::uint32 dialFillColour = 0xffc4a574;
    static constexpr juce::uint32 dialPointerColour = 0xfff0e6d2;
    static constexpr juce::uint32 labelTextColour = 0xff9a948c;
};
