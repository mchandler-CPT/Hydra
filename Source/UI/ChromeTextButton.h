#pragma once

#include "HydraPalette.h"

#include <juce_gui_basics/juce_gui_basics.h>

class ChromeTextButton : public juce::TextButton
{
public:
    ChromeTextButton();

    void paintButton (juce::Graphics& g,
                      bool shouldDrawButtonAsHighlighted,
                      bool shouldDrawButtonAsDown) override;
};
