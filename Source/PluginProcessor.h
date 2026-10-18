#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>

#include "Parameters.h"
#include "DSP/SynthEngine.h"
#include "DSP/Effects.h"
#include "Presets/PresetManager.h"

class AudiosphereProcessor : public juce::AudioProcessor
{
public:
    AudiosphereProcessor();
    ~AudiosphereProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override { return "Audiosphere"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    // A single program: presets are handled by the built-in browser so a host
    // switching programs can never overwrite a saved session.
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return presetManager.getCurrentName(); }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    PresetManager presetManager;
    juce::MidiKeyboardState keyboardState;

    // Oscilloscope feed for the editor (written by the audio thread).
    static constexpr int scopeSize = 1024;
    void copyScope (float* dest, int numSamples) const;
    int getActiveVoiceCount() const noexcept { return activeVoices.load(); }

private:
    using Atomic = std::atomic<float>*;

    // Raw parameter pointers, cached once so the audio thread never looks up IDs.
    struct ParamPointers
    {
        Atomic masterVol, voiceMode, polyphony, glide, bendRange, ampVel;
        std::array<Atomic, Params::numMacros> macros;

        struct Osc { Atomic on, table, pos, oct, semi, fine, unison, detune, spread, level, pan, warpMode, warp, retrig; };
        std::array<Osc, 2> osc;

        Atomic subOn, subShape, subOct, subLevel, noiseOn, noiseType, noiseLevel;
        Atomic fltOn, fltType, fltCutoff, fltRes, fltDrive, fltEnv, fltKey, fltVel;

        struct Env { Atomic a, d, s, r; };
        Env amp, fenv, menv;

        struct Lfo { Atomic shape, rate, sync, div, retrig, fade; };
        std::array<Lfo, 2> lfo;

        struct Mod { Atomic src, dst, amt; };
        std::array<Mod, Params::numModSlots> mod;

        Atomic distOn, distType, distDrive, distTone, distMix;
        Atomic lofiOn, lofiBits, lofiRate, lofiWow, lofiNoise, lofiTone, lofiMix;
        Atomic eqOn, eqLow, eqMid, eqHigh;
        Atomic chorusOn, chorusRate, chorusDepth, chorusMix;
        Atomic delayOn, delaySync, delayTime, delayDiv, delayFb, delayPingPong, delayTone, delayMix;
        Atomic reverbOn, reverbSize, reverbDamp, reverbWidth, reverbMix;
        Atomic limiterOn;
    };

    void cacheParameterPointers();
    void readSynthParams (SynthParams& out, double bpm) const;
    void readFxParams (FxParams& out, double bpm) const;

    SynthEngine engine;
    FxChain fx;
    juce::AudioBuffer<float> workBuffer;

    ParamPointers pp {};
    SynthParams synthParams;
    FxParams fxParams;
    double sampleRate = 44100.0;
    std::array<double, 2> lfoPhase {};

    std::array<float, scopeSize> scope {};
    std::atomic<int> scopeWrite { 0 };
    std::atomic<int> activeVoices { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudiosphereProcessor)
};
