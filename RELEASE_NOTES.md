## Audiosphere 0.1 — first release

A wavetable synthesizer for hip hop production, with 82 factory presets.

### Download

| System | File | Installs |
| --- | --- | --- |
| **Windows 10/11 (64-bit)** | `Audiosphere-0.1.0-Windows-Setup.exe` | VST3 → `C:\Program Files\Common Files\VST3`, standalone app → Start menu |
| **macOS 10.13+ (Apple Silicon & Intel)** | `Audiosphere-0.1.0-macOS.pkg` | VST3 + Audio Unit → `/Library/Audio/Plug-Ins`, standalone app → `/Applications` |
| **Linux (x86-64)** | `Audiosphere-0.1.0-Linux.tar.gz` | run `./install.sh`: VST3 → `~/.vst3`, app → `~/.local/bin` |

Run the installer, then in **Ableton Live 12** open **Preferences / Settings → Plug-Ins**, turn on
**Use VST3 Plug-In System Folders** and click **Rescan**. Audiosphere shows up under **Plug-Ins → VST3**.

**First launch notes** (the installers are not code-signed yet):
- **Windows:** if SmartScreen says "Windows protected your PC", click **More info → Run anyway**.
- **macOS:** if the installer is blocked as from an "unidentified developer", open
  **System Settings → Privacy & Security** and click **Open Anyway**, or right-click the `.pkg` → **Open**.

### What's inside
- 2 wavetable oscillators (16 wavetables, 8-voice unison, warp modes including FM/AM), sub and noise
- Multimode filter, 3 envelopes, 2 tempo-synced LFOs, 8-slot mod matrix and 4 macros
- Poly / Mono / Legato with glide for 808 slides
- Distortion, lo-fi, EQ, chorus, ping-pong delay, reverb and an output limiter
- 82 level-matched presets: 808 & Bass, Keys, Pads, Leads, Plucks, Bells & Mallets, Strings & Brass, Lo-Fi, FX & Atmos
- User presets saved to `Documents/Audiosphere/Presets`

### Known limitations
- Installers are unsigned (see first launch notes above).
- Version 0.1 is an early release; feedback and bug reports are welcome in the Issues tab.
