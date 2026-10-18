#include "UpdateManager.h"

#include <juce_cryptography/juce_cryptography.h>

#ifndef AUDIOSPHERE_VERSION
 #define AUDIOSPHERE_VERSION "0.0.0"
#endif

namespace
{
constexpr juce::int64 checkIntervalMs = 6 * 60 * 60 * 1000;   // automatic checks at most every 6 hours

std::unique_ptr<juce::InputStream> openStream (const juce::String& url, int* statusCode)
{
    return juce::URL (url).createInputStream (
        juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
            .withExtraHeaders ("User-Agent: Audiosphere-Updater\r\nAccept: application/vnd.github+json")
            .withConnectionTimeoutMs (15000)
            .withNumRedirectsToFollow (8)
            .withStatusCode (statusCode));
}
}

UpdateManager::UpdateManager() : juce::Thread ("Audiosphere update")
{
    status.currentVersion = AUDIOSPHERE_VERSION;
    loadCachedRelease();
    startThread (juce::Thread::Priority::low);
}

UpdateManager::~UpdateManager()
{
    signalThreadShouldExit();
    notify();
    stopThread (20000);
}

UpdateManager::Status UpdateManager::getStatus() const
{
    const juce::ScopedLock sl (lock);
    return status;
}

bool UpdateManager::isUpdateAvailable() const
{
    const juce::ScopedLock sl (lock);
    return status.latestVersion.isNotEmpty() && compareVersions (status.latestVersion, status.currentVersion) > 0;
}

bool UpdateManager::getAutoCheck() const
{
    return settings->getBool ("autoCheckUpdates", true);
}

void UpdateManager::setAutoCheck (bool enabled)
{
    settings->setBool ("autoCheckUpdates", enabled);
}

void UpdateManager::checkIfDue()
{
    if (! getAutoCheck())
        return;

    const auto last = settings->getString ("lastUpdateCheck").getLargeIntValue();
    if (juce::Time::currentTimeMillis() - last >= checkIntervalMs)
        checkNow();
}

void UpdateManager::checkNow()
{
    {
        const juce::ScopedLock sl (lock);
        if (status.state == State::Checking || status.state == State::Downloading)
            return;
    }
    settings->setString ("lastUpdateCheck", juce::String (juce::Time::currentTimeMillis()));
    setState (State::Checking, "Checking for updates...");
    pendingJob = 1;
    notify();
}

void UpdateManager::startDownload()
{
    {
        const juce::ScopedLock sl (lock);
        if (status.state == State::Downloading || assetUrl.isEmpty())
            return;
    }
    setState (State::Downloading, "Downloading...");
    pendingJob = 2;
    notify();
}

void UpdateManager::openDownloadedInstaller()
{
    const auto file = getStatus().downloadedFile;
    if (! file.existsAsFile())
        return;

   #if JUCE_LINUX || JUCE_BSD
    file.revealToUser();
   #else
    file.startAsProcess();
   #endif
}

juce::String UpdateManager::installerSuffixForThisPlatform()
{
   #if JUCE_WINDOWS
    return "-Windows-Setup.exe";
   #elif JUCE_MAC
    return "-macOS.pkg";
   #else
    return "-Linux.tar.gz";
   #endif
}

int UpdateManager::compareVersions (const juce::String& a, const juce::String& b)
{
    auto parts = [] (const juce::String& v)
    {
        juce::StringArray tokens;
        tokens.addTokens (v.trim().trimCharactersAtStart ("vV"), ".", {});
        return tokens;
    };

    const auto pa = parts (a), pb = parts (b);
    for (int i = 0; i < std::max (pa.size(), pb.size()); ++i)
    {
        const int x = i < pa.size() ? pa[i].getIntValue() : 0;
        const int y = i < pb.size() ? pb[i].getIntValue() : 0;
        if (x != y)
            return x < y ? -1 : 1;
    }
    return 0;
}

void UpdateManager::setState (State s, const juce::String& message)
{
    const juce::ScopedLock sl (lock);
    status.state = s;
    status.message = message;
}

//==============================================================================
void UpdateManager::run()
{
    while (! threadShouldExit())
    {
        wait (-1);
        if (threadShouldExit())
            break;

        const int job = pendingJob.exchange (0);
        if (job == 1)
            performCheck();
        else if (job == 2)
            performDownload();
    }
}

