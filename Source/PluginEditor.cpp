#include "PluginEditor.h"
#include "DSP/HydraHarmonySnap.h"

#include <array>
#include <limits>

namespace
{
constexpr int kEditorWidth = 750;
constexpr int kKeyboardStripHeight = 90;
constexpr int kColumnWidth = 118;
constexpr int kLabelBandHeight = 24;
constexpr int kCutoffTextBoxWidth = 88;
constexpr int kCutoffTextBoxHeight = 18;
constexpr int kControlColumnBottomInset = 22;
constexpr int kRotaryBodyHeight = 72;
constexpr int kRotarySliderWithReadoutHeight = kRotaryBodyHeight + kCutoffTextBoxHeight;
constexpr int kKnobRowHeight = kLabelBandHeight + kRotarySliderWithReadoutHeight + kControlColumnBottomInset;
constexpr int kPanelHorizontalMargin = 10;
constexpr int kPanelTopMargin = 8;
constexpr int kPanelBottomMargin = 10;
constexpr int kZoneGap = 8;
constexpr int kHarmonicSubRowGap = 6;
constexpr int kHarmonyKnobSize = 98;
constexpr int kHarmonyKnobLabelGap = 6;
constexpr int kHarmonySnapButtonWidth = 54;
constexpr int kHarmonySnapButtonHeight = 22;
constexpr int kHarmonyRowHeight = kLabelBandHeight + kHarmonyKnobLabelGap + kHarmonyKnobSize + kCutoffTextBoxHeight + 6;
constexpr int kInversionModeLabelHeight = 14;
constexpr int kInversionSequenceLabelHeight = 14;
constexpr int kInversionReadoutHeight = kInversionModeLabelHeight + kInversionSequenceLabelHeight;
constexpr int kHarmonicSubRowReadoutHeight = kInversionReadoutHeight > kCutoffTextBoxHeight ? kInversionReadoutHeight
                                                                                          : kCutoffTextBoxHeight;
constexpr int kHarmonicSubRowHeight = kLabelBandHeight + kRotaryBodyHeight + kHarmonicSubRowReadoutHeight + 10;
constexpr int kHarmonicSideStackHeight = kHarmonyRowHeight + kHarmonicSubRowGap + kHarmonicSubRowHeight;
constexpr int kXyPadSize = kHarmonicSideStackHeight;
constexpr int kHarmonicZoneHeight = kPanelTopMargin + kHarmonicSideStackHeight + 4;
constexpr int kHarmonicZone1Top = kPanelTopMargin;
constexpr int kModulationZoneTop = kHarmonicZone1Top + kHarmonicZoneHeight + kZoneGap;
constexpr int kModulationZoneHeight = kKnobRowHeight;
constexpr int kEnvelopeZoneTop = kModulationZoneTop + kModulationZoneHeight + kZoneGap;
constexpr int kFooterBrandHeight = 18;
constexpr int kFooterStripHeight = kFooterBrandHeight + kPanelBottomMargin;
constexpr int kEnvelopeKnobSize = kRotaryBodyHeight;
constexpr int kEnvelopeInnerPadding = 10;
constexpr int kEnvelopeTabButtonHeight = 24;
constexpr int kEnvelopeTabButtonGap = 8;
constexpr int kEnvelopeTabRowHeight = kEnvelopeTabButtonHeight + kEnvelopeTabButtonGap;
constexpr int kEnvelopeAdsrRowHeight = kLabelBandHeight + kRotarySliderWithReadoutHeight + 6;
constexpr int kEnvelopeSectionHeight = kEnvelopeTabRowHeight + kEnvelopeAdsrRowHeight + 4;
constexpr int kControlPanelHeight = kHarmonicZoneHeight + kZoneGap + kModulationZoneHeight + kZoneGap
                                  + kEnvelopeSectionHeight + kFooterStripHeight;
constexpr int kEditorHeightWithKeyboard = kControlPanelHeight + kKeyboardStripHeight;
constexpr juce::uint32 kMutedLabelColour = 0xff9a948c;

void styleKnobLabel (juce::Label& label)
{
    label.setFont (juce::Font (juce::FontOptions { 11.0f, juce::Font::bold }));
    label.setColour (juce::Label::textColourId, juce::Colour (kMutedLabelColour));
}

void styleKnobValueReadout (juce::Slider& slider)
{
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colour (kMutedLabelColour));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
}

