#include "Effects.h"

#include <cmath>

namespace
{
constexpr float twoPi = juce::MathConstants<float>::twoPi;

inline float onePoleCoef (float cutoffHz, double sampleRate)
{
    return 1.0f - std::exp (-twoPi * cutoffHz / (float) sampleRate);
}
}

//==============================================================================
void Distortion::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    oversampling.initProcessing ((size_t) maxBlockSize);
    dry.setSize (2, maxBlockSize);
    drive.reset (sr, 0.03);
    mix.reset (sr, 0.03);
    reset();
}

void Distortion::reset()
{
    oversampling.reset();
    for (int c = 0; c < 2; ++c)
        toneState[c] = dcX[c] = dcY[c] = 0.0f;
    primed = false;
}

void Distortion::process (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (2, buffer.getNumChannels());

    for (int c = 0; c < numChannels; ++c)
        dry.copyFrom (c, 0, buffer, c, 0, numSamples);

    if (! primed)
    {
        drive.setCurrentAndTargetValue (p.distDrive);
        mix.setCurrentAndTargetValue (p.distMix);
        primed = true;
    }

    drive.setTargetValue (p.distDrive);
    mix.setTargetValue (p.distMix);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) numSamples);
    auto up = oversampling.processSamplesUp (block);
    const int upSamples = (int) up.getNumSamples();
    const float startDrive = drive.getCurrentValue();
    const float endDrive = drive.skip (numSamples);

    for (int i = 0; i < upSamples; ++i)
    {
        const float d = startDrive + (endDrive - startDrive) * (float) i / (float) upSamples;
        const float gain = 1.0f + d * d * 40.0f;
        const float makeup = 1.0f / (1.0f + d);

        for (int c = 0; c < numChannels; ++c)
        {
            float* data = up.getChannelPointer ((size_t) c);
            const float x = data[i] * gain;
            float y = 0.0f;

            switch (p.distType)
            {
                case DistType::Soft:    y = std::tanh (x); break;
                case DistType::Hard:    y = juce::jlimit (-1.0f, 1.0f, x); break;
                case DistType::Tube:    y = x >= 0.0f ? std::tanh (x) : std::tanh (0.6f * x) / 0.6f * 0.8f; break;
                case DistType::Fold:    y = std::sin (x); break;
                case DistType::Rectify: y = 0.5f * std::tanh (x) + 0.8f * std::tanh (std::abs (x)); break;
            }

            data[i] = y * makeup;
        }
    }

    oversampling.processSamplesDown (block);

    const float toneHz = 800.0f * std::exp2 (p.distTone * 4.64f);
    const float toneCoef = p.distTone >= 0.999f ? 1.0f : onePoleCoef (toneHz, sampleRate);

    for (int i = 0; i < numSamples; ++i)
    {
        const float m = mix.getNextValue();
        for (int c = 0; c < numChannels; ++c)
        {
            float* data = buffer.getWritePointer (c);
            float y = data[i];

            toneState[c] += toneCoef * (y - toneState[c]);
            y = toneState[c];

            // DC blocker (tube / rectify are asymmetric)
            const float out = y - dcX[c] + 0.9995f * dcY[c];
            dcX[c] = y;
            dcY[c] = out;

            data[i] = dry.getSample (c, i) + m * (out - dry.getSample (c, i));
        }
    }
}

//==============================================================================
void LoFi::prepare (double sr, int)
{
    sampleRate = sr;
    delayLine.setSize (2, (int) (sr * 0.05) + 4);
    mix.reset (sr, 0.03);
    reset();
}

void LoFi::reset()
{
    delayLine.clear();
    writePos = 0;
    wowPhase = flutterPhase = 0.0;
    holdCounter = 1.0f;
    for (int c = 0; c < 2; ++c)
        held[c] = lp1[c] = lp2[c] = crackle[c] = 0.0f;
    primed = false;
}

