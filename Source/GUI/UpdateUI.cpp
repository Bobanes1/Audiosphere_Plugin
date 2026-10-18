#include "UpdateUI.h"
#include "LookAndFeel.h"

UpdatePanel::UpdatePanel()
{
    title.setText ("Audiosphere Updates", juce::dontSendNotification);
    title.setFont (Theme::font (17.0f, true));
    title.setColour (juce::Label::textColourId, Theme::text);

    for (auto* l : { &versions, &message })
    {
        l->setFont (Theme::font (13.0f));
        l->setColour (juce::Label::textColourId, Theme::textDim);
    }
    message.setColour (juce::Label::textColourId, Theme::cyan);

    notes.setMultiLine (true);
    notes.setReadOnly (true);
    notes.setScrollbarsShown (true);
    notes.setCaretVisible (false);
    notes.setFont (Theme::font (12.5f));

    progressBar.setColour (juce::ProgressBar::foregroundColourId, Theme::cyan);
    progressBar.setColour (juce::ProgressBar::backgroundColourId, Theme::background);

    autoCheck.setColour (juce::ToggleButton::tickColourId, Theme::cyan);
    autoCheck.setColour (juce::ToggleButton::textColourId, Theme::text);
    autoCheck.setToggleState (updates->getAutoCheck(), juce::dontSendNotification);
    autoCheck.onClick = [this] { updates->setAutoCheck (autoCheck.getToggleState()); };

    checkButton.onClick = [this] { updates->checkNow(); };
    downloadButton.onClick = [this] { updates->startDownload(); };
    installButton.onClick = [this] { updates->openDownloadedInstaller(); };
    pageButton.onClick = [this]
    {
        const auto page = updates->getStatus().releasePage;
        juce::URL (page.isNotEmpty() ? page : "https://github.com/" + juce::String (UpdateManager::repository) + "/releases").launchInDefaultBrowser();
    };

    for (auto* c : std::initializer_list<juce::Component*> { &title, &versions, &message, &notes, &progressBar,
                                                             &checkButton, &downloadButton, &installButton, &pageButton, &autoCheck })
        addAndMakeVisible (c);

    setSize (440, 330);
    refresh();
    startTimerHz (10);
}

void UpdatePanel::paint (juce::Graphics& g)
{
    g.fillAll (Theme::panel);
}

void UpdatePanel::resized()
{
    auto b = getLocalBounds().reduced (14);
    title.setBounds (b.removeFromTop (24));
    versions.setBounds (b.removeFromTop (20));
    message.setBounds (b.removeFromTop (22));
    b.removeFromTop (4);

    autoCheck.setBounds (b.removeFromBottom (24));
    b.removeFromBottom (6);
    auto buttons = b.removeFromBottom (28);
    const int w = (buttons.getWidth() - 18) / 4;
    for (auto* button : { &checkButton, &downloadButton, &installButton, &pageButton })
    {
        button->setBounds (buttons.removeFromLeft (w));
        buttons.removeFromLeft (6);
    }
    b.removeFromBottom (8);
    progressBar.setBounds (b.removeFromBottom (16));
    b.removeFromBottom (8);
    notes.setBounds (b);
}

void UpdatePanel::timerCallback()
{
    refresh();
}

void UpdatePanel::refresh()
{
    using State = UpdateManager::State;
    const auto s = updates->getStatus();

    versions.setText ("Installed: " + s.currentVersion
                          + (s.latestVersion.isNotEmpty() ? "     Latest: " + s.latestVersion : juce::String()),
                      juce::dontSendNotification);
    message.setText (s.message.isNotEmpty() ? s.message : juce::String ("Click \"Check now\" to look for a new version."),
                     juce::dontSendNotification);

    const bool failed = s.state == State::CheckFailed || s.state == State::DownloadFailed;
    message.setColour (juce::Label::textColourId, failed ? Theme::pink : Theme::cyan);

    const auto notesText = s.releaseNotes.isNotEmpty() ? s.releaseNotes : juce::String ("No release notes loaded yet.");
    if (notesText != shownNotes)
    {
        shownNotes = notesText;
        notes.setText (notesText, false);
    }

    progress = s.state == State::Downloading ? s.progress : (s.state == State::Downloaded ? 1.0 : 0.0);
    progressBar.setVisible (s.state == State::Downloading || s.state == State::Downloaded);

    const bool busy = s.state == State::Checking || s.state == State::Downloading;
    const bool newer = s.latestVersion.isNotEmpty() && UpdateManager::compareVersions (s.latestVersion, s.currentVersion) > 0;
    checkButton.setEnabled (! busy);
    downloadButton.setEnabled (! busy && newer && s.assetName.isNotEmpty() && s.state != State::Downloaded);
    installButton.setEnabled (s.state == State::Downloaded);
   #if JUCE_LINUX || JUCE_BSD
    installButton.setButtonText ("Show file");
   #endif
}

//==============================================================================
UpdateButton::UpdateButton()
{
    setTooltip ("Check for Audiosphere updates");
    onClick = [this] { showPanel(); };
    timerCallback();
    startTimerHz (20);
}

void UpdateButton::timerCallback()
{
    const bool available = updates->isUpdateAvailable();
    const auto s = updates->getStatus();
    const auto label = available ? "UPDATE " + s.latestVersion + " AVAILABLE" : "v" + s.currentVersion;

    if (label != getButtonText())
        setButtonText (label);

    if (available)
    {
        pulse += 0.05f;
        repaint();
    }
    highlight = available;
}

void UpdateButton::paintButton (juce::Graphics& g, bool highlighted, bool down)
{
    const auto b = getLocalBounds().toFloat().reduced (0.5f);
    auto accent = highlight ? Theme::amber : Theme::outline;
    if (highlight)
        accent = accent.withMultipliedAlpha (0.6f + 0.4f * (0.5f + 0.5f * std::sin (pulse)));

    g.setColour (highlight ? Theme::amber.withAlpha (down ? 0.3f : (highlighted ? 0.22f : 0.14f))
                           : (highlighted ? Theme::panelLight.brighter (0.08f) : Theme::panelLight));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (accent);
    g.drawRoundedRectangle (b, 4.0f, 1.0f);

    g.setColour (highlight ? Theme::amber : Theme::textDim);
    g.setFont (Theme::font (11.0f, true));
    g.drawText (getButtonText(), b, juce::Justification::centred);
}

void UpdateButton::showPanel()
{
    auto* top = getTopLevelComponent();
    const auto area = top->getLocalArea (this, getLocalBounds());
    auto panel = std::make_unique<UpdatePanel>();
    panel->setLookAndFeel (&getLookAndFeel());
    juce::CallOutBox::launchAsynchronously (std::move (panel), area, top);

    if (! updates->isUpdateAvailable())
        updates->checkNow();
}
