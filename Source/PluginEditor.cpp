#include "PluginProcessor.h"
#include "PluginEditor.h"

HydraAudioProcessorEditor::HydraAudioProcessorEditor (HydraAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (400, 300);
}

HydraAudioProcessorEditor::~HydraAudioProcessorEditor() {}

void HydraAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    g.drawFittedText ("Hydra Core Initialized", getLocalBounds(), juce::Justification::centred, 1);
}

void HydraAudioProcessorEditor::resized() {}