void LoFi::process (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (2, buffer.getNumChannels());
    const int lineSize = delayLine.getNumSamples();

    if (! primed)
    {
        mix.setCurrentAndTargetValue (p.lofiMix);
        primed = true;
    }
    mix.setTargetValue (p.lofiMix);

    const float sr = (float) sampleRate;
    const float baseDelay = 0.006f * sr;
    const float wowDepth = p.lofiWow * 0.003f * sr;
    const float flutterDepth = p.lofiWow * 0.0003f * sr;
    const double wowInc = 0.6 / sampleRate, flutterInc = 7.0 / sampleRate;

    const float holdInc = p.lofiRate / sr;
    const float steps = std::exp2 (p.lofiBits - 1.0f);
    const float toneCoef = onePoleCoef (1500.0f * std::exp2 (p.lofiTone * 3.7f), sampleRate);
    const float hiss = p.lofiNoise * 0.012f;
    const float crackleChance = p.lofiNoise * 9.0f / sr;

    for (int i = 0; i < numSamples; ++i)
    {
        const float m = mix.getNextValue();

        wowPhase += wowInc;
        flutterPhase += flutterInc;
        if (wowPhase >= 1.0) wowPhase -= 1.0;
        if (flutterPhase >= 1.0) flutterPhase -= 1.0;

        const float delay = baseDelay + wowDepth * std::sin (twoPi * (float) wowPhase)
                                      + flutterDepth * std::sin (twoPi * (float) flutterPhase);

        float readPos = (float) writePos - delay;
        if (readPos < 0.0f)
            readPos += (float) lineSize;
        const int r0 = (int) readPos;
        const int r1 = (r0 + 1) % lineSize;
        const float frac = readPos - (float) r0;

        holdCounter += holdInc;
        const bool sampleNow = holdCounter >= 1.0f;
        if (sampleNow)
            holdCounter -= std::floor (holdCounter);

        for (int c = 0; c < numChannels; ++c)
        {
            float* data = buffer.getWritePointer (c);
            const float in = data[i];

            delayLine.setSample (c, writePos, in);
            const float* line = delayLine.getReadPointer (c);
            float x = p.lofiWow > 0.001f ? line[r0] + frac * (line[r1] - line[r0]) : in;

            if (sampleNow || holdInc >= 1.0f)
                held[c] = x;
            x = held[c];

            x = std::round (x * steps) / steps;

            lp1[c] += toneCoef * (x - lp1[c]);
            lp2[c] += toneCoef * (lp1[c] - lp2[c]);
            x = lp2[c];

            if (p.lofiNoise > 0.0f)
            {
                if (random.nextFloat() < crackleChance)
                    crackle[c] = (random.nextFloat() * 2.0f - 1.0f) * 0.35f * p.lofiNoise;
                crackle[c] *= 0.6f;
                x += crackle[c] + (random.nextFloat() * 2.0f - 1.0f) * hiss;
            }

            data[i] = in + m * (x - in);
        }

        if (++writePos >= lineSize)
            writePos = 0;
    }
}

//==============================================================================
void Biquad::set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
{
    b0 = (float) (nb0 / a0);
    b1 = (float) (nb1 / a0);
    b2 = (float) (nb2 / a0);
    a1 = (float) (na1 / a0);
    a2 = (float) (na2 / a0);
}

void Biquad::setLowShelf (double sr, double freq, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0), w0 = juce::MathConstants<double>::twoPi * freq / sr;
    const double cs = std::cos (w0), alpha = std::sin (w0) / (2.0 * q), sq = 2.0 * std::sqrt (A) * alpha;
    set (A * ((A + 1) - (A - 1) * cs + sq), 2 * A * ((A - 1) - (A + 1) * cs), A * ((A + 1) - (A - 1) * cs - sq),
         (A + 1) + (A - 1) * cs + sq, -2 * ((A - 1) + (A + 1) * cs), (A + 1) + (A - 1) * cs - sq);
}

void Biquad::setHighShelf (double sr, double freq, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0), w0 = juce::MathConstants<double>::twoPi * freq / sr;
    const double cs = std::cos (w0), alpha = std::sin (w0) / (2.0 * q), sq = 2.0 * std::sqrt (A) * alpha;
    set (A * ((A + 1) + (A - 1) * cs + sq), -2 * A * ((A - 1) + (A + 1) * cs), A * ((A + 1) + (A - 1) * cs - sq),
         (A + 1) - (A - 1) * cs + sq, 2 * ((A - 1) - (A + 1) * cs), (A + 1) - (A - 1) * cs - sq);
}

void Biquad::setPeak (double sr, double freq, double q, double gainDb)
{
    const double A = std::pow (10.0, gainDb / 40.0), w0 = juce::MathConstants<double>::twoPi * freq / sr;
    const double cs = std::cos (w0), alpha = std::sin (w0) / (2.0 * q);
    set (1 + alpha * A, -2 * cs, 1 - alpha * A, 1 + alpha / A, -2 * cs, 1 - alpha / A);
}

