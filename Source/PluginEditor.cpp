#include "PluginEditor.h"
#include "DSP/HydraHarmonySnap.h"

#include <array>
#include <limits>

namespace
{
constexpr int kEditorWidth = 750;
constexpr int kKeyboardHeight = 70;
constexpr int kControlPanelHeight = 700;
constexpr int kEditorHeight = kControlPanelHeight + kKeyboardHeight;
constexpr int kColumnWidth = 118;
constexpr int kLabelBandHeight = 24;
constexpr int kCutoffTextBoxWidth = 70;
constexpr int kCutoffTextBoxHeight = 18;
constexpr int kControlColumnBottomInset = 22;
constexpr int kRotaryBodyHeight = 72;
constexpr int kRotarySliderWithReadoutHeight = kRotaryBodyHeight + kCutoffTextBoxHeight;
constexpr int kKnobRowHeight = kLabelBandHeight + kRotarySliderWithReadoutHeight + kControlColumnBottomInset;
constexpr int kPanelHorizontalMargin = 10;
constexpr int kPanelTopMargin = 8;
constexpr int kPanelBottomMargin = 10;
constexpr int kZoneGap = 8;
constexpr int kZoneDividerInset = 14;
constexpr int kHarmonicSubRowGap = 6;
constexpr int kHarmonicSubRowHeight = kLabelBandHeight + kRotarySliderWithReadoutHeight + 10;
constexpr int kHarmonicSideStackHeight = (2 * kHarmonicSubRowHeight) + kHarmonicSubRowGap;
constexpr int kXyPadSize = kHarmonicSideStackHeight;
constexpr int kHarmonicZoneHeight = kPanelTopMargin + kHarmonicSideStackHeight + 24;
constexpr int kHarmonicZone1Top = kPanelTopMargin;
constexpr int kModulationZoneTop = kHarmonicZone1Top + kHarmonicZoneHeight + kZoneGap;
constexpr int kModulationZoneHeight = kKnobRowHeight;
constexpr int kEnvelopeZoneTop = kModulationZoneTop + kModulationZoneHeight + kZoneGap;
constexpr int kEnvelopeGroupPadding = 6;
constexpr int kInnerEnvelopeGap = 8;
constexpr int kEnvelopeInnerPadding = 10;
constexpr int kEnvelopeBottomInset = 6;
constexpr int kEnvelopeFooterPadding = 18;
constexpr int kEnvelopeKnobLabelHeight = kLabelBandHeight;
constexpr int kEnvelopeKnobSize = kRotaryBodyHeight;
constexpr int kEnvelopeRowHeight = kEnvelopeKnobLabelHeight + kEnvelopeKnobSize + kEnvelopeBottomInset + 12;
constexpr int kVolumeEnvelopeRowHeight = kEnvelopeRowHeight - 8;
constexpr int kFilterEnvelopeRowHeight = kEnvelopeRowHeight + 8;
constexpr juce::uint32 kMutedLabelColour = 0xff9a948c;
constexpr int kHarmonySnapButtonWidth = 46;
constexpr int kHarmonyDebugLabelHeight = 18;
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