juce::String formatTimeReadout (double seconds)
{
    if (seconds < 1.0)
        return juce::String (seconds * 1000.0, 3) + " ms";

    return juce::String (seconds, 3) + " s";
}

double parseTimeReadout (const juce::String& text)
{
    auto trimmed = text.trim();

    if (trimmed.endsWithIgnoreCase ("ms"))
        return trimmed.dropLastCharacters (2).trim().getDoubleValue() / 1000.0;

    if (trimmed.endsWithIgnoreCase ("s"))
        return trimmed.dropLastCharacters (1).trim().getDoubleValue();

    return trimmed.getDoubleValue();
}

void applyKnobReadout (juce::Slider& slider,
                       HydraAudioProcessorEditor::KnobReadoutKind kind,
                       const juce::String& valueSuffix = {})
{
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, kCutoffTextBoxWidth, kCutoffTextBoxHeight);
    styleKnobValueReadout (slider);
    slider.setTextValueSuffix ({});
    slider.textFromValueFunction = nullptr;
    slider.valueFromTextFunction = nullptr;

    switch (kind)
    {
        case HydraAudioProcessorEditor::KnobReadoutKind::unitless2dp:
            slider.setNumDecimalPlacesToDisplay (2);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 2); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::unitless3dp:
            slider.setNumDecimalPlacesToDisplay (3);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 3); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::percent:
            slider.setNumDecimalPlacesToDisplay (0);
            slider.textFromValueFunction = [] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; };
            slider.valueFromTextFunction = [] (const juce::String& text)
            {
                return text.retainCharacters ("0123456789.-").getDoubleValue() / 100.0;
            };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::bipolar2dp:
            slider.setNumDecimalPlacesToDisplay (2);
            slider.textFromValueFunction = [] (double value)
            {
                const auto sign = value > 0.0 ? "+" : "";
                return sign + juce::String (value, 2);
            };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::bipolar:
            slider.setNumDecimalPlacesToDisplay (3);
            slider.textFromValueFunction = [] (double value)
            {
                const auto sign = value > 0.0 ? "+" : "";
                return sign + juce::String (value, 3);
            };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::hertz:
            slider.setNumDecimalPlacesToDisplay (6);
            slider.textFromValueFunction = [] (double value) { return juce::String (value, 6); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return text.getDoubleValue(); };
            break;

        case HydraAudioProcessorEditor::KnobReadoutKind::timeSeconds:
            slider.textFromValueFunction = [] (double value) { return formatTimeReadout (value); };
            slider.valueFromTextFunction = [] (const juce::String& text) { return parseTimeReadout (text); };
            break;
    }

    if (valueSuffix.isNotEmpty())
        slider.setTextValueSuffix (valueSuffix);

    slider.updateText();
}

void styleInversionReadoutLabel (juce::Label& label, bool isSequenceLine)
{
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    label.setColour (juce::Label::textColourId, juce::Colour (kMutedLabelColour));
    label.setFont (juce::Font (juce::FontOptions { isSequenceLine ? 10.0f : 11.0f, juce::Font::plain }));
}

struct InversionDisplayText
{
    const char* mode;
    const char* sequence;
};

constexpr std::array<InversionDisplayText, 6> kInversionDisplayTexts {{
    { "LINEAR", "(1-7)" },
    { "SHUFFLE", "(1,3,2,6,4,7,5)" },
    { "ODD/PRIME", "(1,3,5,7,2,4,6)" },
    { "UNDERTONE", "(1,7,5,3,6,4,2)" },
    { "OCTAVE", "(1,2,4,6,3,5,7)" },
    { "BELL", "(1,5,7,6,3,2,4)" },
}};
} // namespace