void Equalizer::prepare (double sr)
{
    sampleRate = sr;
    lastLow = lastMid = lastHigh = 1000.0f;   // force a coefficient update
    reset();
}

void Equalizer::reset()
{
    low.reset();
    mid.reset();
    high.reset();
}

void Equalizer::process (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    if (std::abs (p.eqLow - lastLow) > 0.01f)   { low.setLowShelf (sampleRate, 110.0, 0.7, p.eqLow);    lastLow = p.eqLow; }
    if (std::abs (p.eqMid - lastMid) > 0.01f)   { mid.setPeak (sampleRate, 900.0, 0.8, p.eqMid);        lastMid = p.eqMid; }
    if (std::abs (p.eqHigh - lastHigh) > 0.01f) { high.setHighShelf (sampleRate, 7000.0, 0.7, p.eqHigh); lastHigh = p.eqHigh; }

    for (int c = 0; c < std::min (2, buffer.getNumChannels()); ++c)
    {
        float* data = buffer.getWritePointer (c);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = high.process (mid.process (low.process (data[i], c), c), c);
    }
}

//==============================================================================
void StereoDelay::prepare (double sr, int)
{
    sampleRate = sr;
    line.setSize (2, (int) (sr * 4.5) + 4);
    mix.reset (sr, 0.05);
    feedback.reset (sr, 0.05);
    reset();
}

void StereoDelay::reset()
{
    line.clear();
    writePos = 0;
    smoothedDelay = -1.0f;
    for (int c = 0; c < 2; ++c)
        lpState[c] = hpState[c] = 0.0f;
    primed = false;
}

void StereoDelay::process (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    const int numSamples = buffer.getNumSamples();
    const int lineSize = line.getNumSamples();
    const bool stereo = buffer.getNumChannels() > 1;

    const float target = juce::jlimit (1.0f, (float) lineSize - 4.0f, p.delaySeconds * (float) sampleRate);
    if (smoothedDelay < 0.0f)
        smoothedDelay = target;

    if (! primed)
    {
        mix.setCurrentAndTargetValue (p.delayMix);
        feedback.setCurrentAndTargetValue (p.delayFeedback);
        primed = true;
    }
    mix.setTargetValue (p.delayMix);
    feedback.setTargetValue (p.delayFeedback);

    const float lpCoef = onePoleCoef (1000.0f * std::exp2 (p.delayTone * 4.2f), sampleRate);
    const float hpCoef = onePoleCoef (90.0f, sampleRate);

    float* left = buffer.getWritePointer (0);
    float* right = buffer.getWritePointer (stereo ? 1 : 0);
    float* lineL = line.getWritePointer (0);
    float* lineR = line.getWritePointer (1);

    for (int i = 0; i < numSamples; ++i)
    {
        smoothedDelay += (target - smoothedDelay) * 0.0005f;

        float readPos = (float) writePos - smoothedDelay;
        if (readPos < 0.0f)
            readPos += (float) lineSize;
        const int r0 = (int) readPos;
        const int r1 = (r0 + 1) % lineSize;
        const float frac = readPos - (float) r0;

        float wet[2] = { lineL[r0] + frac * (lineL[r1] - lineL[r0]),
                         lineR[r0] + frac * (lineR[r1] - lineR[r0]) };

        for (int c = 0; c < 2; ++c)
        {
            lpState[c] += lpCoef * (wet[c] - lpState[c]);
            hpState[c] += hpCoef * (lpState[c] - hpState[c]);
            wet[c] = lpState[c] - hpState[c];
        }

        const float fb = feedback.getNextValue();
        const float m = mix.getNextValue();
        const float inL = left[i], inR = stereo ? right[i] : left[i];

        if (p.delayPingPong)
        {
            lineL[writePos] = 0.5f * (inL + inR) + fb * wet[1];
            lineR[writePos] = fb * wet[0];
        }
        else
        {
            lineL[writePos] = inL + fb * wet[0];
            lineR[writePos] = inR + fb * wet[1];
        }

        left[i] = inL + m * wet[0];
        if (stereo)
            right[i] = inR + m * wet[1];

        if (++writePos >= lineSize)
            writePos = 0;
    }
}

//==============================================================================
void Limiter::prepare (double sampleRate)
{
    releaseCoef = std::exp (-1.0f / (0.08f * (float) sampleRate));
    reset();
}

