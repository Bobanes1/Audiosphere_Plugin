#pragma once

#include <vector>

// A bank of procedurally generated, band-limited wavetables.
//
// Every table has `numFrames` single-cycle frames that the oscillator morphs
// between with the "position" control. Each frame is stored at `numLevels`
// mip levels; level L keeps harmonics 1 .. (maxHarmonics >> L) so that a
// level can be picked per note to keep aliasing out of the audible range.
class WavetableBank
{
public:
    static constexpr int tableSize    = 2048;
    static constexpr int numFrames    = 16;
    static constexpr int numLevels    = 11;
    static constexpr int maxHarmonics = tableSize / 2;
    static constexpr int frameStride  = tableSize + 1;   // one guard sample for interpolation

    static const WavetableBank& get();

    int getNumTables() const noexcept { return numTables; }

    // Returns tableSize + 1 samples (the last one equals the first).
    const float* getFrame (int table, int level, int frame) const noexcept
    {
        return data.data() + ((size_t) ((table * numLevels + level) * numFrames + frame)) * frameStride;
    }

    // Picks the mip level for an oscillator advancing `phaseIncrement` cycles per sample.
    static int levelForIncrement (float phaseIncrement) noexcept
    {
        // Allow harmonics up to 0.6 * fs: anything folding back lands above 0.4 * fs.
        int level = 0;
        while (level < numLevels - 1 && (float) (maxHarmonics >> level) * phaseIncrement > 0.6f)
            ++level;
        return level;
    }

private:
    WavetableBank();

    int numTables = 0;
    std::vector<float> data;
};