HydraAudioProcessorEditor::HydraAudioProcessorEditor (HydraAudioProcessor& processor)
    : AudioProcessorEditor (&processor),
      audioProcessor (processor),
      xyExplorer (processor),
      keyboardComponent (processor.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    const bool isStandalone = (processor.wrapperType == juce::AudioProcessor::wrapperType_Standalone);
    keyboardComponent.setVisible (isStandalone);
    setSize (kEditorWidth, isStandalone ? kEditorHeightWithKeyboard : kControlPanelHeight);

    addAndMakeVisible (keyboardComponent);
    addAndMakeVisible (xyExplorer);

    volumeEnvButton.setButtonText ("[ VOLUME ]");
    filterEnvButton.setButtonText ("[ FILTER ]");
    addAndMakeVisible (volumeEnvButton);
    addAndMakeVisible (filterEnvButton);

    volumeEnvButton.onClick = [this] { selectVolumeEnvelopeTab(); };
    filterEnvButton.onClick = [this] { selectFilterEnvelopeTab(); };

    configureRotaryKnob (harmonySlider, harmonyLabel, "HARMONY", KnobReadoutKind::unitless3dp);

    harmonyQuantizeButton.setButtonText ("SNAP");
    addAndMakeVisible (harmonyQuantizeButton);

    harmonyLabel.setJustificationType (juce::Justification::centred);

    configureRotaryKnob (filterCutoffSlider, filterCutoffLabel, "FILTER CUTOFF", KnobReadoutKind::hertz, " Hz");
    configureRotaryKnob (filterResSlider, filterResLabel, "RESONANCE", KnobReadoutKind::unitless2dp);
    configureRotaryKnob (gainSlider, gainLabel, "MASTER GAIN", KnobReadoutKind::unitless3dp);
    configureRotaryKnob (glideSlider, glideLabel, "GLIDE", KnobReadoutKind::timeSeconds);
    configureSteppedRotaryKnob (harmonicInversionSlider, harmonicInversionLabel, "INVERSION");
    harmonicInversionSlider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    styleInversionReadoutLabel (harmonicInversionModeLabel, false);
    styleInversionReadoutLabel (harmonicInversionSequenceLabel, true);
    addAndMakeVisible (harmonicInversionModeLabel);
    addAndMakeVisible (harmonicInversionSequenceLabel);
    configureRotaryKnob (harmonicTiltSlider, harmonicTiltLabel, "HARMONIC TILT", KnobReadoutKind::bipolar);
    harmonicTiltSlider.setDoubleClickReturnValue (true, 0.0);
    configureRotaryKnob (kbTrackSlider, kbTrackLabel, "KB TRACK", KnobReadoutKind::unitless2dp);
    configureRotaryKnob (hpCutoffSlider, hpCutoffLabel, "HP CUTOFF", KnobReadoutKind::hertz, " Hz");
    configureRotaryKnob (overloadSlider, overloadLabel, "OVERLOAD", KnobReadoutKind::unitless2dp);

    configureAdsrKnob (envWarpSlider, envWarpLabel, "ENV WARP", KnobReadoutKind::bipolar2dp);
    configureAdsrKnob (egrAmountSlider, egrAmountLabel, "EGR AMOUNT", KnobReadoutKind::bipolar2dp);
    configureAdsrKnob (attackSlider, attackLabel, "ATTACK", KnobReadoutKind::timeSeconds);
    configureAdsrKnob (decaySlider, decayLabel, "DECAY", KnobReadoutKind::timeSeconds);
    configureAdsrKnob (sustainSlider, sustainLabel, "SUSTAIN", KnobReadoutKind::unitless3dp);
    configureAdsrKnob (releaseSlider, releaseLabel, "RELEASE", KnobReadoutKind::timeSeconds);
    auto& apvts = audioProcessor.getApvts();

    if (auto* depthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("depth")))
        if (auto* girthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("girth")))
            xyExplorer.setParameters (*depthParam, *girthParam);

    harmonyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harmony", harmonySlider);
    harmonyQuantizeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "harmonyQuantize", harmonyQuantizeButton);

    harmonySlider.onValueChange = [this]
    {
        if (isUpdatingHarmonySnap || ! harmonyQuantizeButton.getToggleState())
            return;

        snapHarmonyParameterToNearestStep();
    };

    harmonyQuantizeButton.onClick = [this]
    {
        if (harmonyQuantizeButton.getToggleState())
            snapHarmonyParameterToNearestStep();
    };

    if (harmonyQuantizeButton.getToggleState())
        snapHarmonyParameterToNearestStep();

    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "cutoff", filterCutoffSlider);
    filterResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "res", filterResSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "gain", gainSlider);
    envWarpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "envWarp", envWarpSlider);
    egrAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "egrAmount", egrAmountSlider);
    glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "glideTime", glideSlider);

    selectVolumeEnvelopeTab();
    harmonicInversionSlider.setRange (0.0, 5.0, 1.0);
    harmonicInversionSlider.onValueChange = [this] { updateHarmonicInversionDisplayLabels(); };

    harmonicInversionAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "inversion", harmonicInversionSlider);

    updateHarmonicInversionDisplayLabels();
    harmonicTiltAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "tilt", harmonicTiltSlider);
    kbTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "kbTrack", kbTrackSlider);
    hpCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "hpCutoff", hpCutoffSlider);
    overloadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterOverload", overloadSlider);

    refreshKnobReadouts();
}

