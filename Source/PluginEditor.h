#pragma once

#include "ProceduralDarkLookAndFeel.h"
#include "PluginProcessor.h"
#include "UI/SnapToggleButton.h"
#include "UI/XyExplorer.h"

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class HydraAudioProcessorEditor : public juce::AudioProcessorEditor,
                                  private juce::Timer
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

    void configureSteppedRotaryKnob (juce::Slider& slider,
                                     juce::Label& label,
                                     const juce::String& labelText);

    void updateEnvelopeTabBindings();
    void selectVolumeEnvelopeTab();
    void selectFilterEnvelopeTab();

    void timerCallback() override;
    void updateHarmonyDebugLabel();
    void updateHarmonicInversionDisplayLabels();
    void snapHarmonyParameterToNearestStep();

    HydraAudioProcessor& audioProcessor;
    ProceduralDarkLookAndFeel customLookAndFeel;

    XyExplorer xyExplorer;

    SnapToggleButton volumeEnvButton;
    SnapToggleButton filterEnvButton;

    juce::Slider harmonySlider;
    SnapToggleButton harmonyQuantizeButton;
    juce::Slider filterCutoffSlider;
    juce::Slider filterResSlider;
    juce::Slider gainSlider;

    juce::Label harmonyLabel;
    juce::Label harmonyDebugValueLabel;
    juce::Label filterCutoffLabel;
    juce::Label filterResLabel;
    juce::Label gainLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> harmonyQuantizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> filterResAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::Slider envWarpSlider;
    juce::Slider egrAmountSlider;
    juce::Slider attackSlider;
    juce::Slider decaySlider;
    juce::Slider sustainSlider;
    juce::Slider releaseSlider;
    juce::Slider glideSlider;
    juce::Slider harmonicInversionSlider;
    juce::Slider harmonicTiltSlider;
    juce::Slider kbTrackSlider;
    juce::Slider hpCutoffSlider;
    juce::Slider overloadSlider;

    juce::Label envWarpLabel;
    juce::Label egrAmountLabel;
    juce::Label attackLabel;
    juce::Label decayLabel;
    juce::Label sustainLabel;
    juce::Label releaseLabel;
    juce::Label glideLabel;
    juce::Label harmonicInversionLabel;
    juce::Label harmonicInversionModeLabel;
    juce::Label harmonicInversionSequenceLabel;
    juce::Label harmonicTiltLabel;
    juce::Label kbTrackLabel;
    juce::Label hpCutoffLabel;
    juce::Label overloadLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> envWarpAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> egrAmountAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> glideAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonicInversionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonicTiltAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kbTrackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> hpCutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> overloadAttachment;

    juce::MidiKeyboardComponent keyboardComponent;

    bool isUpdatingHarmonySnap = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HydraAudioProcessorEditor)
};
