#include "SynthEngine.h"

#include <cmath>

namespace
{
constexpr int controlRate = 16;                 // samples between modulation updates
constexpr float twoPi = juce::MathConstants<float>::twoPi;

inline float semitonesToHz (float semitones) noexcept
{
    return 440.0f * std::exp2 ((semitones - 69.0f) * (1.0f / 12.0f));
}

inline float readFrame (const float* a, const float* b, float mix, float phase) noexcept
{
    constexpr int size = WavetableBank::tableSize;
    const float idx = phase * (float) size;
    int i = (int) idx;
    i = i < 0 ? 0 : (i > size - 1 ? size - 1 : i);
    const float f = idx - (float) i;
    const float sa = a[i] + f * (a[i + 1] - a[i]);
    const float sb = b[i] + f * (b[i + 1] - b[i]);
    return sa + mix * (sb - sa);
}

inline float wrap01 (float x) noexcept
{
    return x - std::floor (x);
}

// Equal-power pan where the centre position has unity gain on both sides.
inline void panGains (float pan, float& l, float& r) noexcept
{
    const float angle = (juce::jlimit (-1.0f, 1.0f, pan) + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    l = std::cos (angle) * juce::MathConstants<float>::sqrt2;
    r = std::sin (angle) * juce::MathConstants<float>::sqrt2;
}
}

//==============================================================================
void Envelope::setParameters (const SynthParams::Adsr& p) noexcept
{
    attackInc = 1.0f / (std::max (p.a, 0.0005f) * sampleRate);
    decayCoef = std::exp (-6.9078f / (std::max (p.d, 0.001f) * sampleRate));
    sustain = p.s;

    if (! fastMode)
        releaseCoef = std::exp (-6.9078f / (std::max (p.r, 0.003f) * sampleRate));
}

void Envelope::noteOn (bool fromZero) noexcept
{
    fastMode = false;
    if (fromZero)
        level = 0.0f;
    stage = Stage::Attack;
}

void Envelope::noteOff() noexcept
{
    if (stage != Stage::Idle)
        stage = Stage::Release;
}

void Envelope::fastRelease() noexcept
{
    fastMode = true;
    releaseCoef = std::exp (-6.9078f / (0.004f * sampleRate));
    if (stage != Stage::Idle)
        stage = Stage::Release;
}

//==============================================================================
void Lfo::reset (float startPhase, juce::Random& rng) noexcept
{
    random = &rng;
    phase = startPhase;
    prevValue = 0.0f;
    holdValue = rng.nextFloat() * 2.0f - 1.0f;
}

float Lfo::advance (LfoShape shape, float rateHz, int numSamples) noexcept
{
    phase += rateHz * (float) numSamples / sampleRate;

    if (phase >= 1.0f)
    {
        phase -= std::floor (phase);
        prevValue = holdValue;
        holdValue = random != nullptr ? random->nextFloat() * 2.0f - 1.0f : 0.0f;
    }

    const float p = phase;
    switch (shape)
    {
        case LfoShape::Sine:         return std::sin (twoPi * p);
        case LfoShape::Triangle:     return p < 0.25f ? 4.0f * p : (p < 0.75f ? 2.0f - 4.0f * p : 4.0f * p - 4.0f);
        case LfoShape::SawUp:        return 2.0f * p - 1.0f;
        case LfoShape::SawDown:      return 1.0f - 2.0f * p;
        case LfoShape::Square:       return p < 0.5f ? 1.0f : -1.0f;
        case LfoShape::SampleHold:   return holdValue;
        case LfoShape::SmoothRandom: return prevValue + (holdValue - prevValue) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::pi * p));
    }
    return 0.0f;
}

//==============================================================================
void Voice::prepare (double sr)
{
    sampleRate = sr;
    ampEnv.setSampleRate (sr);
    filterEnv.setSampleRate (sr);
    modEnv.setSampleRate (sr);
    for (auto& l : lfos)
        l.setSampleRate (sr);
    kill();
}

