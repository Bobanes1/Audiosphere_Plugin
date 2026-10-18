## Audiosphere 0.1 — first release

A wavetable synthesizer for hip hop production, with 82 factory presets.

### Download

| System | File | Installs |
| --- | --- | --- |
| **Windows 10/11 (64-bit)** | `Audiosphere-0.1.0-Windows-Setup.exe` | VST3 → `C:\Program Files\Common Files\VST3`, standalone app → Start menu |
| **Linux (x86-64)** | `Audiosphere-0.1.0-Linux.tar.gz` | run `./install.sh`: VST3 → `~/.vst3`, app → `~/.local/bin` |

macOS is not part of this release yet; a macOS installer will follow in a later version.

Run the installer, then in **Ableton Live 12** open **Options → Preferences → Plug-Ins**, turn on
**Use VST3 Plug-In System Folders** and click **Rescan**. Audiosphere shows up under **Plug-Ins → VST3**.

**First launch note** (the Windows installer is not code-signed yet):
- **Windows:** if SmartScreen says "Windows protected your PC", click **More info → Run anyway**.

### What's inside
- 2 wavetable oscillators (16 wavetables, 8-voice unison, warp modes including FM/AM), sub and noise
- Multimode filter, 3 envelopes, 2 tempo-synced LFOs, 8-slot mod matrix and 4 macros
- Poly / Mono / Legato with glide for 808 slides
- Distortion, lo-fi, EQ, chorus, ping-pong delay, reverb and an output limiter
- 82 level-matched presets: 808 & Bass, Keys, Pads, Leads, Plucks, Bells & Mallets, Strings & Brass, Lo-Fi, FX & Atmos
- User presets saved to `Documents/Audiosphere/Presets`

### Known limitations
- The Windows installer is unsigned (see first launch notes above).
- No macOS build in this release.
- Version 0.1 is an early release; feedback and bug reports are welcome in the Issues tab.
