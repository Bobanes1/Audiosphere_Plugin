#include "Parameters.h"

using APF   = juce::AudioParameterFloat;
using APC   = juce::AudioParameterChoice;
using APB   = juce::AudioParameterBool;
using API   = juce::AudioParameterInt;
using Range = juce::NormalisableRange<float>;

double SyncDiv::beats (int division)
{
    static constexpr double values[] = {
        1.0 / 16.0,              // 1/64
        1.0 / 8.0,               // 1/32
        1.0 / 6.0,               // 1/16T
        1.0 / 4.0,               // 1/16
        3.0 / 8.0,               // 1/16D
        1.0 / 3.0,               // 1/8T
        1.0 / 2.0,               // 1/8
        3.0 / 4.0,               // 1/8D
        2.0 / 3.0,               // 1/4T
        1.0,                     // 1/4
        1.5,                     // 1/4D
        2.0,                     // 1/2
        4.0,                     // 1 bar
        8.0,                     // 2 bars
        16.0                     // 4 bars
    };
    return values[juce::jlimit (0, (int) NumDivisions - 1, division)];
}

namespace Params
{
const juce::StringArray& wavetableNames()
{
    static const juce::StringArray names { "Basic Shapes", "Analog Saw", "Pulse Width", "Additive", "Organ",
                                           "E-Piano", "Vowels", "Hard Sync", "Growl", "Hollow", "Mallet",
                                           "Chiptune", "Glass", "Strings", "Choir", "Digital" };
    return names;
}

const juce::StringArray& warpModeNames()       { static const juce::StringArray n { "Off", "Bend", "Squeeze", "Sync", "Quantize", "FM", "AM" }; return n; }
const juce::StringArray& filterTypeNames()     { static const juce::StringArray n { "LP 12", "LP 24", "HP 12", "HP 24", "Band Pass", "Notch" }; return n; }
const juce::StringArray& lfoShapeNames()       { static const juce::StringArray n { "Sine", "Triangle", "Saw Up", "Saw Down", "Square", "S&H", "Smooth Rnd" }; return n; }
const juce::StringArray& voiceModeNames()      { static const juce::StringArray n { "Poly", "Mono", "Legato" }; return n; }
const juce::StringArray& subShapeNames()       { static const juce::StringArray n { "Sine", "Triangle", "Square", "Saw" }; return n; }
const juce::StringArray& noiseTypeNames()      { static const juce::StringArray n { "White", "Pink", "Crackle" }; return n; }
const juce::StringArray& distTypeNames()       { static const juce::StringArray n { "Soft Clip", "Hard Clip", "Tube", "Wavefold", "Rectify" }; return n; }

const juce::StringArray& modSourceNames()
{
    static const juce::StringArray n { "Off", "LFO 1", "LFO 2", "Amp Env", "Filter Env", "Mod Env", "Velocity",
                                       "Mod Wheel", "Aftertouch", "Key Track", "Random", "Macro 1", "Macro 2",
                                       "Macro 3", "Macro 4", "Pitch Bend" };
    return n;
}

const juce::StringArray& modDestinationNames()
{
    static const juce::StringArray n { "Off", "Pitch", "Pitch Fine",
                                       "Osc A Pos", "Osc A Level", "Osc A Warp", "Osc A Detune", "Osc A Pan",
                                       "Osc B Pos", "Osc B Level", "Osc B Warp", "Osc B Detune", "Osc B Pan",
                                       "Sub Level", "Noise Level",
                                       "Cutoff", "Resonance", "Filter Drive",
                                       "Amp", "Pan", "LFO 1 Rate", "LFO 2 Rate" };
    return n;
}

const juce::StringArray& syncDivisionNames()
{
    static const juce::StringArray n { "1/64", "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D",
                                       "1/4T", "1/4", "1/4D", "1/2", "1 Bar", "2 Bars", "4 Bars" };
    return n;
}

namespace
{
    Range skewed (float lo, float hi, float centre)
    {
        Range r (lo, hi);
        r.setSkewForCentre (centre);
        return r;
    }

    juce::AudioParameterFloatAttributes withLabel (const juce::String& label)
    {
        return juce::AudioParameterFloatAttributes().withLabel (label);
    }

