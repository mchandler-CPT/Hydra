#pragma once

#include "HydraPalette.h"

#include <juce_gui_basics/juce_gui_basics.h>

class SnapToggleButton : public juce::ToggleButton
{
public:
    SnapToggleButton();

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};
