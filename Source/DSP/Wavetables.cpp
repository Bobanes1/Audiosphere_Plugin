#include "Wavetables.h"
#include "../Parameters.h"

#include <cmath>
#include <complex>
#include <functional>
#include <random>
#include <algorithm>

namespace
{
constexpr int N = WavetableBank::tableSize;
constexpr double twoPi = 6.283185307179586476925286766559;

using Complex  = std::complex<double>;
using Spectrum = std::vector<Complex>;

// In-place iterative radix-2 FFT. inverse == true computes the unscaled inverse.
void fft (Spectrum& x, bool inverse)
{
    const int n = (int) x.size();

    for (int i = 1, j = 0; i < n; ++i)
    {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j)
            std::swap (x[(size_t) i], x[(size_t) j]);
    }

    for (int len = 2; len <= n; len <<= 1)
    {
        const double angle = (inverse ? twoPi : -twoPi) / len;
        const Complex wlen (std::cos (angle), std::sin (angle));

        for (int i = 0; i < n; i += len)
        {
            Complex w (1.0, 0.0);
            for (int k = 0; k < len / 2; ++k)
            {
                auto u = x[(size_t) (i + k)];
                auto v = x[(size_t) (i + k + len / 2)] * w;
                x[(size_t) (i + k)] = u + v;
                x[(size_t) (i + k + len / 2)] = u - v;
                w *= wlen;
            }
        }
    }
}

// A frame description: either a time-domain function of phase, or a set of
// harmonic amplitudes (sine phase).
struct FrameSource
{
    std::function<double (double)> timeDomain;     // phase in [0, 1) -> sample
    std::vector<double> harmonics;                 // harmonics[k] = amplitude of harmonic k (k >= 1)
};

Spectrum toSpectrum (const FrameSource& src)
{
    Spectrum spec ((size_t) N, Complex());

    if (src.timeDomain)
    {
        for (int i = 0; i < N; ++i)
            spec[(size_t) i] = Complex (src.timeDomain ((double) i / N), 0.0);

        fft (spec, false);
    }
    else
    {
        // x[n] = a * sin(2 pi k n / N)  <=>  X[k] = -i * a * N / 2, X[N-k] = conj
        for (size_t k = 1; k < src.harmonics.size() && k < (size_t) N / 2; ++k)
        {
            const double a = src.harmonics[k];
            spec[k]     = Complex (0.0, -a * N * 0.5);
            spec[N - k] = std::conj (spec[k]);
        }
    }

    spec[0] = Complex();   // no DC
    return spec;
}

void renderLevel (const Spectrum& full, int maxHarm, double scale, float* out)
{
    Spectrum spec ((size_t) N, Complex());

    for (int k = 1; k <= maxHarm && k < N / 2; ++k)
    {
        spec[(size_t) k]     = full[(size_t) k];
        spec[(size_t) (N - k)] = full[(size_t) (N - k)];
    }

    fft (spec, true);

    for (int i = 0; i < N; ++i)
        out[i] = (float) (spec[(size_t) i].real() * scale);

    out[N] = out[0];
}

// ---------------------------------------------------------------------------
// Waveform helpers
// ---------------------------------------------------------------------------
double sine (double p)     { return std::sin (twoPi * p); }
double triangle (double p) { return p < 0.25 ? 4.0 * p : (p < 0.75 ? 2.0 - 4.0 * p : 4.0 * p - 4.0); }
double sawUp (double p)    { return p < 0.5 ? 2.0 * p : 2.0 * p - 2.0; }
double square (double p)   { return p < 0.5 ? 1.0 : -1.0; }
double frac (double x)     { return x - std::floor (x); }
double lerp (double a, double b, double t) { return a + (b - a) * t; }

double gauss (double x, double centre, double width)
{
    const double d = (x - centre) / width;
    return std::exp (-d * d);
}

std::vector<double> lerpSpectra (const std::vector<std::vector<double>>& keys, double t)
{
    const double pos = t * (double) (keys.size() - 1);
    const int i0 = std::min ((int) pos, (int) keys.size() - 2);
    const double f = pos - i0;

    const auto& a = keys[(size_t) i0];
    const auto& b = keys[(size_t) i0 + 1];
    std::vector<double> out (std::max (a.size(), b.size()), 0.0);

    for (size_t k = 0; k < out.size(); ++k)
        out[k] = lerp (k < a.size() ? a[k] : 0.0, k < b.size() ? b[k] : 0.0, f);

    return out;
}

// Harmonic amplitudes shaped by a set of formants (frequencies relative to a reference pitch).
std::vector<double> formantSpectrum (double f0, const double (&freqs)[3], const double (&gains)[3],
                                     const double (&widths)[3], double slope, double breath)
{
    std::vector<double> h (160, 0.0);
    for (size_t k = 1; k < h.size(); ++k)
    {
        const double f = f0 * (double) k;
        double env = 0.02;
        for (int j = 0; j < 3; ++j)
            env += gains[j] * gauss (f, freqs[j], widths[j]);

        h[k] = env / std::pow ((double) k, slope) + breath / (double) k * gauss (f, 5000.0, 2500.0);
    }
    return h;
}

