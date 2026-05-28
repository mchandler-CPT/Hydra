#include "SnapToggleButton.h"

SnapToggleButton::SnapToggleButton()
{
    setClickingTogglesState (true);
}

void SnapToggleButton::paintButton (juce::Graphics& g,
                                    bool shouldDrawButtonAsHighlighted,
                                    bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted);

    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    if (shouldDrawButtonAsDown)
        bounds = bounds.reduced (1.0f);

    const auto isOn = getToggleState();
    const auto fillColour = isOn ? HydraPalette::colour (HydraPalette::snapOnFill)
                                 : HydraPalette::colour (HydraPalette::snapOffFill);
    const auto borderColour = isOn ? HydraPalette::colour (HydraPalette::snapOnFill).brighter (0.12f)
                                   : HydraPalette::colour (HydraPalette::snapOffBorder);
    const auto textColour = isOn ? HydraPalette::colour (HydraPalette::snapOnText)
                                 : HydraPalette::colour (HydraPalette::snapOffText);

    g.setColour (fillColour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (textColour);
    g.setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::bold }));
    g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}
