#include "BoutiqueLookAndFeel.h"

BoutiqueLookAndFeel::BoutiqueLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, juce::Colour (backgroundCharcoal));
    setColour (juce::Label::textColourId, juce::Colour (labelTextColour));
    setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (dialFillColour));
    setColour (juce::Slider::rotarySliderOutlineColourId, juce::Colour (dialTrackColour));
    setColour (juce::Slider::thumbColourId, juce::Colour (dialPointerColour));
}

void BoutiqueLookAndFeel::drawRotarySlider (juce::Graphics& g,
                                            int x,
                                            int y,
                                            int width,
                                            int height,
                                            float sliderPosProportional,
                                            float rotaryStartAngle,
                                            float rotaryEndAngle,
                                            juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height))
                            .reduced (8.0f);

    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centreX = bounds.getCentreX();
    const auto centreY = bounds.getCentreY();
    const auto arcRadius = radius - 4.0f;
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (juce::Colour (dialTrackColour));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    juce::Path arc;
    arc.addCentredArc (centreX,
                       centreY,
                       arcRadius,
                       arcRadius,
                       0.0f,
                       rotaryStartAngle,
                       angle,
                       true);
    g.setColour (juce::Colour (dialFillColour));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path pointer;
    const auto pointerLength = radius * 0.55f;
    const auto pointerThickness = 2.5f;
    pointer.addRectangle (-pointerThickness * 0.5f, -radius + 6.0f, pointerThickness, pointerLength);
    g.setColour (juce::Colour (dialPointerColour));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centreX, centreY));
}

juce::Font BoutiqueLookAndFeel::getLabelFont (juce::Label& label)
{
    juce::ignoreUnused (label);
    return juce::Font (juce::FontOptions { 11.0f, juce::Font::bold });
}
