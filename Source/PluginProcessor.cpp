#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* kDepthParamId = "depth";
constexpr const char* kGirthParamId = "girth";
constexpr const char* kGainParamId = "gain";
constexpr const char* kCutoffParamId = "cutoff";
constexpr const char* kResParamId = "res";
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout HydraAudioProcessor::createParameterLayout()
{
    juce::NormalisableRange<float> cutoffRange (20.0f, 20000.0f, 0.0f, 0.2f);

    return {
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kDepthParamId, 1 },
                                                     "Intelligent Depth",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.2f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGirthParamId, 1 },
                                                     "Spatial Girth",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGainParamId, 1 },
                                                     "Master Gain",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.5f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kCutoffParamId, 1 },
                                                     "Filter Cutoff",
                                                     cutoffRange,
                                                     20000.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kResParamId, 1 },
                                                     "Resonance",
                                                     juce::NormalisableRange<float> { 0.0f, 4.0f },
                                                     1.0f)
    };
}

HydraAudioProcessor::HydraAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2,
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    depthParam = apvts.getRawParameterValue (kDepthParamId);
    girthParam = apvts.getRawParameterValue (kGirthParamId);
    gainParam = apvts.getRawParameterValue (kGainParamId);
    cutoffParam = apvts.getRawParameterValue (kCutoffParamId);
    resParam = apvts.getRawParameterValue (kResParamId);
}

HydraAudioProcessor::~HydraAudioProcessor() {}

void HydraAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));
    const auto oversamplingFactor = static_cast<double> (oversampler->getOversamplingFactor());
    oversampledSampleRate = sampleRate * oversamplingFactor;

    const auto oversampledBlockSize = static_cast<int> (samplesPerBlock * oversampler->getOversamplingFactor());
    hydraEngine.prepare (oversampledSampleRate, oversampledBlockSize);

    filterL.reset();
    filterR.reset();
    monoRightScratch.resize (static_cast<size_t> (oversampledBlockSize));
}

void HydraAudioProcessor::releaseResources()
{
    monoRightScratch.clear();

    if (oversampler != nullptr)
        oversampler->reset();
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

    hydraEngine.setDepth (depth);
    hydraEngine.setGirth (girth);
    hydraEngine.setFilterCutoff (cutoffParam->load());

    keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

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

    const auto resonance = resParam->load();

    juce::dsp::AudioBlock<float> inputBlock (buffer);
    auto oversampledBlock = oversampler->processSamplesUp (inputBlock);

    const auto osNumSamples = static_cast<int> (oversampledBlock.getNumSamples());

    if (oversampledBlock.getNumChannels() > 1)
    {
        auto* leftChannel = oversampledBlock.getChannelPointer (0);
        auto* rightChannel = oversampledBlock.getChannelPointer (1);

        hydraEngine.renderBlock (leftChannel, rightChannel, osNumSamples);

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            leftChannel[sample] = filterL.processSample (leftChannel[sample],
                                                           cutoffParam->load(),
                                                           resonance,
                                                           oversampledSampleRate);
            rightChannel[sample] = filterR.processSample (rightChannel[sample],
                                                            cutoffParam->load(),
                                                            resonance,
                                                            oversampledSampleRate);
        }

        if (hydraEngine.getVoiceAmplitude() == 0.0f)
        {
            filterL.reset();
            filterR.reset();
        }
    }
    else
    {
        auto* leftChannel = oversampledBlock.getChannelPointer (0);

        if (static_cast<int> (monoRightScratch.size()) < osNumSamples)
            monoRightScratch.resize (static_cast<size_t> (osNumSamples));

        hydraEngine.renderBlock (leftChannel, monoRightScratch.data(), osNumSamples);

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            const auto monoSample = 0.5f * (leftChannel[sample] + monoRightScratch[static_cast<size_t> (sample)]);
            leftChannel[sample] = filterL.processSample (monoSample,
                                                           cutoffParam->load(),
                                                           resonance,
                                                           oversampledSampleRate);
        }

        if (hydraEngine.getVoiceAmplitude() == 0.0f)
            filterL.reset();
    }

    oversampler->processSamplesDown (inputBlock);

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