    envelopeControlGroup.setVisible (false);
    volumeEnvelopeGroup.setText ("VOLUME");
    volumeEnvelopeGroup.setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff6a6560));
    volumeEnvelopeGroup.setColour (juce::GroupComponent::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (volumeEnvelopeGroup);
    filterEnvelopeGroup.setText ("FILTER");
    filterEnvelopeGroup.setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff6a6560));
    filterEnvelopeGroup.setColour (juce::GroupComponent::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (filterEnvelopeGroup);

    configureRotaryKnob (harmonySlider, harmonyLabel, "HARMONY", false);

    harmonyQuantizeButton.setButtonText ("STEP");
    harmonyQuantizeButton.setClickingTogglesState (true);
    harmonyQuantizeButton.setColour (juce::ToggleButton::textColourId, juce::Colour (kMutedLabelColour));
    harmonyQuantizeButton.setColour (juce::ToggleButton::tickColourId, juce::Colour (0xffc4a574));
    harmonyQuantizeButton.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff6a6560));
    addAndMakeVisible (harmonyQuantizeButton);

    harmonyDebugValueLabel.setJustificationType (juce::Justification::centred);
    harmonyDebugValueLabel.setInterceptsMouseClicks (false, false);
    harmonyDebugValueLabel.setFont (juce::Font (juce::FontOptions { 10.0f, juce::Font::plain }));
    harmonyDebugValueLabel.setColour (juce::Label::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (harmonyDebugValueLabel);

    configureRotaryKnob (filterCutoffSlider, filterCutoffLabel, "FILTER CUTOFF", true, " Hz");
    configureRotaryKnob (filterResSlider, filterResLabel, "RESONANCE", true);
    configureRotaryKnob (gainSlider, gainLabel, "MASTER GAIN", false);
    configureRotaryKnob (glideSlider, glideLabel, "GLIDE", false);
    configureSteppedRotaryKnob (harmonicInversionSlider, harmonicInversionLabel, "INVERSION");
    configureRotaryKnob (harmonicTiltSlider, harmonicTiltLabel, "HARMONIC TILT", true);
    harmonicTiltSlider.setDoubleClickReturnValue (true, 0.0);
    configureRotaryKnob (kbTrackSlider, kbTrackLabel, "KB TRACK", false);
    configureRotaryKnob (hpCutoffSlider, hpCutoffLabel, "HP CUTOFF", true, " Hz");
    configureRotaryKnob (overloadSlider, overloadLabel, "OVERLOAD", false);

    configureAdsrKnob (envWarpSlider, envWarpLabel, "ENV WARP");
    configureAdsrKnob (egrAmountSlider, egrAmountLabel, "EGR AMOUNT");
    configureAdsrKnob (attackSlider, attackLabel, "ATTACK");
    configureAdsrKnob (decaySlider, decayLabel, "DECAY");
    configureAdsrKnob (sustainSlider, sustainLabel, "SUSTAIN");
    configureAdsrKnob (releaseSlider, releaseLabel, "RELEASE");
    configureAdsrKnob (filterAttackSlider, filterAttackLabel, "ATTACK");
    configureAdsrKnob (filterDecaySlider, filterDecayLabel, "DECAY");
    configureAdsrKnob (filterSustainSlider, filterSustainLabel, "SUSTAIN");
    configureAdsrKnob (filterReleaseSlider, filterReleaseLabel, "RELEASE");

    filterCutoffLabel.setFont (juce::Font (juce::FontOptions { 11.0f, juce::Font::bold }));
    filterCutoffLabel.setColour (juce::Label::textColourId, juce::Colour (kMutedLabelColour));
    filterResLabel.setFont (juce::Font (juce::FontOptions { 12.0f, juce::Font::bold }));
    filterResLabel.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.5f));

    auto& apvts = audioProcessor.getApvts();

    if (auto* depthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("depth")))
        if (auto* girthParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter ("girth")))
            xyExplorer.setParameters (*depthParam, *girthParam);

    harmonyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harmony", harmonySlider);
    harmonyQuantizeAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (apvts, "harmonyQuantize", harmonyQuantizeButton);

    harmonySlider.onValueChange = [this]
    {
        updateHarmonyDebugLabel();

        if (isUpdatingHarmonySnap || ! harmonyQuantizeButton.getToggleState())
            return;

        snapHarmonyParameterToNearestStep();
    };

    harmonyQuantizeButton.onClick = [this]
    {
        if (harmonyQuantizeButton.getToggleState())
            snapHarmonyParameterToNearestStep();

        updateHarmonyDebugLabel();
    };

    updateHarmonyDebugLabel();
    startTimerHz (30);

    if (harmonyQuantizeButton.getToggleState())
        snapHarmonyParameterToNearestStep();

    filterCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "cutoff", filterCutoffSlider);
    filterResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "res", filterResSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "gain", gainSlider);
    envWarpAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "envWarp", envWarpSlider);
    egrAmountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "egrAmount", egrAmountSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "attack", attackSlider);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "decay", decaySlider);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "sustain", sustainSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "release", releaseSlider);
    filterAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterAttack", filterAttackSlider);
    filterDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterDecay", filterDecaySlider);
    filterSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterSustain", filterSustainSlider);
    filterReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterRelease", filterReleaseSlider);
    glideAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "glideTime", glideSlider);
    harmonicInversionSlider.setRange (0.0, 5.0, 1.0);

    harmonicInversionSlider.textFromValueFunction = [] (double value)
    {
        static constexpr std::array<const char*, 6> kInversionLabels {
            "LINEAR",
            "SHUFFLE",
            "ODD/PRIME",
            "UNDERTONE",
            "OCTAVE",
            "BELL"
        };

        const auto index = juce::jlimit (0, 5, juce::roundToInt (value));
        return juce::String { kInversionLabels[static_cast<size_t> (index)] };
    };

    harmonicInversionAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harmonicInversion", harmonicInversionSlider);
    harmonicTiltAttachment =
        std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "harmonicTilt", harmonicTiltSlider);
    kbTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "kbTrack", kbTrackSlider);
    hpCutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "hpCutoff", hpCutoffSlider);
    overloadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterOverload", overloadSlider);
}

HydraAudioProcessorEditor::~HydraAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void HydraAudioProcessorEditor::timerCallback()
{
    updateHarmonyDebugLabel();
}