void Voice::start (int newNote, float vel, float startPitch, const SynthParams& p, const GlobalModState& g, juce::Random& rng)
{
    random = &rng;
    active = true;
    released = false;
    stolen = false;
    note = newNote;
    velocity = vel;
    noteRandom = rng.nextFloat() * 2.0f - 1.0f;

    currentPitch = startPitch;
    glideStep = 0.0f;
    setPitchTarget ((float) newNote, p);

    ampEnv.setParameters (p.ampEnv);
    filterEnv.setParameters (p.filterEnv);
    modEnv.setParameters (p.modEnv);
    ampEnv.noteOn (true);
    filterEnv.noteOn (true);
    modEnv.noteOn (true);

    for (int l = 0; l < 2; ++l)
    {
        lfos[(size_t) l].reset (p.lfo[(size_t) l].retrig ? 0.0f : g.lfoPhase[(size_t) l], rng);
        lfoValue[(size_t) l] = 0.0f;
    }

    for (int o = 0; o < 2; ++o)
    {
        auto& s = osc[(size_t) o];
        for (int u = 0; u < maxUnison; ++u)
            s.phase[(size_t) u] = p.osc[(size_t) o].retrig ? (u == 0 ? 0.0f : wrap01 ((float) u * 0.618034f + 0.13f * (float) o))
                                                            : rng.nextFloat();
        s.lastOut = 0.0f;
    }

    subPhase = 0.0f;
    crackle = 0.0f;
    for (auto& v : pink)
        v = 0.0f;
    noiseSeed = (uint32_t) rng.nextInt();

    for (auto& f : svfA) f.reset();
    for (auto& f : svfB) f.reset();

    gainL = gainR = gainLStep = gainRStep = 0.0f;
    samplesSinceStart = 0;
    controlCounter = 0;
}

void Voice::retrigger (int newNote, float vel, const SynthParams& p, const GlobalModState& g, bool resetEnvelopes)
{
    note = newNote;
    if (vel >= 0.0f)
        velocity = vel;

    setPitchTarget ((float) newNote, p);

    if (resetEnvelopes || released)
    {
        released = false;
        stolen = false;
        ampEnv.noteOn (false);
        filterEnv.noteOn (false);
        modEnv.noteOn (false);

        for (int l = 0; l < 2; ++l)
            if (p.lfo[(size_t) l].retrig)
                lfos[(size_t) l].reset (0.0f, *random);
            else
                lfos[(size_t) l].phase = g.lfoPhase[(size_t) l];

        samplesSinceStart = 0;
    }
}

void Voice::glideTo (int newNote, const SynthParams& p)
{
    note = newNote;
    setPitchTarget ((float) newNote, p);
}

void Voice::setPitchTarget (float target, const SynthParams& p)
{
    targetPitch = target;

    if (p.glide > 0.0005f && std::abs (target - currentPitch) > 0.001f)
    {
        glideStep = (target - currentPitch) / (float) (p.glide * sampleRate);
    }
    else
    {
        currentPitch = target;
        glideStep = 0.0f;
    }
}

void Voice::release()
{
    released = true;
    ampEnv.noteOff();
    filterEnv.noteOff();
    modEnv.noteOff();
}

void Voice::stealFast()
{
    released = true;
    stolen = true;
    ampEnv.fastRelease();
    filterEnv.noteOff();
    modEnv.noteOff();
}

void Voice::kill()
{
    active = false;
    released = false;
    stolen = false;
    ampEnv.reset();
    filterEnv.reset();
    modEnv.reset();
}

void Voice::syncFreeRunningLfos (const SynthParams& p, const GlobalModState& g)
{
    for (int l = 0; l < 2; ++l)
        if (! p.lfo[(size_t) l].retrig)
            lfos[(size_t) l].phase = g.lfoPhase[(size_t) l];
}

