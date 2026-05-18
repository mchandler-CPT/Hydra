#pragma once

#include "BoutiqueLookAndFeel.h"
#include "PluginProcessor.h"
#include "UI/XyExplorer.h"

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

    XyExplorer xyExplorer;

    juce::Slider morphSlider;
    juce::Slider filterCutoffSlider;
    juce::Slider gainSlider;

    juce::Label morphLabel;
    juce::Label filterCutoffLabel;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> morphAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessorEditor)
};