HydraAudioProcessorEditor::~HydraAudioProcessorEditor()
{
    harmonySlider.setLookAndFeel (nullptr);
    filterCutoffSlider.setLookAndFeel (nullptr);
    filterResSlider.setLookAndFeel (nullptr);
    gainSlider.setLookAndFeel (nullptr);
    envWarpSlider.setLookAndFeel (nullptr);
    egrAmountSlider.setLookAndFeel (nullptr);
    attackSlider.setLookAndFeel (nullptr);
    decaySlider.setLookAndFeel (nullptr);
    sustainSlider.setLookAndFeel (nullptr);
    releaseSlider.setLookAndFeel (nullptr);
    glideSlider.setLookAndFeel (nullptr);
    harmonicInversionSlider.setLookAndFeel (nullptr);
    harmonicTiltSlider.setLookAndFeel (nullptr);
    kbTrackSlider.setLookAndFeel (nullptr);
    hpCutoffSlider.setLookAndFeel (nullptr);
    overloadSlider.setLookAndFeel (nullptr);
}

void HydraAudioProcessorEditor::updateHarmonicInversionDisplayLabels()
{
    const auto index = juce::jlimit (0, 5, juce::roundToInt (harmonicInversionSlider.getValue()));
    const auto& text = kInversionDisplayTexts[static_cast<size_t> (index)];

    harmonicInversionModeLabel.setText (text.mode, juce::dontSendNotification);
    harmonicInversionSequenceLabel.setText (text.sequence, juce::dontSendNotification);
}

void HydraAudioProcessorEditor::selectVolumeEnvelopeTab()
{
    volumeEnvButton.setToggleState (true, juce::dontSendNotification);
    filterEnvButton.setToggleState (false, juce::dontSendNotification);
    updateEnvelopeTabBindings();
}

void HydraAudioProcessorEditor::selectFilterEnvelopeTab()
{
    volumeEnvButton.setToggleState (false, juce::dontSendNotification);
    filterEnvButton.setToggleState (true, juce::dontSendNotification);
    updateEnvelopeTabBindings();
}

void HydraAudioProcessorEditor::updateEnvelopeTabBindings()
{
    auto& apvts = audioProcessor.getApvts();

    attackAttachment.reset();
    decayAttachment.reset();
    sustainAttachment.reset();
    releaseAttachment.reset();

    if (volumeEnvButton.getToggleState())
    {
        attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "attack", attackSlider);
        decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "decay", decaySlider);
        sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sustain", sustainSlider);
        releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "release", releaseSlider);
        refreshKnobReadouts();
        return;
    }

    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterAttack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterDecay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterSustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterRelease", releaseSlider);

    refreshKnobReadouts();
}

void HydraAudioProcessorEditor::refreshKnobReadouts()
{
    applyKnobReadout (harmonySlider, KnobReadoutKind::unitless3dp);
    applyKnobReadout (filterCutoffSlider, KnobReadoutKind::hertz, " Hz");
    applyKnobReadout (filterResSlider, KnobReadoutKind::unitless2dp);
    applyKnobReadout (gainSlider, KnobReadoutKind::unitless3dp);
    applyKnobReadout (glideSlider, KnobReadoutKind::timeSeconds);
    applyKnobReadout (harmonicTiltSlider, KnobReadoutKind::bipolar);
    applyKnobReadout (kbTrackSlider, KnobReadoutKind::unitless2dp);
    applyKnobReadout (hpCutoffSlider, KnobReadoutKind::hertz, " Hz");
    applyKnobReadout (overloadSlider, KnobReadoutKind::unitless2dp);
    applyKnobReadout (envWarpSlider, KnobReadoutKind::bipolar2dp);
    applyKnobReadout (egrAmountSlider, KnobReadoutKind::bipolar2dp);
    applyKnobReadout (attackSlider, KnobReadoutKind::timeSeconds);
    applyKnobReadout (decaySlider, KnobReadoutKind::timeSeconds);
    applyKnobReadout (sustainSlider, KnobReadoutKind::unitless3dp);
    applyKnobReadout (releaseSlider, KnobReadoutKind::timeSeconds);
}