void UpdateManager::performCheck()
{
    int code = 0;
    auto stream = openStream ("https://api.github.com/repos/" + juce::String (repository) + "/releases/latest", &code);
    if (stream == nullptr || code != 200)
    {
        setState (State::CheckFailed, "Could not reach GitHub" + (code > 0 ? " (HTTP " + juce::String (code) + ")." : "."));
        return;
    }

    const auto json = juce::JSON::parse (stream->readEntireStreamAsString());
    const auto tag = json.getProperty ("tag_name", {}).toString();
    if (tag.isEmpty())
    {
        setState (State::CheckFailed, "Unexpected answer from GitHub.");
        return;
    }

    juce::String asset, assetLink, sums;
    if (auto* assets = json.getProperty ("assets", {}).getArray())
    {
        for (auto& a : *assets)
        {
            const auto name = a.getProperty ("name", {}).toString();
            const auto link = a.getProperty ("browser_download_url", {}).toString();
            if (name.endsWith (installerSuffixForThisPlatform()))
            {
                asset = name;
                assetLink = link;
            }
            else if (name == "SHA256SUMS.txt")
            {
                sums = link;
            }
        }
    }

    {
        const juce::ScopedLock sl (lock);
        status.latestVersion = tag.trimCharactersAtStart ("vV");
        status.releaseNotes = json.getProperty ("body", {}).toString();
        status.releasePage = json.getProperty ("html_url", {}).toString();
        status.assetName = asset;
        assetUrl = assetLink;
        checksumUrl = sums;
    }

    if (compareVersions (tag, AUDIOSPHERE_VERSION) > 0)
        setState (State::UpdateAvailable, asset.isNotEmpty() ? "Version " + tag.trimCharactersAtStart ("vV") + " is available."
                                                              : "Version " + tag.trimCharactersAtStart ("vV") + " is available (no installer for this system yet).");
    else
        setState (State::UpToDate, "You have the latest version.");

    juce::MessageManager::callAsync ([weak = juce::WeakReference<UpdateManager> (this)]
    {
        if (auto* self = weak.get())
            self->cacheRelease();
    });
}

void UpdateManager::performDownload()
{
    juce::String url, sumsUrl, name;
    {
        const juce::ScopedLock sl (lock);
        url = assetUrl;
        sumsUrl = checksumUrl;
        name = status.assetName;
    }

    const auto folder = juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Downloads");
    folder.createDirectory();
    const auto target = folder.getChildFile (name);
    const auto partial = folder.getChildFile (name + ".part");

    int code = 0;
    auto stream = openStream (url, &code);
    if (stream == nullptr || code != 200)
    {
        setState (State::DownloadFailed, "Download failed" + (code > 0 ? " (HTTP " + juce::String (code) + ")." : "."));
        return;
    }

    partial.deleteFile();
    {
        juce::FileOutputStream out (partial);
        if (! out.openedOk())
        {
            setState (State::DownloadFailed, "Cannot write to " + folder.getFullPathName());
            return;
        }

        const auto total = stream->getTotalLength();
        juce::HeapBlock<char> buffer (65536);
        juce::int64 done = 0;

        while (! stream->isExhausted())
        {
            if (threadShouldExit())
                return;

            const int n = stream->read (buffer, 65536);
            if (n <= 0)
                break;

            out.write (buffer, (size_t) n);
            done += n;

            if (total > 0)
            {
                const juce::ScopedLock sl (lock);
                status.progress = (double) done / (double) total;
            }
        }
        out.flush();
    }

    // Verify the installer against the published checksums.
    if (sumsUrl.isNotEmpty())
    {
        int sumsCode = 0;
        auto sumsStream = openStream (sumsUrl, &sumsCode);
        if (sumsStream == nullptr || sumsCode != 200)
        {
            partial.deleteFile();
            setState (State::DownloadFailed, "Could not download the checksum file.");
            return;
        }

        juce::String expected;
        juce::StringArray lines;
        lines.addLines (sumsStream->readEntireStreamAsString());
        for (auto& line : lines)
            if (line.trim().endsWith (name))
                expected = line.trim().upToFirstOccurrenceOf (" ", false, false).toLowerCase();

        const auto actual = juce::SHA256 (partial).toHexString().toLowerCase();
        if (expected.isEmpty() || expected != actual)
        {
            partial.deleteFile();
            setState (State::DownloadFailed, "Checksum mismatch: the download was corrupted. Please try again.");
            return;
        }
    }

    target.deleteFile();
    if (! partial.moveFileTo (target))
    {
        setState (State::DownloadFailed, "Could not save " + target.getFullPathName());
        return;
    }

    {
        const juce::ScopedLock sl (lock);
        status.downloadedFile = target;
        status.progress = 1.0;
    }
    setState (State::Downloaded, "Downloaded and verified: " + target.getFileName());
}

//==============================================================================
void UpdateManager::loadCachedRelease()
{
    const juce::ScopedLock sl (lock);
    status.latestVersion = settings->getString ("latestVersion");
    status.releaseNotes = settings->getString ("latestNotes");
    status.releasePage = settings->getString ("latestPage");
    status.assetName = settings->getString ("latestAsset");
    assetUrl = settings->getString ("latestAssetUrl");
    checksumUrl = settings->getString ("latestChecksumUrl");

    if (status.latestVersion.isNotEmpty() && compareVersions (status.latestVersion, status.currentVersion) > 0)
    {
        status.state = State::UpdateAvailable;
        status.message = "Version " + status.latestVersion + " is available.";
    }
}

void UpdateManager::cacheRelease()
{
    const auto s = getStatus();
    juce::String asset, sums;
    {
        const juce::ScopedLock sl (lock);
        asset = assetUrl;
        sums = checksumUrl;
    }
    settings->setString ("latestVersion", s.latestVersion);
    settings->setString ("latestNotes", s.releaseNotes);
    settings->setString ("latestPage", s.releasePage);
    settings->setString ("latestAsset", s.assetName);
    settings->setString ("latestAssetUrl", asset);
    settings->setString ("latestChecksumUrl", sums);
}
