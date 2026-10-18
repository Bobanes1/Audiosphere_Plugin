# Audiosphere

A wavetable synthesizer plugin (VST3 / AU / Standalone) built for hip hop production, inspired by
the workflow of Serum and Omnisphere. It ships with **82 factory presets**: 808s, basses, keys,
pads, leads, plucks, bells, strings/brass, lo-fi sounds and FX.

Works in **Ableton Live 12** (and any other VST3/AU host) on Windows and macOS.

## Features

- **2 wavetable oscillators** with 16 built-in, band-limited wavetables (Basic Shapes, Analog Saw,
  Pulse Width, Additive, Organ, E-Piano, Vowels, Hard Sync, Growl, Hollow, Mallet, Chiptune, Glass,
  Strings, Choir, Digital), wavetable position morphing, up to 8-voice unison with detune and stereo spread
- **Warp modes** per oscillator: Bend, Squeeze, Sync, Quantize, FM (from the other oscillator) and AM
- **Sub oscillator** (sine / triangle / square / saw, -1 or -2 octaves) and a **noise** source (white, pink, vinyl crackle)
- **Multimode filter**: LP12, LP24, HP12, HP24, band pass, notch, with drive, envelope, key tracking and velocity
- **3 envelopes** (amp, filter, mod) and **2 LFOs** (7 shapes, tempo sync locked to the song position, fade-in)
- **8-slot modulation matrix** with 15 sources, including velocity, mod wheel, aftertouch, pitch bend and 4 macros
- **Poly / Mono / Legato** voice modes with glide, for classic **808 slides**
- **Effects**: distortion (5 types, 2x oversampled), lo-fi (bit crush, sample rate, wow & flutter,
  vinyl noise), 3-band EQ, chorus, tempo-synced ping-pong delay, reverb, output limiter
- Preset browser with categories, user presets (saved to `Documents/Audiosphere/Presets`), resizable UI,
  on-screen keyboard and oscilloscope

## Download & install

