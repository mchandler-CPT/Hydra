#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <memory>

class HydraAudioProcessor;

class XyExplorer : public juce::Component,
                   private juce::Timer
{
public:
    explicit XyExplorer (HydraAudioProcessor& processor);
    ~XyExplorer() override;

    void setParameters (juce::RangedAudioParameter& depthParam, juce::RangedAudioParameter& girthParam);

    void paint (juce::Graphics& g) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent& event) override;
    void mouseDrag (const juce::MouseEvent& event) override;

private:
    void timerCallback() override;
    void updateParametersFromMouse (juce::Point<float> mousePos);

    HydraAudioProcessor& audioProcessor;

    std::unique_ptr<juce::ParameterAttachment> xAttachment;
    std::unique_ptr<juce::ParameterAttachment> yAttachment;

    float normalizedX { 0.2f };
    float normalizedY { 0.0f };
    float waveformPeakSmoothed { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (XyExplorer)
};
