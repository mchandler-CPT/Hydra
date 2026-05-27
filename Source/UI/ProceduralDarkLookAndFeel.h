#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ProceduralDarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g,
                           int x,
                           int y,
                           int width,
                           int height,
                           float sliderPosProportional,
                           float rotaryStartAngle,
                           float rotaryEndAngle,
                           juce::Slider& slider) override;

    juce::Label* createSliderTextBox (juce::Slider& slider) override;

private:
    // Shared with XyExplorer (kThumbFill) and BoutiqueLookAndFeel dial tokens.
    static constexpr juce::uint32 dialTrackColour = 0xff2a2622;
    static constexpr juce::uint32 dialFillColour = 0xffc4a574;
    static constexpr juce::uint32 dialPointerColour = 0xfff0e6d2;
};