Get the installer for your system from the
[**Releases page**](https://github.com/Bobanes1/Audiosphere_Plugin/releases/latest):

| System | File | What it installs |
| --- | --- | --- |
| Windows 10/11 (64-bit) | `Audiosphere-<version>-Windows-Setup.exe` | VST3 → `C:\Program Files\Common Files\VST3`, standalone app + Start menu entry |
| macOS 10.13+ (Apple Silicon & Intel) | `Audiosphere-<version>-macOS.pkg` (not in v0.1 yet; get it from the Actions artifacts) | VST3 + Audio Unit → `/Library/Audio/Plug-Ins`, standalone app → `/Applications` |
| Linux (x86-64) | `Audiosphere-<version>-Linux.tar.gz` | `./install.sh` → `~/.vst3` and `~/.local/bin` |

Run the installer and follow the steps; it puts everything in the standard plug-in folders, so there is
nothing to copy by hand. Uninstall from **Settings → Apps** on Windows, or run `./install.sh --uninstall` on Linux.

### Load it in Ableton Live 12
1. Open **Options → Preferences → Plug-Ins** (Windows) or **Live → Settings → Plug-Ins** (macOS).
2. Turn on **Use VST3 Plug-In System Folders** (on macOS you can also use **Use Audio Units v2**).
3. Click **Rescan**, then drag **Plug-Ins → VST3 → Audiosphere** onto a MIDI track.

### First launch of an unsigned installer
The installers are not code-signed yet, so the operating system asks for confirmation once:
- **Windows SmartScreen** ("Windows protected your PC"): click **More info → Run anyway**.
- **macOS** ("unidentified developer"): right-click the `.pkg` → **Open**, or click **Open Anyway** in
  **System Settings → Privacy & Security**.

### Development builds
Every push also builds the installers in GitHub Actions: open the **Actions** tab, pick the latest
**Build** run and download them from **Artifacts**. To publish a GitHub Release with the Windows and Linux
installers, either push a tag starting with `v` (for example `v0.2`) or push a commit whose message contains
`[release v0.2]`; the release job then creates the tag itself.

## Factory presets

| Category | Presets |
| --- | --- |
| 808 & Bass | 808 Boom, 808 Hard Trap, 808 Mafia Slide, 808 Punchy Short, 808 Long Rumble, 808 Growl, 808 Distorted Square, Drill Glide 808, Pure Sub, Reese Bass, Wobble Bass, Funk Pluck Bass, Acid Line, Analog Mono Bass, Lo-Fi Upright, Boom Bap Bass |
| Keys | Electric Tines, Soul Keys, Dreamy Keys, Dusty Piano, Gospel Organ, Wurly Grit, Funk Clav, Toy Piano, Trap Piano Keys, Organ Drawbars Soft |
| Pads | Warm Analog Pad, Dark Trap Pad, Choir Aahs, Ethereal Glass Pad, Evolving Spectra, Lo-Fi Tape Pad, Sunset Strings Pad, Cloud Pad, Retro Synth Pad, Ambient Wash |
| Leads | G-Funk Whistle, Trap Lead Saw, Square Lead, Sync Screamer, Breathy Flute, Vocal Lead, Supersaw Anthem, Chip Lead, Siren Lead, Mellow Sine Lead |
| Plucks | Trap Pluck, Kalimba, Harp Pluck, Nylon Pluck, Koto Pluck, Pizzicato, Bright Arp Pluck, Chord Stab |
| Bells & Mallets | Trap Bell, Glockenspiel, Dark Bell, Drill Bell, Music Box, Celesta, Marimba, Vibraphone |
| Strings & Brass | Drill Strings, Cinematic Strings, Brass Stab, Horn Section, Choir Stab, Orchestra Hit |
| Lo-Fi | Tape Keys, Late Night Keys, Lo-Fi Guitar Pluck, Cassette Pad, Vinyl Bell, Bitcrushed Lead, Warm Sine Chords |
| FX & Atmos | Vinyl Atmosphere, Riser, Laser Zap, Dark Drone, Siren FX, Tape Stop Drop, Space Echo Blip |

Presets are level-matched per category with the offline render tool (see below).

## Production tips

- **808 slides**: the 808 presets use *Legato* mode. Overlap two MIDI notes and the pitch glides to the
  second note; **Glide** sets the slide time. Pitch bend range is 12 semitones on the 808s.
- **Macro 1** is wired to something useful on many presets (grit on the 808s, bark on the keys, the
  filter on pads and plucks). In Live, click **Configure** on the plugin device and move a macro knob to
  expose it, then group the device into an Instrument Rack and map it to a Rack macro.
- **Mod wheel** opens the organ drawbars, changes the choir vowel and adds vibrato on several leads.
- The LFOs and delay sync to Live's tempo; synced LFOs with *Retrig* off lock to the song position, so
  wobbles stay on the grid.

## Building from source

Requirements: CMake 3.22+, a C++17 compiler (Visual Studio 2022, Xcode 14+, or GCC/Clang on Linux).
JUCE 8 is downloaded automatically.

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Audiosphere_VST3
```

macOS universal binary: add `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
Add `-DAUDIOSPHERE_COPY_PLUGIN=ON` to copy the plugin into your system plugin folder after building.

On Linux install the JUCE dependencies first:
`sudo apt install libasound2-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libfreetype6-dev libfontconfig1-dev libgl1-mesa-dev`.

### Preset render tool

```
cmake -B build -DAUDIOSPHERE_BUILD_RENDER_TOOL=ON
cmake --build build --target AudiosphereRender
./build/AudiosphereRender_artefacts/Release/AudiosphereRender renders/
```

It plays a phrase through every factory preset, prints peak / RMS / CPU figures, fails on silent or
non-finite output, and writes a WAV per preset into `renders/`. CI runs it on every push.

## Project layout

```
Source/
  PluginProcessor.*       host interface, parameter snapshotting, state save/restore
  PluginEditor.*          the UI (header, Synth / Mod Matrix / Effects pages, keyboard)
  Parameters.*            parameter layout and choice lists
  DSP/Wavetables.*        procedural band-limited wavetable bank (FFT mipmaps)
  DSP/SynthEngine.*       voices, envelopes, LFOs, filter, mod matrix, mono/legato/glide
  DSP/Effects.*           distortion, lo-fi, EQ, delay, limiter (+ JUCE chorus and reverb)
  Presets/                factory presets and the preset manager
  GUI/                    look and feel and custom widgets
tools/RenderPresets.cpp   offline preset renderer / checker
```

## License

GPL-3.0 (see `LICENSE`). Audiosphere uses the [JUCE](https://juce.com) framework under its open-source
(AGPLv3) license. VST is a trademark of Steinberg Media Technologies GmbH. Audiosphere is an independent
project and is not affiliated with Xfer Records or Spectrasonics.
