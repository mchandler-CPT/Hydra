#include "ProceduralDarkLookAndFeel.h"

void ProceduralDarkLookAndFeel::drawRotarySlider (juce::Graphics& g,
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

    auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                          static_cast<float> (y),
                                          static_cast<float> (width),
                                          static_cast<float> (height));

    const auto size = juce::jmin (bounds.getWidth(), bounds.getHeight());
    bounds = juce::Rectangle<float> (size, size).withCentre (bounds.getCentre());

    const auto centre = bounds.getCentre();
    const auto radius = size * 0.5f;
    const auto grooveRadius = radius * 0.98f;
    const auto arcRadius = radius * 0.90f;
    const auto arcThickness = juce::jmax (1.8f, size * 0.06f);
    const auto bodyRadius = arcRadius - (arcThickness * 0.85f);

    const auto angle = rotaryStartAngle + (sliderPosProportional * (rotaryEndAngle - rotaryStartAngle));
    const auto arcStroke = juce::PathStrokeType (arcThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    // Recessed seat behind the knob body.
    g.setColour (HydraPalette::colour (HydraPalette::dialTrack).darker (0.35f));
    g.fillEllipse (centre.x - grooveRadius, centre.y - grooveRadius, grooveRadius * 2.0f, grooveRadius * 2.0f);

    juce::Path trackArc;
    trackArc.addCentredArc (centre.x,
                            centre.y,
                            arcRadius,
                            arcRadius,
                            0.0f,
                            rotaryStartAngle,
                            rotaryEndAngle,
                            true);
    g.setColour (HydraPalette::colour (HydraPalette::dialTrack));
    g.strokePath (trackArc, arcStroke);

    juce::Path progressArc;
    progressArc.addCentredArc (centre.x,
                               centre.y,
                               arcRadius,
                               arcRadius,
                               0.0f,
                               rotaryStartAngle,
                               angle,
                               true);
    g.setColour (HydraPalette::colour (HydraPalette::dialFill));
    g.strokePath (progressArc, arcStroke);

    juce::ColourGradient bodyGradient (HydraPalette::colour (HydraPalette::dialBodyTop),
                                       centre.x,
                                       centre.y - bodyRadius,
                                       HydraPalette::colour (HydraPalette::dialBodyBottom),
                                       centre.x,
                                       centre.y + bodyRadius,
                                       false);
    g.setGradientFill (bodyGradient);
    g.fillEllipse (centre.x - bodyRadius, centre.y - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

    g.setColour (HydraPalette::colour (HydraPalette::dialTrack).brighter (0.22f));
    g.drawEllipse (centre.x - bodyRadius + 1.0f,
                   centre.y - bodyRadius + 1.0f,
                   (bodyRadius * 2.0f) - 2.0f,
                   (bodyRadius * 2.0f) - 2.0f,
                   1.0f);

    const auto pointerLength = bodyRadius * 0.68f;
    const auto pointerThickness = juce::jmax (1.3f, size * 0.028f);
    juce::Path pointer;
    pointer.addRoundedRectangle (-pointerThickness * 0.5f,
                                 -pointerLength,
                                 pointerThickness,
                                 pointerLength,
                                 pointerThickness * 0.5f);

    g.setColour (HydraPalette::colour (HydraPalette::dialPointer));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

juce::Label* ProceduralDarkLookAndFeel::createSliderTextBox (juce::Slider& slider)
{
    auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
    label->setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::plain }));
    return label;
}
