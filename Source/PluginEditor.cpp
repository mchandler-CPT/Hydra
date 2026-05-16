#include "PluginProcessor.h"
#include "PluginEditor.h"

BoutiqueTemplateAudioProcessorEditor::BoutiqueTemplateAudioProcessorEditor (BoutiqueTemplateAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (500, 300);
}

BoutiqueTemplateAudioProcessorEditor::~BoutiqueTemplateAudioProcessorEditor() {}

void BoutiqueTemplateAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    
    g.setFont (18.0f);
    g.setColour (juce::Colours::white);
    g.drawFittedText ("Boutique Base Template", getLocalBounds().removeFromTop(60), juce::Justification::centred, 1);
    
    g.setFont (13.0f);
    g.setColour (juce::Colours::grey);
    g.drawFittedText ("Ready for DSP deployment", getLocalBounds(), juce::Justification::centred, 1);
}

void BoutiqueTemplateAudioProcessorEditor::resized() {}