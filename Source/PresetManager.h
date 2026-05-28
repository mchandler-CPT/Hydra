#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& stateToControl,
                   const juce::String& pluginName,
                   const juce::String& companyName,
                   const juce::String& fileExtension);
    ~PresetManager();

    void updatePresetList();
    bool loadPreset (int index);
    bool loadNextPreset();
    bool loadPreviousPreset();
    bool savePreset (const juce::String& presetName);
    void saveCurrentPresetToFile (juce::File file);

    bool setCustomPresetDirectory (const juce::File& directory);
    juce::File getCurrentPresetDirectory() const { return currentPresetDir; }
    juce::File getDefaultPresetDirectory() const;

    juce::String getCurrentPresetName() const { return loadedPresetName; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }
    const juce::StringArray& getPresetNames() const { return presetNames; }

    void restorePresetNameFromSession (const juce::String& name);

private:
    juce::AudioProcessorValueTreeState& apvts;

    juce::String appName;
    juce::String vendorName;
    juce::String extension;

    juce::File currentPresetDir;
    juce::Array<juce::File> presetFiles;
    juce::StringArray presetNames;

    int currentPresetIndex { -1 };
    juce::String loadedPresetName { "Init" };

    std::unique_ptr<juce::PropertiesFile> globalSettings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
