#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth = 750;
constexpr int kControlPanelHeight = 250;
constexpr int kEditorHeight = 320;
constexpr int kKeyboardHeight = 70;
constexpr int kColumnWidth = 125;
constexpr int kLabelBandHeight = 24;
constexpr int kCutoffTextBoxBandHeight = 20;
constexpr int kRotarySliderBottomBandHeight = kCutoffTextBoxBandHeight;
constexpr int kCutoffTextBoxWidth = 70;
constexpr int kCutoffTextBoxHeight = 18;
constexpr int kControlColumnBottomInset = 22;
constexpr int kXyPadRightMargin = 10;
constexpr int kHarmonyColumnX = 15 + 220 + kXyPadRightMargin;
constexpr int kCutoffColumnX = kHarmonyColumnX + kColumnWidth;
constexpr int kResColumnX = kCutoffColumnX + kColumnWidth;
constexpr int kGainColumnX = kResColumnX + kColumnWidth;
constexpr juce::uint32 kMutedLabelColour = 0xff9a948c;
} // namespace

HydraAudioProcessorEditor::HydraAudioProcessorEditor (HydraAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      xyExplorer (processor),
      keyboardComponent (processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&customLookAndFeel);
    setSize (kEditorWidth, kEditorHeight);

    addAndMakeVisible (keyboardComponent);
    addAndMakeVisible (xyExplorer);

    configureRotaryKnob (harmonySlider, harmonyLabel, "HARMONY", false);
    configureRotaryKnob (filterCutoffSlider, filterCutoffLabel, "FILTER CUTOFF", true, " Hz");
    configureRotaryKnob (filterResSlider, filterResLabel, "RESONANCE", true);
    configureRotaryKnob (gainSlider, gainLabel, "MASTER GAIN", false);

    filterCutoffLabel.setFont (juce::Font (juce::FontOptions { 11.0f, juce::Font::bold }));
    filterCutoffLabel.setColour (juce::Label::textColourId, juce::Colour (kMutedLabelColour));
    filterResLabel.setFont (juce::Font (juce::FontOptions { 12.0f, juce::Font::bold }));
    filterResLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));

    auto& apvts = audioProcessor.getApvts();

    if (auto* depthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("depth")))
        if (auto* girthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("girth")))
            xyExplorer.setParameters (*depthParam, *girthParam);

    harmonyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harmony", harmonySlider);
    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "cutoff", filterCutoffSlider);
    filterResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "res", filterResSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "gain", gainSlider);
}

HydraAudioProcessorEditor::~HydraAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void HydraAudioProcessorEditor::configureRotaryKnob (juce::Slider& slider,
                                                     juce::Label& label,
                                                     const juce::String& labelText,
                                                     const bool showValueTextBox,
                                                     const juce::String& valueSuffix)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow,
                            showValueTextBox,
                            kCutoffTextBoxWidth,
                            kCutoffTextBoxHeight);

    if (showValueTextBox)
    {
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.6f));
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

        if (valueSuffix.isNotEmpty())
            slider.setTextValueSuffix (valueSuffix);
    }
    else
    {
        slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        slider.textFromValueFunction = [] (double) { return juce::String(); };
    }

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

    const auto placeControlColumn = [&controlPanel] (int x,
                                                     juce::Slider& slider,
                                                     juce::Label& label,
                                                     int textBoxBandHeight)
    {
        auto column = controlPanel.withX (x).withWidth (kColumnWidth);
        label.setBounds (column.removeFromTop (kLabelBandHeight));
        column.removeFromBottom (textBoxBandHeight + kControlColumnBottomInset);
        slider.setBounds (column);
    };

    placeControlColumn (kHarmonyColumnX, harmonySlider, harmonyLabel, kRotarySliderBottomBandHeight);
    placeControlColumn (kCutoffColumnX, filterCutoffSlider, filterCutoffLabel, kRotarySliderBottomBandHeight);
    placeControlColumn (kResColumnX, filterResSlider, filterResLabel, kRotarySliderBottomBandHeight);
    placeControlColumn (kGainColumnX, gainSlider, gainLabel, kRotarySliderBottomBandHeight);
}
