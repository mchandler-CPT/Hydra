#include "XyExplorer.h"

#include "PluginProcessor.h"

namespace
{
constexpr juce::uint32 kPanelFill = 0xff0d0c0b;
constexpr juce::uint32 kBorderColour = 0xff3a3530;
constexpr juce::uint32 kGridColour = 0x44c4a574;
constexpr juce::uint32 kLabelColour = 0xff8a847c;
constexpr juce::uint32 kThumbFill = 0xffc4a574;
constexpr juce::uint32 kThumbAura = 0x88c4a574;
constexpr int kScopeTraceSamples = 256;
} // namespace

XyExplorer::XyExplorer (HydraAudioProcessor& processor)
    : audioProcessor (processor)
{
    startTimerHz (60);
}

XyExplorer::~XyExplorer()
{
    stopTimer();
}

void XyExplorer::timerCallback()
{
    repaint();
}

void XyExplorer::setParameters (juce::RangedAudioParameter& depthParam, juce::RangedAudioParameter& girthParam)
{
    xAttachment = std::make_unique<juce::ParameterAttachment> (
        depthParam,
        [this] (float newValue)
        {
            normalizedX = newValue;
            repaint();
        },
        nullptr);

    yAttachment = std::make_unique<juce::ParameterAttachment> (
        girthParam,
        [this] (float newValue)
        {
            normalizedY = newValue;
            repaint();
        },
        nullptr);

    normalizedX = depthParam.getValue();
    normalizedY = girthParam.getValue();
    repaint();
}

void XyExplorer::resized()
{
    repaint();
}

void XyExplorer::updateParametersFromMouse (juce::Point<float> mousePos)
{
    const auto width = static_cast<float> (getWidth());
    const auto height = static_cast<float> (getHeight());

    if (width <= 0.0f || height <= 0.0f)
        return;

    normalizedX = juce::jlimit (0.0f, 1.0f, mousePos.x / width);
    normalizedY = juce::jlimit (0.0f, 1.0f, 1.0f - (mousePos.y / height));

    if (xAttachment != nullptr)
        xAttachment->setValueAsCompleteGesture (normalizedX);

    if (yAttachment != nullptr)
        yAttachment->setValueAsCompleteGesture (normalizedY);

    repaint();
}

void XyExplorer::mouseDown (const juce::MouseEvent& event)
{
    updateParametersFromMouse (event.position);
}

void XyExplorer::mouseDrag (const juce::MouseEvent& event)
{
    updateParametersFromMouse (event.position);
}

void XyExplorer::paint (juce::Graphics& g)
{
    const auto panelBounds = getLocalBounds().toFloat().reduced (1.0f);
    const auto thumbX = panelBounds.getX() + normalizedX * panelBounds.getWidth();
    const auto thumbY = panelBounds.getY() + (1.0f - normalizedY) * panelBounds.getHeight();

    g.setColour (juce::Colour (kPanelFill));
    g.fillRoundedRectangle (panelBounds, 6.0f);

    const auto latestWrite = audioProcessor.getLatestWriteIndex();
    const auto midY = panelBounds.getCentreY();
    const auto widthFactor = panelBounds.getWidth() / static_cast<float> (kScopeTraceSamples);

    juce::Path wavePath;

    for (int i = 0; i < kScopeTraceSamples; ++i)
    {
        const auto samplePair = audioProcessor.popVisualSample (latestWrite - kScopeTraceSamples + i);
        const auto mixedSample = (samplePair.first + samplePair.second) * 0.5f;

        const auto x = panelBounds.getX() + static_cast<float> (i) * widthFactor;
        const auto y = midY - (juce::jlimit (-1.0f, 1.0f, mixedSample) * midY * 0.6f);

        if (i == 0)
            wavePath.startNewSubPath (x, y);
        else
            wavePath.lineTo (x, y);
    }

    g.setColour (juce::Colours::gold.withAlpha (0.12f));
    g.strokePath (wavePath, juce::PathStrokeType (1.5f));

    g.setColour (juce::Colour (kBorderColour));
    g.drawRoundedRectangle (panelBounds, 6.0f, 1.0f);

    const auto centreX = panelBounds.getCentreX();
    const auto centreY = panelBounds.getCentreY();
    const float dashPattern[] { 4.0f, 4.0f };

    g.setColour (juce::Colour (kGridColour));

    g.drawDashedLine ({ panelBounds.getX(), centreY, panelBounds.getRight(), centreY },
                      dashPattern,
                      2,
                      1.0f);

    g.drawDashedLine ({ centreX, panelBounds.getY(), centreX, panelBounds.getBottom() },
                      dashPattern,
                      2,
                      1.0f);

    g.setColour (juce::Colour (kLabelColour));
    g.setFont (juce::Font (juce::FontOptions { 9.0f }));

    auto labelBounds = panelBounds;
    g.drawText ("HARMONIC BLOOM (X)",
                labelBounds.removeFromBottom (14.0f),
                juce::Justification::centred,
                true);

    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                                                     panelBounds.getX() + 8.0f,
                                                     panelBounds.getCentreY()));
    g.drawText ("SPATIAL SPREAD (Y)",
                juce::Rectangle<float> (panelBounds.getX() - 40.0f,
                                        panelBounds.getCentreY() - 50.0f,
                                        100.0f,
                                        100.0f),
                juce::Justification::centred,
                true);
    g.restoreState();

    g.setColour (juce::Colour (kThumbAura));
    g.drawEllipse (thumbX - 10.0f, thumbY - 10.0f, 20.0f, 20.0f, 1.5f);

    g.setColour (juce::Colour (kThumbFill));
    g.fillEllipse (thumbX - 4.0f, thumbY - 4.0f, 8.0f, 8.0f);
}