    juce::AudioParameterFloatAttributes percent()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int) { return juce::String (juce::roundToInt (v * 100.0f)) + " %"; });
    }

    juce::AudioParameterFloatAttributes bipolarPercent()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                auto p = juce::roundToInt (v * 100.0f);
                return (p > 0 ? "+" : "") + juce::String (p) + " %";
            });
    }

    juce::AudioParameterFloatAttributes seconds()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1.0f ? juce::String (juce::roundToInt (v * 1000.0f)) + " ms"
                                : juce::String (v, 2) + " s";
            });
    }

    juce::AudioParameterFloatAttributes hertz()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return v < 1000.0f ? juce::String (v, v < 10.0f ? 2 : 0) + " Hz"
                                   : juce::String (v / 1000.0f, 2) + " kHz";
            });
    }

    juce::AudioParameterFloatAttributes decibels()
    {
        return juce::AudioParameterFloatAttributes()
            .withStringFromValueFunction ([] (float v, int)
            {
                return (v > 0.0f ? "+" : "") + juce::String (v, 1) + " dB";
            });
    }

    template <typename Param, typename... Args>
    void add (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& id, Args&&... args)
    {
        layout.add (std::make_unique<Param> (juce::ParameterID { id, 1 }, std::forward<Args> (args)...));
    }

    void addEnvelope (juce::AudioProcessorValueTreeState::ParameterLayout& layout, const juce::String& prefix,
                      const juce::String& name, float a, float d, float s, float r)
    {
        add<APF> (layout, prefix + "_a", name + " Attack",  skewed (0.0f, 10.0f, 0.5f), a, seconds());
        add<APF> (layout, prefix + "_d", name + " Decay",   skewed (0.0f, 10.0f, 0.8f), d, seconds());
        add<APF> (layout, prefix + "_s", name + " Sustain", Range (0.0f, 1.0f),         s, percent());
        add<APF> (layout, prefix + "_r", name + " Release", skewed (0.0f, 10.0f, 0.8f), r, seconds());
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // ---- Global ------------------------------------------------------------
    add<APF> (layout, "master_vol", "Master Volume", Range (-48.0f, 6.0f, 0.01f), -3.0f, decibels());
    add<APC> (layout, "voice_mode", "Voice Mode", voiceModeNames(), 0);
    add<API> (layout, "polyphony", "Polyphony", 1, 16, 8);
    add<APF> (layout, "glide", "Glide", skewed (0.0f, 2.0f, 0.25f), 0.0f, seconds());
    add<API> (layout, "bend_range", "Pitch Bend Range", 0, 24, 2);
    add<APF> (layout, "amp_vel", "Velocity Sensitivity", Range (0.0f, 1.0f), 0.5f, percent());

    for (int m = 0; m < numMacros; ++m)
        add<APF> (layout, macro (m), "Macro " + juce::String (m + 1), Range (0.0f, 1.0f), 0.0f, percent());

    // ---- Wavetable oscillators -------------------------------------------
    for (int o = 0; o < 2; ++o)
    {
        const juce::String n = o == 0 ? "Osc A " : "Osc B ";
        add<APB> (layout, osc (o, "on"),        n + "On", o == 0);
        add<APC> (layout, osc (o, "table"),     n + "Wavetable", wavetableNames(), o == 0 ? (int) WT::BasicShapes : (int) WT::AnalogSaw);
        add<APF> (layout, osc (o, "pos"),       n + "Position", Range (0.0f, 1.0f), o == 0 ? 0.33f : 0.0f, percent());
        add<API> (layout, osc (o, "oct"),       n + "Octave", -3, 3, 0);
        add<API> (layout, osc (o, "semi"),      n + "Semitone", -12, 12, 0);
        add<APF> (layout, osc (o, "fine"),      n + "Fine", Range (-100.0f, 100.0f, 1.0f), 0.0f, withLabel ("ct"));
        add<API> (layout, osc (o, "unison"),    n + "Unison", 1, 8, 1);
        add<APF> (layout, osc (o, "detune"),    n + "Detune", Range (0.0f, 1.0f), 0.25f, percent());
        add<APF> (layout, osc (o, "spread"),    n + "Stereo Spread", Range (0.0f, 1.0f), 0.6f, percent());
        add<APF> (layout, osc (o, "level"),     n + "Level", Range (0.0f, 1.0f), 0.75f, percent());
        add<APF> (layout, osc (o, "pan"),       n + "Pan", Range (-1.0f, 1.0f), 0.0f, bipolarPercent());
        add<APC> (layout, osc (o, "warp_mode"), n + "Warp Mode", warpModeNames(), 0);
        add<APF> (layout, osc (o, "warp"),      n + "Warp", Range (0.0f, 1.0f), 0.0f, percent());
        add<APB> (layout, osc (o, "retrig"),    n + "Phase Retrigger", true);
    }

    // ---- Sub / noise -------------------------------------------------------
    add<APB> (layout, "sub_on",    "Sub On", false);
    add<APC> (layout, "sub_shape", "Sub Shape", subShapeNames(), 0);
    add<API> (layout, "sub_oct",   "Sub Octave", -2, 0, -1);
    add<APF> (layout, "sub_level", "Sub Level", Range (0.0f, 1.0f), 0.6f, percent());

    add<APB> (layout, "noise_on",    "Noise On", false);
    add<APC> (layout, "noise_type",  "Noise Type", noiseTypeNames(), 0);
    add<APF> (layout, "noise_level", "Noise Level", Range (0.0f, 1.0f), 0.3f, percent());

    // ---- Filter ------------------------------------------------------------
    add<APB> (layout, "flt_on",     "Filter On", true);
    add<APC> (layout, "flt_type",   "Filter Type", filterTypeNames(), 0);
    add<APF> (layout, "flt_cutoff", "Filter Cutoff", skewed (20.0f, 20000.0f, 1000.0f), 20000.0f, hertz());
    add<APF> (layout, "flt_res",    "Filter Resonance", Range (0.0f, 1.0f), 0.1f, percent());
    add<APF> (layout, "flt_drive",  "Filter Drive", Range (0.0f, 1.0f), 0.0f, percent());
    add<APF> (layout, "flt_env",    "Filter Env Amount", Range (-1.0f, 1.0f), 0.0f, bipolarPercent());
    add<APF> (layout, "flt_key",    "Filter Key Track", Range (0.0f, 1.0f), 0.0f, percent());
    add<APF> (layout, "flt_vel",    "Filter Velocity", Range (0.0f, 1.0f), 0.0f, percent());

    // ---- Envelopes ---------------------------------------------------------
    addEnvelope (layout, "amp",  "Amp",        0.002f, 0.3f, 1.0f, 0.15f);
    addEnvelope (layout, "fenv", "Filter Env", 0.002f, 0.4f, 0.3f, 0.3f);
    addEnvelope (layout, "menv", "Mod Env",    0.0f,   0.3f, 0.0f, 0.3f);

    // ---- LFOs --------------------------------------------------------------
    for (int l = 0; l < 2; ++l)
    {
        const juce::String n = "LFO " + juce::String (l + 1) + " ";
        add<APC> (layout, lfo (l, "shape"),  n + "Shape", lfoShapeNames(), 0);
        add<APF> (layout, lfo (l, "rate"),   n + "Rate", skewed (0.01f, 40.0f, 2.0f), l == 0 ? 2.0f : 0.5f, hertz());
        add<APB> (layout, lfo (l, "sync"),   n + "Tempo Sync", false);
        add<APC> (layout, lfo (l, "div"),    n + "Sync Rate", syncDivisionNames(), (int) SyncDiv::D1_4);
        add<APB> (layout, lfo (l, "retrig"), n + "Retrigger", true);
        add<APF> (layout, lfo (l, "fade"),   n + "Fade In", skewed (0.0f, 5.0f, 0.7f), 0.0f, seconds());
    }

    // ---- Mod matrix --------------------------------------------------------
    for (int s = 0; s < numModSlots; ++s)
    {
        const juce::String n = "Mod " + juce::String (s + 1) + " ";
        add<APC> (layout, mod (s, "src"), n + "Source", modSourceNames(), 0);
        add<APC> (layout, mod (s, "dst"), n + "Destination", modDestinationNames(), 0);
        add<APF> (layout, mod (s, "amt"), n + "Amount", Range (-1.0f, 1.0f), 0.0f, bipolarPercent());
    }

    // ---- Effects -----------------------------------------------------------
    add<APB> (layout, "dist_on",    "Distortion On", false);
    add<APC> (layout, "dist_type",  "Distortion Type", distTypeNames(), 0);
    add<APF> (layout, "dist_drive", "Distortion Drive", Range (0.0f, 1.0f), 0.3f, percent());
    add<APF> (layout, "dist_tone",  "Distortion Tone", Range (0.0f, 1.0f), 0.8f, percent());
    add<APF> (layout, "dist_mix",   "Distortion Mix", Range (0.0f, 1.0f), 1.0f, percent());

    add<APB> (layout, "lofi_on",    "Lo-Fi On", false);
    add<APF> (layout, "lofi_bits",  "Lo-Fi Bit Depth", Range (4.0f, 16.0f), 12.0f, withLabel ("bit"));
    add<APF> (layout, "lofi_rate",  "Lo-Fi Sample Rate", skewed (2000.0f, 48000.0f, 12000.0f), 26000.0f, hertz());
    add<APF> (layout, "lofi_wow",   "Lo-Fi Wow & Flutter", Range (0.0f, 1.0f), 0.3f, percent());
    add<APF> (layout, "lofi_noise", "Lo-Fi Vinyl Noise", Range (0.0f, 1.0f), 0.2f, percent());
    add<APF> (layout, "lofi_tone",  "Lo-Fi Tone", Range (0.0f, 1.0f), 0.6f, percent());
    add<APF> (layout, "lofi_mix",   "Lo-Fi Mix", Range (0.0f, 1.0f), 1.0f, percent());

    add<APB> (layout, "eq_on",   "EQ On", false);
    add<APF> (layout, "eq_low",  "EQ Low", Range (-12.0f, 12.0f), 0.0f, decibels());
    add<APF> (layout, "eq_mid",  "EQ Mid", Range (-12.0f, 12.0f), 0.0f, decibels());
    add<APF> (layout, "eq_high", "EQ High", Range (-12.0f, 12.0f), 0.0f, decibels());

    add<APB> (layout, "chorus_on",    "Chorus On", false);
    add<APF> (layout, "chorus_rate",  "Chorus Rate", skewed (0.05f, 5.0f, 0.8f), 0.6f, hertz());
    add<APF> (layout, "chorus_depth", "Chorus Depth", Range (0.0f, 1.0f), 0.4f, percent());
    add<APF> (layout, "chorus_mix",   "Chorus Mix", Range (0.0f, 1.0f), 0.4f, percent());

    add<APB> (layout, "delay_on",       "Delay On", false);
    add<APB> (layout, "delay_sync",     "Delay Sync", true);
    add<APF> (layout, "delay_time",     "Delay Time", skewed (1.0f, 2000.0f, 350.0f), 350.0f, withLabel ("ms"));
    add<APC> (layout, "delay_div",      "Delay Sync Rate", syncDivisionNames(), (int) SyncDiv::D1_8D);
    add<APF> (layout, "delay_fb",       "Delay Feedback", Range (0.0f, 0.95f), 0.35f, percent());
    add<APB> (layout, "delay_pingpong", "Delay Ping Pong", true);
    add<APF> (layout, "delay_tone",     "Delay Tone", Range (0.0f, 1.0f), 0.6f, percent());
    add<APF> (layout, "delay_mix",      "Delay Mix", Range (0.0f, 1.0f), 0.25f, percent());

    add<APB> (layout, "reverb_on",    "Reverb On", false);
    add<APF> (layout, "reverb_size",  "Reverb Size", Range (0.0f, 1.0f), 0.6f, percent());
    add<APF> (layout, "reverb_damp",  "Reverb Damping", Range (0.0f, 1.0f), 0.5f, percent());
    add<APF> (layout, "reverb_width", "Reverb Width", Range (0.0f, 1.0f), 1.0f, percent());
    add<APF> (layout, "reverb_mix",   "Reverb Mix", Range (0.0f, 1.0f), 0.25f, percent());

    add<APB> (layout, "limiter_on", "Output Limiter", true);

    return layout;
}
}
