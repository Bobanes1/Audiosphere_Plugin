#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <vector>

#include "../Parameters.h"
#include "Wavetables.h"

// Plain snapshot of every synth parameter, filled once per block by the processor.
struct SynthParams
{
    struct Osc
    {
        bool on = true;
        int table = 0;
        float pos = 0.0f;
        float pitch = 0.0f;          // semitone offset (octave + semi + fine)
        int unison = 1;
        float detune = 0.25f;
        float spread = 0.6f;
        float level = 0.75f;
        float pan = 0.0f;
        WarpMode warpMode = WarpMode::Off;
        float warp = 0.0f;
        bool retrig = true;
    };

    struct Adsr { float a = 0.002f, d = 0.3f, s = 1.0f, r = 0.15f; };

    struct Lfo
    {
        LfoShape shape = LfoShape::Sine;
        float rateHz = 2.0f;         // already resolved from tempo sync
        bool retrig = true;
        float fade = 0.0f;
    };

    struct ModSlot { int src = 0, dst = 0; float amt = 0.0f; };

    std::array<Osc, 2> osc;

    bool subOn = false;
    SubShape subShape = SubShape::Sine;
    int subOct = -1;
    float subLevel = 0.6f;

    bool noiseOn = false;
    NoiseType noiseType = NoiseType::White;
    float noiseLevel = 0.3f;

    bool filterOn = true;
    FilterType filterType = FilterType::LP12;
    float cutoff = 20000.0f, resonance = 0.1f, drive = 0.0f, envAmount = 0.0f, keyTrack = 0.0f, velToCutoff = 0.0f;

    Adsr ampEnv, filterEnv, modEnv { 0.0f, 0.3f, 0.0f, 0.3f };
    std::array<Lfo, 2> lfo;
    std::array<ModSlot, Params::numModSlots> mods;
    std::array<float, Params::numMacros> macros {};

    VoiceMode voiceMode = VoiceMode::Poly;
    int polyphony = 8;
    float glide = 0.0f;
    int bendRange = 2;
    float ampVelocity = 0.5f;
};

// Values shared by all voices for the current block.
struct GlobalModState
{
    float modWheel = 0.0f;
    float aftertouch = 0.0f;
    float pitchBend = 0.0f;                  // -1 .. 1
    std::array<float, 2> lfoPhase {};        // free-running LFO phases at block start
};

//==============================================================================
class Envelope
{
public:
    enum class Stage { Idle, Attack, Decay, Sustain, Release };

    void setSampleRate (double sr) noexcept { sampleRate = (float) sr; }
    void setParameters (const SynthParams::Adsr& p) noexcept;

    void noteOn (bool fromZero) noexcept;
    void noteOff() noexcept;
    void fastRelease() noexcept;
    void reset() noexcept { stage = Stage::Idle; level = 0.0f; }

    bool isActive() const noexcept { return stage != Stage::Idle; }
    Stage getStage() const noexcept { return stage; }
    float getLevel() const noexcept { return level; }

    float next() noexcept
    {
        switch (stage)
        {
            case Stage::Attack:
                level += attackInc;
                if (level >= 1.0f) { level = 1.0f; stage = Stage::Decay; }
                break;
            case Stage::Decay:
                level = sustain + (level - sustain) * decayCoef;
                if (level - sustain < 1.0e-4f) { level = sustain; stage = Stage::Sustain; }
                break;
            case Stage::Sustain:
                level = sustain;
                break;
            case Stage::Release:
                level *= releaseCoef;
                if (level < 1.0e-4f) { level = 0.0f; stage = Stage::Idle; }
                break;
            case Stage::Idle:
                break;
        }
        return level;
    }

private:
    float sampleRate = 44100.0f;
    Stage stage = Stage::Idle;
    float level = 0.0f;
    float attackInc = 1.0f, decayCoef = 0.0f, sustain = 1.0f, releaseCoef = 0.0f;
    bool fastMode = false;
};

//==============================================================================
class Lfo
{
public:
    void setSampleRate (double sr) noexcept { sampleRate = (float) sr; }
    void reset (float startPhase, juce::Random& rng) noexcept;

    // Advances by `numSamples` and returns the bipolar value at the new position.
    float advance (LfoShape shape, float rateHz, int numSamples) noexcept;

    float phase = 0.0f;

private:
    float sampleRate = 44100.0f;
    float holdValue = 0.0f, prevValue = 0.0f;
    juce::Random* random = nullptr;
};

//==============================================================================
// Zero-delay-feedback state variable filter (one channel).
struct Svf
{
    float ic1 = 0.0f, ic2 = 0.0f;

    void reset() noexcept { ic1 = ic2 = 0.0f; }

