#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Factory + user preset handling. User presets are XML files stored in
// Documents/Audiosphere/Presets so they can be shared between projects and DAWs.
class PresetManager
{
public:
    struct Entry
    {
        juce::String name;
        juce::String category;
        int factoryIndex = -1;      // >= 0 for factory presets
        juce::File file;            // user presets
    };

    static constexpr const char* fileExtension = ".aspreset";

    explicit PresetManager (juce::AudioProcessorValueTreeState& state);

    static juce::File getUserPresetFolder();

    void refresh();
    const std::vector<Entry>& getEntries() const noexcept { return entries; }

    void loadInitialPreset();
    void loadInit();
    void load (int entryIndex);
    void loadNext();
    void loadPrevious();
    bool saveUserPreset (const juce::String& name);
    bool deleteUserPreset (int entryIndex);

    int getCurrentIndex() const noexcept { return currentIndex; }
    juce::String getCurrentName() const { return currentName; }
    void setCurrentName (const juce::String& name);

    // Incremented whenever the current preset changes (lets the editor poll cheaply).
    int getChangeCounter() const noexcept { return changeCounter.load(); }

private:
    void applyFactory (int factoryIndex);
    void resetToDefaults();

    juce::AudioProcessorValueTreeState& apvts;
    std::vector<Entry> entries;
    int currentIndex = -1;
    juce::String currentName { "Init" };
    std::atomic<int> changeCounter { 0 };
};
