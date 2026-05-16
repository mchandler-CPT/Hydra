#include "PluginProcessor.h"
#include "PluginEditor.h"

BoutiqueTemplateAudioProcessor::BoutiqueTemplateAudioProcessor()
    : AudioProcessor (BusesProperties().withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{}

BoutiqueTemplateAudioProcessor::~BoutiqueTemplateAudioProcessor() {}

const juce::String BoutiqueTemplateAudioProcessor::getName() const { return JucePlugin_Name; }

// Match these to your CMake configuration (Defaults to Synth/MIDI enabled)
bool BoutiqueTemplateAudioProcessor::acceptsMidi() const { return true; }
bool BoutiqueTemplateAudioProcessor::producesMidi() const { return false; }
bool BoutiqueTemplateAudioProcessor::isMidiEffect() const { return false; }
double BoutiqueTemplateAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int BoutiqueTemplateAudioProcessor::getNumPrograms() { return 1; }
int BoutiqueTemplateAudioProcessor::getCurrentProgram() { return 0; }
void BoutiqueTemplateAudioProcessor::setCurrentProgram (int index) {}
const juce::String BoutiqueTemplateAudioProcessor::getProgramName (int index) { return {}; }
void BoutiqueTemplateAudioProcessor::changeProgramName (int index, const juce::String& newName) {}

void BoutiqueTemplateAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock) {}
void BoutiqueTemplateAudioProcessor::releaseResources() {}

bool BoutiqueTemplateAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutput() != juce::AudioChannelSet::mono()
     && layouts.getMainOutput() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutput() != layouts.getMainInput())
        return false;

    return true;
}

void BoutiqueTemplateAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Basic pass-through processing template loop
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        juce::ignoreUnused (channelData);
    }
}

bool BoutiqueTemplateAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* BoutiqueTemplateAudioProcessor::createEditor() { return new BoutiqueTemplateAudioProcessorEditor (*this); }

void BoutiqueTemplateAudioProcessor::getStateInformation (juce::MemoryBlock& destData) {}
void BoutiqueTemplateAudioProcessor::setStateInformation (const void* data, int sizeInBytes) {}

//==============================================================================
// Factory instance function targeting the generic template class name
//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BoutiqueTemplateAudioProcessor();
}