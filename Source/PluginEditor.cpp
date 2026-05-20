#include "PluginEditor.h"

namespace
{
constexpr int kEditorWidth = 750;
constexpr int kControlPanelHeight = 560;
constexpr int kEditorHeight = 630;
constexpr int kKeyboardHeight = 70;
constexpr int kColumnWidth = 125;
constexpr int kLabelBandHeight = 24;
constexpr int kCutoffTextBoxWidth = 70;
constexpr int kCutoffTextBoxHeight = 18;
constexpr int kControlColumnBottomInset = 22;
constexpr int kRotaryBodyHeight = 84;
constexpr int kRotarySliderWithReadoutHeight = kRotaryBodyHeight + kCutoffTextBoxHeight;
constexpr int kTopKnobRowHeight = kLabelBandHeight + kRotarySliderWithReadoutHeight + kControlColumnBottomInset;
constexpr int kBottomKnobRowHeight = kLabelBandHeight + kRotaryBodyHeight + kControlColumnBottomInset;
constexpr int kPanelHorizontalMargin = 10;
constexpr int kPanelBottomMargin = 10;
constexpr int kXyPadY = 15;
constexpr int kXyPadSize = 220;
constexpr int kXyPadRightMargin = 10;
constexpr int kUtilityKnobRowGap = 6;
constexpr int kUtilityKnobRowTop = kTopKnobRowHeight + kUtilityKnobRowGap;
constexpr int kUtilityKnobRowHeight = kBottomKnobRowHeight;
constexpr int kEnvelopeSectionTop = kUtilityKnobRowTop + kUtilityKnobRowHeight + kUtilityKnobRowGap;
constexpr int kEnvWarpColumnX = 15;
constexpr int kEnvelopeGroupPadding = 6;
constexpr int kInnerEnvelopeGap = 8;
constexpr int kEnvelopeInnerPadding = 10;
constexpr int kEnvelopeBottomInset = 6;
constexpr int kEnvelopeFooterPadding = 22;
constexpr int kEnvelopeKnobLabelHeight = 20;
constexpr int kEnvelopeKnobMinSize = 72;
constexpr int kHarmonyColumnX = 15 + kXyPadSize + kXyPadRightMargin;
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

