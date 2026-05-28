#include "PresetManager.h"

#include <algorithm>

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& stateToControl,
                               const juce::String& pluginName,
                               const juce::String& companyName,
                               const juce::String& fileExtension)
    : apvts (stateToControl), appName (pluginName), vendorName (companyName), extension (fileExtension)
{
    juce::PropertiesFile::Options options;
    options.applicationName = appName;
    options.filenameSuffix = "settings";
    options.folderName = vendorName;
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    globalSettings = std::make_unique<juce::PropertiesFile> (options);

    currentPresetDir = getDefaultPresetDirectory();
    if (globalSettings != nullptr)
    {
        const auto savedPresetPath = globalSettings->getValue ("presetPath");
        if (savedPresetPath.isNotEmpty())
        {
            const juce::File storedPath (savedPresetPath);
            if (storedPath.isDirectory() || ! storedPath.exists())
                currentPresetDir = storedPath;
        }
    }

    if (! currentPresetDir.exists())
        currentPresetDir.createDirectory();

    updatePresetList();
}

PresetManager::~PresetManager() = default;

juce::File PresetManager::getDefaultPresetDirectory() const
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
        .getChildFile (vendorName)
        .getChildFile (appName)
        .getChildFile ("Presets");
}

void PresetManager::updatePresetList()
{
    if (! currentPresetDir.exists())
        currentPresetDir.createDirectory();

    presetFiles.clear();
    presetNames.clear();

    const auto files = currentPresetDir.findChildFiles (juce::File::TypesOfFileToFind::findFiles, false, "*" + extension);
    for (const auto& file : files)
    {
        if (file.existsAsFile())
            presetFiles.add (file);
    }

    std::sort (presetFiles.begin(), presetFiles.end(), [] (const juce::File& a, const juce::File& b) {
        return a.getFileName().compareNatural (b.getFileName()) < 0;
    });

    for (const auto& file : presetFiles)
        presetNames.add (file.getFileNameWithoutExtension());

    if (presetNames.isEmpty())
    {
        currentPresetIndex = -1;
        return;
    }

    currentPresetIndex = juce::jlimit (0, presetNames.size() - 1, currentPresetIndex);
}

bool PresetManager::loadPreset (int index)
{
    updatePresetList();
    if (index < 0 || index >= presetFiles.size())
        return false;

    const auto presetFile = presetFiles[index];
    if (! presetFile.existsAsFile() || ! presetFile.hasReadAccess())
        return false;

    if (auto xml = juce::XmlDocument::parse (presetFile))
    {
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            currentPresetIndex = index;
            loadedPresetName = presetFile.getFileNameWithoutExtension();
            return true;
        }
    }

    return false;
}

bool PresetManager::loadNextPreset()
{
    updatePresetList();
    if (presetNames.isEmpty())
        return false;

    currentPresetIndex = (currentPresetIndex + 1) % presetNames.size();
    return loadPreset (currentPresetIndex);
}

bool PresetManager::loadPreviousPreset()
{
    updatePresetList();
    if (presetNames.isEmpty())
        return false;

    currentPresetIndex = (currentPresetIndex - 1 + presetNames.size()) % presetNames.size();
    return loadPreset (currentPresetIndex);
}

void PresetManager::saveCurrentPresetToFile (juce::File file)
{
    if (file == juce::File{})
        return;

    if (! file.getFileName().endsWithIgnoreCase (extension))
        file = file.withFileExtension (extension);

    auto parent = file.getParentDirectory();
    if (parent != juce::File{} && ! parent.exists())
        parent.createDirectory();

    auto stateTree = apvts.copyState();
    loadedPresetName = file.getFileNameWithoutExtension();
    stateTree.setProperty ("metaLoadedPresetName", loadedPresetName, nullptr);

    if (auto xml = stateTree.createXml())
        xml->writeTo (file);
}

bool PresetManager::savePreset (const juce::String& presetName)
{
    const juce::String trimmedName = presetName.trim();
    const juce::String rawFileName = juce::File (trimmedName).getFileName();
    juce::String safeName = rawFileName.retainCharacters ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_. ");
    if (! safeName.endsWithIgnoreCase (extension))
        safeName << extension;

    if (safeName.isEmpty())
        return false;

    auto presetFile = currentPresetDir.getChildFile (safeName);
    saveCurrentPresetToFile (presetFile);

    if (presetFile.existsAsFile())
    {
        updatePresetList();
        for (int i = 0; i < presetFiles.size(); ++i)
        {
            if (presetFiles.getReference (i).getFileName() == presetFile.getFileName())
            {
                currentPresetIndex = i;
                break;
            }
        }
        return true;
    }

    return false;
}

bool PresetManager::setCustomPresetDirectory (const juce::File& directory)
{
    if (directory == juce::File{})
        return false;

    juce::File resolvedDir = directory.existsAsFile() ? directory.getParentDirectory() : directory;

    if (! resolvedDir.exists() && ! resolvedDir.createDirectory())
        return false;

    currentPresetDir = resolvedDir;
    if (globalSettings != nullptr)
    {
        globalSettings->setValue ("presetPath", currentPresetDir.getFullPathName());
        globalSettings->saveIfNeeded();
    }

    updatePresetList();
    return true;
}

void PresetManager::restorePresetNameFromSession (const juce::String& name)
{
    loadedPresetName = name.isNotEmpty() ? name : "Init";
}