void HydraAudioProcessorEditor::configureRotaryKnob (juce::Slider& slider,
                                                     juce::Label& label,
                                                     const juce::String& labelText,
                                                     const KnobReadoutKind readoutKind,
                                                     const juce::String& valueSuffix)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setLookAndFeel (&customLookAndFeel);
    applyKnobReadout (slider, readoutKind, valueSuffix);

    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    styleKnobLabel (label);
    addAndMakeVisible (label);
}

void HydraAudioProcessorEditor::snapHarmonyParameterToNearestStep()
{
    auto* harmonyParam = audioProcessor.getApvts().getParameter ("harmony");

    if (harmonyParam == nullptr)
        return;

    const auto& range = harmonyParam->getNormalisableRange();
    const auto denormalizedValue = range.convertFrom0to1 (harmonyParam->getValue());
    const auto snappedValue = HydraHarmonySnap::quantizeHarmonyValue (denormalizedValue);
    const auto normalizedValue = range.convertTo0to1 (snappedValue);

    isUpdatingHarmonySnap = true;

    if (std::abs (harmonyParam->getValue() - normalizedValue) > 1.0e-5f)
        harmonyParam->setValueNotifyingHost (normalizedValue);

    isUpdatingHarmonySnap = false;
}

void HydraAudioProcessorEditor::configureSteppedRotaryKnob (juce::Slider& slider,
                                                             juce::Label& label,
                                                             const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setLookAndFeel (&customLookAndFeel);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, kCutoffTextBoxWidth, kCutoffTextBoxHeight);
    styleKnobValueReadout (slider);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    styleKnobLabel (label);
    addAndMakeVisible (label);
}

void HydraAudioProcessorEditor::configureAdsrKnob (juce::Slider& slider,
                                                    juce::Label& label,
                                                    const juce::String& labelText,
                                                    const HydraAudioProcessorEditor::KnobReadoutKind readoutKind)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setLookAndFeel (&customLookAndFeel);
    applyKnobReadout (slider, readoutKind);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    styleKnobLabel (label);
    addAndMakeVisible (label);
}

void HydraAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff161412));

    auto controlPanel = getLocalBounds();

    if (keyboardComponent.isVisible())
        controlPanel.removeFromBottom (kKeyboardStripHeight);

    const auto footerBounds = juce::Rectangle<int> (kPanelHorizontalMargin,
                                                    controlPanel.getBottom() - kFooterStripHeight,
                                                    controlPanel.getWidth() - (2 * kPanelHorizontalMargin),
                                                    kFooterBrandHeight);

    g.setColour (juce::Colour (0xff6a6560));
    g.setFont (juce::Font (juce::FontOptions { 10.0f }));
    g.drawText ("THE HYDRA // HARMONIC SCAFFOLD SYNTH",
                footerBounds,
                juce::Justification::centredRight,
                true);
}

void HydraAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    if (keyboardComponent.isVisible())
        keyboardComponent.setBounds (bounds.removeFromBottom (kKeyboardStripHeight));

    const auto controlPanel = bounds;
    const auto controlPanelOriginY = controlPanel.getY();

    const auto harmonicZone = juce::Rectangle<int> (controlPanel.getX(),
                                                    controlPanelOriginY + kHarmonicZone1Top,
                                                    controlPanel.getWidth(),
                                                    kHarmonicZoneHeight);
    const auto harmonicStackTop = harmonicZone.getY();
    const auto xyPadX = (getWidth() - kXyPadSize) / 2;
    const auto sideGap = 10;

    xyExplorer.setBounds (xyPadX, harmonicStackTop, kXyPadSize, kHarmonicSideStackHeight);
    xyExplorer.toBack();

    auto leftHarmonicCluster = juce::Rectangle<int> (kPanelHorizontalMargin,
                                                   harmonicStackTop,
                                                   xyPadX - kPanelHorizontalMargin - sideGap,
                                                   kHarmonicSideStackHeight);
    auto rightHarmonicCluster = juce::Rectangle<int> (xyPadX + kXyPadSize + sideGap,
                                                      harmonicStackTop,
                                                      getWidth() - (xyPadX + kXyPadSize + sideGap) - kPanelHorizontalMargin,
                                                      kHarmonicSideStackHeight);

    const auto placeKnobColumn = [] (juce::Rectangle<int> column,
                                     juce::Slider& slider,
                                     juce::Label& label,
                                     int sliderHeight,
                                     bool compact = false)
    {
        label.setBounds (column.removeFromTop (kLabelBandHeight));
        column.removeFromBottom (compact ? 6 : kControlColumnBottomInset);
        slider.setBounds (column.removeFromTop (juce::jmin (sliderHeight, column.getHeight())));
    };

    const auto bringHarmonicControlsToFront = [&]
    {
        harmonySlider.toFront (false);
        harmonyLabel.toFront (false);
        harmonyQuantizeButton.toFront (false);
        harmonicInversionSlider.toFront (false);
        harmonicInversionLabel.toFront (false);
        harmonicInversionModeLabel.toFront (false);
        harmonicInversionSequenceLabel.toFront (false);
        harmonicTiltSlider.toFront (false);
        harmonicTiltLabel.toFront (false);
        filterCutoffSlider.toFront (false);
        filterCutoffLabel.toFront (false);
        filterResSlider.toFront (false);
        filterResLabel.toFront (false);
        glideSlider.toFront (false);
        glideLabel.toFront (false);
        overloadSlider.toFront (false);
        overloadLabel.toFront (false);
        gainSlider.toFront (false);
        gainLabel.toFront (false);
        volumeEnvButton.toFront (false);
        filterEnvButton.toFront (false);
        attackSlider.toFront (false);
        attackLabel.toFront (false);
        decaySlider.toFront (false);
        decayLabel.toFront (false);
        sustainSlider.toFront (false);
        sustainLabel.toFront (false);
        releaseSlider.toFront (false);
        releaseLabel.toFront (false);
    };

    {
        auto harmonyRow = leftHarmonicCluster.removeFromTop (kHarmonyRowHeight);
        leftHarmonicCluster.removeFromTop (kHarmonicSubRowGap);
        auto bottomRow = leftHarmonicCluster;

        harmonyQuantizeButton.setBounds (harmonyRow.getRight() - kHarmonySnapButtonWidth,
                                         harmonyRow.getY() + ((kLabelBandHeight - kHarmonySnapButtonHeight) / 2),
                                         kHarmonySnapButtonWidth,
                                         kHarmonySnapButtonHeight);

        auto harmonyStack = harmonyRow.withSizeKeepingCentre (kHarmonyKnobSize, harmonyRow.getHeight());

        harmonyLabel.setBounds (harmonyStack.removeFromTop (kLabelBandHeight));
        harmonyStack.removeFromTop (kHarmonyKnobLabelGap);
        harmonySlider.setBounds (harmonyStack.removeFromTop (kHarmonyKnobSize + kCutoffTextBoxHeight)
                                           .withSizeKeepingCentre (kHarmonyKnobSize, kHarmonyKnobSize + kCutoffTextBoxHeight));

        const auto bottomColumnWidth = bottomRow.getWidth() / 2;
        auto inversionColumn = bottomRow.removeFromLeft (bottomColumnWidth);
        auto tiltColumn = bottomRow;
        harmonicInversionLabel.setBounds (inversionColumn.removeFromTop (kLabelBandHeight));
        inversionColumn.removeFromBottom (6);
        auto inversionReadoutColumn = inversionColumn.removeFromBottom (kInversionReadoutHeight);
        harmonicInversionSlider.setBounds (inversionColumn.removeFromTop (juce::jmin (kRotaryBodyHeight, inversionColumn.getHeight())));
        harmonicInversionModeLabel.setBounds (inversionReadoutColumn.removeFromTop (kInversionModeLabelHeight));
        harmonicInversionSequenceLabel.setBounds (inversionReadoutColumn);
        placeKnobColumn (tiltColumn,
                         harmonicTiltSlider,
                         harmonicTiltLabel,
                         kRotaryBodyHeight + kCutoffTextBoxHeight,
                         true);
    }

    {
        auto gainRow = rightHarmonicCluster.removeFromTop (kHarmonyRowHeight);
        rightHarmonicCluster.removeFromTop (kHarmonicSubRowGap);
        auto bottomRow = rightHarmonicCluster;

        auto gainStack = gainRow.withSizeKeepingCentre (kHarmonyKnobSize, gainRow.getHeight());
        gainLabel.setBounds (gainStack.removeFromTop (kLabelBandHeight));
        gainStack.removeFromTop (kHarmonyKnobLabelGap);
        gainSlider.setBounds (gainStack.removeFromTop (kHarmonyKnobSize + kCutoffTextBoxHeight)
                                       .withSizeKeepingCentre (kHarmonyKnobSize, kHarmonyKnobSize + kCutoffTextBoxHeight));

        const auto bottomColumnWidth = bottomRow.getWidth() / 2;
        auto glideColumn = bottomRow.removeFromLeft (bottomColumnWidth);
        auto overloadColumn = bottomRow;
        placeKnobColumn (glideColumn, glideSlider, glideLabel, kRotarySliderWithReadoutHeight, true);
        placeKnobColumn (overloadColumn, overloadSlider, overloadLabel, kRotarySliderWithReadoutHeight, true);
    }

    bringHarmonicControlsToFront();

    const auto modulationRow = juce::Rectangle<int> (controlPanel.getX(),
                                                   controlPanelOriginY + kModulationZoneTop,
                                                   controlPanel.getWidth(),
                                                   kModulationZoneHeight);
    const auto modulationInnerWidth = getWidth() - (2 * kPanelHorizontalMargin);
    const auto modulationColumnWidth = modulationInnerWidth / 6;

    const auto placeModulationColumn = [&] (int columnIndex,
                                            juce::Slider& slider,
                                            juce::Label& label,
                                            int sliderHeight)
    {
        auto column = modulationRow.withX (kPanelHorizontalMargin + (columnIndex * modulationColumnWidth))
                                   .withWidth (modulationColumnWidth);
        placeKnobColumn (column, slider, label, sliderHeight);
    };

    placeModulationColumn (0, filterCutoffSlider, filterCutoffLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (1, filterResSlider, filterResLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (2, kbTrackSlider, kbTrackLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (3, hpCutoffSlider, hpCutoffLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (4, egrAmountSlider, egrAmountLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (5, envWarpSlider, envWarpLabel, kRotarySliderWithReadoutHeight);

    const auto adsrClusterWidth = modulationColumnWidth * 4;

    auto envelopeArea = juce::Rectangle<int> (kPanelHorizontalMargin,
                                              controlPanelOriginY + kEnvelopeZoneTop,
                                              modulationInnerWidth,
                                              kEnvelopeSectionHeight);

    auto tabRow = envelopeArea.removeFromTop (kEnvelopeTabRowHeight);
    auto tabCluster = tabRow.withSizeKeepingCentre (adsrClusterWidth, tabRow.getHeight());
    const auto tabButtonWidth = (tabCluster.getWidth() - kEnvelopeTabButtonGap) / 2;
    auto volumeTabBounds = tabCluster.removeFromLeft (tabButtonWidth);
    tabCluster.removeFromLeft (kEnvelopeTabButtonGap);
    auto filterTabBounds = tabCluster;
    volumeEnvButton.setBounds (volumeTabBounds.reduced (0, 1));
    filterEnvButton.setBounds (filterTabBounds.reduced (0, 1));

    auto adsrRow = envelopeArea.reduced (kEnvelopeInnerPadding, 0);
    adsrRow = adsrRow.withSizeKeepingCentre (adsrClusterWidth, adsrRow.getHeight());

    const auto placeEnvelopeDial = [] (juce::Rectangle<int>& row,
                                       int columnWidth,
                                       juce::Slider& slider,
                                       juce::Label& label)
    {
        auto column = row.removeFromLeft (columnWidth);
        label.setBounds (column.removeFromTop (kLabelBandHeight));
        column.removeFromBottom (6);
        slider.setBounds (column.removeFromTop (juce::jmin (kRotarySliderWithReadoutHeight, column.getHeight())));
    };

    placeEnvelopeDial (adsrRow, modulationColumnWidth, attackSlider, attackLabel);
    placeEnvelopeDial (adsrRow, modulationColumnWidth, decaySlider, decayLabel);
    placeEnvelopeDial (adsrRow, modulationColumnWidth, sustainSlider, sustainLabel);
    placeEnvelopeDial (adsrRow, modulationColumnWidth, releaseSlider, releaseLabel);
}
