#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth = 500;
constexpr int kControlPanelHeight = 250;
constexpr int kEditorHeight = 320;
constexpr int kKeyboardHeight = 70;
constexpr int kColumnWidth = 125;
constexpr int kLabelBandHeight = 24;
constexpr int kMorphColumnX = 250;
constexpr int kGainColumnX = 375;
} // namespace

HydraAudioProcessorEditor::HydraAudioProcessorEditor (HydraAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      keyboardComponent (processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&customLookAndFeel);
    setSize (kEditorWidth, kEditorHeight);

    addAndMakeVisible (keyboardComponent);
    addAndMakeVisible (xyExplorer);

    configureSlider (morphSlider, "WAVE MORPH", morphLabel);
    configureSlider (gainSlider, "MASTER GAIN", gainLabel);

    auto& apvts = audioProcessor.getApvts();

    if (auto* depthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("depth")))
        if (auto* girthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("girth")))
            xyExplorer.setParameters (*depthParam, *girthParam);

    morphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "morph", morphSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "gain", gainSlider);
}

HydraAudioProcessorEditor::~HydraAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HydraAudioProcessorEditor::configureSlider (juce::Slider& slider,
                                                 const juce::String& labelText,
                                                 juce::Label& label)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void HydraAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff161412));

    g.setColour (juce::Colour (0xff6a6560));
    g.setFont (juce::Font (juce::FontOptions { 10.0f }));
    g.drawText ("THE HYDRA — HARMONIC SCAFFOLD SYNTH",
                getLocalBounds().withHeight (kControlPanelHeight).reduced (10).removeFromBottom (18),
                juce::Justification::bottomRight,
                true);
}

void HydraAudioProcessorEditor::resized()
{
    keyboardComponent.setBounds (0, kControlPanelHeight, kEditorWidth, kKeyboardHeight);
    xyExplorer.setBounds (15, 15, 220, 220);

    auto controlPanel = getLocalBounds().withHeight (kControlPanelHeight);

    const auto placeColumnAtX = [&controlPanel] (int x,
                                                 juce::Slider& slider,
                                                 juce::Label& label)
    {
        auto column = controlPanel.withX (x).withWidth (kColumnWidth);

        label.setBounds (column.removeFromTop (kLabelBandHeight));
        slider.setBounds (column);
    };

    placeColumnAtX (kMorphColumnX, morphSlider, morphLabel);
    placeColumnAtX (kGainColumnX, gainSlider, gainLabel);
}
