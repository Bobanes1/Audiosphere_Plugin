#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "LookAndFeel.h"
#include "../Presets/PresetManager.h"

using APVTS = juce::AudioProcessorValueTreeState;

//==============================================================================
// Rotary knob bound to a parameter. The caption shows the value while hovered or dragged.
class Knob : public juce::Component
{
public:
    Knob (APVTS& state, const juce::String& paramId, const juce::String& caption, juce::Colour accent = Theme::cyan);

    void resized() override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;

    juce::Slider slider;

private:
    void updateCaption();

    juce::RangedAudioParameter* param = nullptr;
    juce::Label label;
    juce::String captionText;
    bool hovering = false, dragging = false;
    std::unique_ptr<APVTS::SliderAttachment> attachment;
};

//==============================================================================
// Combo box bound to a choice parameter, with an optional caption above it.
class ChoiceBox : public juce::Component
{
public:
    ChoiceBox (APVTS& state, const juce::String& paramId, const juce::String& caption = {});
    void resized() override;

    juce::ComboBox box;

private:
    juce::Label label;
    std::unique_ptr<APVTS::ComboBoxAttachment> attachment;
};

//==============================================================================
// Toggle bound to a bool parameter: a power button when text is empty, otherwise a chip.
class ParamToggle : public juce::ToggleButton
{
public:
    ParamToggle (APVTS& state, const juce::String& paramId, const juce::String& text = {}, juce::Colour accent = Theme::cyan);

private:
    std::unique_ptr<APVTS::ButtonAttachment> attachment;
};

//==============================================================================
// Decorative panel with a title strip. Controls are siblings placed on top of it.
class Section : public juce::Component
{
public:
    Section (const juce::String& title, juce::Colour accent, bool hasPowerButton = false);
    void paint (juce::Graphics&) override;

    juce::Rectangle<int> getContentBounds() const;          // in parent coordinates
    juce::Rectangle<int> getPowerButtonBounds() const;      // in parent coordinates
    juce::Rectangle<int> getTitleRightArea (int width) const;

    static constexpr int titleHeight = 24;

private:
    juce::String title;
    juce::Colour accent;
    bool hasPower;
};

//==============================================================================
// Base for small displays that repaint when any of a set of parameters changes.
class ParamWatcherView : public juce::Component, private juce::Timer
{
public:
    ParamWatcherView (APVTS& state, const juce::StringArray& paramIds);

protected:
    float value (int index) const { return values[(size_t) index]->load(); }

private:
    void timerCallback() override;
    std::vector<std::atomic<float>*> values;
    std::vector<float> lastValues;
};

class WavetableView : public ParamWatcherView
{
public:
    WavetableView (APVTS& state, int oscIndex, juce::Colour accent);
    void paint (juce::Graphics&) override;

private:
    juce::Colour accent;
};

class EnvelopeView : public ParamWatcherView
{
public:
    EnvelopeView (APVTS& state, const juce::String& prefix, juce::Colour accent);
    void paint (juce::Graphics&) override;

private:
    juce::Colour accent;
};

class LfoView : public ParamWatcherView
{
public:
    LfoView (APVTS& state, int lfoIndex, juce::Colour accent);
    void paint (juce::Graphics&) override;

private:
    juce::Colour accent;
};

//==============================================================================
class ScopeView : public juce::Component, private juce::Timer
{
public:
    using Source = std::function<void (float*, int)>;
    explicit ScopeView (Source source);
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override { repaint(); }
    Source source;
    std::vector<float> samples;
};

//==============================================================================
// Star toggle used to like a preset (drawn, so it does not depend on font glyphs).
class StarButton : public juce::Button
{
public:
    StarButton() : juce::Button ("Favourite") {}
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;
};

//==============================================================================
class PresetBar : public juce::Component, private juce::Timer
{
public:
    explicit PresetBar (PresetManager& manager);
    void resized() override;
    void paint (juce::Graphics&) override;

private:
    void timerCallback() override;
    void showMenu();
    void showSaveDialog();

    PresetManager& presets;
    juce::TextButton prevButton { "<" }, nextButton { ">" }, nameButton, saveButton { "SAVE" };
    StarButton starButton;
    juce::Label categoryLabel;
    int lastChange = -1;
};