void Voice::updateControl (const SynthParams& p, const GlobalModState& g, int blockLength)
{
    const auto& bank = WavetableBank::get();
    const float sr = (float) sampleRate;

    ampEnv.setParameters (p.ampEnv);
    filterEnv.setParameters (p.filterEnv);
    modEnv.setParameters (p.modEnv);

    // ---- Modulation matrix ---------------------------------------------------
    float src[ModSrc::NumSources] {};
    for (int l = 0; l < 2; ++l)
    {
        const float fade = p.lfo[(size_t) l].fade;
        const float fadeGain = fade > 0.0f ? std::min (1.0f, (float) samplesSinceStart / (fade * sr)) : 1.0f;
        src[ModSrc::Lfo1 + l] = lfoValue[(size_t) l] * fadeGain;
    }
    src[ModSrc::AmpEnv]     = ampEnv.getLevel();
    src[ModSrc::FiltEnv]    = filterEnv.getLevel();
    src[ModSrc::ModEnv]     = modEnv.getLevel();
    src[ModSrc::Velocity]   = velocity;
    src[ModSrc::ModWheel]   = g.modWheel;
    src[ModSrc::Aftertouch] = g.aftertouch;
    src[ModSrc::KeyTrack]   = ((float) note - 60.0f) / 60.0f;
    src[ModSrc::Random]     = noteRandom;
    for (int m = 0; m < Params::numMacros; ++m)
        src[ModSrc::Macro1 + m] = p.macros[(size_t) m];
    src[ModSrc::PitchBend]  = g.pitchBend;

    float dst[ModDst::NumDestinations] {};
    for (const auto& slot : p.mods)
        if (slot.src > 0 && slot.dst > 0 && slot.src < ModSrc::NumSources && slot.dst < ModDst::NumDestinations)
            dst[slot.dst] += slot.amt * src[slot.src];

    // ---- LFOs ------------------------------------------------------------------
    for (int l = 0; l < 2; ++l)
    {
        const float rate = p.lfo[(size_t) l].rateHz * std::exp2 (dst[ModDst::Lfo1Rate + l] * 4.0f);
        lfoValue[(size_t) l] = lfos[(size_t) l].advance (p.lfo[(size_t) l].shape, rate, blockLength);
    }

    // ---- Glide -----------------------------------------------------------------
    if (glideStep > 0.0f || glideStep < 0.0f)
    {
        currentPitch += glideStep * (float) blockLength;
        if ((glideStep > 0.0f && currentPitch >= targetPitch) || (glideStep < 0.0f && currentPitch <= targetPitch))
        {
            currentPitch = targetPitch;
            glideStep = 0.0f;
        }
    }

    const float basePitch = currentPitch + g.pitchBend * (float) p.bendRange
                          + dst[ModDst::Pitch] * 24.0f + dst[ModDst::PitchFine];

    // ---- Oscillators -------------------------------------------------------------
    for (int o = 0; o < 2; ++o)
    {
        const auto& op = p.osc[(size_t) o];
        auto& s = osc[(size_t) o];
        s.enabled = op.on;
        if (! op.on)
        {
            s.lastOut = 0.0f;
            continue;
        }

        const int base = o == 0 ? 0 : (ModDst::BPos - ModDst::APos);
        const float freq = semitonesToHz (basePitch + op.pitch);
        const int n = juce::jlimit (1, maxUnison, op.unison);
        const float detune = juce::jlimit (0.0f, 1.0f, op.detune + dst[ModDst::ADetune + base]);
        const float spreadCents = detune * detune * 60.0f;
        const float level = juce::jlimit (0.0f, 1.0f, op.level + dst[ModDst::ALevel + base]);
        const float pan = juce::jlimit (-1.0f, 1.0f, op.pan + dst[ModDst::APan + base]);
        const float norm = level / std::sqrt ((float) n);

        float maxInc = 0.0f;
        for (int u = 0; u < n; ++u)
        {
            const float spreadPos = n == 1 ? 0.0f : ((float) u / (float) (n - 1)) * 2.0f - 1.0f;
            const float inc = std::min (0.49f, freq * std::exp2 (spreadPos * spreadCents / 1200.0f) / sr);
            s.inc[(size_t) u] = inc;
            maxInc = std::max (maxInc, inc);

            float l, r;
            panGains (pan + spreadPos * op.spread, l, r);
            s.gainL[(size_t) u] = norm * l;
            s.gainR[(size_t) u] = norm * r;
        }
        s.voices = n;

        s.warpMode = op.warpMode;
        s.warp = juce::jlimit (0.0f, 1.0f, op.warp + dst[ModDst::AWarp + base]);

        float harmonicFactor = 1.0f;
        switch (s.warpMode)
        {
            case WarpMode::Sync:    harmonicFactor = 1.0f + s.warp * 7.0f; break;
            case WarpMode::Bend:
            case WarpMode::Squeeze: harmonicFactor = 1.0f + s.warp * 3.0f; break;
            case WarpMode::FM:      harmonicFactor = 1.0f + s.warp * s.warp * 6.0f; break;
            case WarpMode::Off:
            case WarpMode::Quantize:
            case WarpMode::AM:      break;
        }

        const int mip = WavetableBank::levelForIncrement (maxInc * harmonicFactor);
        const float position = juce::jlimit (0.0f, 1.0f, op.pos + dst[ModDst::APos + base]) * (float) (WavetableBank::numFrames - 1);
        int frame = (int) position;
        frame = std::min (frame, WavetableBank::numFrames - 2);
        s.frameMix = position - (float) frame;
        const int table = juce::jlimit (0, bank.getNumTables() - 1, op.table);
        s.frameA = bank.getFrame (table, mip, frame);
        s.frameB = bank.getFrame (table, mip, frame + 1);
    }

    // ---- Sub & noise -------------------------------------------------------------
    subGain = p.subOn ? juce::jlimit (0.0f, 1.0f, p.subLevel + dst[ModDst::SubLevel]) : 0.0f;
    if (subGain > 0.0f)
    {
        subInc = std::min (0.49f, semitonesToHz (basePitch + 12.0f * (float) p.subOct) / sr);
        // Sub shapes come from the band-limited "Basic Shapes" table.
        int frame = 0;
        switch (p.subShape)
        {
            case SubShape::Sine:     frame = 0;  break;
            case SubShape::Triangle: frame = 5;  break;
            case SubShape::Saw:      frame = 10; break;
            case SubShape::Square:   frame = 15; break;
        }
        subFrame = bank.getFrame (WT::BasicShapes, WavetableBank::levelForIncrement (subInc), frame);
    }

    noiseType = p.noiseType;
    noiseGain = p.noiseOn ? juce::jlimit (0.0f, 1.0f, p.noiseLevel + dst[ModDst::NoiseLevel]) : 0.0f;

    // ---- Filter ------------------------------------------------------------------
    filterEnabled = p.filterOn;
    filterType = p.filterType;
    if (filterEnabled)
    {
        const float octaves = filterEnv.getLevel() * p.envAmount * 8.0f
                            + p.keyTrack * (basePitch - 60.0f) / 12.0f
                            + p.velToCutoff * (velocity - 1.0f) * 4.0f
                            + dst[ModDst::Cutoff] * 5.0f;

        const float cutoff = juce::jlimit (16.0f, std::min (20000.0f, sr * 0.45f), p.cutoff * std::exp2 (octaves));
        filterG = std::tan (juce::MathConstants<float>::pi * cutoff / sr);

        const float res = juce::jlimit (0.0f, 1.0f, p.resonance + dst[ModDst::Resonance]);
        filterK = 2.0f - 1.96f * res;

        const float drive = juce::jlimit (0.0f, 1.0f, p.drive + dst[ModDst::FilterDrive]);
        driveGain = 1.0f + drive * 15.0f;
        driveNorm = 1.0f / std::sqrt (driveGain);
    }

    // ---- Amp / pan -----------------------------------------------------------------
    const float velGain = 1.0f - p.ampVelocity * (1.0f - std::pow (velocity, 1.5f));
    const float ampMod = juce::jlimit (0.0f, 2.0f, 1.0f + dst[ModDst::Amp]);
    float pl, pr;
    panGains (dst[ModDst::Pan], pl, pr);
    gainLStep = (velGain * ampMod * pl - gainL) / (float) controlRate;
    gainRStep = (velGain * ampMod * pr - gainR) / (float) controlRate;
}

