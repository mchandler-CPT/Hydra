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
    const auto fillColour = isOn ? juce::Colour (snapOnFill) : juce::Colour (snapOffFill);
    const auto borderColour = isOn ? juce::Colour (snapOnFill).brighter (0.12f) : juce::Colour (snapOffBorder);
    const auto textColour = isOn ? juce::Colour (snapOnText) : juce::Colour (snapOffText);

    g.setColour (fillColour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (textColour);
    g.setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::bold }));
    g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}
