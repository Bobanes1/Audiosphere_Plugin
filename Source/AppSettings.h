#pragma once

#include <juce_data_structures/juce_data_structures.h>

// Settings shared by every Audiosphere instance on this computer (favourites,
// update checks). Stored in the user's application data folder, e.g.
// %APPDATA%\Audiosphere\Audiosphere.settings or ~/.config/Audiosphere.
// Hold it through juce::SharedResourcePointer<AppSettings>. Message thread only.
class AppSettings
{
public:
    AppSettings();
    ~AppSettings();

    juce::PropertiesFile& file() { return *properties; }

    juce::String getString (const juce::String& key, const juce::String& fallback = {}) const;
    void setString (const juce::String& key, const juce::String& value);
    bool getBool (const juce::String& key, bool fallback) const;
    void setBool (const juce::String& key, bool value);

private:
    std::unique_ptr<juce::PropertiesFile> properties;
};