float Voice::renderOscSample (int oscIndex, float fmInput, float& outL, float& outR) noexcept
{
    auto& s = osc[(size_t) oscIndex];
    float mono = 0.0f;

    for (int u = 0; u < s.voices; ++u)
    {
        const float ph = s.phase[(size_t) u];
        float rp = ph;

        switch (s.warpMode)
        {
            case WarpMode::Bend:
            {
                const float a = s.warp * 8.0f;
                rp = ph * (1.0f + a) / (1.0f + a * ph);
                break;
            }
            case WarpMode::Squeeze:
            {
                const float d = 0.5f - 0.48f * s.warp;
                rp = ph < d ? ph * 0.5f / d : 0.5f + (ph - d) * 0.5f / (1.0f - d);
                break;
            }
            case WarpMode::Sync:     rp = wrap01 (ph * (1.0f + s.warp * 7.0f)); break;
            case WarpMode::Quantize:
            {
                const float steps = std::exp2 (10.0f - s.warp * 8.0f);
                rp = std::floor (ph * steps) / steps;
                break;
            }
            case WarpMode::FM:       rp = wrap01 (ph + s.warp * s.warp * 3.0f * fmInput); break;
            case WarpMode::AM:
            case WarpMode::Off:      break;
        }

        float sample = readFrame (s.frameA, s.frameB, s.frameMix, rp);

        if (s.warpMode == WarpMode::AM)
            sample *= (1.0f - s.warp) + s.warp * fmInput;

        outL += sample * s.gainL[(size_t) u];
        outR += sample * s.gainR[(size_t) u];
        mono += sample;

        float next = ph + s.inc[(size_t) u];
        if (next >= 1.0f)
            next -= 1.0f;
        s.phase[(size_t) u] = next;
    }

    s.lastOut = mono / (float) s.voices;
    return s.lastOut;
}

