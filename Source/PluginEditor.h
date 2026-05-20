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

    void configureAdsrKnob (juce::Slider& slider, juce::Label& label, const juce::String& labelText);

    HydraAudioProcessor& audioProcessor;
    BoutiqueLookAndFeel customLookAndFeel;

    XyExplorer xyExplorer;
    juce::GroupComponent envelopeControlGroup;
    juce::GroupComponent volumeEnvelopeGroup;
    juce::GroupComponent filterEnvelopeGroup;

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

    juce::Slider envWarpSlider;
    juce::Slider egrAmountSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider filterAttackSlider;
    juce::Slider filterDecaySlider;
    juce::Slider filterSustainSlider;
    juce::Slider filterReleaseSlider;
    juce::Slider glideSlider;

    juce::Label envWarpLabel;
    juce::Label egrAmountLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label filterAttackLabel;
    juce::Label filterDecayLabel;
    juce::Label filterSustainLabel;
    juce::Label filterReleaseLabel;
    juce::Label glideLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envWarpAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> egrAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;

    juce::MidiKeyboardComponent keyboardComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessorEditor)
};
