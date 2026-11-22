// Quadrilo - four-tap stereo delay
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace quadrilo
{
constexpr int numTaps = 4;
constexpr int numNotes = 14;
constexpr float maxDelaySeconds = 2.0f;

/** Note lengths in quarter-note beats, shortest to longest. */
inline constexpr std::array<float, numNotes> noteBeats {
    0.125f, 1.0f / 6.0f, 0.25f, 1.0f / 3.0f, 0.375f, 0.5f, 2.0f / 3.0f,
    0.75f, 1.0f, 4.0f / 3.0f, 1.5f, 2.0f, 3.0f, 4.0f };

inline constexpr const char* noteNames[numNotes] {
    "1/32", "1/16T", "1/16", "1/8T", "1/16.", "1/8", "1/4T",
    "1/8.", "1/4", "1/2T", "1/4.", "1/2", "1/2.", "1/1" };

inline float syncDelaySeconds (int noteIndex, double bpm)
{
    const int i = std::clamp (noteIndex, 0, numNotes - 1);
    return (float) (60.0 / std::max (bpm, 1.0) * (double) noteBeats[(size_t) i]);
}

struct TapParams
{
    float delaySeconds = 0.25f;
    float feedback = 0.5f;   // 0..1
    float damp = 0.0f;       // 0..1, one-pole low-pass amount in the feedback path
    float volume = 0.5f;     // 0..1 linear
    float pan = 0.5f;        // 0 = left, 1 = right
};

class QuadDelay
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        smoothCoef = 1.0f - std::exp (-1.0f / (0.02f * (float) sampleRate));
        const int size = (int) (sampleRate * (double) maxDelaySeconds) + 4;
        for (auto& t : taps)
            t.buffer.assign ((size_t) size, 0.0f);
        reset();
    }

    void reset()
    {
        for (auto& t : taps)
        {
            std::fill (t.buffer.begin(), t.buffer.end(), 0.0f);
            t.writePos = 0;
            t.lp = 0.0f;
        }
    }

    void setTap (int i, const TapParams& p)
    {
        auto& t = taps[(size_t) i];
        const float maxSamples = (float) t.buffer.size() - 3.0f;
        t.delayTarget = std::clamp (p.delaySeconds * (float) sr, 1.0f, maxSamples);
        t.feedbackTarget = std::clamp (p.feedback, 0.0f, 1.0f);
        t.dampTarget = std::clamp (p.damp, 0.0f, 0.999f);
        t.volumeTarget = std::clamp (p.volume, 0.0f, 1.0f);
        t.panTarget = std::clamp (p.pan, 0.0f, 1.0f);
    }

    /** 0 = dry only, 1 = delay taps only. Bypass is simply a mix of 0. */
    void setMix (float m) { mixTarget = std::clamp (m, 0.0f, 1.0f); }

    /** Jump all smoothed values to their targets (after prepare, or in tests). */
    void snap()
    {
        for (auto& t : taps)
        {
            t.delay = t.delayTarget;
            t.feedback = t.feedbackTarget;
            t.damp = t.dampTarget;
            t.volume = t.volumeTarget;
            t.pan = t.panTarget;
        }
        mix = mixTarget;
    }

    void process (float* l, float* r, int numSamples)
    {
        for (int s = 0; s < numSamples; ++s)
        {
            mix += (mixTarget - mix) * smoothCoef;
            const float inL = l[s], inR = r[s];
            float outL = inL * (1.0f - mix), outR = inR * (1.0f - mix);

            for (size_t i = 0; i < (size_t) numTaps; ++i)
            {
                auto& t = taps[i];
                t.delay += (t.delayTarget - t.delay) * smoothCoef;
                t.feedback += (t.feedbackTarget - t.feedback) * smoothCoef;
                t.damp += (t.dampTarget - t.damp) * smoothCoef;
                t.volume += (t.volumeTarget - t.volume) * smoothCoef;
                t.pan += (t.panTarget - t.pan) * smoothCoef;

                const int size = (int) t.buffer.size();
                float readPos = (float) t.writePos - t.delay;
                if (readPos < 0.0f)
                    readPos += (float) size;
                const int i0 = std::min ((int) readPos, size - 1);
                const int i1 = (i0 + 1) % size;
                const float frac = readPos - (float) i0;
                const float delayed = t.buffer[(size_t) i0] * (1.0f - frac) + t.buffer[(size_t) i1] * frac;

                t.lp = delayed * (1.0f - t.damp) + t.damp * t.lp;
                const float y = t.lp;

                t.buffer[(size_t) t.writePos] = inL * inputLeft[i] + inR * (1.0f - inputLeft[i]) + y * t.feedback;
                t.writePos = (t.writePos + 1) % size;

                const float wet = y * 0.5f * mix * t.volume;
                outL += wet * (1.0f - t.pan);
                outR += wet * t.pan;
            }
            l[s] = outL;
            r[s] = outR;
        }
    }

private:
    struct Tap
    {
        std::vector<float> buffer;
        int writePos = 0;
        float lp = 0.0f;
        float delay = 1.0f, feedback = 0.5f, damp = 0.0f, volume = 0.5f, pan = 0.5f;
        float delayTarget = 1.0f, feedbackTarget = 0.5f, dampTarget = 0.0f, volumeTarget = 0.5f, panTarget = 0.5f;
    };

    /** How much of the left input channel feeds each tap (the right channel gets the rest). */
    static constexpr std::array<float, numTaps> inputLeft { 0.25f, 0.4f, 0.6f, 0.75f };

    std::array<Tap, numTaps> taps;
    double sr = 44100.0;
    float smoothCoef = 0.001f;
    float mix = 0.5f, mixTarget = 0.5f;
};
}
