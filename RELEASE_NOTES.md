## Audiosphere 0.2 — favourites, built-in updater, 71 new presets

### New
- **Favourites**: like any preset with the star next to its name. Liked presets appear under
  **★ Favourites** at the top of the preset menu, shared across all your projects and DAWs.
- **Update notifications**: Audiosphere tells you when a new version is released. Download the installer
  straight from the plug-in; it is verified against the release's SHA256 checksums before you install it.
- **71 new presets (153 total)**: rage leads and plucks, UK drill and Jersey 808s, Memphis and tape-saturated
  808s, an 808 stutter, Growl Reese, G-Funk synth bass, more electric pianos, gospel chords, felt and grand keys,
  shimmer, vocal and haunted pads, trumpet, violin, cello and horn, crystal, church and tubular bells, gamelan,
  steel drum, lo-fi choir, radio lead and vinyl keys, plus downlifters, wind, glitches and impacts.

### Download

| System | File | Installs |
| --- | --- | --- |
| **Windows 10/11 (64-bit)** | `Audiosphere-0.2.0-Windows-Setup.exe` | VST3 → `C:\Program Files\Common Files\VST3`, standalone app → Start menu |
| **Linux (x86-64)** | `Audiosphere-0.2.0-Linux.tar.gz` | run `./install.sh`: VST3 → `~/.vst3`, app → `~/.local/bin` (needs libcurl) |

Run the installer (close your DAW first), then in **Ableton Live 12** open **Options → Preferences → Plug-Ins**,
make sure **Use VST3 Plug-In System Folders** is on and click **Rescan**.

**First launch note** (the Windows installer is not code-signed yet): if SmartScreen says
"Windows protected your PC", click **More info → Run anyway**.

### Notes
- Updating from 0.1: 0.1 has no update checker yet, so install this version once by hand. From 0.2 on,
  Audiosphere tells you about new versions itself.
- Your projects, user presets and favourites are kept when you update.
- The update check only asks `api.github.com` for the latest release and can be switched off in the update panel.
- No macOS build in this release.
