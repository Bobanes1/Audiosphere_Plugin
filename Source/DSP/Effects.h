#pragma once

#include <juce_dsp/juce_dsp.h>
#include "../Parameters.h"

struct FxParams
{
    bool distOn = false;
    DistType distType = DistType::Soft;
    float distDrive = 0.3f, distTone = 0.8f, distMix = 1.0f;

    bool lofiOn = false;
    float lofiBits = 12.0f, lofiRate = 26000.0f, lofiWow = 0.3f, lofiNoise = 0.2f, lofiTone = 0.6f, lofiMix = 1.0f;

    bool eqOn = false;
    float eqLow = 0.0f, eqMid = 0.0f, eqHigh = 0.0f;

    bool chorusOn = false;
    float chorusRate = 0.6f, chorusDepth = 0.4f, chorusMix = 0.4f;

    bool delayOn = false;
    float delaySeconds = 0.35f, delayFeedback = 0.35f, delayTone = 0.6f, delayMix = 0.25f;
    bool delayPingPong = true;

    bool reverbOn = false;
    float reverbSize = 0.6f, reverbDamp = 0.5f, reverbWidth = 1.0f, reverbMix = 0.25f;

    float masterGain = 1.0f;
    bool limiterOn = true;
};

//==============================================================================
class Distortion
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p);
    float getLatency() const { return oversampling.getLatencyInSamples(); }

private:
    juce::dsp::Oversampling<float> oversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    juce::AudioBuffer<float> dry;
    double sampleRate = 44100.0;
    float toneState[2] {}, dcX[2] {}, dcY[2] {};
    juce::SmoothedValue<float> drive, mix;
    bool primed = false;
};

//==============================================================================
class LoFi
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p);

private:
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> delayLine;
    int writePos = 0;
    double wowPhase = 0.0, flutterPhase = 0.0;
    float holdCounter = 1.0f, held[2] {};
    float lp1[2] {}, lp2[2] {};
    float crackle[2] {};
    juce::Random random;
    juce::SmoothedValue<float> mix;
    bool primed = false;
};

//==============================================================================
class Biquad
{
public:
    void setLowShelf (double sr, double freq, double q, double gainDb);
    void setPeak (double sr, double freq, double q, double gainDb);
    void setHighShelf (double sr, double freq, double q, double gainDb);
    void reset() { for (auto& s : state) s = { 0.0f, 0.0f }; }

    inline float process (float x, int ch) noexcept
    {
        auto& s = state[(size_t) ch];
        const float y = b0 * x + s[0];
        s[0] = b1 * x - a1 * y + s[1];
        s[1] = b2 * x - a2 * y;
        return y;
    }

private:
    void set (double b0, double b1, double b2, double a0, double a1, double a2);
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    std::array<std::array<float, 2>, 2> state {};
};

class Equalizer
{
public:
    void prepare (double sampleRate);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p);

private:
    double sampleRate = 44100.0;
    Biquad low, mid, high;
    float lastLow = 1000.0f, lastMid = 1000.0f, lastHigh = 1000.0f;
};

//==============================================================================
class StereoDelay
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p);

private:
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> line;
    int writePos = 0;
    float smoothedDelay = -1.0f;
    float lpState[2] {}, hpState[2] {};
    juce::SmoothedValue<float> mix, feedback;
    bool primed = false;
};

//==============================================================================
class Limiter
{
public:
    void prepare (double sampleRate);
    void reset() { envelope = 0.0f; }
    void process (juce::AudioBuffer<float>& buffer);

private:
    float envelope = 0.0f, releaseCoef = 0.999f;
    static constexpr float ceiling = 0.944f;   // -0.5 dBFS
};

//==============================================================================
class FxChain
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();
    void process (juce::AudioBuffer<float>& buffer, const FxParams& p);
    int getLatencySamples() const { return juce::roundToInt (distortion.getLatency()); }

private:
    Distortion distortion;
    LoFi lofi;
    Equalizer eq;
    juce::dsp::Chorus<float> chorus;
    StereoDelay delay;
    juce::Reverb reverb;
    Limiter limiter;

    juce::AudioBuffer<float> reverbBuffer;
    float reverbHp[2] {};
    juce::SmoothedValue<float> reverbMix, masterGain;
    double sampleRate = 44100.0;
    bool wasChorusOn = false, wasReverbOn = false, primed = false;
};
