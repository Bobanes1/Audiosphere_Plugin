// Offline preset renderer / sanity checker.
//
//   AudiosphereRender [outputDir] [--nolimit]
//
// Plays a category-appropriate phrase through every factory preset, prints
// peak / RMS / CPU figures and optionally writes one WAV per preset.

#include <juce_audio_formats/juce_audio_formats.h>
#include "PluginProcessor.h"
#include "Presets/FactoryPresets.h"

namespace
{
struct NoteEvent { double start, end; int note; float velocity; };

std::vector<NoteEvent> phraseFor (const juce::String& category)
{
    if (category == "808 & Bass")
        return { { 0.0, 1.4, 36, 0.9f }, { 1.2, 2.4, 41, 0.9f }, { 2.6, 3.4, 34, 0.8f } };

    if (category == "Leads")
        return { { 0.0, 1.0, 72, 0.8f }, { 0.9, 2.0, 76, 0.8f }, { 2.0, 3.2, 74, 0.8f } };

    if (category == "Plucks" || category == "Bells & Mallets")
        return { { 0.0, 0.3, 60, 0.8f }, { 0.375, 0.6, 64, 0.8f }, { 0.75, 1.0, 67, 0.8f }, { 1.125, 1.4, 72, 0.8f },
                 { 1.5, 3.0, 60, 0.8f }, { 1.5, 3.0, 64, 0.8f }, { 1.5, 3.0, 67, 0.8f } };

    if (category == "FX & Atmos")
        return { { 0.0, 3.0, 60, 0.8f } };

    // Keys, pads, strings, lo-fi: a held chord.
    return { { 0.0, 3.0, 48, 0.8f }, { 0.0, 3.0, 55, 0.8f }, { 0.0, 3.0, 60, 0.8f }, { 0.0, 3.0, 64, 0.8f } };
}
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI init;

    const double sampleRate = 44100.0;
    const int blockSize = 256;
    const double length = 5.0;

    juce::File outDir;
    bool noLimiter = false;
    for (int a = 1; a < argc; ++a)
    {
        const juce::String arg (argv[a]);
        if (arg == "--nolimit")
        {
            noLimiter = true;
        }
        else
        {
            outDir = juce::File::getCurrentWorkingDirectory().getChildFile (arg);
            outDir.createDirectory();
        }
    }

    AudiosphereProcessor processor;
    processor.setPlayConfigDetails (0, 2, sampleRate, blockSize);

    const auto& presets = getFactoryPresets();
    int failures = 0;

    std::printf ("%-26s %-18s %8s %8s %8s %7s\n", "preset", "category", "peakdB", "rmsdB", "taildB", "cpu%");

    for (int i = 0; i < (int) presets.size(); ++i)
    {
        processor.presetManager.load (i);
        if (noLimiter)
            if (auto* limiter = processor.apvts.getParameter ("limiter_on"))
                limiter->setValueNotifyingHost (0.0f);
        processor.prepareToPlay (sampleRate, blockSize);

        const auto phrase = phraseFor (presets[(size_t) i].category);
        const int total = (int) (length * sampleRate);
        juce::AudioBuffer<float> output (2, total);
        juce::AudioBuffer<float> block (2, blockSize);
        juce::MidiBuffer midi;

        const auto startTime = juce::Time::getHighResolutionTicks();

        for (int pos = 0; pos < total; pos += blockSize)
        {
            const int n = std::min (blockSize, total - pos);
            midi.clear();

            for (const auto& e : phrase)
            {
                const int on = (int) (e.start * sampleRate), off = (int) (e.end * sampleRate);
                if (on >= pos && on < pos + n)
                    midi.addEvent (juce::MidiMessage::noteOn (1, e.note, e.velocity), on - pos);
                if (off >= pos && off < pos + n)
                    midi.addEvent (juce::MidiMessage::noteOff (1, e.note), off - pos);
            }

            juce::AudioBuffer<float> view (block.getArrayOfWritePointers(), 2, n);
            processor.processBlock (view, midi);
            for (int c = 0; c < 2; ++c)
                output.copyFrom (c, pos, view, c, 0, n);
        }

        const double seconds = juce::Time::highResolutionTicksToSeconds (juce::Time::getHighResolutionTicks() - startTime);

        float peak = 0.0f;
        double sum = 0.0;
        bool bad = false;
        const int soundEnd = (int) (3.0 * sampleRate);
        for (int c = 0; c < 2; ++c)
        {
            const float* d = output.getReadPointer (c);
            for (int s = 0; s < total; ++s)
            {
                if (! std::isfinite (d[s]))
                    bad = true;
                peak = std::max (peak, std::abs (d[s]));
                if (s < soundEnd)
                    sum += (double) d[s] * d[s];
            }
        }

        const double rms = std::sqrt (sum / (2.0 * soundEnd));
        const float tail = output.getMagnitude (total - (int) (0.25 * sampleRate), (int) (0.25 * sampleRate));
        const auto toDb = [] (double v) { return juce::Decibels::gainToDecibels (v, -120.0); };

        std::printf ("%-26s %-18s %8.1f %8.1f %8.1f %7.1f%s\n",
                     presets[(size_t) i].name.toRawUTF8(), presets[(size_t) i].category.toRawUTF8(),
                     toDb (peak), toDb (rms), toDb (tail), 100.0 * seconds / length,
                     bad ? "  <-- NON-FINITE" : (peak < 1.0e-3f ? "  <-- SILENT" : ""));

        if (bad || peak < 1.0e-3f)
            ++failures;

        if (outDir.exists())
        {
            auto file = outDir.getChildFile (juce::String (i).paddedLeft ('0', 2) + " " + presets[(size_t) i].name + ".wav");
            file.deleteFile();
            juce::WavAudioFormat wav;
            if (auto stream = file.createOutputStream())
            {
                if (auto writer = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (stream.get(), sampleRate, 2, 16, {}, 0)))
                {
                    stream.release();
                    writer->writeFromAudioSampleBuffer (output, 0, total);
                }
            }
        }
    }

    std::printf ("\n%d presets, %d problems\n", (int) presets.size(), failures);
    return failures == 0 ? 0 : 1;
}