void Limiter::process (juce::AudioBuffer<float>& buffer)
{
    const int numChannels = std::min (2, buffer.getNumChannels());
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float peak = 0.0f;
        for (int c = 0; c < numChannels; ++c)
            peak = std::max (peak, std::abs (buffer.getSample (c, i)));

        envelope = std::max (peak, envelope * releaseCoef);
        if (envelope > ceiling)
        {
            const float gain = ceiling / envelope;
            for (int c = 0; c < numChannels; ++c)
                buffer.getWritePointer (c)[i] *= gain;
        }
    }
}

//==============================================================================
void FxChain::prepare (double sr, int maxBlockSize)
{
    sampleRate = sr;
    distortion.prepare (sr, maxBlockSize);
    lofi.prepare (sr, maxBlockSize);
    eq.prepare (sr);
    chorus.prepare ({ sr, (juce::uint32) maxBlockSize, 2 });
    delay.prepare (sr, maxBlockSize);
    reverb.setSampleRate (sr);
    limiter.prepare (sr);
    reverbBuffer.setSize (2, maxBlockSize);
    reverbMix.reset (sr, 0.05);
    masterGain.reset (sr, 0.03);
    reset();
}

void FxChain::reset()
{
    distortion.reset();
    lofi.reset();
    eq.reset();
    chorus.reset();
    delay.reset();
    reverb.reset();
    limiter.reset();
    reverbHp[0] = reverbHp[1] = 0.0f;
    primed = false;
}

void FxChain::process (juce::AudioBuffer<float>& buffer, const FxParams& p)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (2, buffer.getNumChannels());

    if (! primed)
    {
        masterGain.setCurrentAndTargetValue (p.masterGain);
        reverbMix.setCurrentAndTargetValue (p.reverbMix);
        primed = true;
    }

    if (p.distOn)
        distortion.process (buffer, p);

    if (p.lofiOn)
        lofi.process (buffer, p);

    if (p.eqOn)
        eq.process (buffer, p);

    if (p.chorusOn)
    {
        if (! wasChorusOn)
            chorus.reset();
        chorus.setRate (p.chorusRate);
        chorus.setDepth (p.chorusDepth * 0.6f);
        chorus.setCentreDelay (8.0f);
        chorus.setFeedback (0.1f);
        chorus.setMix (p.chorusMix);
        juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) numChannels, (size_t) numSamples);
        chorus.process (juce::dsp::ProcessContextReplacing<float> (block));
    }
    wasChorusOn = p.chorusOn;

    if (p.delayOn)
        delay.process (buffer, p);
    else
        delay.reset();

    if (p.reverbOn && numChannels == 2)
    {
        if (! wasReverbOn)
            reverb.reset();

        juce::Reverb::Parameters rp;
        rp.roomSize = 0.35f + 0.64f * p.reverbSize;
        rp.damping = p.reverbDamp;
        rp.wetLevel = 1.0f;
        rp.dryLevel = 0.0f;
        rp.width = p.reverbWidth;
        rp.freezeMode = 0.0f;
        reverb.setParameters (rp);

        // High-passed send keeps the low end (808s, sub) out of the reverb.
        const float hp = onePoleCoef (180.0f, sampleRate);
        for (int c = 0; c < 2; ++c)
        {
            const float* in = buffer.getReadPointer (c);
            float* out = reverbBuffer.getWritePointer (c);
            for (int i = 0; i < numSamples; ++i)
            {
                reverbHp[c] += hp * (in[i] - reverbHp[c]);
                out[i] = in[i] - reverbHp[c];
            }
        }

        reverb.processStereo (reverbBuffer.getWritePointer (0), reverbBuffer.getWritePointer (1), numSamples);

        reverbMix.setTargetValue (p.reverbMix);
        for (int i = 0; i < numSamples; ++i)
        {
            const float m = reverbMix.getNextValue();
            const float dryGain = 1.0f - 0.4f * m;
            for (int c = 0; c < 2; ++c)
            {
                float* data = buffer.getWritePointer (c);
                data[i] = data[i] * dryGain + reverbBuffer.getSample (c, i) * m * 0.6f;
            }
        }
    }
    wasReverbOn = p.reverbOn;

    masterGain.setTargetValue (p.masterGain);
    if (masterGain.isSmoothing())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const float g = masterGain.getNextValue();
            for (int c = 0; c < numChannels; ++c)
                buffer.getWritePointer (c)[i] *= g;
        }
    }
    else
    {
        buffer.applyGain (masterGain.getTargetValue());
    }

    if (p.limiterOn)
        limiter.process (buffer);
}
