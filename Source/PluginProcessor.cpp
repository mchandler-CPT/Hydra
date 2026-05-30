#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* kDepthParamId = "depth";
constexpr const char* kGirthParamId = "girth";
constexpr const char* kHarmonyParamId = "harmony";
constexpr const char* kGainParamId = "gain";
constexpr const char* kCutoffParamId = "cutoff";
constexpr const char* kResParamId = "res";
constexpr const char* kAttackParamId = "attack";
constexpr const char* kDecayParamId = "decay";
constexpr const char* kSustainParamId = "sustain";
constexpr const char* kReleaseParamId = "release";
constexpr const char* kFilterAttackParamId = "filterAttack";
constexpr const char* kFilterDecayParamId = "filterDecay";
constexpr const char* kFilterSustainParamId = "filterSustain";
constexpr const char* kFilterReleaseParamId = "filterRelease";
constexpr const char* kEgrAmountParamId = "egrAmount";
constexpr const char* kEnvWarpParamId = "envWarp";
constexpr const char* kGlideTimeParamId = "glideTime";
constexpr const char* kTiltParamId = "tilt";
constexpr const char* kInversionParamId = "inversion";
constexpr const char* kHpCutoffParamId = "hpCutoff";
constexpr const char* kKbTrackParamId = "kbTrack";
constexpr const char* kFilterOverloadParamId = "filterOverload";
constexpr const char* kHarmonyQuantizeParamId = "harmonyQuantize";

int readHarmonicInversionIndex (const juce::AudioProcessorValueTreeState& apvts) noexcept
{
    if (auto* param = dynamic_cast<juce::AudioParameterInt*> (apvts.getParameter (kInversionParamId)))
        return param->get();

    return juce::jlimit (0, 5, juce::roundToInt (apvts.getRawParameterValue (kInversionParamId)->load()));
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout HydraAudioProcessor::createParameterLayout()
{
    juce::NormalisableRange<float> cutoffRange (20.0f, 21000.0f, 0.0f, 0.2f);
    juce::NormalisableRange<float> attackRange (0.001f, 5.0f, 0.0f, 0.5f);
    juce::NormalisableRange<float> decayRange (0.01f, 5.0f, 0.0f, 0.5f);
    juce::NormalisableRange<float> releaseRange (0.01f, 10.0f, 0.0f, 0.5f);
    juce::NormalisableRange<float> hpCutoffRange (20.0f, 2000.0f, 0.0f, 0.35f);

    return {
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kDepthParamId, 1 },
                                                     "Harmonic Bloom (X)",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.2f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGirthParamId, 1 },
                                                     "Spatial Spread (Y)",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kHarmonyParamId, 1 },
                                                     "Harmony",
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
                                                     1.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kAttackParamId, 1 },
                                                     "Attack",
                                                     attackRange,
                                                     0.1f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kDecayParamId, 1 },
                                                     "Decay",
                                                     decayRange,
                                                     0.3f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kSustainParamId, 1 },
                                                     "Sustain",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.8f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kReleaseParamId, 1 },
                                                     "Release",
                                                     releaseRange,
                                                     0.5f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kFilterAttackParamId, 1 },
                                                     "Filter Attack",
                                                     juce::NormalisableRange<float> { 0.001f, 5.0f, 0.0f, 0.5f },
                                                     0.1f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kFilterDecayParamId, 1 },
                                                     "Filter Decay",
                                                     juce::NormalisableRange<float> { 0.001f, 5.0f, 0.0f, 0.5f },
                                                     0.3f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kFilterSustainParamId, 1 },
                                                     "Filter Sustain",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f },
                                                     0.7f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kFilterReleaseParamId, 1 },
                                                     "Filter Release",
                                                     juce::NormalisableRange<float> { 0.001f, 5.0f, 0.0f, 0.5f },
                                                     0.5f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kEgrAmountParamId, 1 },
                                                     "EGR Amount",
                                                     juce::NormalisableRange<float> { -1.0f, 1.0f, 0.01f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kEnvWarpParamId, 1 },
                                                     "Env Warp",
                                                     juce::NormalisableRange<float> { -1.0f, 1.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kGlideTimeParamId, 1 },
                                                     "Glide Time",
                                                     juce::NormalisableRange<float> { 0.0f, 2.0f, 0.001f },
                                                     0.05f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kTiltParamId, 1 },
                                                     "Harmonic Tilt",
                                                     juce::NormalisableRange<float> { -1.0f, 1.0f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterInt> (juce::ParameterID { kInversionParamId, 1 },
                                                   "Inversion",
                                                   0,
                                                   5,
                                                   0),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kHpCutoffParamId, 1 },
                                                     "HP Cutoff",
                                                     hpCutoffRange,
                                                     35.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kKbTrackParamId, 1 },
                                                     "KB Tracking",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { kFilterOverloadParamId, 1 },
                                                     "Overload",
                                                     juce::NormalisableRange<float> { 0.0f, 1.0f, 0.01f },
                                                     0.0f),
        std::make_unique<juce::AudioParameterBool> (juce::ParameterID { kHarmonyQuantizeParamId, 1 },
                                                    "Harmony Stepped",
                                                    false)
    };
}

