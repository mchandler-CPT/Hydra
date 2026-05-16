#pragma once
#include "PluginProcessor.h"

class BoutiqueTemplateAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    BoutiqueTemplateAudioProcessorEditor (BoutiqueTemplateAudioProcessor&);
    ~BoutiqueTemplateAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    BoutiqueTemplateAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BoutiqueTemplateAudioProcessorEditor)
};