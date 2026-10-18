#pragma once

#include <juce_events/juce_events.h>
#include "../AppSettings.h"

// Checks GitHub Releases for a newer Audiosphere version and downloads the
// installer for this platform, verifying it against the release's SHA256SUMS.txt.
//
// One instance is shared by all plugin windows (hold it through
// juce::SharedResourcePointer<UpdateManager>). Network work happens on a
// background thread; the UI polls getStatus() from a timer.
class UpdateManager : private juce::Thread
{
public:
    static constexpr const char* repository = "Bobanes1/Audiosphere_Plugin";

    enum class State
    {
        Idle, Checking, UpToDate, UpdateAvailable, CheckFailed,
        Downloading, Downloaded, DownloadFailed
    };

    struct Status
    {
        State state = State::Idle;
        juce::String currentVersion, latestVersion, releaseNotes, releasePage, assetName, message;
        juce::File downloadedFile;
        double progress = 0.0;
    };

    UpdateManager();
    ~UpdateManager() override;

    Status getStatus() const;
    bool isUpdateAvailable() const;

    // Checks if automatic checks are on and the last check is old enough.
    void checkIfDue();
    void checkNow();
    void startDownload();

    // Opens the downloaded installer (Windows / macOS) or shows it in the file browser (Linux).
    void openDownloadedInstaller();

    bool getAutoCheck() const;
    void setAutoCheck (bool enabled);

    // "0.1" < "0.1.1" < "0.2"; a leading "v" is ignored.
    static int compareVersions (const juce::String& a, const juce::String& b);
    static juce::String installerSuffixForThisPlatform();

private:
    void run() override;
    void performCheck();
    void performDownload();
    void setState (State s, const juce::String& message = {});
    void loadCachedRelease();
    void cacheRelease();

    juce::SharedResourcePointer<AppSettings> settings;
    mutable juce::CriticalSection lock;
    Status status;
    juce::String assetUrl, checksumUrl;
    std::atomic<int> pendingJob { 0 };   // 1 = check, 2 = download

    JUCE_DECLARE_WEAK_REFERENCEABLE (UpdateManager)
};
