#include "AppSettings.h"

AppSettings::AppSettings()
{
    juce::PropertiesFile::Options options;
    options.applicationName = "Audiosphere";
   #if JUCE_LINUX || JUCE_BSD
    options.folderName = ".config/Audiosphere";
   #else
    options.folderName = "Audiosphere";
   #endif
    options.filenameSuffix = ".settings";
    options.osxLibrarySubFolder = "Application Support";
    options.storageFormat = juce::PropertiesFile::storeAsXML;
    options.millisecondsBeforeSaving = 0;
    options.processLock = nullptr;
    properties = std::make_unique<juce::PropertiesFile> (options);
}

AppSettings::~AppSettings()
{
    properties->saveIfNeeded();
}

juce::String AppSettings::getString (const juce::String& key, const juce::String& fallback) const
{
    return properties->getValue (key, fallback);
}

void AppSettings::setString (const juce::String& key, const juce::String& value)
{
    properties->setValue (key, value);
    properties->saveIfNeeded();
}

bool AppSettings::getBool (const juce::String& key, bool fallback) const
{
    return properties->getBoolValue (key, fallback);
}

void AppSettings::setBool (const juce::String& key, bool value)
{
    properties->setValue (key, value);
    properties->saveIfNeeded();
}
