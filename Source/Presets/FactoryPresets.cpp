#include "FactoryPresets.h"
#include "../Parameters.h"

namespace
{
constexpr int A = 0, B = 1;

// Small fluent builder so each preset reads like a patch sheet.
struct P
{
    FactoryPreset preset;

    P (const char* name, const char* category)
    {
        preset.name = name;
        preset.category = category;
    }

    operator FactoryPreset() const { return preset; }

    P& set (const juce::String& id, float v) { preset.values.emplace_back (id, v); return *this; }
    P& category (const char* c)              { preset.category = c; return *this; }

    // ---- Oscillators -------------------------------------------------------
    P& osc (int o, int table, float pos, float level = 0.75f)
    {
        return set (Params::osc (o, "on"), 1.0f).set (Params::osc (o, "table"), (float) table)
              .set (Params::osc (o, "pos"), pos).set (Params::osc (o, "level"), level);
    }
    P& oscOff (int o)                                 { return set (Params::osc (o, "on"), 0.0f); }
    P& pitch (int o, int oct, int semi = 0, float fine = 0.0f)
    {
        return set (Params::osc (o, "oct"), (float) oct).set (Params::osc (o, "semi"), (float) semi).set (Params::osc (o, "fine"), fine);
    }
    P& uni (int o, int voices, float detune, float spread = 0.7f)
    {
        return set (Params::osc (o, "unison"), (float) voices).set (Params::osc (o, "detune"), detune).set (Params::osc (o, "spread"), spread);
    }
    P& warp (int o, WarpMode mode, float amount)
    {
        return set (Params::osc (o, "warp_mode"), (float) mode).set (Params::osc (o, "warp"), amount);
    }
    P& pan (int o, float p)                           { return set (Params::osc (o, "pan"), p); }
    P& freeRun (int o)                                { return set (Params::osc (o, "retrig"), 0.0f); }

    P& sub (SubShape shape, int oct, float level)
    {
        return set ("sub_on", 1.0f).set ("sub_shape", (float) shape).set ("sub_oct", (float) oct).set ("sub_level", level);
    }
    P& noise (NoiseType type, float level)
    {
        return set ("noise_on", 1.0f).set ("noise_type", (float) type).set ("noise_level", level);
    }

    // ---- Filter ------------------------------------------------------------
    P& filter (FilterType type, float cutoff, float res, float env = 0.0f, float key = 0.0f)
    {
        return set ("flt_on", 1.0f).set ("flt_type", (float) type).set ("flt_cutoff", cutoff)
              .set ("flt_res", res).set ("flt_env", env).set ("flt_key", key);
    }
    P& filterOff()                                    { return set ("flt_on", 0.0f); }
    P& fdrive (float d)                               { return set ("flt_drive", d); }
    P& fvel (float v)                                 { return set ("flt_vel", v); }

    // ---- Envelopes -----------------------------------------------------------
    P& env (const char* prefix, float a, float d, float s, float r)
    {
        const juce::String p (prefix);
        return set (p + "_a", a).set (p + "_d", d).set (p + "_s", s).set (p + "_r", r);
    }
    P& amp (float a, float d, float s, float r)       { return env ("amp", a, d, s, r); }
    P& fenv (float a, float d, float s, float r)      { return env ("fenv", a, d, s, r); }
    P& menv (float a, float d, float s, float r)      { return env ("menv", a, d, s, r); }

    // ---- LFOs / modulation -----------------------------------------------------
    P& lfo (int l, LfoShape shape, float rate, bool retrig = true, float fade = 0.0f)
    {
        return set (Params::lfo (l, "shape"), (float) shape).set (Params::lfo (l, "rate"), rate)
              .set (Params::lfo (l, "sync"), 0.0f).set (Params::lfo (l, "retrig"), retrig ? 1.0f : 0.0f)
              .set (Params::lfo (l, "fade"), fade);
    }
    P& lfoSync (int l, LfoShape shape, int division, bool retrig = false)
    {
        return set (Params::lfo (l, "shape"), (float) shape).set (Params::lfo (l, "sync"), 1.0f)
              .set (Params::lfo (l, "div"), (float) division).set (Params::lfo (l, "retrig"), retrig ? 1.0f : 0.0f);
    }
    P& mod (int slot, int source, int destination, float amount)
    {
        const int s = slot - 1;
        return set (Params::mod (s, "src"), (float) source).set (Params::mod (s, "dst"), (float) destination)
              .set (Params::mod (s, "amt"), amount);
    }
    P& macro (int index, float value)                  { return set (Params::macro (index - 1), value); }

    // ---- Voicing -------------------------------------------------------------------
    P& mono (float glide, bool legato = true)
    {
        return set ("voice_mode", (float) (legato ? VoiceMode::Legato : VoiceMode::Mono)).set ("glide", glide);
    }
    P& poly (int voices, float glide = 0.0f)
    {
        return set ("voice_mode", (float) VoiceMode::Poly).set ("polyphony", (float) voices).set ("glide", glide);
    }
    P& vel (float sensitivity)                         { return set ("amp_vel", sensitivity); }
    P& bend (int semitones)                            { return set ("bend_range", (float) semitones); }
    P& vol (float db)                                  { return set ("master_vol", db); }

