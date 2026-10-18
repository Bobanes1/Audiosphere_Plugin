#pragma once

#include <juce_core/juce_core.h>
#include <utility>
#include <vector>

// A factory preset is a list of parameter overrides (in real-world units, choice
// parameters by index) applied on top of the default parameter values.
struct FactoryPreset
{
    juce::String name;
    juce::String category;
    std::vector<std::pair<juce::String, float>> values;
};

const std::vector<FactoryPreset>& getFactoryPresets();
const juce::StringArray& getFactoryCategories();
