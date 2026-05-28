#include "HydraPanelBackground.h"

#include <array>

namespace
{
constexpr float kZoneCornerRadius = 8.0f;
constexpr int kGrainTileSize = 128;

void drawZoneCard (juce::Graphics& g,
                   juce::Rectangle<float> bounds,
                   juce::Colour accent,
                   juce::Colour fill)
{
    auto card = bounds.reduced (2.0f);

    juce::ColourGradient cardFill (HydraPalette::colour (HydraPalette::panelRaisedEdge),
                                   card.getTopLeft(),
                                   fill.darker (0.08f),
                                   card.getBottomRight(),
                                   false);
    g.setGradientFill (cardFill);
    g.fillRoundedRectangle (card, kZoneCornerRadius);

    auto accentStrip = card.removeFromLeft (4.0f).reduced (0.0f, 6.0f);
    juce::ColourGradient stripGradient (accent.brighter (0.25f),
                                        accentStrip.getTopLeft(),
                                        accent.darker (0.15f),
                                        accentStrip.getBottomLeft(),
                                        false);
    g.setGradientFill (stripGradient);
    g.fillRoundedRectangle (accentStrip, 2.0f);

    g.setColour (HydraPalette::colour (HydraPalette::borderMuted).withAlpha (0.85f));
    g.drawRoundedRectangle (card, kZoneCornerRadius, 1.25f);

    g.setColour (accent.withAlpha (0.4f));
    g.drawRoundedRectangle (card.expanded (0.5f), kZoneCornerRadius + 0.5f, 0.75f);
}
} // namespace

void HydraPanelBackground::ensureCachesBuilt (int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    if (width == cachedWidth && height == cachedHeight && watermarkImage.isValid() && grainTile.isValid())
        return;

    cachedWidth = width;
    cachedHeight = height;
    rebuildWatermark (width, height);
    rebuildGrain();
}

void HydraPanelBackground::rebuildWatermark (int width, int height)
{
    watermarkImage = juce::Image (juce::Image::ARGB, width, height, true);
    juce::Graphics g (watermarkImage);
    g.fillAll (juce::Colours::transparentBlack);

    const auto panel = watermarkImage.getBounds().toFloat();
    const auto gold = HydraPalette::colour (HydraPalette::accentGold);

    constexpr std::array<float, 8> kHarmonicRatios { 1.0f, 0.5f, 0.333f, 0.25f, 0.2f, 0.166f, 0.142f, 0.125f };

    for (size_t i = 0; i < kHarmonicRatios.size(); ++i)
    {
        const auto x = panel.getX() + panel.getWidth() * kHarmonicRatios[i];
        const auto alpha = 0.04f + (0.07f * (1.0f - static_cast<float> (i) / static_cast<float> (kHarmonicRatios.size())));
        g.setColour (gold.withAlpha (alpha));
        g.drawVerticalLine (juce::roundToInt (x), panel.getY(), panel.getBottom());
    }

    const auto centre = panel.getCentre();
    const auto maxRadius = juce::jmin (panel.getWidth(), panel.getHeight()) * 0.48f;

    for (int harmonic = 1; harmonic <= 7; ++harmonic)
    {
        const auto radius = maxRadius * (static_cast<float> (harmonic) / 7.0f);
        g.setColour (gold.withAlpha (0.035f + 0.01f * static_cast<float> (8 - harmonic)));
        g.drawEllipse (centre.x - radius,
                       centre.y - radius * 0.72f,
                       radius * 2.0f,
                       radius * 1.44f,
                       1.0f);
    }

    juce::Path scaffold;
    for (int i = 0; i < 12; ++i)
    {
        const auto angle = (static_cast<float> (i) / 12.0f) * juce::MathConstants<float>::twoPi;
        const auto len = maxRadius * 0.92f;
        scaffold.startNewSubPath (centre.x, centre.y);
        scaffold.lineTo (centre.x + std::cos (angle) * len, centre.y + std::sin (angle) * len * 0.55f);
    }

    g.setColour (gold.withAlpha (0.05f));
    g.strokePath (scaffold, juce::PathStrokeType (0.85f));
}

void HydraPanelBackground::rebuildGrain()
{
    grainTile = juce::Image (juce::Image::ARGB, kGrainTileSize, kGrainTileSize, true);
    juce::Graphics g (grainTile);

    juce::Random rng (0x48594452);

    for (int y = 0; y < kGrainTileSize; ++y)
    {
        for (int x = 0; x < kGrainTileSize; ++x)
        {
            const auto n = rng.nextFloat();
            const auto alpha = juce::jlimit (0.02f, 0.09f, n * 0.11f);
            grainTile.setPixelAt (x, y, juce::Colours::white.withAlpha (alpha));
        }
    }
}

