#pragma once

#include "HydraPalette.h"

#include <juce_gui_basics/juce_gui_basics.h>

struct HydraPanelBackgroundLayout
{
    juce::Rectangle<int> backgroundPanel;
    juce::Rectangle<int> headerZone;
    juce::Rectangle<int> controlPanel;
    juce::Rectangle<int> harmonicZone;
    juce::Rectangle<int> modulationZone;
    juce::Rectangle<int> envelopeZone;
    juce::Rectangle<int> xyPadBounds;
    juce::Rectangle<int> footerBounds;
    juce::Rectangle<int> keyboardStrip;
};

class HydraPanelBackground
{
public:
    void ensureCachesBuilt (int width, int height);
    void paint (juce::Graphics& g,
                const HydraPanelBackgroundLayout& layout,
                float depthNorm,
                float girthNorm,
                float audioPeak) const;

private:
    void rebuildWatermark (int width, int height);
    void rebuildGrain();

    juce::Image watermarkImage;
    juce::Image grainTile;
    int cachedWidth = 0;
    int cachedHeight = 0;
};