void Voice::render (float* left, float* right, int numSamples, const SynthParams& p, const GlobalModState& g)
{
    int done = 0;

    while (done < numSamples && active)
    {
        if (controlCounter <= 0)
        {
            updateControl (p, g, controlRate);
            controlCounter = controlRate;
        }

        const int n = std::min (controlCounter, numSamples - done);

        for (int i = 0; i < n; ++i)
        {
            float l = 0.0f, r = 0.0f;

            // Osc B runs first so osc A can use its current output for FM / AM.
            if (osc[1].enabled)
                renderOscSample (1, osc[0].lastOut, l, r);
            if (osc[0].enabled)
                renderOscSample (0, osc[1].lastOut, l, r);

            float mono = 0.0f;

            if (subGain > 0.0f)
            {
                const float idx = subPhase * (float) WavetableBank::tableSize;
                const int si = std::min ((int) idx, WavetableBank::tableSize - 1);
                const float f = idx - (float) si;
                mono += (subFrame[si] + f * (subFrame[si + 1] - subFrame[si])) * subGain;
                subPhase += subInc;
                if (subPhase >= 1.0f)
                    subPhase -= 1.0f;
            }

            if (noiseGain > 0.0f)
            {
                const float white = nextNoise();
                float nz = white;

                if (noiseType == NoiseType::Pink)
                {
                    pink[0] = 0.99886f * pink[0] + white * 0.0555179f;
                    pink[1] = 0.99332f * pink[1] + white * 0.0750759f;
                    pink[2] = 0.96900f * pink[2] + white * 0.1538520f;
                    pink[3] = 0.86650f * pink[3] + white * 0.3104856f;
                    pink[4] = 0.55000f * pink[4] + white * 0.5329522f;
                    pink[5] = -0.7616f * pink[5] - white * 0.0168980f;
                    nz = (pink[0] + pink[1] + pink[2] + pink[3] + pink[4] + pink[5] + pink[6] + white * 0.5362f) * 0.11f;
                    pink[6] = white * 0.115926f;
                }
                else if (noiseType == NoiseType::Crackle)
                {
                    const float chance = nextNoise() * 0.5f + 0.5f;
                    if (chance < 30.0f / (float) sampleRate)
                        crackle = nextNoise();
                    crackle *= 0.8f;
                    nz = crackle * 1.5f + white * 0.06f;
                }

                mono += nz * noiseGain;
            }

            l += mono;
            r += mono;

            if (filterEnabled)
            {
                if (driveGain > 1.001f)
                {
                    l = std::tanh (l * driveGain) * driveNorm;
                    r = std::tanh (r * driveGain) * driveNorm;
                }

                float* ch[2] = { &l, &r };
                for (int c = 0; c < 2; ++c)
                {
                    float lp, bp, hp;
                    float& x = *ch[c];

                    switch (filterType)
                    {
                        case FilterType::LP12:
                            svfA[(size_t) c].process (x, filterG, filterK, lp, bp, hp);
                            x = lp;
                            break;
                        case FilterType::LP24:
                            svfA[(size_t) c].process (x, filterG, 1.41421f, lp, bp, hp);
                            svfB[(size_t) c].process (lp, filterG, filterK, lp, bp, hp);
                            x = lp;
                            break;
                        case FilterType::HP12:
                            svfA[(size_t) c].process (x, filterG, filterK, lp, bp, hp);
                            x = hp;
                            break;
                        case FilterType::HP24:
                            svfA[(size_t) c].process (x, filterG, 1.41421f, lp, bp, hp);
                            svfB[(size_t) c].process (hp, filterG, filterK, lp, bp, hp);
                            x = hp;
                            break;
                        case FilterType::BP:
                            svfA[(size_t) c].process (x, filterG, filterK, lp, bp, hp);
                            x = bp * filterK;
                            break;
                        case FilterType::Notch:
                            svfA[(size_t) c].process (x, filterG, filterK, lp, bp, hp);
                            x = lp + hp;
                            break;
                    }
                }
            }

            filterEnv.next();
            modEnv.next();
            const float env = ampEnv.next();

            gainL += gainLStep;
            gainR += gainRStep;

            left[done + i]  += l * env * gainL;
            right[done + i] += r * env * gainR;
        }

        done += n;
        controlCounter -= n;
        samplesSinceStart += n;

        if (! ampEnv.isActive()
            || (ampEnv.getStage() == Envelope::Stage::Sustain && ampEnv.getLevel() < 1.0e-4f))
            kill();
    }
}