HydraAudioProcessor::HydraAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        2,
        2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    depthParam = apvts.getRawParameterValue (kDepthParamId);
    girthParam = apvts.getRawParameterValue (kGirthParamId);
    harmonyParam = apvts.getRawParameterValue (kHarmonyParamId);
    gainParam = apvts.getRawParameterValue (kGainParamId);
    cutoffParam = apvts.getRawParameterValue (kCutoffParamId);
    resParam = apvts.getRawParameterValue (kResParamId);
    attackParam = apvts.getRawParameterValue (kAttackParamId);
    decayParam = apvts.getRawParameterValue (kDecayParamId);
    sustainParam = apvts.getRawParameterValue (kSustainParamId);
    releaseParam = apvts.getRawParameterValue (kReleaseParamId);
    filterAttackParam = apvts.getRawParameterValue (kFilterAttackParamId);
    filterDecayParam = apvts.getRawParameterValue (kFilterDecayParamId);
    filterSustainParam = apvts.getRawParameterValue (kFilterSustainParamId);
    filterReleaseParam = apvts.getRawParameterValue (kFilterReleaseParamId);
    egrAmountParam = apvts.getRawParameterValue (kEgrAmountParamId);
    envWarpParam = apvts.getRawParameterValue (kEnvWarpParamId);
    glideTimeParam = apvts.getRawParameterValue (kGlideTimeParamId);
    harmonicTiltParam = apvts.getRawParameterValue (kTiltParamId);
    harmonicInversionParam = apvts.getRawParameterValue (kInversionParamId);
    hpCutoffParam = apvts.getRawParameterValue (kHpCutoffParamId);
    kbTrackParam = apvts.getRawParameterValue (kKbTrackParamId);
    filterOverloadParam = apvts.getRawParameterValue (kFilterOverloadParamId);
    harmonyQuantizeParam = apvts.getRawParameterValue (kHarmonyQuantizeParamId);
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

    constexpr auto parameterSmoothingSeconds = 0.02;

    juce::dsp::ProcessSpec hpFilterSpec;
    hpFilterSpec.sampleRate = oversampledSampleRate;
    hpFilterSpec.maximumBlockSize = static_cast<juce::uint32> (oversampledBlockSize);
    hpFilterSpec.numChannels = 1;

    mHpFilterL.prepare (hpFilterSpec);
    mHpFilterR.prepare (hpFilterSpec);
    mHpFilterL.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    mHpFilterR.setType (juce::dsp::StateVariableTPTFilterType::highpass);
    mHpFilterL.reset();
    mHpFilterR.reset();

    mSmoothedHpCutoff.reset (oversampledSampleRate, parameterSmoothingSeconds);
    mSmoothedHpCutoff.setCurrentAndTargetValue (hpCutoffParam->load());

    hydraEngine.setHarmonicTiltTarget (harmonicTiltParam->load());
    hydraEngine.setHarmonicInversionIndexTarget (readHarmonicInversionIndex (apvts));

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
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
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
    const auto harmony = harmonyParam->load();
    const auto kbTrack = kbTrackParam->load();
    const auto filterOverload = filterOverloadParam->load();
    const auto harmonyQuantize = harmonyQuantizeParam->load() > 0.5f;

    hydraEngine.setDepth (depth);
    hydraEngine.setGirth (girth);
    hydraEngine.setHarmony (harmony);
    hydraEngine.setHarmonyQuantize (harmonyQuantize);
    hydraEngine.setFilterCutoff (cutoffParam->load());
    hydraEngine.setEnvelopeParameters (attackParam->load(),
                                       decayParam->load(),
                                       sustainParam->load(),
                                       releaseParam->load());
    hydraEngine.setFilterEnvelopeParameters (filterAttackParam->load(),
                                             filterDecayParam->load(),
                                             filterSustainParam->load(),
                                             filterReleaseParam->load());
    hydraEngine.setEgrAmount (egrAmountParam->load());
    hydraEngine.setEnvWarp (envWarpParam->load());
    hydraEngine.setGlideTime (glideTimeParam->load());
    hydraEngine.setKbTrack (kbTrack);
    hydraEngine.setHarmonicTiltTarget (harmonicTiltParam->load());
    hydraEngine.setHarmonicInversionIndexTarget (readHarmonicInversionIndex (apvts));
    hydraEngine.setFilterOverload (filterOverload);

    mSmoothedHpCutoff.setTargetValue (hpCutoffParam->load());

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
        const auto* modulatedCutoffL = hydraEngine.getFilterCutoffBuffer();
        const auto* modulatedCutoffR = hydraEngine.getFilterCutoffBufferR();

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            const auto fallbackCutoff = cutoffParam->load();
            const auto cutoffL = modulatedCutoffL != nullptr ? modulatedCutoffL[sample] : fallbackCutoff;
            const auto cutoffR = modulatedCutoffR != nullptr ? modulatedCutoffR[sample] : fallbackCutoff;
            const auto warpedCutoffL = clampLowPassCutoffHz (cutoffL, oversampledSampleRate);
            const auto warpedCutoffR = clampLowPassCutoffHz (cutoffR, oversampledSampleRate);
            leftChannel[sample] = filterL.processSample (leftChannel[sample],
                                                           warpedCutoffL,
                                                           resonance,
                                                           oversampledSampleRate);
            rightChannel[sample] = filterR.processSample (rightChannel[sample],
                                                            warpedCutoffR,
                                                            resonance,
                                                            oversampledSampleRate);
        }

        hydraEngine.applyFilterOverload (leftChannel, rightChannel, osNumSamples);

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            const auto hpCutoffHz = mSmoothedHpCutoff.getNextValue();
            mHpFilterL.setCutoffFrequency (hpCutoffHz);
            mHpFilterR.setCutoffFrequency (hpCutoffHz);
            leftChannel[sample] = mHpFilterL.processSample (0, leftChannel[sample]);
            rightChannel[sample] = mHpFilterR.processSample (0, rightChannel[sample]);
        }

        if (hydraEngine.getVoiceAmplitude() == 0.0f)
        {
            filterL.reset();
            filterR.reset();
            mHpFilterL.reset();
            mHpFilterR.reset();
        }
    }
    else
    {
        auto* leftChannel = oversampledBlock.getChannelPointer (0);

        if (static_cast<int> (monoRightScratch.size()) < osNumSamples)
            monoRightScratch.resize (static_cast<size_t> (osNumSamples));

        hydraEngine.renderBlock (leftChannel, monoRightScratch.data(), osNumSamples);
        const auto* modulatedCutoff = hydraEngine.getFilterCutoffBuffer();

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            const auto monoSample = 0.5f * (leftChannel[sample] + monoRightScratch[static_cast<size_t> (sample)]);
            const auto cutoffHz = modulatedCutoff != nullptr ? modulatedCutoff[sample] : cutoffParam->load();
            const auto warpedCutoff = clampLowPassCutoffHz (cutoffHz, oversampledSampleRate);
            leftChannel[sample] = filterL.processSample (monoSample,
                                                           warpedCutoff,
                                                           resonance,
                                                           oversampledSampleRate);
        }

        hydraEngine.applyFilterOverload (leftChannel, nullptr, osNumSamples);

        for (int sample = 0; sample < osNumSamples; ++sample)
        {
            const auto hpCutoffHz = mSmoothedHpCutoff.getNextValue();
            mHpFilterL.setCutoffFrequency (hpCutoffHz);
            leftChannel[sample] = mHpFilterL.processSample (0, leftChannel[sample]);
        }

        if (hydraEngine.getVoiceAmplitude() == 0.0f)
        {
            filterL.reset();
            mHpFilterL.reset();
        }
    }

    oversampler->processSamplesDown (inputBlock);

    const auto gain = gainParam->load();
    buffer.applyGain (gain);

    const auto* leftChannelData = buffer.getReadPointer (0);
    const auto* rightChannelData = buffer.getNumChannels() > 1 ? buffer.getReadPointer (1) : leftChannelData;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
        pushVisualSample (leftChannelData[i], rightChannelData[i]);
}

void HydraAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto stateTree = apvts.copyState();
    stateTree.setProperty ("metaLoadedPresetName", presetManager.getCurrentPresetName(), nullptr);

    if (auto xml = stateTree.createXml())
        copyXmlToBinary (*xml, destData);
}

void HydraAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (data == nullptr || sizeInBytes <= 0)
        return;

    const std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        const auto restoredTree = juce::ValueTree::fromXml (*xml);
        apvts.replaceState (restoredTree);

        if (restoredTree.hasProperty ("metaLoadedPresetName"))
            presetManager.restorePresetNameFromSession (restoredTree.getProperty ("metaLoadedPresetName").toString());
    }
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