void HydraPanelBackground::paint (juce::Graphics& g,
                                  const HydraPanelBackgroundLayout& layout,
                                  float depthNorm,
                                  float girthNorm,
                                  float audioPeak) const
{
    const auto panel = layout.controlPanel.toFloat();
    if (panel.isEmpty())
        return;

    depthNorm = juce::jlimit (0.0f, 1.0f, depthNorm);
    girthNorm = juce::jlimit (0.0f, 1.0f, girthNorm);
    audioPeak = juce::jlimit (0.0f, 1.0f, audioPeak);

    const auto macroColour = HydraPalette::macroAccent (depthNorm, girthNorm);
    const auto gold = HydraPalette::colour (HydraPalette::accentGold);

    juce::ColourGradient baseGradient (HydraPalette::colour (HydraPalette::backgroundTop),
                                       panel.getTopLeft(),
                                       HydraPalette::colour (HydraPalette::backgroundBottom),
                                       panel.getBottomLeft(),
                                       false);
    g.setGradientFill (baseGradient);
    g.fillRect (panel);

    juce::ColourGradient warmWash (gold.withAlpha (0.07f + depthNorm * 0.05f),
                                   panel.getBottomLeft(),
                                   juce::Colours::transparentBlack,
                                   panel.getCentre(),
                                   false);
    g.setGradientFill (warmWash);
    g.fillRect (panel);

    if (watermarkImage.isValid())
        g.drawImageAt (watermarkImage, layout.controlPanel.getX(), layout.controlPanel.getY());

    if (grainTile.isValid())
    {
        g.setOpacity (0.38f);
        for (int y = layout.controlPanel.getY(); y < layout.controlPanel.getBottom(); y += kGrainTileSize)
        {
            for (int x = layout.controlPanel.getX(); x < layout.controlPanel.getRight(); x += kGrainTileSize)
                g.drawImageAt (grainTile, x, y);
        }

        g.setOpacity (1.0f);
    }

    const auto xy = layout.xyPadBounds.toFloat();
    if (! xy.isEmpty())
    {
        const auto xyCentre = xy.getCentre();
        const auto bloomRadius = juce::jmax (xy.getWidth(), xy.getHeight()) * (0.65f + girthNorm * 0.2f);
        const auto bloomAlpha = 0.06f + audioPeak * 0.22f;

        juce::ColourGradient bloom (gold.withAlpha (bloomAlpha),
                                    xyCentre.x,
                                    xyCentre.y,
                                    juce::Colours::transparentBlack,
                                    xyCentre.x,
                                    xyCentre.y + bloomRadius,
                                    true);
        g.setGradientFill (bloom);
        g.fillEllipse (xyCentre.x - bloomRadius,
                       xyCentre.y - bloomRadius * 0.85f,
                       bloomRadius * 2.0f,
                       bloomRadius * 1.7f);

        const auto spotlightRadius = bloomRadius * 0.55f;
        juce::ColourGradient spotlight (macroColour.withAlpha (0.10f + depthNorm * 0.08f),
                                        xyCentre.x,
                                        xyCentre.y,
                                        juce::Colours::transparentBlack,
                                        xyCentre.x,
                                        xyCentre.y + spotlightRadius,
                                        true);
        g.setGradientFill (spotlight);
        g.fillEllipse (xyCentre.x - spotlightRadius,
                       xyCentre.y - spotlightRadius,
                       spotlightRadius * 2.0f,
                       spotlightRadius * 2.0f);
    }

    const auto raisedFill = HydraPalette::colour (HydraPalette::panelRaised).withAlpha (0.88f);
    const auto border = HydraPalette::colour (HydraPalette::borderMuted);
    const auto zoneAccent = HydraPalette::colour (HydraPalette::accentGold);

    drawZoneCard (g, layout.harmonicZone.toFloat(), zoneAccent, raisedFill);
    drawZoneCard (g,
                  layout.modulationZone.toFloat(),
                  HydraPalette::colour (HydraPalette::accentGoldDim),
                  raisedFill);
    drawZoneCard (g,
                  layout.envelopeZone.toFloat(),
                  HydraPalette::colour (HydraPalette::accentGoldBright),
                  raisedFill);

    if (! layout.keyboardStrip.isEmpty())
    {
        const auto keyStrip = layout.keyboardStrip.toFloat();
        juce::ColourGradient keyGradient (HydraPalette::colour (HydraPalette::keyboardBackground).brighter (0.06f),
                                          keyStrip.getTopLeft(),
                                          HydraPalette::colour (HydraPalette::keyboardBackground),
                                          keyStrip.getBottomLeft(),
                                          false);
        g.setGradientFill (keyGradient);
        g.fillRect (keyStrip);

        g.setColour (gold.withAlpha (0.25f));
        g.fillRect (keyStrip.getX(), keyStrip.getY(), keyStrip.getWidth(), 2.0f);
    }

    g.setColour (border.withAlpha (0.5f));
    g.drawHorizontalLine (layout.controlPanel.getBottom(), static_cast<float> (layout.controlPanel.getX()),
                          static_cast<float> (layout.controlPanel.getRight()));

    if (! layout.footerBounds.isEmpty())
    {
        g.setColour (HydraPalette::colour (HydraPalette::textFooter));
        g.setFont (juce::Font (juce::FontOptions { 10.0f }));
        g.drawText ("THE HYDRA // HARMONIC SCAFFOLD SYNTH",
                    layout.footerBounds,
                    juce::Justification::centredRight,
                    true);
    }
}
