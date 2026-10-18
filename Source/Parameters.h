#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

// Choice-parameter enumerations shared by the DSP, the GUI and the factory presets.
// The order here must match the string lists in Parameters.cpp.

namespace WT
{
    enum Table
    {
        BasicShapes = 0, AnalogSaw, PulseWidth, Additive, Organ, EPiano, Vowels,
        HardSync, Growl, Hollow, Mallet, Chiptune, Glass, Strings, Choir, Digital,
        NumTables
    };
}

enum class WarpMode   { Off = 0, Bend, Squeeze, Sync, Quantize, FM, AM };
enum class FilterType { LP12 = 0, LP24, HP12, HP24, BP, Notch };
enum class LfoShape   { Sine = 0, Triangle, SawUp, SawDown, Square, SampleHold, SmoothRandom };
enum class VoiceMode  { Poly = 0, Mono, Legato };
enum class SubShape   { Sine = 0, Triangle, Square, Saw };
enum class NoiseType  { White = 0, Pink, Crackle };
enum class DistType   { Soft = 0, Hard, Tube, Fold, Rectify };

namespace ModSrc
{
    enum Source
    {
        Off = 0, Lfo1, Lfo2, AmpEnv, FiltEnv, ModEnv, Velocity, ModWheel,
        Aftertouch, KeyTrack, Random, Macro1, Macro2, Macro3, Macro4, PitchBend,
        NumSources
    };
}

namespace ModDst
{
    enum Destination
    {
        Off = 0, Pitch, PitchFine,
        APos, ALevel, AWarp, ADetune, APan,
        BPos, BLevel, BWarp, BDetune, BPan,
        SubLevel, NoiseLevel,
        Cutoff, Resonance, FilterDrive,
        Amp, Pan, Lfo1Rate, Lfo2Rate,
        NumDestinations
    };
}

// Tempo-sync divisions, expressed in quarter-note beats.
namespace SyncDiv
{
    enum Division
    {
        D1_64 = 0, D1_32, D1_16T, D1_16, D1_16D, D1_8T, D1_8, D1_8D,
        D1_4T, D1_4, D1_4D, D1_2, D1Bar, D2Bars, D4Bars,
        NumDivisions
    };

    double beats (int division);
}

namespace Params
{
    constexpr int numModSlots = 8;
    constexpr int numMacros   = 4;

    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    const juce::StringArray& wavetableNames();
    const juce::StringArray& warpModeNames();
    const juce::StringArray& filterTypeNames();
    const juce::StringArray& lfoShapeNames();
    const juce::StringArray& voiceModeNames();
    const juce::StringArray& subShapeNames();
    const juce::StringArray& noiseTypeNames();
    const juce::StringArray& distTypeNames();
    const juce::StringArray& modSourceNames();
    const juce::StringArray& modDestinationNames();
    const juce::StringArray& syncDivisionNames();

    inline juce::String osc (int index, const char* name)   { return juce::String ("osc") + (index == 0 ? "A_" : "B_") + name; }
    inline juce::String lfo (int index, const char* name)   { return "lfo" + juce::String (index + 1) + "_" + name; }
    inline juce::String mod (int slot, const char* name)    { return "mod" + juce::String (slot + 1) + "_" + name; }
    inline juce::String macro (int index)                   { return "macro" + juce::String (index + 1); }
}