    envelopeControlGroup.setText ("ENVELOPE CONTROL");
    envelopeControlGroup.setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff6a6560));
    envelopeControlGroup.setColour (juce::GroupComponent::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (envelopeControlGroup);
    volumeEnvelopeGroup.setText ("VOLUME");
    volumeEnvelopeGroup.setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff6a6560));
    volumeEnvelopeGroup.setColour (juce::GroupComponent::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (volumeEnvelopeGroup);
    filterEnvelopeGroup.setText ("FILTER");
    filterEnvelopeGroup.setColour (juce::GroupComponent::outlineColourId, juce::Colour (0xff6a6560));
    filterEnvelopeGroup.setColour (juce::GroupComponent::textColourId, juce::Colour (kMutedLabelColour));
    addAndMakeVisible (filterEnvelopeGroup);

    configureRotaryKnob (harmonySlider, harmonyLabel, "HARMONY", false);
    configureRotaryKnob (filterCutoffSlider, filterCutoffLabel, "FILTER CUTOFF", true, " Hz");
    configureRotaryKnob (filterResSlider, filterResLabel, "RESONANCE", true);
    configureRotaryKnob (gainSlider, gainLabel, "MASTER GAIN", false);
    configureRotaryKnob (glideSlider, glideLabel, "GLIDE TIME", false);
    configureRotaryKnob (scaleMorphSlider, scaleMorphLabel, "SCALE MORPH", false);
    configureRotaryKnob (kbTrackSlider, kbTrackLabel, "KB TRACKING", false);
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
    scaleMorphAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "scaleMorph", scaleMorphSlider);
    kbTrackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "kbTrack", kbTrackSlider);
    overloadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (apvts, "filterOverload", overloadSlider);
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
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    }

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
    xyExplorer.setBounds (15, kXyPadY, kXyPadSize, kXyPadSize);

    auto controlPanel = getLocalBounds().withHeight (kControlPanelHeight);
    const auto topKnobRow = controlPanel.withHeight (kTopKnobRowHeight);

    const auto envelopeGroupTop = kEnvelopeSectionTop - kEnvelopeGroupPadding;
    const auto envelopeGroupBottom = juce::jmin (kControlPanelHeight - (kPanelBottomMargin + kEnvelopeFooterPadding),
                                                 kControlPanelHeight - (kPanelBottomMargin + kEnvelopeFooterPadding));
    const auto envelopeGroupBounds = juce::Rectangle<int>::leftTopRightBottom (kPanelHorizontalMargin,
                                                                               envelopeGroupTop,
                                                                               kEditorWidth - kPanelHorizontalMargin,
                                                                               envelopeGroupBottom);
    envelopeControlGroup.setBounds (envelopeGroupBounds);
    envelopeControlGroup.toBack();
    volumeEnvelopeGroup.toBack();
    filterEnvelopeGroup.toBack();

    const auto placeKnobColumn = [] (juce::Rectangle<int> rowBand,
                                     int x,
                                     juce::Slider& slider,
                                     juce::Label& label,
                                     int sliderHeight)
    {
        auto column = rowBand.withX (x).withWidth (kColumnWidth);
        label.setBounds (column.removeFromTop (kLabelBandHeight));
        column.removeFromBottom (kControlColumnBottomInset);
        slider.setBounds (column.removeFromTop (juce::jmin (sliderHeight, column.getHeight())));
    };

    placeKnobColumn (topKnobRow, kHarmonyColumnX, harmonySlider, harmonyLabel, kRotaryBodyHeight);
    placeKnobColumn (topKnobRow, kCutoffColumnX, filterCutoffSlider, filterCutoffLabel, kRotarySliderWithReadoutHeight);
    placeKnobColumn (topKnobRow, kResColumnX, filterResSlider, filterResLabel, kRotarySliderWithReadoutHeight);
    placeKnobColumn (topKnobRow, kGainColumnX, gainSlider, gainLabel, kRotaryBodyHeight);

    const auto utilityKnobRow = controlPanel.withY (kUtilityKnobRowTop)
                                               .withHeight (kUtilityKnobRowHeight);
    placeKnobColumn (utilityKnobRow, kHarmonyColumnX, glideSlider, glideLabel, kRotaryBodyHeight);
    placeKnobColumn (utilityKnobRow, kCutoffColumnX, scaleMorphSlider, scaleMorphLabel, kRotaryBodyHeight);
    placeKnobColumn (utilityKnobRow, kResColumnX, kbTrackSlider, kbTrackLabel, kRotaryBodyHeight);
    placeKnobColumn (utilityKnobRow, kGainColumnX, overloadSlider, overloadLabel, kRotaryBodyHeight);

    auto envelopeArea = envelopeGroupBounds.reduced (10, 16);
    constexpr int kEnvelopeGridColumns = 5;
    const int pillarWidth = envelopeArea.getWidth() / kEnvelopeGridColumns;
    auto leftPillar = envelopeArea.removeFromLeft (pillarWidth);
    auto envWarpCell = leftPillar.removeFromTop ((leftPillar.getHeight() - kInnerEnvelopeGap) / 2);
    leftPillar.removeFromTop (kInnerEnvelopeGap);
    auto egrCell = leftPillar;

    envWarpLabel.setBounds (envWarpCell.removeFromTop (kLabelBandHeight));
    envWarpSlider.setBounds (envWarpCell.reduced (0, 2));
    egrAmountLabel.setBounds (egrCell.removeFromTop (kLabelBandHeight));
    egrAmountSlider.setBounds (egrCell.reduced (0, 2));

    envelopeArea.removeFromLeft (6);

    const auto totalInnerHeight = envelopeArea.getHeight();
    const auto eachBoxHeight = (totalInnerHeight - kInnerEnvelopeGap) / 2;
    auto volumeBoxBounds = envelopeArea.removeFromTop (eachBoxHeight);
    envelopeArea.removeFromTop (kInnerEnvelopeGap);
    auto filterBoxBounds = envelopeArea.removeFromTop (eachBoxHeight);

    volumeEnvelopeGroup.setBounds (volumeBoxBounds);
    filterEnvelopeGroup.setBounds (filterBoxBounds);

    const auto placeRowInGroup = [] (juce::Rectangle<int> groupBounds,
                                     juce::Slider& s1, juce::Label& l1,
                                     juce::Slider& s2, juce::Label& l2,
                                     juce::Slider& s3, juce::Label& l3,
                                     juce::Slider& s4, juce::Label& l4)
    {
        auto row = groupBounds.reduced (kEnvelopeInnerPadding, 18);
        const int columnWidth = row.getWidth() / 4;

        auto placeLocalColumn = [columnWidth] (juce::Rectangle<int>& r, juce::Slider& slider, juce::Label& label)
        {
            auto col = r.removeFromLeft (columnWidth);
            label.setBounds (col.removeFromTop (kEnvelopeKnobLabelHeight));
            col.removeFromBottom (kEnvelopeBottomInset);
            auto knobArea = col.reduced (4, 0);
            const auto knobSize = juce::jmax (kEnvelopeKnobMinSize, juce::jmin (knobArea.getWidth(), knobArea.getHeight()));
            slider.setBounds (juce::Rectangle<int> (knobSize, knobSize).withCentre (knobArea.getCentre()));
        };

        placeLocalColumn (row, s1, l1);
        placeLocalColumn (row, s2, l2);
        placeLocalColumn (row, s3, l3);
        placeLocalColumn (row, s4, l4);
    };

    placeRowInGroup (volumeBoxBounds, attackSlider, attackLabel, decaySlider, decayLabel, sustainSlider, sustainLabel, releaseSlider, releaseLabel);
    placeRowInGroup (filterBoxBounds, filterAttackSlider, filterAttackLabel, filterDecaySlider, filterDecayLabel, filterSustainSlider, filterSustainLabel, filterReleaseSlider, filterReleaseLabel);
}