//==============================================================================
void SynthEngine::prepare (double sr, int)
{
    sampleRate = sr;
    for (auto& v : voices)
        v.prepare (sr);
    reset();
}

void SynthEngine::reset()
{
    allNotesOff (true);
    globals = {};
    lastNotePitch = -1.0f;
}

int SynthEngine::getNumActiveVoices() const noexcept
{
    int count = 0;
    for (auto& v : voices)
        count += v.isActive() ? 1 : 0;
    return count;
}

void SynthEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi,
                           const SynthParams& params, const std::array<float, 2>& freeLfoPhases)
{
    buffer.clear();
    globals.lfoPhase = freeLfoPhases;

    for (auto& v : voices)
        if (v.isActive())
            v.syncFreeRunningLfos (params, globals);

    int position = 0;
    for (const auto meta : midi)
    {
        const int eventPos = juce::jlimit (0, buffer.getNumSamples(), meta.samplePosition);
        renderVoices (buffer, position, eventPos, params);
        handleMidi (meta.getMessage(), params);
        position = eventPos;
    }

    renderVoices (buffer, position, buffer.getNumSamples(), params);
}

void SynthEngine::renderVoices (juce::AudioBuffer<float>& buffer, int start, int end, const SynthParams& p)
{
    if (end <= start)
        return;

    float* left = buffer.getWritePointer (0) + start;
    float* right = buffer.getWritePointer (buffer.getNumChannels() > 1 ? 1 : 0) + start;

    for (auto& v : voices)
        if (v.isActive())
            v.render (left, right, end - start, p, globals);
}

void SynthEngine::handleMidi (const juce::MidiMessage& m, const SynthParams& p)
{
    if (m.isNoteOn())
    {
        noteOn (m.getNoteNumber(), m.getFloatVelocity(), p);
    }
    else if (m.isNoteOff())
    {
        noteOff (m.getNoteNumber(), p);
    }
    else if (m.isPitchWheel())
    {
        globals.pitchBend = juce::jlimit (-1.0f, 1.0f, (float) (m.getPitchWheelValue() - 8192) / 8192.0f);
    }
    else if (m.isControllerOfType (1))
    {
        globals.modWheel = (float) m.getControllerValue() / 127.0f;
    }
    else if (m.isSustainPedalOn())
    {
        sustainPedal = true;
    }
    else if (m.isSustainPedalOff())
    {
        sustainPedal = false;
        for (int n = 0; n < 128; ++n)
        {
            if (sustainedNote[(size_t) n] && ! keyDown[(size_t) n])
            {
                sustainedNote[(size_t) n] = false;
                releaseNote (n, p);
            }
        }
    }
    else if (m.isChannelPressure())
    {
        globals.aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    }
    else if (m.isAftertouch())
    {
        globals.aftertouch = (float) m.getAfterTouchValue() / 127.0f;
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff() || m.isResetAllControllers())
    {
        allNotesOff (m.isAllSoundOff());
        if (m.isResetAllControllers())
        {
            globals.modWheel = globals.aftertouch = globals.pitchBend = 0.0f;
            sustainPedal = false;
        }
    }
}

