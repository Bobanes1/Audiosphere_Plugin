#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Update/UpdateManager.h"

// Pop-up panel: shows the latest release, downloads and opens the installer.
class UpdatePanel : public juce::Component, private juce::Timer
{
public:
    UpdatePanel();
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void refresh();

    juce::SharedResourcePointer<UpdateManager> updates;
    juce::Label title, versions, message;
    juce::TextEditor notes;
    double progress = 0.0;
    juce::ProgressBar progressBar { progress };
    juce::TextButton checkButton { "Check now" }, downloadButton { "Download update" },
                     installButton { "Install" }, pageButton { "Release page" };
    juce::ToggleButton autoCheck { "Check for updates automatically" };
    juce::String shownNotes;
};

// Small header button: shows the installed version, or a highlighted
// "update available" message, and opens the UpdatePanel.
class UpdateButton : public juce::TextButton, private juce::Timer
{
public:
    UpdateButton();
    void paintButton (juce::Graphics&, bool highlighted, bool down) override;

private:
    void timerCallback() override;
    void showPanel();

    juce::SharedResourcePointer<UpdateManager> updates;
    bool highlight = false;
    float pulse = 0.0f;
};
