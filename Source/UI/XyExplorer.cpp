#include "XyExplorer.h"

#include "PluginProcessor.h"

#include <array>
#include <cmath>

namespace
{
constexpr juce::uint32 kPanelFillTop = 0xff141210;
constexpr juce::uint32 kPanelFillBottom = 0xff090807;
constexpr juce::uint32 kBorderColour = 0xff3a3530;
constexpr juce::uint32 kGridColour = 0x44c4a574;
constexpr juce::uint32 kLabelColour = 0xff8a847c;
constexpr juce::uint32 kThumbFill = 0xffc4a574;
constexpr juce::uint32 kThumbAura = 0x88c4a574;
constexpr int kScopeTraceSamples = 256;
constexpr float kCornerRadius = 6.0f;
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

    juce::ColourGradient panelGradient (juce::Colour (kPanelFillTop),
                                        panelBounds.getTopLeft(),
                                        juce::Colour (kPanelFillBottom),
                                        panelBounds.getBottomLeft(),
                                        false);
    g.setGradientFill (panelGradient);
    g.fillRoundedRectangle (panelBounds, kCornerRadius);

    const auto latestWrite = audioProcessor.getLatestWriteIndex();
    const auto midY = panelBounds.getCentreY();
    const auto widthFactor = panelBounds.getWidth() / static_cast<float> (kScopeTraceSamples);
    const auto waveHalfHeight = panelBounds.getHeight() * 0.42f;

    juce::Path wavePath;
    float peak = 0.0f;
    float firstX = panelBounds.getX();
    float lastX = firstX;

    for (int i = 0; i < kScopeTraceSamples; ++i)
    {
        const auto samplePair = audioProcessor.popVisualSample (latestWrite - kScopeTraceSamples + i);
        const auto mixedSample = (samplePair.first + samplePair.second) * 0.5f;
        peak = juce::jmax (peak, std::abs (mixedSample));

        const auto x = panelBounds.getX() + static_cast<float> (i) * widthFactor;
        lastX = x;
        const auto y = midY - (juce::jlimit (-1.0f, 1.0f, mixedSample) * waveHalfHeight);

        if (i == 0)
            wavePath.startNewSubPath (x, y);
        else
            wavePath.lineTo (x, y);
    }

    waveformPeakSmoothed = waveformPeakSmoothed * 0.82f + peak * 0.18f;
    const auto peakNorm = juce::jlimit (0.0f, 1.0f, waveformPeakSmoothed * 2.2f);
    const auto accent = juce::Colour (kThumbFill);

    juce::Path waveFill = wavePath;
    waveFill.lineTo (lastX, midY);
    waveFill.lineTo (firstX, midY);
    waveFill.closeSubPath();

    juce::ColourGradient fillGradient (accent.withAlpha (0.06f + peakNorm * 0.28f),
                                       juce::Point<float> (panelBounds.getX(), midY - waveHalfHeight),
                                       accent.withAlpha (0.0f),
                                       juce::Point<float> (panelBounds.getX(), midY),
                                       false);
    g.setGradientFill (fillGradient);
    g.fillPath (waveFill);

    const auto glowAlpha = 0.14f + peakNorm * 0.42f;
    g.setColour (accent.withAlpha (glowAlpha));
    g.strokePath (wavePath,
                  juce::PathStrokeType (3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const auto coreAlpha = 0.32f + peakNorm * 0.58f;
    g.setColour (accent.withAlpha (coreAlpha));
    g.strokePath (wavePath,
                  juce::PathStrokeType (1.35f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

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

    g.setColour (juce::Colour (kBorderColour));
    g.drawRoundedRectangle (panelBounds, kCornerRadius, 1.0f);

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

    const auto thumbGlowRadius = 14.0f + peakNorm * 6.0f;
    g.setColour (accent.withAlpha (0.10f + peakNorm * 0.18f));
    g.fillEllipse (thumbX - thumbGlowRadius,
                   thumbY - thumbGlowRadius,
                   thumbGlowRadius * 2.0f,
                   thumbGlowRadius * 2.0f);

    g.setColour (juce::Colour (kThumbAura));
    g.drawEllipse (thumbX - 10.0f, thumbY - 10.0f, 20.0f, 20.0f, 1.5f);

    g.setColour (juce::Colour (kThumbFill));
    g.fillEllipse (thumbX - 4.0f, thumbY - 4.0f, 8.0f, 8.0f);
}