    // Returns {low, band, high}
    inline void process (float x, float g, float k, float& lp, float& bp, float& hp) noexcept
    {
        const float a1 = 1.0f / (1.0f + g * (g + k));
        const float v3 = x - ic2;
        const float v1 = a1 * ic1 + g * a1 * v3;
        const float v2 = ic2 + g * v1;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2;
        bp = v1;
        hp = x - k * v1 - v2;
    }
};

//==============================================================================
class Voice
{
public:
    static constexpr int maxUnison = 8;

    void prepare (double sampleRate);

    void start (int note, float velocity, float startPitch, const SynthParams& p, const GlobalModState& g, juce::Random& rng);
    // velocity < 0 keeps the current velocity
    void retrigger (int note, float velocity, const SynthParams& p, const GlobalModState& g, bool resetEnvelopes);
    void glideTo (int note, const SynthParams& p);
    void release();
    void stealFast();
    void kill();

    bool isActive() const noexcept { return active; }
    bool isReleased() const noexcept { return released; }
    bool isStolen() const noexcept { return stolen; }
    int getNote() const noexcept { return note; }
    float getCurrentPitch() const noexcept { return currentPitch; }
    uint64_t getAge() const noexcept { return startOrder; }
    void setAge (uint64_t order) noexcept { startOrder = order; }

    void syncFreeRunningLfos (const SynthParams& p, const GlobalModState& g);
    void render (float* left, float* right, int numSamples, const SynthParams& p, const GlobalModState& g);

private:
    void setPitchTarget (float target, const SynthParams& p);
    void updateControl (const SynthParams& p, const GlobalModState& g, int blockLength);
    float renderOscSample (int oscIndex, float fmInput, float& outL, float& outR) noexcept;

    double sampleRate = 44100.0;
    bool active = false, released = false, stolen = false;
    int note = 60;
    float velocity = 1.0f;
    float noteRandom = 0.0f;
    uint64_t startOrder = 0;

    // Pitch / glide (in semitones)
    float currentPitch = 60.0f, targetPitch = 60.0f, glideStep = 0.0f;

    Envelope ampEnv, filterEnv, modEnv;
    std::array<Lfo, 2> lfos;
    std::array<float, 2> lfoValue {};
    int samplesSinceStart = 0;
    int controlCounter = 0;

    struct OscState
    {
        std::array<float, maxUnison> phase {};
        std::array<float, maxUnison> inc {};
        std::array<float, maxUnison> gainL {}, gainR {};
        const float* frameA = nullptr;
        const float* frameB = nullptr;
        float frameMix = 0.0f;
        int voices = 1;
        bool enabled = false;
        WarpMode warpMode = WarpMode::Off;
        float warp = 0.0f;
        float lastOut = 0.0f;
    };
    std::array<OscState, 2> osc;

    // Sub & noise
    float subPhase = 0.0f, subInc = 0.0f, subGain = 0.0f;
    const float* subFrame = nullptr;
    float noiseGain = 0.0f, crackle = 0.0f;
    NoiseType noiseType = NoiseType::White;
    float pink[7] {};
    uint32_t noiseSeed = 22222;

    // Filter
    std::array<Svf, 2> svfA, svfB;   // [channel]
    float filterG = 0.5f, filterK = 1.4f;
    float driveGain = 1.0f, driveNorm = 1.0f;
    bool filterEnabled = true;
    FilterType filterType = FilterType::LP12;

    // Output gain/pan, interpolated across each control block
    float gainL = 0.0f, gainR = 0.0f, gainLStep = 0.0f, gainRStep = 0.0f;

    juce::Random* random = nullptr;

    inline float nextNoise() noexcept
    {
        noiseSeed = noiseSeed * 196314165u + 907633515u;
        return (float) (int32_t) noiseSeed * (1.0f / 2147483648.0f);
    }
};

//==============================================================================
class SynthEngine
{
public:
    static constexpr int maxVoices = 32;

    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Renders the block (overwrites `buffer`), handling the MIDI events inside it.
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                  const SynthParams& params, const std::array<float, 2>& freeLfoPhases);

    int getNumActiveVoices() const noexcept;

private:
    void renderVoices (juce::AudioBuffer<float>& buffer, int start, int end, const SynthParams& p);
    void handleMidi (const juce::MidiMessage& m, const SynthParams& p);
    void noteOn (int note, float velocity, const SynthParams& p);
    void noteOff (int note, const SynthParams& p);
    void releaseNote (int note, const SynthParams& p);
    void allNotesOff (bool hard);
    Voice* findFreeVoice();

    std::array<Voice, maxVoices> voices;
    double sampleRate = 44100.0;
    uint64_t noteCounter = 0;

    GlobalModState globals;
    std::vector<int> heldNotes;                  // for mono / legato note priority
    std::array<bool, 128> keyDown {}, sustainedNote {};
    bool sustainPedal = false;
    float lastNotePitch = -1.0f;
    juce::Random random;
};
