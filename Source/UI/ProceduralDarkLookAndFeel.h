#pragma once

#include "HydraPalette.h"

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
};