void HydraAudioProcessorEditor::updateHarmonyDebugLabel()
{
    auto* harmonyParam = audioProcessor.getApvts().getParameter ("harmony");

    if (harmonyParam == nullptr)
        return;

    const auto& range = harmonyParam->getNormalisableRange();
    const auto harmonyValue = range.convertFrom0to1 (harmonyParam->getValue());
    harmonyDebugValueLabel.setText (juce::String::formatted ("%.3f", harmonyValue),
                                    juce::dontSendNotification);
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
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }

    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
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

    updateHarmonyDebugLabel();
}

void HydraAudioProcessorEditor::configureSteppedRotaryKnob (juce::Slider& slider,
                                                             juce::Label& label,
                                                             const juce::String& labelText)
{
    slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, true, kCutoffTextBoxWidth, kCutoffTextBoxHeight);
    slider.setColour (juce::Slider::textBoxTextColourId, juce::Colours::white.withAlpha (0.6f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (labelText, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (label);
}

void HydraAudioProcessorEditor::configureAdsrKnob (juce::Slider& slider,
                                                    juce::Label& label,
                                                    const juce::String& labelText)
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

    const auto controlPanel = getLocalBounds().withHeight (kControlPanelHeight);
    const auto dividerX0 = kPanelHorizontalMargin + kZoneDividerInset;
    const auto dividerX1 = getWidth() - kPanelHorizontalMargin - kZoneDividerInset;

    const auto drawZoneDivider = [&] (int y)
    {
        g.setColour (juce::Colour (0xff2a2622));
        g.drawHorizontalLine (y, static_cast<float> (dividerX0), static_cast<float> (dividerX1));
    };

    drawZoneDivider (kModulationZoneTop - (kZoneGap / 2));
    drawZoneDivider (kEnvelopeZoneTop - (kZoneGap / 2));

    g.setColour (juce::Colour (0xff6a6560).withAlpha (0.55f));
    g.setFont (juce::Font (juce::FontOptions { 9.0f, juce::Font::bold }));
    g.drawText ("HARMONIC ENGINE",
                juce::Rectangle<int> (dividerX0, kHarmonicZone1Top, 160, 14),
                juce::Justification::centredLeft,
                true);
    g.drawText ("GLOBAL MODULATION & FILTERS",
                juce::Rectangle<int> (dividerX0, kModulationZoneTop - 2, 240, 14),
                juce::Justification::centredLeft,
                true);
    g.drawText ("ENVELOPES",
                juce::Rectangle<int> (dividerX0, kEnvelopeZoneTop - 2, 120, 14),
                juce::Justification::centredLeft,
                true);

    g.setColour (juce::Colour (0xff6a6560));
    g.setFont (juce::Font (juce::FontOptions { 10.0f }));
    g.drawText ("THE HYDRA — HARMONIC SCAFFOLD SYNTH",
                controlPanel.reduced (10).removeFromBottom (18),
                juce::Justification::bottomRight,
                true);
}

void HydraAudioProcessorEditor::resized()
{
    keyboardComponent.setBounds (0, kControlPanelHeight, kEditorWidth, kKeyboardHeight);

    auto controlPanel = getLocalBounds().withHeight (kControlPanelHeight);

    const auto harmonicZone = controlPanel.withY (kHarmonicZone1Top).withHeight (kHarmonicZoneHeight);
    const auto harmonicStackTop = harmonicZone.getY() + 14;
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
        harmonyDebugValueLabel.toFront (false);
        harmonicInversionSlider.toFront (false);
        harmonicInversionLabel.toFront (false);
        harmonicTiltSlider.toFront (false);
        harmonicTiltLabel.toFront (false);
        glideSlider.toFront (false);
        glideLabel.toFront (false);
        overloadSlider.toFront (false);
        overloadLabel.toFront (false);
        gainSlider.toFront (false);
        gainLabel.toFront (false);
    };

    {
        auto harmonyRow = leftHarmonicCluster.removeFromTop (kHarmonicSubRowHeight);
        leftHarmonicCluster.removeFromTop (kHarmonicSubRowGap);
        auto bottomRow = leftHarmonicCluster;

        auto labelRow = harmonyRow.removeFromTop (kLabelBandHeight);
        harmonyQuantizeButton.setBounds (labelRow.removeFromRight (kHarmonySnapButtonWidth).reduced (2, 4));
        harmonyLabel.setBounds (labelRow);
        harmonyRow.removeFromBottom (kHarmonyDebugLabelHeight + 4);
        harmonyDebugValueLabel.setBounds (harmonyRow.removeFromBottom (kHarmonyDebugLabelHeight));
        harmonySlider.setBounds (harmonyRow.withSizeKeepingCentre (kColumnWidth, kEnvelopeKnobSize));

        const auto bottomColumnWidth = bottomRow.getWidth() / 2;
        auto inversionColumn = bottomRow.removeFromLeft (bottomColumnWidth);
        auto tiltColumn = bottomRow;
        placeKnobColumn (inversionColumn,
                         harmonicInversionSlider,
                         harmonicInversionLabel,
                         kRotarySliderWithReadoutHeight,
                         true);
        placeKnobColumn (tiltColumn,
                         harmonicTiltSlider,
                         harmonicTiltLabel,
                         kRotarySliderWithReadoutHeight,
                         true);
    }

    {
        auto gainRow = rightHarmonicCluster.removeFromTop (kHarmonicSubRowHeight);
        rightHarmonicCluster.removeFromTop (kHarmonicSubRowGap);
        auto bottomRow = rightHarmonicCluster;

        placeKnobColumn (gainRow, gainSlider, gainLabel, kRotaryBodyHeight, true);

        const auto bottomColumnWidth = bottomRow.getWidth() / 2;
        auto glideColumn = bottomRow.removeFromLeft (bottomColumnWidth);
        auto overloadColumn = bottomRow;
        placeKnobColumn (glideColumn, glideSlider, glideLabel, kRotaryBodyHeight, true);
        placeKnobColumn (overloadColumn, overloadSlider, overloadLabel, kRotaryBodyHeight, true);
    }

    bringHarmonicControlsToFront();

    const auto modulationRow = controlPanel.withY (kModulationZoneTop + 12).withHeight (kModulationZoneHeight);
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
    placeModulationColumn (2, kbTrackSlider, kbTrackLabel, kRotaryBodyHeight);
    placeModulationColumn (3, hpCutoffSlider, hpCutoffLabel, kRotarySliderWithReadoutHeight);
    placeModulationColumn (4, egrAmountSlider, egrAmountLabel, kRotaryBodyHeight);
    placeModulationColumn (5, envWarpSlider, envWarpLabel, kRotaryBodyHeight);

    const auto envelopeAreaTop = kEnvelopeZoneTop + 8;
    const auto envelopeAreaBottom = kControlPanelHeight - (kPanelBottomMargin + kEnvelopeFooterPadding);
    auto envelopeArea = juce::Rectangle<int>::leftTopRightBottom (kPanelHorizontalMargin,
                                                                  envelopeAreaTop,
                                                                  kEditorWidth - kPanelHorizontalMargin,
                                                                  envelopeAreaBottom)
                            .reduced (4, 4);

    volumeEnvelopeGroup.toBack();
    filterEnvelopeGroup.toBack();

    const auto envelopeStackHeight = kVolumeEnvelopeRowHeight + kInnerEnvelopeGap + kFilterEnvelopeRowHeight;
    auto envelopeStack = envelopeArea.withHeight (envelopeStackHeight)
                                      .withCentre (envelopeArea.getCentre());
    auto volumeBoxBounds = envelopeStack.removeFromTop (kVolumeEnvelopeRowHeight);
    envelopeStack.removeFromTop (kInnerEnvelopeGap);
    auto filterBoxBounds = envelopeStack;

    volumeEnvelopeGroup.setBounds (volumeBoxBounds);
    filterEnvelopeGroup.setBounds (filterBoxBounds);

    const auto placeRowInGroup = [] (juce::Rectangle<int> groupBounds,
                                     juce::Slider& s1, juce::Label& l1,
                                     juce::Slider& s2, juce::Label& l2,
                                     juce::Slider& s3, juce::Label& l3,
                                     juce::Slider& s4, juce::Label& l4)
    {
        auto row = groupBounds.reduced (kEnvelopeInnerPadding, 14);
        const int columnWidth = row.getWidth() / 4;

        auto placeLocalColumn = [columnWidth] (juce::Rectangle<int>& r, juce::Slider& slider, juce::Label& label)
        {
            auto col = r.removeFromLeft (columnWidth);
            label.setBounds (col.removeFromTop (kEnvelopeKnobLabelHeight));
            col.removeFromBottom (kEnvelopeBottomInset);
            slider.setBounds (juce::Rectangle<int> (kEnvelopeKnobSize, kEnvelopeKnobSize).withCentre (col.getCentre()));
        };

        placeLocalColumn (row, s1, l1);
        placeLocalColumn (row, s2, l2);
        placeLocalColumn (row, s3, l3);
        placeLocalColumn (row, s4, l4);
    };

    placeRowInGroup (volumeBoxBounds, attackSlider, attackLabel, decaySlider, decayLabel, sustainSlider, sustainLabel, releaseSlider, releaseLabel);
    placeRowInGroup (filterBoxBounds, filterAttackSlider, filterAttackLabel, filterDecaySlider, filterDecayLabel, filterSustainSlider, filterSustainLabel, filterReleaseSlider, filterReleaseLabel);
}
