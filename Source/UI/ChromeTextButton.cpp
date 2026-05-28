#include "ChromeTextButton.h"

ChromeTextButton::ChromeTextButton() = default;

void ChromeTextButton::paintButton (juce::Graphics& g,
                                      bool shouldDrawButtonAsHighlighted,
                                      bool shouldDrawButtonAsDown)
{
    auto bounds = getLocalBounds().toFloat().reduced (0.5f);

    if (shouldDrawButtonAsDown)
        bounds = bounds.reduced (1.0f);

    auto fillColour = HydraPalette::colour (HydraPalette::snapOffFill);
    auto borderColour = HydraPalette::colour (HydraPalette::snapOffBorder);
    auto textColour = HydraPalette::colour (HydraPalette::snapOffText);

    if (shouldDrawButtonAsHighlighted)
        borderColour = HydraPalette::colour (HydraPalette::accentGoldDim);

    if (! isEnabled())
    {
        fillColour = fillColour.withAlpha (0.55f);
        borderColour = borderColour.withAlpha (0.45f);
        textColour = textColour.withAlpha (0.35f);
    }

    g.setColour (fillColour);
    g.fillRoundedRectangle (bounds, 4.0f);

    g.setColour (borderColour);
    g.drawRoundedRectangle (bounds, 4.0f, 1.0f);

    g.setColour (textColour);
    g.setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::bold }));
    g.drawText (getButtonText(), bounds.toNearestInt(), juce::Justification::centred, false);
}
