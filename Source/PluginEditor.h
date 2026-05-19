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
    void configureRotaryKnob (juce::Slider& slider,
                              juce::Label& label,
                              const juce::String& labelText,
                              bool showValueTextBox,
                              const juce::String& valueSuffix = {});

    HydraAudioProcessor& audioProcessor;
    BoutiqueLookAndFeel customLookAndFeel;

    XyExplorer xyExplorer;

    juce::Slider harmonySlider;
    juce::Slider filterCutoffSlider;
    juce::Slider filterResSlider;
    juce::Slider gainSlider;

    juce::Label harmonyLabel;
    juce::Label filterCutoffLabel;
    juce::Label filterResLabel;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessorEditor)
};