// ---------------------------------------------------------------------------
// Table definitions: each returns the source for frame position t in [0, 1].
// ---------------------------------------------------------------------------
FrameSource makeFrame (int table, double t)
{
    FrameSource src;

    switch (table)
    {
        case WT::BasicShapes:
            src.timeDomain = [t] (double p)
            {
                const double seg = t * 3.0;
                if (seg < 1.0) return lerp (sine (p), triangle (p), seg);
                if (seg < 2.0) return lerp (triangle (p), sawUp (p), seg - 1.0);
                return lerp (sawUp (p), square (p), std::min (1.0, seg - 2.0));
            };
            break;

        case WT::AnalogSaw:
        {
            src.harmonics.assign (WavetableBank::maxHarmonics, 0.0);
            const double kc = std::pow (2.0, 1.0 + t * 9.0);
            for (size_t k = 1; k < src.harmonics.size(); ++k)
            {
                const double r = (double) k / kc;
                src.harmonics[k] = (1.0 / (double) k) / std::sqrt (1.0 + r * r * r * r);
            }
            break;
        }

        case WT::PulseWidth:
        {
            const double width = lerp (0.5, 0.04, t);
            src.timeDomain = [width] (double p) { return p < width ? 1.0 : -1.0; };
            break;
        }

        case WT::Additive:
        {
            src.harmonics.assign (64, 0.0);
            const double count = 1.0 + t * 40.0;
            for (size_t k = 1; k < src.harmonics.size(); ++k)
                src.harmonics[k] = std::clamp (count - (double) k + 1.0, 0.0, 1.0) / std::pow ((double) k, 0.8);
            break;
        }

        case WT::Organ:
        {
            // Drawbar registrations (16', 5 1/3', 8', 4', 2 2/3', 2', 1 3/5', 1 1/3', 1')
            static const int harmonicOf[9] = { 1, 3, 2, 4, 6, 8, 10, 12, 16 };
            static const int regs[5][9] = {
                { 8, 0, 8, 0, 0, 0, 0, 0, 0 },
                { 8, 8, 8, 0, 0, 0, 0, 0, 0 },
                { 8, 8, 8, 8, 0, 0, 0, 0, 0 },
                { 8, 8, 8, 8, 6, 4, 0, 0, 0 },
                { 8, 8, 8, 8, 8, 8, 8, 8, 8 } };

            std::vector<std::vector<double>> keys;
            for (auto& reg : regs)
            {
                std::vector<double> h (17, 0.0);
                for (int d = 0; d < 9; ++d)
                    if (reg[d] > 0)
                        h[(size_t) harmonicOf[d]] += std::pow (10.0, -(8 - reg[d]) * 3.0 / 20.0);
                keys.push_back (h);
            }
            src.harmonics = lerpSpectra (keys, t);
            break;
        }

        case WT::EPiano:
        {
            const double index = t * 4.5;
            src.timeDomain = [index] (double p)
            {
                return std::sin (twoPi * p + index * std::sin (twoPi * p))
                       + 0.15 * index / 4.5 * std::sin (twoPi * 2.0 * p + index * 0.5 * std::sin (twoPi * 7.0 * p));
            };
            break;
        }

        case WT::Vowels:
        {
            static const double vowels[5][3] = {
                { 730, 1090, 2440 },   // A
                { 530, 1840, 2480 },   // E
                { 290, 2290, 3010 },   // I
                { 570,  840, 2410 },   // O
                { 320,  870, 2240 } }; // U

            const double pos = t * 4.0;
            const int i0 = std::min ((int) pos, 3);
            const double f = pos - i0;
            double freqs[3];
            for (int j = 0; j < 3; ++j)
                freqs[j] = lerp (vowels[i0][j], vowels[i0 + 1][j], f);

            const double gains[3]  = { 1.0, 0.6, 0.3 };
            const double widths[3] = { 110.0, 140.0, 220.0 };
            src.harmonics = formantSpectrum (130.0, freqs, gains, widths, 0.6, 0.0);
            break;
        }

        case WT::HardSync:
        {
            const double ratio = 1.0 + t * 7.0;
            src.timeDomain = [ratio] (double p)
            {
                // Fade the synced saw at the cycle end to soften the reset.
                const double win = 1.0 - std::pow (p, 6.0);
                return (2.0 * frac (p * ratio) - 1.0) * win;
            };
            break;
        }

        case WT::Growl:
        {
            const double gain = 1.0 + t * 11.0;
            const double asym = 0.35 * t;
            src.timeDomain = [gain, asym] (double p)
            {
                return std::sin (gain * std::sin (twoPi * p) + asym * std::sin (2.0 * twoPi * p));
            };
            break;
        }

        case WT::Hollow:
        {
            src.harmonics.assign (WavetableBank::maxHarmonics, 0.0);
            const double slope = lerp (2.0, 0.6, t);
            for (size_t k = 1; k < src.harmonics.size(); k += 2)
                src.harmonics[k] = 1.0 / std::pow ((double) k, slope);
            break;
        }

        case WT::Mallet:
        {
            std::vector<double> soft (17, 0.0), mid (17, 0.0), hard (17, 0.0);
            soft[1] = 1.0; soft[4] = 0.35;
            mid[1]  = 1.0; mid[3] = 0.2; mid[4] = 0.55; mid[10] = 0.2;
            hard[1] = 1.0; hard[2] = 0.3; hard[4] = 0.7; hard[7] = 0.25; hard[10] = 0.45; hard[14] = 0.25; hard[16] = 0.15;
            src.harmonics = lerpSpectra ({ soft, mid, hard }, t);
            break;
        }

        case WT::Chiptune:
        {
            if (t < 0.5)
            {
                const double steps = std::round (lerp (16.0, 4.0, t * 2.0));
                src.timeDomain = [steps] (double p)
                {
                    return std::round (triangle (p) * steps * 0.5) / (steps * 0.5);
                };
            }
            else
            {
                const double width = lerp (0.5, 0.125, (t - 0.5) * 2.0);
                src.timeDomain = [width] (double p) { return p < width ? 1.0 : -1.0; };
            }
            break;
        }

        case WT::Glass:
        {
            src.harmonics.assign (33, 0.0);
            const std::pair<int, double> partials[] = { { 2, 0.15 }, { 3, 0.3 }, { 5, 0.12 }, { 8, 0.25 },
                                                        { 12, 0.18 }, { 16, 0.2 }, { 24, 0.12 }, { 32, 0.1 } };
            src.harmonics[1] = 1.0;
            const double amount = 0.2 + 1.8 * t;
            for (auto& [k, a] : partials)
                src.harmonics[(size_t) k] = a * amount;
            break;
        }

        case WT::Strings:
        {
            src.harmonics.assign (200, 0.0);
            const double kc = lerp (6.0, 60.0, t);
            const double f0 = 220.0;
            for (size_t k = 1; k < src.harmonics.size(); ++k)
            {
                const double f = f0 * (double) k;
                const double body = 1.0 + 0.9 * gauss (f, 280.0, 80.0) + 0.7 * gauss (f, 460.0, 120.0)
                                  + 0.8 * gauss (f, 1000.0, 300.0) + 0.9 * gauss (f, 2900.0, 700.0);
                const double r = (double) k / kc;
                src.harmonics[k] = body / (double) k / std::sqrt (1.0 + r * r * r * r);
            }
            break;
        }

        case WT::Choir:
        {
            static const double vowels[4][3] = {
                { 350,  600, 2700 },   // Ooh
                { 450,  800, 2830 },   // Oh
                { 800, 1150, 2800 },   // Aah
                { 400, 2000, 2800 } }; // Eeh

            const double pos = t * 3.0;
            const int i0 = std::min ((int) pos, 2);
            const double f = pos - i0;
            double freqs[3];
            for (int j = 0; j < 3; ++j)
                freqs[j] = lerp (vowels[i0][j], vowels[i0 + 1][j], f);

            const double gains[3]  = { 1.0, 0.5, 0.18 };
            const double widths[3] = { 90.0, 120.0, 200.0 };
            src.harmonics = formantSpectrum (220.0, freqs, gains, widths, 0.9, 0.15);
            break;
        }

        case WT::Digital:
        default:
        {
            // Morph between a handful of seeded random spectra.
            std::vector<std::vector<double>> keys;
            for (int key = 0; key < 5; ++key)
            {
                std::mt19937 rng ((unsigned) (1234 + key * 77));
                std::uniform_real_distribution<double> dist (0.0, 1.0);
                std::vector<double> h (96, 0.0);
                for (size_t k = 1; k < h.size(); ++k)
                    h[k] = (k == 1 ? 1.0 : std::pow (dist (rng), 3.0)) / std::pow ((double) k, 0.7);
                keys.push_back (h);
            }
            src.harmonics = lerpSpectra (keys, t);
            break;
        }
    }

    return src;
}
}

const WavetableBank& WavetableBank::get()
{
    static const WavetableBank bank;
    return bank;
}

WavetableBank::WavetableBank()
{
    numTables = WT::NumTables;
    data.assign ((size_t) numTables * numLevels * numFrames * frameStride, 0.0f);

    for (int table = 0; table < numTables; ++table)
    {
        for (int frame = 0; frame < numFrames; ++frame)
        {
            const double t = (double) frame / (double) (numFrames - 1);
            const auto spectrum = toSpectrum (makeFrame (table, t));

            // Normalise each frame to a peak of 1 using the full-bandwidth version.
            float* top = const_cast<float*> (getFrame (table, 0, frame));
            renderLevel (spectrum, maxHarmonics, 1.0, top);

            float peak = 0.0f;
            for (int i = 0; i < tableSize; ++i)
                peak = std::max (peak, std::abs (top[i]));

            const double scale = peak > 0.0f ? 1.0 / peak : 1.0;

            for (int level = 0; level < numLevels; ++level)
                renderLevel (spectrum, maxHarmonics >> level, scale, const_cast<float*> (getFrame (table, level, frame)));
        }
    }
}