Voice* SynthEngine::findFreeVoice()
{
    for (auto& v : voices)
        if (! v.isActive())
            return &v;
    return nullptr;
}

void SynthEngine::noteOn (int note, float velocity, const SynthParams& p)
{
    keyDown[(size_t) note] = true;
    sustainedNote[(size_t) note] = false;

    if (p.voiceMode == VoiceMode::Poly)
    {
        // Re-striking a note takes over from its previous voice.
        for (auto& v : voices)
            if (v.isActive() && v.getNote() == note && ! v.isStolen())
                v.stealFast();

        int held = 0;
        for (auto& v : voices)
            held += (v.isActive() && ! v.isReleased()) ? 1 : 0;

        while (held >= std::max (1, p.polyphony))
        {
            Voice* oldest = nullptr;
            for (auto& v : voices)
                if (v.isActive() && ! v.isReleased() && (oldest == nullptr || v.getAge() < oldest->getAge()))
                    oldest = &v;
            if (oldest == nullptr)
                break;
            oldest->stealFast();
            --held;
        }

        Voice* voice = findFreeVoice();
        if (voice == nullptr)
        {
            for (auto& v : voices)
                if (voice == nullptr || v.getAge() < voice->getAge())
                    voice = &v;
            voice->kill();
        }

        const float startPitch = (p.glide > 0.0005f && lastNotePitch >= 0.0f) ? lastNotePitch : (float) note;
        voice->start (note, velocity, startPitch, p, globals, random);
        voice->setAge (++noteCounter);
        lastNotePitch = (float) note;
        return;
    }

    // ---- Mono / legato ------------------------------------------------------
    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());
    heldNotes.push_back (note);

    auto& voice = voices[0];

    // Other voices may still ring from poly mode; let them fade.
    for (size_t i = 1; i < voices.size(); ++i)
        if (voices[i].isActive() && ! voices[i].isReleased())
            voices[i].stealFast();

    if (voice.isActive() && ! voice.isReleased())
    {
        voice.retrigger (note, velocity, p, globals, p.voiceMode == VoiceMode::Mono);
    }
    else if (voice.isActive())
    {
        voice.retrigger (note, velocity, p, globals, true);
    }
    else
    {
        const bool glideFromLast = p.voiceMode == VoiceMode::Mono && p.glide > 0.0005f && lastNotePitch >= 0.0f;
        voice.start (note, velocity, glideFromLast ? lastNotePitch : (float) note, p, globals, random);
        voice.setAge (++noteCounter);
    }

    lastNotePitch = (float) note;
}

void SynthEngine::noteOff (int note, const SynthParams& p)
{
    keyDown[(size_t) note] = false;

    if (sustainPedal)
    {
        sustainedNote[(size_t) note] = true;
        return;
    }

    releaseNote (note, p);
}

void SynthEngine::releaseNote (int note, const SynthParams& p)
{
    if (p.voiceMode == VoiceMode::Poly)
    {
        for (auto& v : voices)
            if (v.isActive() && ! v.isReleased() && v.getNote() == note)
                v.release();
        return;
    }

    heldNotes.erase (std::remove (heldNotes.begin(), heldNotes.end(), note), heldNotes.end());

    auto& voice = voices[0];
    if (! voice.isActive() || voice.isReleased() || voice.getNote() != note)
        return;

    if (! heldNotes.empty())
    {
        const int previous = heldNotes.back();
        voice.glideTo (previous, p);
        lastNotePitch = (float) previous;
    }
    else
    {
        voice.release();
    }
}

void SynthEngine::allNotesOff (bool hard)
{
    for (auto& v : voices)
    {
        if (hard)
            v.kill();
        else if (v.isActive())
            v.release();
    }

    heldNotes.clear();
    keyDown.fill (false);
    sustainedNote.fill (false);
}
