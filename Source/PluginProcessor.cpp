#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* kDepthParamId = "depth";
constexpr const char* kGirthParamId = "girth";
constexpr const char* kMorphParamId = "morph";
constexpr const char* kGainParamId = "gain";
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout HydraAudioProcessor::createParameterLayout()
{
    return {
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kDepthParamId, 1 },
                                                     "Intelligent Depth",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.2f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGirthParamId, 1 },
                                                     "Spatial Girth",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kMorphParamId, 1 },
                                                     "Wave Morph",
                                                     juce::NormalisableRange<float> { 0.0f, 3.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGainParamId, 1 },
                                                     "Master Gain",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.5f)
    };
}

HydraAudioProcessor::HydraAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    depthParam = apvts.getRawParameterValue (kDepthParamId);
    girthParam = apvts.getRawParameterValue (kGirthParamId);
    morphParam = apvts.getRawParameterValue (kMorphParamId);
    gainParam = apvts.getRawParameterValue (kGainParamId);
}

HydraAudioProcessor::~HydraAudioProcessor() {}

void HydraAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    hydraEngine.prepare (sampleRate, samplesPerBlock);
    monoRightScratch.resize (static_cast<size_t> (samplesPerBlock));
}

void HydraAudioProcessor::releaseResources()
{
    monoRightScratch.clear();
}

bool HydraAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void HydraAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto channel = getTotalNumInputChannels(); channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    buffer.clear();

    const auto depth = depthParam->load();
    const auto girth = girthParam->load();
    const auto morph = morphParam->load();

    hydraEngine.setDepth (depth);
    hydraEngine.setGirth (girth);
    hydraEngine.setMorph (morph);

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            hydraEngine.noteOn (message.getNoteNumber(), message.getFloatVelocity());
        }
        else if (message.isNoteOff())
        {
            hydraEngine.noteOff (message.getNoteNumber());
        }
    }

    const auto numSamples = buffer.getNumSamples();
    auto* leftChannel = buffer.getWritePointer (0);

    if (buffer.getNumChannels() > 1)
    {
        auto* rightChannel = buffer.getWritePointer (1);
        hydraEngine.renderBlock (leftChannel, rightChannel, numSamples);
    }
    else
    {
        if (static_cast<int> (monoRightScratch.size()) < numSamples)
            monoRightScratch.resize (static_cast<size_t> (numSamples));

        hydraEngine.renderBlock (leftChannel, monoRightScratch.data(), numSamples);

        for (int sample = 0; sample < numSamples; ++sample)
            leftChannel[sample] = 0.5f * (leftChannel[sample] + monoRightScratch[static_cast<size_t> (sample)]);
    }

    const auto gain = gainParam->load();
    buffer.applyGain (gain);
}

void HydraAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto state = apvts.copyState();
    const std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HydraAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

void HydraAudioProcessor::setCurrentProgram (int) {}
const juce::String HydraAudioProcessor::getProgramName (int) { return {}; }
void HydraAudioProcessor::changeProgramName (int, const juce::String&) {}

juce::AudioProcessorEditor* HydraAudioProcessor::createEditor()
{
    return new HydraAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HydraAudioProcessor();
}
