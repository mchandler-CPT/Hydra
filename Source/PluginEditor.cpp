#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth = 500;
constexpr int kControlPanelHeight = 250;
constexpr int kEditorHeight = 320;
constexpr int kKeyboardHeight = 70;
constexpr int kColumnWidth = 125;
constexpr int kLabelBandHeight = 24;
} // namespace

HydraAudioProcessorEditor::HydraAudioProcessorEditor (HydraAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      keyboardComponent (processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&customLookAndFeel);
    setSize (kEditorWidth, kEditorHeight);

    addAndMakeVisible (keyboardComponent);

    configureSlider (depthSlider, "INTELLIGENT DEPTH", depthLabel);
    configureSlider (girthSlider, "SPATIAL GIRTH", girthLabel);
    configureSlider (morphSlider, "WAVE MORPH", morphLabel);
    configureSlider (gainSlider, "MASTER GAIN", gainLabel);

    auto& apvts = audioProcessor.getApvts();
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "depth", depthSlider);
    girthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "girth", girthSlider);
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

    auto controlPanel = getLocalBounds().withHeight (kControlPanelHeight);

    const auto placeColumn = [&controlPanel] (int columnIndex, juce::Slider& slider, juce::Label& label)
    {
        auto column = controlPanel.withX (columnIndex * kColumnWidth).withWidth (kColumnWidth);

        label.setBounds (column.removeFromTop (kLabelBandHeight));
        slider.setBounds (column);
    };

    placeColumn (0, depthSlider, depthLabel);
    placeColumn (1, girthSlider, girthLabel);
    placeColumn (2, morphSlider, morphLabel);
    placeColumn (3, gainSlider, gainLabel);
}
