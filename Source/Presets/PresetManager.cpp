#include "PresetManager.h"
#include "FactoryPresets.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& state)
    : apvts (state)
{
    refresh();
}

juce::File PresetManager::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("Audiosphere")
               .getChildFile ("Presets");
}

void PresetManager::refresh()
{
    entries.clear();

    const auto& factory = getFactoryPresets();
    for (int i = 0; i < (int) factory.size(); ++i)
        entries.push_back ({ factory[(size_t) i].name, factory[(size_t) i].category, i, {} });

    const auto folder = getUserPresetFolder();
    if (folder.isDirectory())
    {
        auto files = folder.findChildFiles (juce::File::findFiles, false, juce::String ("*") + fileExtension);
        files.sort();
        for (auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), "User", -1, f });
    }

    setCurrentName (currentName);
}

void PresetManager::resetToDefaults()
{
    for (auto* param : apvts.processor.getParameters())
        if (auto* p = dynamic_cast<juce::RangedAudioParameter*> (param))
            p->setValueNotifyingHost (p->getDefaultValue());
}

void PresetManager::applyFactory (int factoryIndex)
{
    const auto& preset = getFactoryPresets()[(size_t) factoryIndex];
    resetToDefaults();

    for (const auto& [id, value] : preset.values)
    {
        if (auto* p = apvts.getParameter (id))
            p->setValueNotifyingHost (p->convertTo0to1 (value));
        else
            jassertfalse;   // unknown parameter ID in a factory preset
    }
}

void PresetManager::loadInitialPreset()
{
    if (! entries.empty())
        load (0);
}

void PresetManager::loadInit()
{
    resetToDefaults();
    currentIndex = -1;
    currentName = "Init";
    ++changeCounter;
}

void PresetManager::load (int entryIndex)
{
    if (! juce::isPositiveAndBelow (entryIndex, (int) entries.size()))
        return;

    const auto entry = entries[(size_t) entryIndex];

    if (entry.factoryIndex >= 0)
    {
        applyFactory (entry.factoryIndex);
    }
    else
    {
        auto xml = juce::XmlDocument::parse (entry.file);
        if (xml == nullptr)
            return;

        auto* stateXml = xml->getChildByName (apvts.state.getType());
        if (stateXml == nullptr)
            return;

        resetToDefaults();
        apvts.replaceState (juce::ValueTree::fromXml (*stateXml));
    }

    currentIndex = entryIndex;
    currentName = entry.name;
    ++changeCounter;
}

void PresetManager::loadNext()
{
    if (entries.empty())
        return;
    load (currentIndex < 0 ? 0 : (currentIndex + 1) % (int) entries.size());
}

void PresetManager::loadPrevious()
{
    if (entries.empty())
        return;
    load (currentIndex <= 0 ? (int) entries.size() - 1 : currentIndex - 1);
}

bool PresetManager::saveUserPreset (const juce::String& rawName)
{
    const auto name = juce::File::createLegalFileName (rawName.trim());
    if (name.isEmpty())
        return false;

    const auto folder = getUserPresetFolder();
    if (! folder.createDirectory())
        return false;

    juce::XmlElement root ("AudiospherePreset");
    root.setAttribute ("name", name);
    root.setAttribute ("version", 1);
    if (auto stateXml = apvts.copyState().createXml())
        root.addChildElement (stateXml.release());

    const auto file = folder.getChildFile (name + fileExtension);
    if (! root.writeTo (file))
        return false;

    currentName = name;
    refresh();
    ++changeCounter;
    return true;
}

bool PresetManager::deleteUserPreset (int entryIndex)
{
    if (! juce::isPositiveAndBelow (entryIndex, (int) entries.size()))
        return false;

    const auto entry = entries[(size_t) entryIndex];
    if (entry.factoryIndex >= 0 || ! entry.file.deleteFile())
        return false;

    refresh();
    ++changeCounter;
    return true;
}

void PresetManager::setCurrentName (const juce::String& name)
{
    currentName = name;
    currentIndex = -1;

    // Prefer user presets when names clash, since a user preset is what got saved.
    for (int i = (int) entries.size(); --i >= 0;)
    {
        if (entries[(size_t) i].name == name)
        {
            currentIndex = i;
            break;
        }
    }

    ++changeCounter;
}

//==============================================================================
juce::String PresetManager::favouriteKey (int entryIndex) const
{
    if (! juce::isPositiveAndBelow (entryIndex, (int) entries.size()))
        return {};

    const auto& e = entries[(size_t) entryIndex];
    return (e.factoryIndex >= 0 ? "factory:" : "user:") + e.name;
}

juce::StringArray PresetManager::loadFavourites() const
{
    if (settings == nullptr)
        settings = std::make_unique<juce::SharedResourcePointer<AppSettings>>();

    juce::StringArray list;
    list.addLines ((*settings)->getString ("favourites"));
    list.removeEmptyStrings();
    return list;
}

bool PresetManager::isFavourite (int entryIndex) const
{
    const auto key = favouriteKey (entryIndex);
    return key.isNotEmpty() && loadFavourites().contains (key);
}

void PresetManager::toggleFavourite (int entryIndex)
{
    const auto key = favouriteKey (entryIndex);
    if (key.isEmpty())
        return;

    auto list = loadFavourites();
    if (list.contains (key))
        list.removeString (key);
    else
        list.add (key);

    (*settings)->setString ("favourites", list.joinIntoString ("\n"));
    ++changeCounter;
}

std::vector<int> PresetManager::getFavouriteIndices() const
{
    const auto list = loadFavourites();
    std::vector<int> result;
    for (int i = 0; i < (int) entries.size(); ++i)
        if (list.contains (favouriteKey (i)))
            result.push_back (i);
    return result;
}