    // ---- Effects -------------------------------------------------------------------
    P& dist (DistType type, float drive, float tone = 0.8f, float mix = 1.0f)
    {
        return set ("dist_on", 1.0f).set ("dist_type", (float) type).set ("dist_drive", drive)
              .set ("dist_tone", tone).set ("dist_mix", mix);
    }
    P& lofi (float bits, float rate, float wow, float noiseAmount, float tone, float mix = 1.0f)
    {
        return set ("lofi_on", 1.0f).set ("lofi_bits", bits).set ("lofi_rate", rate).set ("lofi_wow", wow)
              .set ("lofi_noise", noiseAmount).set ("lofi_tone", tone).set ("lofi_mix", mix);
    }
    P& eq (float low, float mid, float high)
    {
        return set ("eq_on", 1.0f).set ("eq_low", low).set ("eq_mid", mid).set ("eq_high", high);
    }
    P& chorus (float rate, float depth, float mix)
    {
        return set ("chorus_on", 1.0f).set ("chorus_rate", rate).set ("chorus_depth", depth).set ("chorus_mix", mix);
    }
    P& delay (int division, float feedback, float mix, bool pingPong = true, float tone = 0.55f)
    {
        return set ("delay_on", 1.0f).set ("delay_sync", 1.0f).set ("delay_div", (float) division)
              .set ("delay_fb", feedback).set ("delay_mix", mix).set ("delay_pingpong", pingPong ? 1.0f : 0.0f)
              .set ("delay_tone", tone);
    }
    P& reverb (float size, float damp, float mix, float width = 1.0f)
    {
        return set ("reverb_on", 1.0f).set ("reverb_size", size).set ("reverb_damp", damp)
              .set ("reverb_mix", mix).set ("reverb_width", width);
    }
};

using namespace ModSrc;
using namespace ModDst;
using WM = WarpMode;
using FT = FilterType;
using LS = LfoShape;
using SD = SyncDiv::Division;

const char* const bass   = "808 & Bass";
const char* const keys   = "Keys";
const char* const pads   = "Pads";
const char* const leads  = "Leads";
const char* const plucks = "Plucks";
const char* const bells  = "Bells & Mallets";
const char* const orch   = "Strings & Brass";
const char* const lofiC  = "Lo-Fi";
const char* const fx     = "FX & Atmos";

// Shared building blocks -------------------------------------------------------------
// Classic 808: sine body with a fast pitch drop and legato slides.
P base808 (const char* name, float decay, float sustain, float glide, float pitchDrop)
{
    P p (name, bass);
    p.osc (A, WT::BasicShapes, 0.0f, 0.9f)
     .filter (FT::LP24, 20000.0f, 0.0f)
     .amp (0.001f, decay, sustain, 0.12f)
     .menv (0.0f, 0.06f, 0.0f, 0.05f)
     .mod (1, ModEnv, Pitch, pitchDrop)
     .mod (8, Macro1, FilterDrive, 0.8f)      // Macro 1: grit
     .mono (glide).bend (12).vel (0.25f);
    return p;
}

// FM electric piano core: tine bark from velocity + mod envelope on the FM index.
P baseEPiano (const char* name, const char* category)
{
    P p (name, category);
    p.osc (A, WT::EPiano, 0.04f, 0.8f)
     .osc (B, WT::BasicShapes, 0.0f, 0.0f).pitch (B, 2)
     .filter (FT::LP12, 6000.0f, 0.05f).fvel (0.35f)
     .amp (0.002f, 4.0f, 0.12f, 0.45f)
     .fenv (0.0f, 0.08f, 0.0f, 0.05f)
     .menv (0.0f, 1.4f, 0.0f, 0.6f)
     .mod (1, ModEnv, APos, 0.3f)
     .mod (2, Velocity, APos, 0.22f)
     .mod (3, FiltEnv, BLevel, 0.22f)        // short tine "ping" an octave+ above
     .mod (8, Macro1, APos, 0.4f)            // Macro 1: bark
     .poly (12).vel (0.7f);
    return p;
}

// Two-operator FM bell: osc B (silent) modulates osc A's phase.
P baseFmBell (const char* name, int semi, float fine, float index, float indexEnv, float decay)
{
    P p (name, bells);
    p.osc (A, WT::BasicShapes, 0.0f, 0.8f)
     .osc (B, WT::BasicShapes, 0.0f, 0.0f).pitch (B, 0, semi, fine)
     .warp (A, WM::FM, index)
     .filterOff()
     .amp (0.001f, decay, 0.0f, decay * 0.4f)
     .menv (0.0f, decay * 0.35f, 0.0f, 0.4f)
     .mod (1, ModEnv, AWarp, indexEnv)
     .mod (2, Velocity, AWarp, 0.15f)
     .mod (8, Macro1, AWarp, 0.35f)          // Macro 1: metallic
     .poly (12).vel (0.6f);
    return p;
}

std::vector<FactoryPreset> buildPresets()
{
    std::vector<FactoryPreset> list;
    auto add = [&list] (const P& p) { list.push_back (p); };

    // =====================================================================
    // 808 & BASS
    // =====================================================================
    add (base808 ("808 Boom", 2.8f, 0.0f, 0.06f, 0.3f)
            .dist (DistType::Soft, 0.18f, 0.6f).vol (-3.8f));

    add (base808 ("808 Hard Trap", 1.2f, 0.55f, 0.07f, 0.25f)
            .osc (B, WT::BasicShapes, 0.33f, 0.3f)
            .filter (FT::LP24, 6000.0f, 0.0f)
            .dist (DistType::Tube, 0.55f, 0.55f).vol (-6.1f));

    add (base808 ("808 Mafia Slide", 1.5f, 0.7f, 0.12f, 0.2f)
            .dist (DistType::Soft, 0.4f, 0.6f).eq (3.0f, 0.0f, 0.0f).vol (-8.0f));

    add (base808 ("808 Punchy Short", 0.6f, 0.0f, 0.0f, 0.45f)
            .set ("voice_mode", (float) VoiceMode::Mono)
            .menv (0.0f, 0.04f, 0.0f, 0.04f)
            .dist (DistType::Soft, 0.3f, 0.7f).vol (-0.4f));

    add (base808 ("808 Long Rumble", 6.0f, 0.0f, 0.09f, 0.22f)
            .osc (A, WT::BasicShapes, 0.1f, 0.9f)
            .filter (FT::LP24, 900.0f, 0.0f).fdrive (0.2f)
            .amp (0.001f, 6.0f, 0.0f, 0.5f).vol (0.3f));

    add (base808 ("808 Growl", 1.4f, 0.6f, 0.08f, 0.25f)
            .osc (A, WT::Growl, 0.25f, 0.8f)
            .sub (SubShape::Sine, 0, 0.6f)
            .filter (FT::LP24, 2500.0f, 0.1f)
            .mod (2, ModEnv, APos, 0.3f)
            .dist (DistType::Soft, 0.3f, 0.6f).vol (-5.4f));

    add (base808 ("808 Distorted Square", 1.3f, 0.6f, 0.07f, 0.25f)
            .osc (A, WT::BasicShapes, 0.15f, 0.85f)
            .filter (FT::LP24, 3500.0f, 0.0f)
            .dist (DistType::Hard, 0.45f, 0.5f).vol (-5.3f));

    add (base808 ("Drill Glide 808", 1.2f, 0.8f, 0.15f, 0.2f)
            .dist (DistType::Hard, 0.35f, 0.45f).eq (2.0f, 0.0f, -3.0f).vol (-7.6f));

    add (P ("Pure Sub", bass)
            .osc (A, WT::BasicShapes, 0.0f, 0.9f).filterOff()
            .amp (0.003f, 0.3f, 1.0f, 0.1f)
            .mono (0.03f).vel (0.2f).vol (-4.8f));

    add (P ("Reese Bass", bass)
            .osc (A, WT::AnalogSaw, 1.0f, 0.7f).uni (A, 4, 0.35f, 0.5f)
            .osc (B, WT::AnalogSaw, 1.0f, 0.5f).pitch (B, 0, 0, 12.0f).uni (B, 2, 0.3f, 0.6f)
            .sub (SubShape::Sine, -1, 0.5f)
            .filter (FT::LP24, 700.0f, 0.2f).fdrive (0.2f)
            .amp (0.005f, 0.5f, 1.0f, 0.15f)
            .lfo (0, LS::Sine, 0.3f, false)
            .mod (1, Lfo1, Cutoff, 0.15f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .mono (0.05f).vel (0.2f).vol (-1.2f));

    add (P ("Wobble Bass", bass)
            .osc (A, WT::AnalogSaw, 1.0f, 0.7f).uni (A, 3, 0.2f, 0.5f)
            .osc (B, WT::BasicShapes, 1.0f, 0.45f).pitch (B, -1)
            .sub (SubShape::Sine, -1, 0.6f)
            .filter (FT::LP24, 300.0f, 0.45f)
            .lfoSync (0, LS::Sine, SD::D1_8)
            .mod (1, Lfo1, Cutoff, 0.6f)
            .mod (8, Macro1, Cutoff, 0.4f)
            .dist (DistType::Soft, 0.3f, 0.7f)
            .mono (0.04f).vel (0.2f).vol (-6.4f));

    add (P ("Funk Pluck Bass", bass)
            .osc (A, WT::BasicShapes, 0.67f, 0.7f)
            .osc (B, WT::BasicShapes, 1.0f, 0.4f).pitch (B, -1)
            .filter (FT::LP24, 250.0f, 0.35f, 0.45f).fvel (0.4f)
            .fenv (0.0f, 0.25f, 0.1f, 0.1f)
            .amp (0.002f, 0.6f, 0.6f, 0.1f)
            .mono (0.03f).vel (0.5f).vol (-0.8f));

    add (P ("Acid Line", bass)
            .osc (A, WT::BasicShapes, 0.67f, 0.8f)
            .filter (FT::LP24, 300.0f, 0.75f, 0.5f).fdrive (0.3f)
            .fenv (0.0f, 0.2f, 0.0f, 0.1f)
            .amp (0.002f, 0.4f, 0.8f, 0.08f)
            .mod (1, Velocity, Cutoff, 0.3f)
            .mod (8, Macro1, Resonance, 0.25f)
            .dist (DistType::Soft, 0.25f, 0.7f)
            .mono (0.08f).vol (-4.4f));

    add (P ("Analog Mono Bass", bass)
            .osc (A, WT::AnalogSaw, 0.8f, 0.7f)
            .osc (B, WT::BasicShapes, 1.0f, 0.45f).pitch (B, -1)
            .filter (FT::LP24, 500.0f, 0.3f, 0.25f).fdrive (0.25f)
            .fenv (0.0f, 0.35f, 0.3f, 0.15f)
            .amp (0.003f, 0.5f, 0.85f, 0.12f)
            .mono (0.02f).vel (0.35f).vol (0.9f));

    add (P ("Lo-Fi Upright", bass)
            .osc (A, WT::BasicShapes, 0.2f, 0.85f)
            .filter (FT::LP12, 600.0f, 0.1f, 0.3f, 0.5f)
            .fenv (0.005f, 0.3f, 0.0f, 0.2f)
            .amp (0.004f, 1.2f, 0.0f, 0.25f)
            .lofi (12.0f, 22000.0f, 0.2f, 0.1f, 0.5f)
            .poly (4).vel (0.6f).vol (1.4f));

    add (P ("Boom Bap Bass", bass)
            .osc (A, WT::BasicShapes, 0.25f, 0.85f)
            .sub (SubShape::Sine, 0, 0.4f)
            .filter (FT::LP12, 450.0f, 0.15f, 0.15f)
            .fenv (0.0f, 0.2f, 0.0f, 0.1f)
            .amp (0.002f, 0.9f, 0.4f, 0.15f)
            .lofi (11.0f, 20000.0f, 0.1f, 0.1f, 0.45f)
            .mono (0.0f, false).vel (0.5f).vol (-2.0f));

    // =====================================================================
    // KEYS
    // =====================================================================
    add (baseEPiano ("Electric Tines", keys)
            .lfo (0, LS::Sine, 4.5f, false)
            .mod (4, Lfo1, Pan, 0.25f)
            .chorus (0.5f, 0.3f, 0.2f)
            .reverb (0.45f, 0.5f, 0.18f).vol (-8.2f));

    add (baseEPiano ("Soul Keys", keys)
            .uni (A, 2, 0.12f, 0.6f)
            .chorus (0.6f, 0.5f, 0.4f)
            .reverb (0.55f, 0.5f, 0.22f).vol (-7.4f));

    add (baseEPiano ("Dreamy Keys", keys)
            .osc (A, WT::EPiano, 0.12f, 0.8f).uni (A, 3, 0.15f, 0.8f)
            .chorus (0.4f, 0.5f, 0.4f)
            .delay (SD::D1_4D, 0.3f, 0.2f)
            .reverb (0.8f, 0.4f, 0.35f).vol (-6.8f));

    add (P ("Dusty Piano", keys)
            .osc (A, WT::Mallet, 0.4f, 0.7f)
            .osc (B, WT::AnalogSaw, 0.55f, 0.35f).pitch (B, 0, 0, 4.0f)
            .filter (FT::LP12, 1400.0f, 0.05f, 0.35f, 0.6f).fvel (0.4f)
            .fenv (0.0f, 0.6f, 0.1f, 0.4f)
            .amp (0.001f, 2.5f, 0.0f, 0.4f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .lofi (12.0f, 24000.0f, 0.15f, 0.2f, 0.45f)
            .reverb (0.5f, 0.6f, 0.2f)
            .poly (12).vel (0.7f).vol (-7.4f));

    add (P ("Gospel Organ", keys)
            .osc (A, WT::Organ, 0.55f, 0.75f)
            .filter (FT::LP12, 9000.0f, 0.0f)
            .amp (0.005f, 0.1f, 1.0f, 0.08f)
            .lfo (0, LS::Sine, 6.0f, false)
            .mod (1, Lfo1, Pan, 0.15f)
            .mod (2, Lfo1, PitchFine, 0.04f)
            .mod (3, ModWheel, APos, 0.45f)
            .mod (8, Macro1, APos, 0.45f)
            .dist (DistType::Tube, 0.15f, 0.7f)
            .chorus (0.8f, 0.4f, 0.35f)
            .reverb (0.5f, 0.5f, 0.15f)
            .poly (12).vel (0.0f).vol (-6.6f));

    add (P ("Wurly Grit", keys)
            .osc (A, WT::Hollow, 0.35f, 0.7f)
            .osc (B, WT::BasicShapes, 0.0f, 0.5f)
            .filter (FT::LP12, 2500.0f, 0.05f).fvel (0.5f)
            .amp (0.002f, 2.5f, 0.1f, 0.3f)
            .lfo (0, LS::Sine, 5.0f, false)
            .mod (1, Lfo1, Amp, 0.2f)
            .mod (2, Velocity, APos, 0.3f)
            .dist (DistType::Tube, 0.25f, 0.6f)
            .poly (12).vel (0.7f).vol (-10.5f));

    add (P ("Funk Clav", keys)
            .osc (A, WT::PulseWidth, 0.6f, 0.75f)
            .filter (FT::LP12, 1500.0f, 0.3f, 0.4f).fvel (0.4f)
            .fenv (0.0f, 0.15f, 0.2f, 0.1f)
            .amp (0.001f, 0.5f, 0.3f, 0.08f)
            .poly (8).vel (0.6f).vol (-8.2f));

    add (P ("Toy Piano", keys)
            .osc (A, WT::Glass, 0.3f, 0.7f)
            .osc (B, WT::Mallet, 0.5f, 0.3f).pitch (B, 1)
            .filter (FT::LP12, 7000.0f, 0.0f)
            .amp (0.001f, 0.9f, 0.0f, 0.3f)
            .reverb (0.4f, 0.4f, 0.25f)
            .poly (8).vel (0.6f).vol (-6.1f));

    add (P ("Trap Piano Keys", keys)
            .osc (A, WT::AnalogSaw, 0.7f, 0.55f)
            .osc (B, WT::Mallet, 0.7f, 0.55f)
            .filter (FT::LP12, 1800.0f, 0.05f, 0.4f, 0.5f).fvel (0.4f)
            .fenv (0.0f, 0.9f, 0.2f, 0.5f)
            .amp (0.001f, 3.0f, 0.0f, 0.5f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .eq (0.0f, 0.0f, 2.0f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (12).vel (0.7f).vol (-8.4f));

    add (P ("Organ Drawbars Soft", keys)
            .osc (A, WT::Organ, 0.1f, 0.75f)
            .filter (FT::LP12, 6000.0f, 0.0f)
            .amp (0.008f, 0.1f, 1.0f, 0.1f)
            .lfo (0, LS::Sine, 5.5f, false)
            .mod (1, Lfo1, Pan, 0.12f)
            .chorus (0.7f, 0.35f, 0.3f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (12).vel (0.0f).vol (-8.9f));

    // =====================================================================
    // PADS
    // =====================================================================
    add (P ("Warm Analog Pad", pads)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 5, 0.3f, 0.8f).freeRun (A)
            .osc (B, WT::AnalogSaw, 0.8f, 0.45f).pitch (B, -1).uni (B, 3, 0.25f, 0.6f).freeRun (B)
            .filter (FT::LP24, 1200.0f, 0.15f, 0.2f)
            .fenv (0.8f, 2.0f, 0.6f, 2.0f)
            .amp (0.6f, 1.0f, 1.0f, 1.5f)
            .lfo (0, LS::Triangle, 0.2f, false)
            .mod (1, Lfo1, Cutoff, 0.08f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .chorus (0.3f, 0.5f, 0.3f)
            .reverb (0.7f, 0.5f, 0.35f)
            .poly (12).vel (0.3f).vol (-10.8f));

    add (P ("Dark Trap Pad", pads)
            .osc (A, WT::Hollow, 0.6f, 0.6f).uni (A, 4, 0.25f, 0.8f).freeRun (A)
            .osc (B, WT::Strings, 0.3f, 0.5f).uni (B, 3, 0.2f, 0.7f).freeRun (B)
            .filter (FT::LP24, 700.0f, 0.2f)
            .amp (1.0f, 1.0f, 1.0f, 2.5f)
            .lfo (0, LS::SmoothRandom, 0.3f, false)
            .mod (1, Lfo1, APos, 0.2f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .reverb (0.85f, 0.5f, 0.4f)
            .poly (12).vel (0.2f).vol (-14.5f));

    add (P ("Choir Aahs", pads)
            .osc (A, WT::Choir, 0.66f, 0.65f).uni (A, 4, 0.18f, 0.9f).freeRun (A)
            .osc (B, WT::Choir, 0.33f, 0.45f).uni (B, 3, 0.22f, 0.8f).freeRun (B)
            .filter (FT::LP12, 4500.0f, 0.05f)
            .amp (0.35f, 1.0f, 1.0f, 1.2f)
            .lfo (0, LS::Sine, 5.0f, true, 1.0f)
            .lfo (1, LS::SmoothRandom, 0.2f, false)
            .mod (1, Lfo1, PitchFine, 0.1f)
            .mod (2, Lfo2, APos, 0.1f)
            .mod (3, ModWheel, APos, -0.6f)
            .chorus (0.4f, 0.4f, 0.3f)
            .reverb (0.8f, 0.4f, 0.4f)
            .poly (12).vel (0.3f).vol (-10.4f));

    add (P ("Ethereal Glass Pad", pads)
            .osc (A, WT::Glass, 0.6f, 0.6f).uni (A, 4, 0.2f, 0.9f).freeRun (A)
            .osc (B, WT::BasicShapes, 0.0f, 0.3f).pitch (B, 1)
            .filter (FT::LP12, 6000.0f, 0.05f)
            .amp (1.2f, 1.0f, 1.0f, 2.5f)
            .lfo (0, LS::Sine, 0.15f, false)
            .mod (1, Lfo1, APos, 0.3f)
            .delay (SD::D1_4D, 0.4f, 0.2f)
            .reverb (0.9f, 0.3f, 0.45f)
            .poly (12).vel (0.2f).vol (-12.6f));

    add (P ("Evolving Spectra", pads)
            .osc (A, WT::Digital, 0.2f, 0.55f).uni (A, 3, 0.2f, 0.8f).freeRun (A)
            .osc (B, WT::Vowels, 0.5f, 0.5f).uni (B, 3, 0.15f, 0.8f).freeRun (B)
            .filter (FT::LP24, 3000.0f, 0.2f)
            .amp (1.5f, 1.0f, 1.0f, 3.0f)
            .lfo (0, LS::Triangle, 0.08f, false)
            .lfo (1, LS::SmoothRandom, 0.2f, false)
            .mod (1, Lfo1, APos, 0.5f)
            .mod (2, Lfo2, BPos, 0.4f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .chorus (0.3f, 0.5f, 0.3f)
            .reverb (0.85f, 0.4f, 0.4f)
            .poly (12).vel (0.2f).vol (-9.3f));

    add (P ("Lo-Fi Tape Pad", pads)
            .osc (A, WT::AnalogSaw, 0.5f, 0.6f).uni (A, 3, 0.2f, 0.7f).freeRun (A)
            .osc (B, WT::BasicShapes, 0.33f, 0.5f).freeRun (B)
            .filter (FT::LP12, 1600.0f, 0.1f)
            .amp (0.5f, 1.0f, 1.0f, 1.2f)
            .lofi (11.0f, 20000.0f, 0.55f, 0.25f, 0.4f)
            .reverb (0.6f, 0.6f, 0.3f)
            .poly (12).vel (0.3f).vol (-11.5f));

    add (P ("Sunset Strings Pad", pads)
            .osc (A, WT::Strings, 0.6f, 0.6f).uni (A, 6, 0.25f, 0.9f).freeRun (A)
            .osc (B, WT::Strings, 0.3f, 0.45f).pitch (B, -1).uni (B, 3, 0.2f, 0.6f).freeRun (B)
            .filter (FT::LP12, 3500.0f, 0.05f)
            .amp (0.7f, 1.0f, 1.0f, 1.8f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .chorus (0.35f, 0.4f, 0.3f)
            .reverb (0.75f, 0.5f, 0.35f)
            .poly (12).vel (0.3f).vol (-9.9f));

    add (P ("Cloud Pad", pads)
            .osc (A, WT::Vowels, 0.8f, 0.6f).uni (A, 5, 0.3f, 0.9f).freeRun (A)
            .sub (SubShape::Sine, 0, 0.3f)
            .filter (FT::LP12, 2000.0f, 0.1f, 0.15f)
            .fenv (1.5f, 3.0f, 0.5f, 3.0f)
            .amp (1.8f, 1.0f, 1.0f, 3.0f)
            .delay (SD::D1_4, 0.35f, 0.15f)
            .reverb (0.95f, 0.4f, 0.5f)
            .poly (12).vel (0.2f).vol (-11.5f));

    add (P ("Retro Synth Pad", pads)
            .osc (A, WT::PulseWidth, 0.3f, 0.6f).uni (A, 4, 0.2f, 0.8f).freeRun (A)
            .osc (B, WT::AnalogSaw, 0.9f, 0.45f).uni (B, 2, 0.15f, 0.6f).freeRun (B)
            .filter (FT::LP24, 1500.0f, 0.25f, 0.3f)
            .fenv (0.4f, 1.5f, 0.4f, 1.5f)
            .amp (0.3f, 1.0f, 1.0f, 1.0f)
            .lfo (0, LS::Triangle, 0.4f, false)
            .mod (1, Lfo1, APos, 0.25f)
            .chorus (0.4f, 0.7f, 0.5f)
            .reverb (0.7f, 0.5f, 0.3f)
            .poly (12).vel (0.3f).vol (-11.3f));

    add (P ("Ambient Wash", pads)
            .osc (A, WT::Additive, 0.3f, 0.6f).uni (A, 6, 0.4f, 1.0f).freeRun (A)
            .osc (B, WT::Glass, 0.4f, 0.35f).pitch (B, 1).freeRun (B)
            .filter (FT::LP12, 3000.0f, 0.05f)
            .amp (2.5f, 1.0f, 1.0f, 4.0f)
            .lfo (0, LS::Sine, 0.07f, false)
            .mod (1, Lfo1, APos, 0.4f)
            .delay (SD::D1_2, 0.5f, 0.25f)
            .reverb (1.0f, 0.4f, 0.6f)
            .poly (12).vel (0.2f).vol (-12.2f));

    // =====================================================================
    // LEADS
    // =====================================================================
    add (P ("G-Funk Whistle", leads)
            .osc (A, WT::BasicShapes, 0.05f, 0.8f).pitch (A, 1)
            .filter (FT::LP12, 4000.0f, 0.0f)
            .amp (0.04f, 0.2f, 1.0f, 0.25f)
            .lfo (0, LS::Sine, 5.2f, true, 0.5f)
            .mod (1, Lfo1, PitchFine, 0.35f)
            .mod (2, ModWheel, PitchFine, 0.4f)
            .delay (SD::D1_8D, 0.25f, 0.15f)
            .reverb (0.5f, 0.5f, 0.2f)
            .mono (0.09f).vel (0.3f).vol (-6.5f));

    add (P ("Trap Lead Saw", leads)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 7, 0.3f, 0.8f)
            .osc (B, WT::AnalogSaw, 1.0f, 0.35f).pitch (B, 1).uni (B, 5, 0.4f, 0.8f)
            .filter (FT::LP24, 7000.0f, 0.05f)
            .amp (0.003f, 0.3f, 0.85f, 0.2f)
            .mod (8, Macro1, Cutoff, -0.5f)
            .delay (SD::D1_8D, 0.3f, 0.2f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (8).vel (0.3f).vol (-6.2f));

    add (P ("Square Lead", leads)
            .osc (A, WT::PulseWidth, 0.0f, 0.65f)
            .osc (B, WT::PulseWidth, 0.5f, 0.4f).pitch (B, 0, 0, 7.0f)
            .filter (FT::LP12, 3500.0f, 0.2f)
            .amp (0.005f, 0.3f, 0.9f, 0.15f)
            .lfo (0, LS::Sine, 5.5f, true, 0.4f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .delay (SD::D1_8, 0.25f, 0.15f)
            .mono (0.04f).vel (0.3f).vol (-8.1f));

    add (P ("Sync Screamer", leads)
            .osc (A, WT::BasicShapes, 0.67f, 0.75f).warp (A, WM::Sync, 0.4f)
            .filter (FT::LP24, 6000.0f, 0.1f)
            .menv (0.0f, 0.5f, 0.0f, 0.3f)
            .amp (0.003f, 0.3f, 0.9f, 0.15f)
            .mod (1, ModEnv, AWarp, 0.4f)
            .mod (2, ModWheel, AWarp, 0.5f)
            .mod (8, Macro1, AWarp, 0.5f)
            .dist (DistType::Soft, 0.2f, 0.7f)
            .delay (SD::D1_8D, 0.3f, 0.15f)
            .mono (0.05f).vel (0.3f).vol (-6.7f));

    add (P ("Breathy Flute", leads)
            .osc (A, WT::BasicShapes, 0.1f, 0.8f)
            .noise (NoiseType::Pink, 0.12f)
            .filter (FT::LP12, 2500.0f, 0.05f, 0.0f, 0.5f)
            .amp (0.08f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.0f, true, 0.7f)
            .lfo (1, LS::Sine, 5.0f, true, 0.7f)
            .mod (1, Lfo1, PitchFine, 0.15f)
            .mod (2, Lfo2, Amp, 0.06f)
            .reverb (0.5f, 0.4f, 0.25f)
            .mono (0.03f).vel (0.4f).vol (-6.5f));

    add (P ("Vocal Lead", leads)
            .osc (A, WT::Vowels, 0.0f, 0.75f).uni (A, 2, 0.1f, 0.5f)
            .filter (FT::LP12, 6000.0f, 0.0f)
            .menv (0.0f, 0.6f, 0.3f, 0.3f)
            .amp (0.02f, 0.3f, 1.0f, 0.2f)
            .lfo (0, LS::Sine, 5.3f, true, 0.6f)
            .mod (1, ModEnv, APos, 0.5f)
            .mod (2, Lfo1, PitchFine, 0.25f)
            .mod (3, ModWheel, APos, 0.5f)
            .reverb (0.6f, 0.5f, 0.25f)
            .mono (0.06f).vel (0.3f).vol (-1.5f));

    add (P ("Supersaw Anthem", leads)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 8, 0.45f, 1.0f).freeRun (A)
            .osc (B, WT::AnalogSaw, 1.0f, 0.45f).pitch (B, 1).uni (B, 8, 0.45f, 1.0f).freeRun (B)
            .filter (FT::LP24, 9000.0f, 0.05f)
            .amp (0.01f, 0.5f, 0.9f, 0.35f)
            .mod (8, Macro1, Cutoff, -0.6f)
            .chorus (0.5f, 0.4f, 0.3f)
            .delay (SD::D1_8D, 0.35f, 0.2f)
            .reverb (0.7f, 0.4f, 0.3f)
            .poly (8).vel (0.3f).vol (-2.6f));

    add (P ("Chip Lead", leads)
            .osc (A, WT::Chiptune, 0.75f, 0.6f)
            .filterOff()
            .amp (0.001f, 0.2f, 0.8f, 0.05f)
            .lfo (0, LS::Sine, 6.0f, true, 0.3f)
            .mod (1, Lfo1, PitchFine, 0.3f)
            .mono (0.02f, false).vel (0.2f).vol (-2.6f));

    add (P ("Siren Lead", leads)
            .osc (A, WT::AnalogSaw, 1.0f, 0.6f).uni (A, 2, 0.15f, 0.5f)
            .osc (B, WT::BasicShapes, 1.0f, 0.4f).pitch (B, -1)
            .filter (FT::LP24, 3000.0f, 0.2f)
            .menv (0.0f, 0.15f, 0.0f, 0.1f)
            .amp (0.01f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.5f, true, 0.4f)
            .mod (1, ModEnv, Pitch, -0.1f)
            .mod (2, Lfo1, PitchFine, 0.25f)
            .dist (DistType::Soft, 0.25f, 0.6f)
            .reverb (0.5f, 0.5f, 0.2f)
            .mono (0.1f).vel (0.3f).vol (-8.7f));

    add (P ("Mellow Sine Lead", leads)
            .osc (A, WT::BasicShapes, 0.2f, 0.75f)
            .sub (SubShape::Sine, -1, 0.3f)
            .filter (FT::LP12, 2500.0f, 0.05f)
            .amp (0.02f, 0.3f, 0.9f, 0.3f)
            .lfo (0, LS::Sine, 5.0f, true, 0.6f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .delay (SD::D1_4D, 0.3f, 0.18f)
            .reverb (0.6f, 0.5f, 0.25f)
            .mono (0.06f).vel (0.3f).vol (-6.2f));

    // =====================================================================
    // PLUCKS
    // =====================================================================
    add (P ("Trap Pluck", plucks)
            .osc (A, WT::AnalogSaw, 0.9f, 0.65f).uni (A, 3, 0.2f, 0.7f)
            .osc (B, WT::BasicShapes, 0.3f, 0.45f)
            .filter (FT::LP24, 600.0f, 0.1f, 0.5f).fvel (0.3f)
            .fenv (0.0f, 0.25f, 0.0f, 0.25f)
            .amp (0.001f, 0.6f, 0.0f, 0.3f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .delay (SD::D1_8D, 0.3f, 0.2f)
            .reverb (0.5f, 0.5f, 0.25f)
            .poly (12).vel (0.5f).vol (-3.0f));

    add (P ("Kalimba", plucks)
            .osc (A, WT::BasicShapes, 0.0f, 0.8f)
            .osc (B, WT::BasicShapes, 0.0f, 0.0f).pitch (B, 2, 7, 2.0f)
            .warp (A, WM::FM, 0.05f)
            .filterOff()
            .menv (0.0f, 0.12f, 0.0f, 0.1f)
            .amp (0.001f, 1.2f, 0.0f, 0.5f)
            .mod (1, ModEnv, AWarp, 0.45f)
            .mod (2, Velocity, AWarp, 0.1f)
            .reverb (0.4f, 0.5f, 0.2f)
            .poly (12).vel (0.6f).vol (-6.0f));

    add (P ("Harp Pluck", plucks)
            .osc (A, WT::AnalogSaw, 0.6f, 0.75f)
            .filter (FT::LP12, 900.0f, 0.05f, 0.5f, 0.7f)
            .fenv (0.0f, 0.4f, 0.0f, 0.4f)
            .amp (0.001f, 2.5f, 0.0f, 0.8f)
            .reverb (0.6f, 0.5f, 0.3f)
            .poly (16).vel (0.6f).vol (-5.4f));

    add (P ("Nylon Pluck", plucks)
            .osc (A, WT::BasicShapes, 0.5f, 0.8f)
            .filter (FT::LP12, 1200.0f, 0.1f, 0.45f, 0.6f).fvel (0.4f)
            .fenv (0.0f, 0.18f, 0.0f, 0.2f)
            .amp (0.001f, 1.6f, 0.0f, 0.4f)
            .chorus (0.5f, 0.3f, 0.2f)
            .reverb (0.45f, 0.5f, 0.2f)
            .poly (12).vel (0.6f).vol (-4.8f));

    add (P ("Koto Pluck", plucks)
            .osc (A, WT::Hollow, 0.4f, 0.75f)
            .filter (FT::LP12, 2500.0f, 0.15f, 0.3f)
            .fenv (0.0f, 0.1f, 0.0f, 0.1f)
            .menv (0.0f, 0.08f, 0.0f, 0.05f)
            .amp (0.001f, 1.0f, 0.0f, 0.4f)
            .mod (1, ModEnv, Pitch, -0.02f)
            .reverb (0.5f, 0.5f, 0.25f)
            .poly (12).vel (0.6f).vol (-6.1f));

    add (P ("Pizzicato", plucks)
            .osc (A, WT::Strings, 0.5f, 0.75f).uni (A, 2, 0.1f, 0.5f)
            .filter (FT::LP12, 1500.0f, 0.05f, 0.4f, 0.5f)
            .fenv (0.0f, 0.15f, 0.0f, 0.1f)
            .amp (0.001f, 0.35f, 0.0f, 0.25f)
            .reverb (0.6f, 0.5f, 0.3f)
            .poly (12).vel (0.6f).vol (-2.5f));

    add (P ("Bright Arp Pluck", plucks)
            .osc (A, WT::PulseWidth, 0.3f, 0.6f).uni (A, 2, 0.12f, 0.6f)
            .osc (B, WT::AnalogSaw, 1.0f, 0.45f)
            .filter (FT::LP24, 800.0f, 0.3f, 0.55f)
            .fenv (0.0f, 0.18f, 0.0f, 0.15f)
            .amp (0.001f, 0.4f, 0.0f, 0.2f)
            .delay (SD::D1_16, 0.35f, 0.25f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (12).vel (0.5f).vol (-5.3f));

    add (P ("Chord Stab", plucks)
            .osc (A, WT::AnalogSaw, 1.0f, 0.6f).uni (A, 4, 0.3f, 0.8f)
            .osc (B, WT::BasicShapes, 1.0f, 0.35f).pitch (B, -1)
            .filter (FT::LP24, 1200.0f, 0.1f, 0.4f)
            .fenv (0.0f, 0.25f, 0.1f, 0.2f)
            .amp (0.001f, 0.4f, 0.0f, 0.2f)
            .reverb (0.4f, 0.5f, 0.2f)
            .poly (12).vel (0.4f).vol (-3.2f));

    // =====================================================================
    // BELLS & MALLETS
    // =====================================================================
    add (baseFmBell ("Trap Bell", 21, 69.0f, 0.2f, 0.3f, 2.8f)
            .delay (SD::D1_8D, 0.3f, 0.15f)
            .reverb (0.6f, 0.4f, 0.3f).vol (-9.7f));

    add (baseFmBell ("Glockenspiel", 17, 58.0f, 0.25f, 0.25f, 1.8f)
            .pitch (A, 1).pitch (B, 1, 17, 58.0f)
            .menv (0.0f, 0.3f, 0.0f, 0.3f)
            .reverb (0.5f, 0.4f, 0.25f).vol (-7.9f));

    add (baseFmBell ("Dark Bell", 6, 0.0f, 0.25f, 0.25f, 3.5f)
            .filter (FT::LP12, 2500.0f, 0.05f)
            .delay (SD::D1_4, 0.3f, 0.15f)
            .reverb (0.8f, 0.5f, 0.35f).vol (-8.4f));

    add (baseFmBell ("Drill Bell", 13, 0.0f, 0.3f, 0.3f, 3.0f)
            .reverb (0.85f, 0.5f, 0.4f)
            .lofi (14.0f, 30000.0f, 0.1f, 0.05f, 0.7f).vol (-9.4f));

    add (P ("Music Box", bells)
            .osc (A, WT::Glass, 0.2f, 0.75f).pitch (A, 1)
            .filterOff()
            .amp (0.001f, 1.6f, 0.0f, 1.0f)
            .reverb (0.7f, 0.4f, 0.35f)
            .poly (16).vel (0.5f).vol (-7.0f));

    add (P ("Celesta", bells)
            .osc (A, WT::Mallet, 0.8f, 0.75f).pitch (A, 1)
            .filter (FT::LP12, 6000.0f, 0.0f).fvel (0.4f)
            .amp (0.001f, 1.4f, 0.0f, 0.8f)
            .chorus (0.4f, 0.2f, 0.15f)
            .reverb (0.6f, 0.4f, 0.3f)
            .poly (16).vel (0.5f).vol (-2.1f));

    add (P ("Marimba", bells)
            .osc (A, WT::Mallet, 0.25f, 0.85f)
            .filter (FT::LP12, 4000.0f, 0.0f, 0.0f, 0.3f).fvel (0.5f)
            .amp (0.001f, 0.55f, 0.0f, 0.3f)
            .reverb (0.35f, 0.5f, 0.15f)
            .poly (16).vel (0.6f).vol (-4.0f));

    add (P ("Vibraphone", bells)
            .osc (A, WT::Mallet, 0.45f, 0.75f)
            .filter (FT::LP12, 7000.0f, 0.0f).fvel (0.3f)
            .amp (0.001f, 3.0f, 0.0f, 1.2f)
            .lfo (0, LS::Sine, 5.5f, false)
            .mod (1, Lfo1, Amp, 0.35f)
            .reverb (0.45f, 0.4f, 0.2f)
            .poly (16).vel (0.6f).vol (-6.2f));

    // =====================================================================
    // STRINGS & BRASS
    // =====================================================================
    add (P ("Drill Strings", orch)
            .osc (A, WT::Strings, 0.7f, 0.6f).uni (A, 5, 0.2f, 0.8f)
            .osc (B, WT::Strings, 0.4f, 0.45f).pitch (B, -1).uni (B, 3, 0.2f, 0.6f)
            .filter (FT::LP12, 4000.0f, 0.05f).fvel (0.4f)
            .amp (0.01f, 0.35f, 0.15f, 0.2f)
            .reverb (0.55f, 0.5f, 0.25f)
            .poly (12).vel (0.5f).vol (-5.1f));

    add (P ("Cinematic Strings", orch)
            .osc (A, WT::Strings, 0.6f, 0.6f).uni (A, 6, 0.22f, 0.9f).freeRun (A)
            .osc (B, WT::Strings, 0.35f, 0.4f).pitch (B, -1).uni (B, 3, 0.2f, 0.7f).freeRun (B)
            .filter (FT::LP12, 3200.0f, 0.05f)
            .amp (0.35f, 1.0f, 1.0f, 0.8f)
            .lfo (0, LS::Sine, 5.0f, true, 0.8f)
            .mod (1, Lfo1, PitchFine, 0.12f)
            .mod (2, ModWheel, Cutoff, 0.4f)
            .reverb (0.8f, 0.4f, 0.35f)
            .poly (12).vel (0.4f).vol (-11.1f));

    add (P ("Brass Stab", orch)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 3, 0.15f, 0.6f)
            .osc (B, WT::BasicShapes, 0.67f, 0.3f).pitch (B, 1)
            .filter (FT::LP24, 600.0f, 0.1f, 0.35f).fvel (0.5f)
            .fenv (0.03f, 0.25f, 0.3f, 0.2f)
            .menv (0.0f, 0.08f, 0.0f, 0.1f)
            .amp (0.02f, 0.3f, 0.7f, 0.2f)
            .mod (1, ModEnv, Pitch, -0.02f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (12).vel (0.5f).vol (-5.7f));

    add (P ("Horn Section", orch)
            .osc (A, WT::AnalogSaw, 0.9f, 0.6f).uni (A, 4, 0.18f, 0.7f)
            .osc (B, WT::PulseWidth, 0.2f, 0.35f)
            .filter (FT::LP24, 900.0f, 0.1f, 0.3f).fvel (0.5f)
            .fenv (0.08f, 0.5f, 0.5f, 0.3f)
            .amp (0.06f, 0.4f, 0.85f, 0.25f)
            .lfo (0, LS::Sine, 5.2f, true, 0.6f)
            .mod (1, Lfo1, PitchFine, 0.1f)
            .reverb (0.5f, 0.5f, 0.22f)
            .poly (12).vel (0.5f).vol (-8.5f));

    add (P ("Choir Stab", orch)
            .osc (A, WT::Choir, 0.7f, 0.65f).uni (A, 5, 0.2f, 0.9f)
            .filter (FT::LP12, 5000.0f, 0.0f)
            .amp (0.003f, 0.45f, 0.2f, 0.4f)
            .reverb (0.6f, 0.4f, 0.35f)
            .poly (12).vel (0.4f).vol (-3.8f));

    add (P ("Orchestra Hit", orch)
            .osc (A, WT::Strings, 0.8f, 0.6f).uni (A, 6, 0.25f, 0.9f)
            .osc (B, WT::AnalogSaw, 1.0f, 0.45f).pitch (B, -1).uni (B, 3, 0.2f, 0.6f)
            .noise (NoiseType::Pink, 0.1f)
            .filter (FT::LP24, 2500.0f, 0.1f, 0.3f)
            .fenv (0.0f, 0.3f, 0.0f, 0.3f)
            .amp (0.001f, 0.5f, 0.0f, 0.4f)
            .reverb (0.7f, 0.4f, 0.35f)
            .poly (12).vel (0.4f).vol (-3.3f));

    // =====================================================================
    // LO-FI
    // =====================================================================
    add (baseEPiano ("Tape Keys", lofiC)
            .lfo (0, LS::Sine, 4.0f, false)
            .mod (4, Lfo1, Pan, 0.2f)
            .lofi (10.0f, 18000.0f, 0.45f, 0.3f, 0.4f)
            .reverb (0.55f, 0.6f, 0.22f).vol (-8.4f));

    add (baseEPiano ("Late Night Keys", lofiC)
            .uni (A, 2, 0.1f, 0.6f)
            .filter (FT::LP12, 2500.0f, 0.05f).fvel (0.35f)
            .chorus (0.5f, 0.5f, 0.35f)
            .lofi (12.0f, 22000.0f, 0.3f, 0.2f, 0.5f)
            .reverb (0.7f, 0.6f, 0.3f).vol (-6.8f));

    add (P ("Lo-Fi Guitar Pluck", lofiC)
            .osc (A, WT::BasicShapes, 0.55f, 0.8f)
            .filter (FT::LP12, 1100.0f, 0.1f, 0.4f, 0.6f).fvel (0.4f)
            .fenv (0.0f, 0.2f, 0.0f, 0.2f)
            .amp (0.001f, 1.8f, 0.0f, 0.5f)
            .chorus (0.6f, 0.5f, 0.35f)
            .lofi (11.0f, 20000.0f, 0.4f, 0.25f, 0.45f)
            .reverb (0.5f, 0.6f, 0.2f)
            .poly (12).vel (0.6f).vol (-4.6f));

    add (P ("Cassette Pad", lofiC)
            .osc (A, WT::AnalogSaw, 0.6f, 0.6f).uni (A, 4, 0.25f, 0.8f).freeRun (A)
            .osc (B, WT::Choir, 0.5f, 0.35f).freeRun (B)
            .filter (FT::LP12, 1800.0f, 0.1f)
            .amp (0.6f, 1.0f, 1.0f, 1.5f)
            .lofi (10.0f, 16000.0f, 0.8f, 0.3f, 0.35f)
            .reverb (0.7f, 0.6f, 0.3f)
            .poly (12).vel (0.3f).vol (-10.0f));

    add (baseFmBell ("Vinyl Bell", 21, 69.0f, 0.18f, 0.25f, 2.2f).category (lofiC)
            .lofi (8.0f, 14000.0f, 0.35f, 0.35f, 0.35f)
            .reverb (0.6f, 0.6f, 0.25f).vol (-6.9f));

    add (P ("Bitcrushed Lead", lofiC)
            .osc (A, WT::PulseWidth, 0.1f, 0.7f)
            .filter (FT::LP12, 5000.0f, 0.1f)
            .amp (0.005f, 0.3f, 0.85f, 0.15f)
            .lfo (0, LS::Sine, 5.0f, true, 0.4f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .lofi (6.0f, 8000.0f, 0.2f, 0.15f, 0.6f)
            .delay (SD::D1_8D, 0.3f, 0.15f)
            .mono (0.04f).vel (0.3f).vol (-6.7f));

    add (P ("Warm Sine Chords", lofiC)
            .osc (A, WT::BasicShapes, 0.12f, 0.7f).uni (A, 2, 0.12f, 0.6f)
            .osc (B, WT::BasicShapes, 0.0f, 0.3f).pitch (B, 1)
            .filter (FT::LP12, 3000.0f, 0.0f)
            .amp (0.01f, 2.0f, 0.4f, 0.6f)
            .lofi (12.0f, 22000.0f, 0.5f, 0.2f, 0.5f)
            .chorus (0.4f, 0.4f, 0.3f)
            .reverb (0.6f, 0.6f, 0.25f)
            .poly (12).vel (0.5f).vol (-5.1f));

    // =====================================================================
    // FX & ATMOS
    // =====================================================================
    add (P ("Vinyl Atmosphere", fx)
            .oscOff (A)
            .noise (NoiseType::Crackle, 1.0f)
            .filter (FT::LP12, 7000.0f, 0.0f)
            .amp (0.5f, 1.0f, 1.0f, 1.0f)
            .lofi (12.0f, 22000.0f, 0.2f, 0.6f, 0.5f)
            .poly (2).vel (0.0f).vol (3.1f));

    add (P ("Riser", fx)
            .osc (A, WT::AnalogSaw, 1.0f, 0.6f).uni (A, 7, 0.35f, 1.0f)
            .noise (NoiseType::White, 0.25f)
            .filter (FT::LP24, 200.0f, 0.3f)
            .menv (4.0f, 0.1f, 1.0f, 1.0f)
            .amp (0.5f, 1.0f, 1.0f, 1.5f)
            .mod (1, ModEnv, Cutoff, 1.0f)
            .mod (2, ModEnv, Pitch, 0.5f)
            .reverb (0.8f, 0.4f, 0.35f)
            .poly (4).vel (0.0f).vol (-9.2f));

    add (P ("Laser Zap", fx)
            .osc (A, WT::BasicShapes, 0.1f, 0.8f)
            .filterOff()
            .menv (0.0f, 0.15f, 0.0f, 0.1f)
            .amp (0.001f, 0.3f, 0.0f, 0.1f)
            .mod (1, ModEnv, Pitch, 1.0f)
            .delay (SD::D1_8D, 0.4f, 0.25f)
            .poly (8).vel (0.3f).vol (1.8f));

    add (P ("Dark Drone", fx)
            .osc (A, WT::Growl, 0.3f, 0.6f).uni (A, 4, 0.3f, 0.9f).freeRun (A)
            .osc (B, WT::Digital, 0.5f, 0.45f).pitch (B, -1).freeRun (B)
            .filter (FT::LP24, 400.0f, 0.4f)
            .amp (2.0f, 1.0f, 1.0f, 3.0f)
            .lfo (0, LS::SmoothRandom, 0.15f, false)
            .lfo (1, LS::Sine, 0.05f, false)
            .mod (1, Lfo1, Cutoff, 0.3f)
            .mod (2, Lfo2, BPos, 0.4f)
            .reverb (0.95f, 0.5f, 0.45f)
            .poly (8).vel (0.0f).vol (-3.8f));

    add (P ("Siren FX", fx)
            .osc (A, WT::BasicShapes, 0.3f, 0.7f)
            .filter (FT::LP12, 5000.0f, 0.0f)
            .amp (0.05f, 1.0f, 1.0f, 0.5f)
            .lfo (0, LS::Triangle, 0.5f, true)
            .mod (1, Lfo1, Pitch, 0.5f)
            .reverb (0.7f, 0.4f, 0.3f)
            .mono (0.0f).vel (0.0f).vol (-7.8f));

    add (P ("Tape Stop Drop", fx)
            .osc (A, WT::AnalogSaw, 0.8f, 0.7f)
            .filter (FT::LP24, 3000.0f, 0.1f)
            .menv (0.9f, 1.0f, 1.0f, 0.5f)
            .amp (0.001f, 1.0f, 1.0f, 0.2f)
            .mod (1, ModEnv, Pitch, -1.0f)
            .mod (2, ModEnv, Cutoff, -0.6f)
            .poly (4).vel (0.0f).vol (-6.1f));

    add (P ("Space Echo Blip", fx)
            .osc (A, WT::BasicShapes, 0.05f, 0.8f)
            .filter (FT::LP12, 3000.0f, 0.2f)
            .amp (0.001f, 0.15f, 0.0f, 0.1f)
            .delay (SD::D1_8D, 0.6f, 0.45f, true, 0.45f)
            .reverb (0.8f, 0.5f, 0.3f)
            .poly (8).vel (0.4f).vol (3.2f));

    // =====================================================================
    // EXPANSION 0.2 — 808 & BASS
    // =====================================================================
    add (base808 ("808 Rage Clipper", 1.0f, 0.75f, 0.08f, 0.3f)
            .osc (B, WT::BasicShapes, 0.67f, 0.15f)
            .dist (DistType::Hard, 0.7f, 0.5f).eq (2.0f, 0.0f, -2.0f).vol (-5.9f));

    add (base808 ("808 Ghost Slide", 2.0f, 0.5f, 0.22f, 0.15f)
            .dist (DistType::Soft, 0.25f, 0.6f).vol (-5.5f));

    add (base808 ("808 Knock", 0.45f, 0.0f, 0.0f, 0.55f)
            .set ("voice_mode", (float) VoiceMode::Mono)
            .noise (NoiseType::White, 0.0f)
            .mod (2, ModEnv, NoiseLevel, 0.5f)
            .dist (DistType::Soft, 0.35f, 0.7f).vol (0.4f));

    add (base808 ("808 Detroit Buzz", 1.4f, 0.6f, 0.07f, 0.25f)
            .osc (A, WT::BasicShapes, 0.9f, 0.8f)
            .filter (FT::LP24, 1800.0f, 0.1f)
            .dist (DistType::Rectify, 0.4f, 0.5f).vol (-0.6f));

    add (base808 ("808 Memphis Dust", 2.2f, 0.0f, 0.05f, 0.3f)
            .lofi (10.0f, 16000.0f, 0.15f, 0.25f, 0.45f)
            .dist (DistType::Soft, 0.25f, 0.55f).vol (-4.4f));

    add (base808 ("808 Tape Saturated", 1.6f, 0.6f, 0.08f, 0.22f)
            .dist (DistType::Tube, 0.45f, 0.6f)
            .lofi (14.0f, 30000.0f, 0.12f, 0.05f, 0.7f).vol (-6.4f));

    add (base808 ("808 Jersey Bounce", 0.5f, 0.0f, 0.0f, 0.6f)
            .set ("voice_mode", (float) VoiceMode::Mono)
            .menv (0.0f, 0.08f, 0.0f, 0.05f)
            .dist (DistType::Soft, 0.3f, 0.7f).vol (0.4f));

    add (base808 ("UK Drill Slide", 1.3f, 0.85f, 0.2f, 0.18f)
            .dist (DistType::Hard, 0.4f, 0.45f).vol (-5.7f));

    add (base808 ("808 Stutter", 2.0f, 0.8f, 0.06f, 0.25f)
            .lfoSync (0, LS::Square, SD::D1_16, true)
            .mod (3, Lfo1, Amp, 0.9f)
            .dist (DistType::Soft, 0.3f, 0.6f).vol (-3.8f));

    add (P ("Growl Reese", bass)
            .osc (A, WT::Growl, 0.4f, 0.7f).uni (A, 4, 0.3f, 0.5f)
            .sub (SubShape::Sine, -1, 0.5f)
            .filter (FT::LP24, 900.0f, 0.25f)
            .amp (0.005f, 0.5f, 1.0f, 0.15f)
            .lfo (0, LS::Sine, 0.4f, false)
            .mod (1, Lfo1, APos, 0.3f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .dist (DistType::Soft, 0.35f, 0.6f)
            .mono (0.05f).vel (0.2f).vol (-6.0f));

    add (P ("Plucked Synth Bass", bass)
            .osc (A, WT::PulseWidth, 0.25f, 0.75f)
            .osc (B, WT::BasicShapes, 0.0f, 0.4f).pitch (B, -1)
            .filter (FT::LP24, 350.0f, 0.3f, 0.45f)
            .fenv (0.0f, 0.18f, 0.05f, 0.1f)
            .amp (0.001f, 0.5f, 0.3f, 0.1f)
            .mono (0.02f, false).vel (0.5f).vol (-0.9f));

    add (P ("Juicy Sine Bass", bass)
            .osc (A, WT::BasicShapes, 0.08f, 0.85f)
            .sub (SubShape::Sine, -1, 0.4f)
            .filter (FT::LP12, 1200.0f, 0.1f).fdrive (0.35f)
            .amp (0.003f, 0.4f, 0.9f, 0.12f)
            .mono (0.04f).vel (0.3f).vol (0.9f));

    add (P ("G-Funk Synth Bass", bass)
            .osc (A, WT::AnalogSaw, 0.7f, 0.7f)
            .osc (B, WT::PulseWidth, 0.0f, 0.45f).pitch (B, -1)
            .filter (FT::LP24, 450.0f, 0.35f, 0.35f).fdrive (0.2f)
            .fenv (0.0f, 0.3f, 0.25f, 0.15f)
            .amp (0.002f, 0.6f, 0.8f, 0.12f)
            .mono (0.06f).vel (0.45f).vol (0.9f));

    // =====================================================================
    // EXPANSION 0.2 — KEYS
    // =====================================================================
    add (baseEPiano ("Tine Dream", keys)
            .uni (A, 2, 0.1f, 0.7f)
            .chorus (0.4f, 0.5f, 0.35f)
            .delay (SD::D1_8D, 0.25f, 0.12f)
            .reverb (0.7f, 0.5f, 0.3f).vol (-6.7f));

    add (baseEPiano ("Bright Tines", keys)
            .osc (A, WT::EPiano, 0.15f, 0.8f)
            .filter (FT::LP12, 9000.0f, 0.05f)
            .eq (0.0f, 0.0f, 3.0f)
            .reverb (0.4f, 0.5f, 0.15f).vol (-8.9f));

    add (P ("Felt Keys", keys)
            .osc (A, WT::Mallet, 0.15f, 0.75f)
            .osc (B, WT::BasicShapes, 0.05f, 0.4f)
            .filter (FT::LP12, 1200.0f, 0.0f, 0.3f, 0.5f).fvel (0.5f)
            .fenv (0.0f, 0.5f, 0.1f, 0.4f)
            .amp (0.004f, 2.2f, 0.0f, 0.5f)
            .lofi (13.0f, 26000.0f, 0.1f, 0.15f, 0.5f)
            .reverb (0.5f, 0.6f, 0.25f)
            .poly (12).vel (0.7f).vol (-8.8f));

    add (P ("Grand Keys", keys)
            .osc (A, WT::AnalogSaw, 0.75f, 0.5f)
            .osc (B, WT::Mallet, 0.6f, 0.6f).pitch (B, 0, 0, 3.0f)
            .filter (FT::LP12, 2200.0f, 0.05f, 0.45f, 0.6f).fvel (0.5f)
            .fenv (0.0f, 1.2f, 0.15f, 0.6f)
            .amp (0.001f, 3.5f, 0.0f, 0.7f)
            .reverb (0.55f, 0.5f, 0.22f)
            .poly (16).vel (0.75f).vol (-8.2f));

    add (P ("Gospel Chords", keys)
            .osc (A, WT::Organ, 0.75f, 0.75f).uni (A, 2, 0.08f, 0.5f)
            .filter (FT::LP12, 7000.0f, 0.0f)
            .amp (0.006f, 0.1f, 1.0f, 0.1f)
            .lfo (0, LS::Sine, 6.5f, false)
            .mod (1, Lfo1, Pan, 0.2f)
            .mod (2, Lfo1, PitchFine, 0.05f)
            .mod (3, ModWheel, APos, 0.25f)
            .dist (DistType::Tube, 0.25f, 0.65f)
            .chorus (1.0f, 0.4f, 0.3f)
            .reverb (0.55f, 0.5f, 0.2f)
            .poly (12).vel (0.0f).vol (-7.6f));

    add (baseEPiano ("Dark Trap Keys", keys)
            .filter (FT::LP12, 1800.0f, 0.05f).fvel (0.3f)
            .delay (SD::D1_4D, 0.3f, 0.15f)
            .reverb (0.75f, 0.6f, 0.35f).vol (-7.6f));

    add (P ("Bell Piano", keys)
            .osc (A, WT::Mallet, 0.55f, 0.6f)
            .osc (B, WT::Glass, 0.3f, 0.35f).pitch (B, 1)
            .filter (FT::LP12, 5000.0f, 0.0f, 0.2f, 0.4f)
            .fenv (0.0f, 0.8f, 0.2f, 0.5f)
            .amp (0.001f, 2.5f, 0.0f, 0.8f)
            .reverb (0.6f, 0.4f, 0.28f)
            .poly (16).vel (0.65f).vol (-6.0f));

    add (P ("Tremolo Wurly", keys)
            .osc (A, WT::Hollow, 0.3f, 0.7f)
            .osc (B, WT::BasicShapes, 0.05f, 0.45f)
            .filter (FT::LP12, 2800.0f, 0.05f).fvel (0.5f)
            .amp (0.002f, 2.5f, 0.15f, 0.35f)
            .lfo (0, LS::Triangle, 6.0f, false)
            .mod (1, Lfo1, Amp, 0.35f)
            .mod (2, Velocity, APos, 0.3f)
            .dist (DistType::Tube, 0.2f, 0.65f)
            .reverb (0.4f, 0.5f, 0.15f)
            .poly (12).vel (0.7f).vol (-9.8f));

    // =====================================================================
    // EXPANSION 0.2 — PADS
    // =====================================================================
    add (P ("Analog Strings Pad", pads)
            .osc (A, WT::AnalogSaw, 0.85f, 0.6f).uni (A, 6, 0.3f, 0.9f).freeRun (A)
            .osc (B, WT::PulseWidth, 0.2f, 0.4f).uni (B, 3, 0.2f, 0.7f).freeRun (B)
            .filter (FT::LP24, 2200.0f, 0.1f, 0.15f)
            .fenv (0.8f, 2.0f, 0.7f, 2.0f)
            .amp (0.5f, 1.0f, 1.0f, 1.5f)
            .chorus (0.35f, 0.6f, 0.4f)
            .reverb (0.75f, 0.5f, 0.35f)
            .poly (12).vel (0.3f).vol (-12.0f));

    add (P ("Night Drive Pad", pads)
            .osc (A, WT::PulseWidth, 0.4f, 0.6f).uni (A, 4, 0.25f, 0.8f).freeRun (A)
            .osc (B, WT::AnalogSaw, 0.8f, 0.45f).pitch (B, -1).freeRun (B)
            .filter (FT::LP24, 900.0f, 0.3f)
            .amp (0.6f, 1.0f, 1.0f, 1.8f)
            .lfo (0, LS::Triangle, 0.12f, false)
            .lfo (1, LS::Sine, 0.3f, false)
            .mod (1, Lfo1, Cutoff, 0.2f)
            .mod (2, Lfo2, APos, 0.2f)
            .mod (8, Macro1, Cutoff, 0.5f)
            .delay (SD::D1_4D, 0.4f, 0.2f)
            .reverb (0.85f, 0.5f, 0.4f)
            .poly (12).vel (0.3f).vol (-14.5f));

    add (P ("Shimmer Pad", pads)
            .osc (A, WT::Glass, 0.7f, 0.55f).uni (A, 5, 0.25f, 1.0f).freeRun (A)
            .osc (B, WT::Additive, 0.8f, 0.35f).pitch (B, 1).uni (B, 3, 0.2f, 0.8f).freeRun (B)
            .filter (FT::LP12, 8000.0f, 0.0f)
            .amp (1.5f, 1.0f, 1.0f, 3.5f)
            .lfo (0, LS::Sine, 0.1f, false)
            .mod (1, Lfo1, APos, 0.3f)
            .eq (-3.0f, 0.0f, 2.0f)
            .delay (SD::D1_2, 0.5f, 0.25f)
            .reverb (1.0f, 0.3f, 0.55f)
            .poly (12).vel (0.2f).vol (-13.7f));

    add (P ("Vocal Pad", pads)
            .osc (A, WT::Vowels, 0.3f, 0.6f).uni (A, 5, 0.2f, 0.9f).freeRun (A)
            .osc (B, WT::Choir, 0.6f, 0.45f).uni (B, 3, 0.2f, 0.8f).freeRun (B)
            .filter (FT::LP12, 4000.0f, 0.05f)
            .amp (0.8f, 1.0f, 1.0f, 2.0f)
            .lfo (0, LS::SmoothRandom, 0.15f, false)
            .mod (1, Lfo1, APos, 0.4f)
            .chorus (0.35f, 0.5f, 0.3f)
            .reverb (0.85f, 0.4f, 0.45f)
            .poly (12).vel (0.3f).vol (-9.8f));

    add (P ("Haunted Pad", pads)
            .osc (A, WT::Digital, 0.6f, 0.55f).uni (A, 4, 0.35f, 0.9f).freeRun (A)
            .osc (B, WT::Vowels, 0.9f, 0.45f).pitch (B, -1).freeRun (B)
            .filter (FT::LP24, 1200.0f, 0.35f)
            .amp (1.5f, 1.0f, 1.0f, 3.0f)
            .lfo (0, LS::SmoothRandom, 0.2f, false)
            .lfo (1, LS::Sine, 0.08f, false)
            .mod (1, Lfo1, Cutoff, 0.25f)
            .mod (2, Lfo2, PitchFine, 0.3f)
            .lofi (12.0f, 22000.0f, 0.4f, 0.15f, 0.4f)
            .reverb (0.95f, 0.6f, 0.5f)
            .poly (12).vel (0.2f).vol (-12.6f));

    add (P ("Warm Sub Pad", pads)
            .osc (A, WT::BasicShapes, 0.25f, 0.6f).uni (A, 4, 0.25f, 0.8f).freeRun (A)
            .sub (SubShape::Sine, -1, 0.5f)
            .filter (FT::LP12, 1500.0f, 0.1f)
            .amp (0.8f, 1.0f, 1.0f, 2.0f)
            .chorus (0.3f, 0.5f, 0.35f)
            .reverb (0.7f, 0.6f, 0.3f)
            .poly (12).vel (0.3f).vol (-11.2f));

    add (P ("Dreamscape", pads)
            .osc (A, WT::Additive, 0.6f, 0.55f).uni (A, 6, 0.35f, 1.0f).freeRun (A)
            .osc (B, WT::Glass, 0.3f, 0.35f).pitch (B, 1).freeRun (B)
            .filter (FT::LP12, 5000.0f, 0.0f)
            .amp (2.0f, 1.0f, 1.0f, 4.0f)
            .lfo (0, LS::Triangle, 0.06f, false)
            .mod (1, Lfo1, APos, 0.5f)
            .delay (SD::D1_4D, 0.45f, 0.25f)
            .reverb (1.0f, 0.35f, 0.55f)
            .poly (12).vel (0.2f).vol (-10.0f));

    add (P ("PWM Pad", pads)
            .osc (A, WT::PulseWidth, 0.3f, 0.6f).uni (A, 3, 0.2f, 0.8f).freeRun (A)
            .osc (B, WT::PulseWidth, 0.5f, 0.4f).pitch (B, -1).freeRun (B)
            .filter (FT::LP24, 2500.0f, 0.15f)
            .amp (0.4f, 1.0f, 1.0f, 1.2f)
            .lfo (0, LS::Triangle, 0.3f, false)
            .mod (1, Lfo1, APos, 0.35f)
            .mod (2, Lfo1, BPos, -0.3f)
            .chorus (0.4f, 0.6f, 0.4f)
            .reverb (0.6f, 0.5f, 0.3f)
            .poly (12).vel (0.3f).vol (-12.1f));

    // =====================================================================
    // EXPANSION 0.2 — LEADS
    // =====================================================================
    add (P ("Rage Lead", leads)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 8, 0.5f, 1.0f)
            .osc (B, WT::AnalogSaw, 1.0f, 0.4f).pitch (B, 1).uni (B, 6, 0.45f, 1.0f)
            .filter (FT::LP24, 8000.0f, 0.1f)
            .amp (0.003f, 0.3f, 0.9f, 0.25f)
            .mod (8, Macro1, Cutoff, -0.5f)
            .dist (DistType::Soft, 0.45f, 0.7f)
            .delay (SD::D1_8D, 0.3f, 0.2f)
            .reverb (0.6f, 0.4f, 0.25f)
            .poly (8).vel (0.2f).vol (-8.6f));

    add (P ("Hyper Saw", leads)
            .osc (A, WT::AnalogSaw, 0.9f, 0.7f).uni (A, 7, 0.4f, 0.9f)
            .filter (FT::LP24, 6000.0f, 0.1f)
            .amp (0.004f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.5f, true, 0.4f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .dist (DistType::Hard, 0.25f, 0.7f)
            .delay (SD::D1_16, 0.3f, 0.15f)
            .mono (0.04f).vel (0.2f).vol (-8.9f));

    add (P ("Detuned Square Lead", leads)
            .osc (A, WT::PulseWidth, 0.0f, 0.6f).uni (A, 2, 0.2f, 0.6f)
            .osc (B, WT::PulseWidth, 0.0f, 0.45f).pitch (B, 0, 0, -8.0f)
            .filter (FT::LP12, 4000.0f, 0.2f)
            .amp (0.005f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.2f, true, 0.5f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .delay (SD::D1_8D, 0.3f, 0.15f)
            .mono (0.05f).vel (0.3f).vol (-9.4f));

    add (P ("Portamento Sine", leads)
            .osc (A, WT::BasicShapes, 0.0f, 0.8f).pitch (A, 1)
            .filter (FT::LP12, 3000.0f, 0.0f)
            .amp (0.03f, 0.3f, 1.0f, 0.3f)
            .lfo (0, LS::Sine, 5.0f, true, 0.4f)
            .mod (1, Lfo1, PitchFine, 0.3f)
            .delay (SD::D1_4, 0.3f, 0.15f)
            .reverb (0.6f, 0.5f, 0.25f)
            .mono (0.15f).vel (0.3f).vol (-6.3f));

    add (P ("Nasal Lead", leads)
            .osc (A, WT::Hollow, 0.8f, 0.8f)
            .filter (FT::BP, 1400.0f, 0.5f, 0.0f, 0.5f)
            .amp (0.01f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.5f, true, 0.5f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .dist (DistType::Soft, 0.2f, 0.7f)
            .reverb (0.5f, 0.5f, 0.2f)
            .mono (0.05f).vel (0.3f).vol (-1.1f));

    add (P ("Acid Lead", leads)
            .osc (A, WT::BasicShapes, 0.67f, 0.8f)
            .filter (FT::LP24, 800.0f, 0.7f, 0.45f).fdrive (0.35f)
            .fenv (0.0f, 0.25f, 0.1f, 0.15f)
            .amp (0.002f, 0.3f, 0.85f, 0.1f)
            .mod (1, Velocity, Cutoff, 0.3f)
            .mod (8, Macro1, Resonance, 0.25f)
            .dist (DistType::Soft, 0.3f, 0.7f)
            .delay (SD::D1_16, 0.35f, 0.2f)
            .mono (0.08f).vol (-8.6f));

    add (P ("Airy Flute Lead", leads)
            .osc (A, WT::BasicShapes, 0.05f, 0.75f)
            .noise (NoiseType::Pink, 0.2f)
            .filter (FT::LP12, 3500.0f, 0.05f, 0.0f, 0.5f)
            .amp (0.12f, 0.3f, 0.85f, 0.3f)
            .lfo (0, LS::Sine, 5.0f, true, 0.8f)
            .mod (1, Lfo1, PitchFine, 0.15f)
            .reverb (0.7f, 0.4f, 0.35f)
            .mono (0.04f).vel (0.4f).vol (-6.8f));

    add (P ("Trumpet Lead", leads)
            .osc (A, WT::AnalogSaw, 0.9f, 0.75f).uni (A, 2, 0.1f, 0.4f)
            .filter (FT::LP24, 700.0f, 0.15f, 0.35f).fvel (0.5f)
            .fenv (0.04f, 0.3f, 0.5f, 0.2f)
            .menv (0.0f, 0.06f, 0.0f, 0.05f)
            .amp (0.03f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.3f, true, 0.5f)
            .mod (1, ModEnv, Pitch, -0.015f)
            .mod (2, Lfo1, PitchFine, 0.12f)
            .reverb (0.5f, 0.5f, 0.2f)
            .mono (0.04f).vel (0.4f).vol (-3.5f));

    add (P ("Chiptune Arp Lead", leads)
            .osc (A, WT::Chiptune, 0.9f, 0.6f)
            .filterOff()
            .amp (0.001f, 0.15f, 0.7f, 0.05f)
            .lfo (0, LS::Sine, 6.0f, true, 0.3f)
            .mod (1, Lfo1, PitchFine, 0.25f)
            .delay (SD::D1_16, 0.3f, 0.2f)
            .mono (0.0f, false).vel (0.2f).vol (1.2f));

    // =====================================================================
    // EXPANSION 0.2 — PLUCKS
    // =====================================================================
    add (P ("Rage Pluck", plucks)
            .osc (A, WT::AnalogSaw, 1.0f, 0.65f).uni (A, 6, 0.4f, 0.9f)
            .filter (FT::LP24, 1500.0f, 0.15f, 0.5f)
            .fenv (0.0f, 0.18f, 0.0f, 0.15f)
            .amp (0.001f, 0.35f, 0.0f, 0.2f)
            .dist (DistType::Soft, 0.35f, 0.7f)
            .delay (SD::D1_8D, 0.3f, 0.2f)
            .reverb (0.5f, 0.5f, 0.2f)
            .poly (12).vel (0.4f).vol (-0.4f));

    add (P ("Bell Pluck", plucks)
            .osc (A, WT::Glass, 0.5f, 0.6f)
            .osc (B, WT::Mallet, 0.6f, 0.45f).pitch (B, 1)
            .filter (FT::LP12, 3000.0f, 0.05f, 0.4f)
            .fenv (0.0f, 0.3f, 0.0f, 0.3f)
            .amp (0.001f, 1.2f, 0.0f, 0.6f)
            .delay (SD::D1_8D, 0.3f, 0.18f)
            .reverb (0.6f, 0.5f, 0.3f)
            .poly (12).vel (0.5f).vol (-5.1f));

    add (P ("Dark Pluck", plucks)
            .osc (A, WT::AnalogSaw, 0.6f, 0.65f)
            .osc (B, WT::BasicShapes, 0.9f, 0.4f).pitch (B, -1)
            .filter (FT::LP24, 300.0f, 0.2f, 0.35f)
            .fenv (0.0f, 0.3f, 0.0f, 0.2f)
            .amp (0.001f, 0.8f, 0.0f, 0.3f)
            .reverb (0.5f, 0.6f, 0.25f)
            .poly (12).vel (0.5f).vol (-4.9f));

    add (P ("Glass Pluck", plucks)
            .osc (A, WT::Glass, 0.8f, 0.65f).uni (A, 2, 0.12f, 0.6f)
            .filter (FT::LP12, 6000.0f, 0.0f, 0.2f)
            .fenv (0.0f, 0.2f, 0.0f, 0.2f)
            .amp (0.001f, 0.6f, 0.0f, 0.4f)
            .delay (SD::D1_16, 0.35f, 0.2f)
            .reverb (0.6f, 0.4f, 0.3f)
            .poly (12).vel (0.5f).vol (-3.3f));

    add (P ("Synth Harp", plucks)
            .osc (A, WT::PulseWidth, 0.2f, 0.55f)
            .osc (B, WT::AnalogSaw, 0.5f, 0.45f)
            .filter (FT::LP12, 1500.0f, 0.05f, 0.45f, 0.6f)
            .fenv (0.0f, 0.5f, 0.0f, 0.5f)
            .amp (0.001f, 2.0f, 0.0f, 1.0f)
            .chorus (0.4f, 0.4f, 0.3f)
            .reverb (0.7f, 0.5f, 0.35f)
            .poly (16).vel (0.6f).vol (-6.8f));

    add (baseFmBell ("Steel Drum", 12, 0.0f, 0.25f, 0.2f, 0.9f).category (plucks)
            .reverb (0.4f, 0.5f, 0.15f).vol (-5.4f));

    add (P ("Pizz Short", plucks)
            .osc (A, WT::Strings, 0.5f, 0.7f).uni (A, 3, 0.12f, 0.7f)
            .filter (FT::LP12, 2500.0f, 0.05f, 0.4f)
            .fenv (0.0f, 0.1f, 0.0f, 0.1f)
            .amp (0.001f, 0.2f, 0.0f, 0.15f)
            .reverb (0.55f, 0.5f, 0.3f)
            .poly (16).vel (0.6f).vol (2.2f));

    add (P ("Mallet Pluck", plucks)
            .osc (A, WT::Mallet, 0.5f, 0.8f)
            .filter (FT::LP12, 4000.0f, 0.0f).fvel (0.5f)
            .amp (0.001f, 0.5f, 0.0f, 0.3f)
            .delay (SD::D1_8, 0.25f, 0.15f)
            .reverb (0.4f, 0.5f, 0.2f)
            .poly (16).vel (0.6f).vol (-2.3f));

    // =====================================================================
    // EXPANSION 0.2 — BELLS & MALLETS
    // =====================================================================
    add (baseFmBell ("Crystal Bell", 24, 0.0f, 0.15f, 0.3f, 2.5f)
            .pitch (A, 1).pitch (B, 1, 24, 0.0f)
            .reverb (0.7f, 0.3f, 0.35f).vol (-10.2f));

    add (baseFmBell ("Church Bell", 19, 40.0f, 0.35f, 0.25f, 5.0f)
            .pitch (A, -1).pitch (B, -1, 19, 40.0f)
            .reverb (0.9f, 0.4f, 0.4f).vol (-12.4f));

    add (baseFmBell ("Tubular Bell", 27, 30.0f, 0.3f, 0.25f, 4.0f)
            .reverb (0.7f, 0.4f, 0.3f).vol (-10.6f));

    add (baseFmBell ("Gamelan", 15, 50.0f, 0.3f, 0.3f, 2.0f)
            .delay (SD::D1_8D, 0.3f, 0.15f)
            .reverb (0.6f, 0.4f, 0.3f).vol (-8.4f));

    add (P ("Lo-Fi Glock", bells)
            .osc (A, WT::Mallet, 0.85f, 0.75f).pitch (A, 1)
            .filterOff()
            .amp (0.001f, 1.2f, 0.0f, 0.7f)
            .lofi (9.0f, 14000.0f, 0.35f, 0.3f, 0.4f)
            .reverb (0.5f, 0.5f, 0.25f)
            .poly (16).vel (0.5f).vol (-1.3f));

    add (P ("Horror Music Box", bells)
            .osc (A, WT::Glass, 0.15f, 0.75f).pitch (A, 1).uni (A, 2, 0.15f, 0.6f)
            .filterOff()
            .amp (0.001f, 1.8f, 0.0f, 1.2f)
            .lfo (0, LS::Sine, 4.0f, false)
            .mod (1, Lfo1, PitchFine, 0.15f)
            .lofi (11.0f, 18000.0f, 0.6f, 0.25f, 0.45f)
            .reverb (0.85f, 0.4f, 0.45f)
            .poly (16).vel (0.5f).vol (-7.1f));

    // =====================================================================
    // EXPANSION 0.2 — STRINGS & BRASS
    // =====================================================================
    add (P ("Staccato Strings Dark", orch)
            .osc (A, WT::Strings, 0.5f, 0.6f).uni (A, 5, 0.2f, 0.8f)
            .osc (B, WT::Strings, 0.3f, 0.45f).pitch (B, -1).uni (B, 3, 0.2f, 0.6f)
            .filter (FT::LP12, 2000.0f, 0.05f).fvel (0.5f)
            .amp (0.008f, 0.25f, 0.05f, 0.15f)
            .reverb (0.6f, 0.5f, 0.28f)
            .poly (12).vel (0.5f).vol (-2.1f));

    add (P ("Legato Violin", orch)
            .osc (A, WT::Strings, 0.75f, 0.7f).uni (A, 2, 0.1f, 0.5f)
            .filter (FT::LP12, 5000.0f, 0.05f)
            .amp (0.15f, 0.5f, 0.9f, 0.4f)
            .lfo (0, LS::Sine, 5.5f, true, 0.5f)
            .mod (1, Lfo1, PitchFine, 0.15f)
            .reverb (0.7f, 0.4f, 0.35f)
            .mono (0.06f).vel (0.4f).vol (-6.4f));

    add (P ("Cello Low", orch)
            .osc (A, WT::Strings, 0.55f, 0.7f).pitch (A, -1).uni (A, 3, 0.15f, 0.6f)
            .filter (FT::LP12, 2500.0f, 0.05f)
            .amp (0.2f, 1.0f, 1.0f, 0.6f)
            .lfo (0, LS::Sine, 5.0f, true, 0.6f)
            .mod (1, Lfo1, PitchFine, 0.1f)
            .reverb (0.7f, 0.5f, 0.3f)
            .poly (8).vel (0.4f).vol (-12.4f));

    add (P ("Brass Swell", orch)
            .osc (A, WT::AnalogSaw, 0.95f, 0.65f).uni (A, 4, 0.18f, 0.7f)
            .filter (FT::LP24, 400.0f, 0.1f, 0.45f)
            .fenv (0.6f, 0.8f, 0.6f, 0.4f)
            .amp (0.5f, 0.5f, 0.9f, 0.4f)
            .reverb (0.6f, 0.5f, 0.25f)
            .poly (12).vel (0.4f).vol (-10.0f));

    add (P ("French Horn", orch)
            .osc (A, WT::AnalogSaw, 0.6f, 0.6f)
            .osc (B, WT::Hollow, 0.5f, 0.45f)
            .filter (FT::LP24, 600.0f, 0.1f, 0.25f).fvel (0.4f)
            .fenv (0.08f, 0.6f, 0.4f, 0.3f)
            .amp (0.08f, 0.4f, 0.85f, 0.3f)
            .lfo (0, LS::Sine, 5.0f, true, 0.6f)
            .mod (1, Lfo1, PitchFine, 0.08f)
            .reverb (0.6f, 0.5f, 0.3f)
            .poly (12).vel (0.5f).vol (-12.8f));

    add (P ("Sad Strings", orch)
            .osc (A, WT::Strings, 0.65f, 0.6f).uni (A, 6, 0.25f, 0.9f).freeRun (A)
            .osc (B, WT::Choir, 0.4f, 0.3f).freeRun (B)
            .filter (FT::LP12, 2800.0f, 0.05f)
            .amp (0.6f, 1.0f, 1.0f, 1.6f)
            .lofi (13.0f, 24000.0f, 0.25f, 0.1f, 0.5f)
            .reverb (0.85f, 0.45f, 0.4f)
            .poly (12).vel (0.3f).vol (-11.5f));

    // =====================================================================
    // EXPANSION 0.2 — LO-FI
    // =====================================================================
    add (baseEPiano ("Dusty Rhodes", lofiC)
            .lofi (10.0f, 16000.0f, 0.5f, 0.35f, 0.35f)
            .chorus (0.5f, 0.4f, 0.3f)
            .reverb (0.6f, 0.6f, 0.25f).vol (-7.4f));

    add (P ("Cassette Choir", lofiC)
            .osc (A, WT::Choir, 0.66f, 0.7f).uni (A, 4, 0.2f, 0.9f).freeRun (A)
            .filter (FT::LP12, 3000.0f, 0.05f)
            .amp (0.4f, 1.0f, 1.0f, 1.2f)
            .lofi (10.0f, 15000.0f, 0.7f, 0.3f, 0.35f)
            .reverb (0.7f, 0.5f, 0.35f)
            .poly (12).vel (0.3f).vol (-8.6f));

    add (P ("Old Radio Lead", lofiC)
            .osc (A, WT::BasicShapes, 0.4f, 0.8f)
            .filter (FT::BP, 1500.0f, 0.3f)
            .amp (0.01f, 0.3f, 0.9f, 0.2f)
            .lfo (0, LS::Sine, 5.0f, true, 0.4f)
            .mod (1, Lfo1, PitchFine, 0.2f)
            .lofi (8.0f, 11000.0f, 0.3f, 0.4f, 0.3f)
            .dist (DistType::Soft, 0.3f, 0.5f)
            .mono (0.05f).vel (0.3f).vol (-4.7f));

    add (P ("Lo-Fi Bass", lofiC)
            .osc (A, WT::BasicShapes, 0.2f, 0.85f)
            .sub (SubShape::Sine, 0, 0.4f)
            .filter (FT::LP12, 500.0f, 0.1f, 0.2f)
            .fenv (0.0f, 0.3f, 0.0f, 0.2f)
            .amp (0.003f, 1.0f, 0.3f, 0.2f)
            .lofi (11.0f, 18000.0f, 0.2f, 0.2f, 0.4f)
            .mono (0.0f, false).vel (0.5f).vol (-3.4f));

    add (P ("Warped Pad", lofiC)
            .osc (A, WT::AnalogSaw, 0.7f, 0.6f).uni (A, 5, 0.3f, 0.9f).freeRun (A)
            .filter (FT::LP12, 2000.0f, 0.1f)
            .amp (0.8f, 1.0f, 1.0f, 2.0f)
            .lofi (11.0f, 18000.0f, 1.0f, 0.25f, 0.35f)
            .chorus (0.3f, 0.6f, 0.4f)
            .reverb (0.8f, 0.5f, 0.4f)
            .poly (12).vel (0.3f).vol (-7.1f));

    add (P ("Vinyl Keys", lofiC)
            .osc (A, WT::Mallet, 0.35f, 0.7f)
            .osc (B, WT::EPiano, 0.1f, 0.45f)
            .filter (FT::LP12, 1500.0f, 0.05f, 0.3f, 0.5f)
            .fenv (0.0f, 0.6f, 0.1f, 0.4f)
            .amp (0.002f, 2.5f, 0.0f, 0.5f)
            .lofi (10.0f, 16000.0f, 0.35f, 0.45f, 0.35f)
            .reverb (0.55f, 0.6f, 0.25f)
            .poly (12).vel (0.65f).vol (-8.8f));

    add (P ("Jazz Hop Pluck", lofiC)
            .osc (A, WT::BasicShapes, 0.45f, 0.75f)
            .osc (B, WT::Mallet, 0.3f, 0.35f)
            .filter (FT::LP12, 1300.0f, 0.1f, 0.4f, 0.6f).fvel (0.4f)
            .fenv (0.0f, 0.25f, 0.0f, 0.25f)
            .amp (0.001f, 1.3f, 0.0f, 0.45f)
            .chorus (0.6f, 0.4f, 0.3f)
            .lofi (12.0f, 20000.0f, 0.3f, 0.25f, 0.45f)
            .reverb (0.5f, 0.6f, 0.22f)
            .poly (12).vel (0.6f).vol (-4.7f));

    // =====================================================================
    // EXPANSION 0.2 — FX & ATMOS
    // =====================================================================
    add (P ("Downlifter", fx)
            .osc (A, WT::AnalogSaw, 1.0f, 0.6f).uni (A, 7, 0.35f, 1.0f)
            .noise (NoiseType::White, 0.3f)
            .filter (FT::LP24, 300.0f, 0.25f)
            .menv (0.0f, 3.0f, 0.0f, 1.0f)
            .amp (0.01f, 3.0f, 0.0f, 0.5f)
            .mod (1, ModEnv, Cutoff, 1.0f)
            .mod (2, ModEnv, Pitch, 0.5f)
            .reverb (0.8f, 0.4f, 0.35f)
            .poly (4).vel (0.0f).vol (3.7f));

    add (P ("Noise Sweep", fx)
            .oscOff (A)
            .noise (NoiseType::White, 0.8f)
            .filter (FT::BP, 500.0f, 0.5f)
            .amp (0.5f, 1.0f, 1.0f, 1.5f)
            .lfo (0, LS::Triangle, 0.15f, true)
            .mod (1, Lfo1, Cutoff, 0.8f)
            .reverb (0.8f, 0.4f, 0.35f)
            .poly (2).vel (0.0f).vol (-3.3f));

    add (P ("Alien Talk", fx)
            .osc (A, WT::Vowels, 0.5f, 0.75f)
            .filter (FT::LP12, 4000.0f, 0.1f)
            .amp (0.01f, 0.5f, 1.0f, 0.3f)
            .lfo (0, LS::SampleHold, 8.0f, true)
            .lfo (1, LS::Sine, 0.5f, true)
            .mod (1, Lfo1, APos, 0.8f)
            .mod (2, Lfo2, Pitch, 0.05f)
            .delay (SD::D1_16, 0.4f, 0.2f)
            .poly (4).vel (0.2f).vol (-5.0f));

    add (P ("Glitch Zap", fx)
            .osc (A, WT::BasicShapes, 0.9f, 0.7f)
            .filterOff()
            .menv (0.0f, 0.08f, 0.0f, 0.05f)
            .amp (0.001f, 0.25f, 0.0f, 0.05f)
            .lfoSync (0, LS::SampleHold, SD::D1_32, true)
            .mod (1, ModEnv, Pitch, 0.8f)
            .mod (2, Lfo1, Pitch, 0.2f)
            .lofi (6.0f, 6000.0f, 0.0f, 0.0f, 0.7f)
            .delay (SD::D1_16, 0.35f, 0.25f)
            .poly (8).vel (0.3f).vol (3.1f));

    add (P ("Wind", fx)
            .oscOff (A)
            .noise (NoiseType::Pink, 1.0f)
            .filter (FT::BP, 900.0f, 0.55f)
            .amp (1.5f, 1.0f, 1.0f, 2.5f)
            .lfo (0, LS::SmoothRandom, 0.2f, false)
            .lfo (1, LS::SmoothRandom, 0.1f, false)
            .mod (1, Lfo1, Cutoff, 0.6f)
            .mod (2, Lfo2, Amp, 0.4f)
            .reverb (0.9f, 0.5f, 0.5f)
            .poly (2).vel (0.0f).vol (4.3f));

    add (P ("Impact Boom", fx)
            .osc (A, WT::BasicShapes, 0.0f, 0.85f).pitch (A, -1)
            .noise (NoiseType::White, 0.0f)
            .filterOff()
            .menv (0.0f, 0.4f, 0.0f, 0.3f)
            .amp (0.001f, 2.5f, 0.0f, 1.5f)
            .mod (1, ModEnv, Pitch, 0.5f)
            .mod (2, ModEnv, NoiseLevel, 0.6f)
            .dist (DistType::Soft, 0.4f, 0.6f)
            .reverb (0.9f, 0.5f, 0.4f)
            .poly (4).vel (0.0f).vol (-10.4f));

    return list;
}
}

const std::vector<FactoryPreset>& getFactoryPresets()
{
    static const std::vector<FactoryPreset> presets = buildPresets();
    return presets;
}

const juce::StringArray& getFactoryCategories()
{
    static const juce::StringArray categories { bass, keys, pads, leads, plucks, bells, orch, lofiC, fx };
    return categories;
}
