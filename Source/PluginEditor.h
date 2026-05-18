#pragma once

#include "BoutiqueLookAndFeel.h"
#include "PluginProcessor.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class HydraAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit HydraAudioProcessorEditor (HydraAudioProcessor&);
    ~HydraAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void configureSlider (juce::Slider& slider, const juce::String& labelText, juce::Label& label);

    HydraAudioProcessor& audioProcessor;
    BoutiqueLookAndFeel customLookAndFeel;

    juce::Slider depthSlider;
    juce::Slider girthSlider;
    juce::Slider morphSlider;
    juce::Slider gainSlider;

    juce::Label depthLabel;
    juce::Label girthLabel;
    juce::Label morphLabel;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> girthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessorEditor)
};
