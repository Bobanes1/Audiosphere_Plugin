#pragma once

#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "GUI/LookAndFeel.h"
#include "GUI/Components.h"
#include "GUI/UpdateUI.h"

class AudiosphereEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseWidth = 1200;
    static constexpr int baseHeight = 790;

    explicit AudiosphereEditor (AudiosphereProcessor&);
    ~AudiosphereEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void showPage (int index);

    AudiosphereProcessor& processor;
    AudiosphereLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 600 };

    // Everything is laid out at the base size inside `content`, which is scaled as a whole.
    juce::Component content;

    struct Header;
    struct SynthPage;
    struct ModPage;
    struct FxPage;

    std::unique_ptr<Header> header;
    std::unique_ptr<SynthPage> synthPage;
    std::unique_ptr<ModPage> modPage;
    std::unique_ptr<FxPage> fxPage;

    juce::TextButton synthTab { "SYNTH" }, modTab { "MOD MATRIX" }, fxTab { "EFFECTS" };
    juce::Label voiceLabel;
    UpdateButton updateButton;
    juce::SharedResourcePointer<UpdateManager> updates;
    juce::MidiKeyboardComponent keyboard;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudiosphereEditor)
};
