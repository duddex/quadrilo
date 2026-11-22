// Quadrilo - four-tap stereo delay
// Copyright (C) 2026 duddex
// SPDX-License-Identifier: GPL-3.0-or-later

// JUCE-free tests for the delay engine. Exit code is the number of failed checks.

#include "dsp/QuadDelay.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace
{
int failures = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        if (! (cond)) {                                                          \
            std::printf ("FAIL  %s:%d  %s\n", __FILE__, __LINE__, #cond);        \
            ++failures;                                                          \
        }                                                                        \
    } while (0)

bool near (float a, float b, float tol) { return std::fabs (a - b) <= tol; }

constexpr double sr = 48000.0;

struct Rig
{
    quadrilo::QuadDelay engine;

    Rig()
    {
        engine.prepare (sr);
        for (int i = 0; i < quadrilo::numTaps; ++i)
            set (i, 0.1f, 0.0f, 0.0f, 0.0f, 0.5f);
        engine.setMix (1.0f);
        engine.snap();
    }

    void set (int tap, float delaySec, float feedback, float damp, float volume, float pan)
    {
        quadrilo::TapParams p;
        p.delaySeconds = delaySec; p.feedback = feedback; p.damp = damp; p.volume = volume; p.pan = pan;
        engine.setTap (tap, p);
        engine.snap();
    }

    void run (std::vector<float>& l, std::vector<float>& r)
    {
        engine.process (l.data(), r.data(), (int) l.size());
    }
};

std::vector<float> impulse (int n) { std::vector<float> v ((size_t) n, 0.0f); v[0] = 1.0f; return v; }

int peakIndex (const std::vector<float>& v, int from = 0)
{
    int idx = from;
    for (int i = from; i < (int) v.size(); ++i)
        if (std::fabs (v[(size_t) i]) > std::fabs (v[(size_t) idx]))
            idx = i;
    return idx;
}

void testEchoTimingAndLevel()
{
    Rig rig;
    rig.set (0, 0.1f, 0.0f, 0.0f, 1.0f, 0.5f);
    auto l = impulse (12000), r = impulse (12000);
    rig.run (l, r);
    CHECK (std::abs (peakIndex (l, 1) - 4800) <= 1);
    // mono impulse -> buffer 1.0; wet = 1.0 * 0.5 (wet gain) * mix 1 * volume 1 * centre pan 0.5
    CHECK (near (l[(size_t) peakIndex (l, 1)], 0.25f, 0.01f));
    CHECK (near (r[(size_t) peakIndex (r, 1)], 0.25f, 0.01f));
}

void testNoDryAtFullWet()
{
    Rig rig;
    rig.set (0, 0.1f, 0.0f, 0.0f, 1.0f, 0.5f);
    auto l = impulse (200), r = impulse (200);
    rig.run (l, r);
    CHECK (near (l[0], 0.0f, 1e-6f));
    CHECK (near (r[0], 0.0f, 1e-6f));
}

void testMixZeroIsTransparent()
{
    Rig rig;
    for (int i = 0; i < quadrilo::numTaps; ++i)
        rig.set (i, 0.05f * (float) (i + 1), 0.7f, 0.3f, 1.0f, 0.5f);
    rig.engine.setMix (0.0f);
    rig.engine.snap();
    std::vector<float> l (4800), r (4800), refL, refR;
    for (size_t i = 0; i < l.size(); ++i) { l[i] = std::sin (0.05f * (float) i); r[i] = std::cos (0.03f * (float) i); }
    refL = l; refR = r;
    rig.run (l, r);
    bool same = true;
    for (size_t i = 0; i < l.size(); ++i)
        same = same && near (l[i], refL[i], 1e-6f) && near (r[i], refR[i], 1e-6f);
    CHECK (same);
}

void testFeedbackDecay()
{
    Rig rig;
    rig.set (0, 0.1f, 0.5f, 0.0f, 1.0f, 0.5f);
    auto l = impulse (20000), r = impulse (20000);
    rig.run (l, r);
    const float first = l[4800], second = l[9600], third = l[14400];
    CHECK (first > 0.2f);
    CHECK (near (second / first, 0.5f, 0.02f));
    CHECK (near (third / second, 0.5f, 0.02f));
}

void testDamp()
{
    Rig rig;
    rig.set (0, 0.1f, 0.0f, 0.9f, 1.0f, 0.5f);
    auto l = impulse (6000), r = impulse (6000);
    rig.run (l, r);
    // first echo sample passes y = x * (1 - damp) = 0.1 -> 0.25 * 0.1
    CHECK (near (l[4800], 0.025f, 0.002f));
    // the one-pole smears the echo: energy continues after the first sample
    CHECK (near (l[4801], 0.025f * 0.9f, 0.002f));
    float sum = 0.0f;
    for (int i = 4800; i < 6000; ++i) sum += l[(size_t) i];
    CHECK (near (sum, 0.25f, 0.01f));   // unity DC gain, so the total stays the same
}

void testPan()
{
    for (float pan : { 0.0f, 1.0f })
    {
        Rig rig;
        rig.set (0, 0.1f, 0.0f, 0.0f, 1.0f, pan);
        auto l = impulse (6000), r = impulse (6000);
        rig.run (l, r);
        const float echoL = std::fabs (l[4800]), echoR = std::fabs (r[4800]);
        if (pan == 0.0f) { CHECK (echoL > 0.4f); CHECK (echoR < 1e-5f); }
        else             { CHECK (echoR > 0.4f); CHECK (echoL < 1e-5f); }
    }
}

void testInputSpread()
{
    // tap 1 takes 25 % of the left input channel and 75 % of the right one
    Rig rig;
    rig.set (0, 0.1f, 0.0f, 0.0f, 1.0f, 0.5f);
    std::vector<float> l (6000, 0.0f), r (6000, 0.0f);
    l[0] = 1.0f;
    rig.run (l, r);
    CHECK (near (l[4800], 0.25f * 0.25f, 0.003f));

    Rig rig2;
    rig2.set (0, 0.1f, 0.0f, 0.0f, 1.0f, 0.5f);
    std::vector<float> l2 (6000, 0.0f), r2 (6000, 0.0f);
    r2[0] = 1.0f;
    rig2.run (l2, r2);
    CHECK (near (l2[4800], 0.25f * 0.75f, 0.003f));
}

void testSyncMaths()
{
    CHECK (near (quadrilo::syncDelaySeconds (8, 120.0), 0.5f, 1e-6f));       // 1/4 at 120 BPM
    CHECK (near (quadrilo::syncDelaySeconds (13, 120.0), 2.0f, 1e-6f));      // 1/1
    CHECK (near (quadrilo::syncDelaySeconds (0, 120.0), 0.0625f, 1e-6f));    // 1/32
    CHECK (near (quadrilo::syncDelaySeconds (10, 60.0), 1.5f, 1e-6f));       // dotted 1/4 at 60 BPM
    CHECK (near (quadrilo::syncDelaySeconds (-5, 120.0), quadrilo::syncDelaySeconds (0, 120.0), 1e-9f));
    CHECK (near (quadrilo::syncDelaySeconds (99, 120.0), quadrilo::syncDelaySeconds (13, 120.0), 1e-9f));
    for (size_t i = 1; i < quadrilo::noteBeats.size(); ++i)
        CHECK (quadrilo::noteBeats[i] > quadrilo::noteBeats[i - 1]);
}

void testDelayIsClamped()
{
    Rig rig;
    rig.set (0, 10.0f, 0.0f, 0.0f, 1.0f, 0.5f);   // far beyond the 2 s line
    auto l = impulse (110000), r = impulse (110000);
    rig.run (l, r);
    const int idx = peakIndex (l, 1);
    CHECK (idx > 90000 && idx < 96100);
}

void testStability()
{
    Rig rig;
    for (int i = 0; i < quadrilo::numTaps; ++i)
        rig.set (i, 0.013f * (float) (i + 1), 0.99f, 0.99f, 1.0f, (float) i / 3.0f);
    rig.engine.setMix (1.0f);
    rig.engine.snap();

    unsigned seed = 12345;
    bool finite = true;
    float peak = 0.0f;
    for (int block = 0; block < 200; ++block)
    {
        std::vector<float> l (480), r (480);
        for (size_t i = 0; i < l.size(); ++i)
        {
            seed = seed * 1664525u + 1013904223u;
            l[i] = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.5f;
            seed = seed * 1664525u + 1013904223u;
            r[i] = ((float) (seed >> 8) / 8388608.0f - 1.0f) * 0.5f;
        }
        rig.run (l, r);
        for (size_t i = 0; i < l.size(); ++i)
        {
            finite = finite && std::isfinite (l[i]) && std::isfinite (r[i]);
            peak = std::max (peak, std::max (std::fabs (l[i]), std::fabs (r[i])));
        }
        // sweep the delay times while running
        rig.set (block % 4, 0.01f + 0.005f * (float) (block % 37), 0.99f, 0.5f, 1.0f, 0.5f);
    }
    CHECK (finite);
    CHECK (peak < 50.0f);
}

void testResetClearsTails()
{
    Rig rig;
    rig.set (0, 0.1f, 0.9f, 0.0f, 1.0f, 0.5f);
    auto l = impulse (6000), r = impulse (6000);
    rig.run (l, r);
    rig.engine.reset();
    std::vector<float> zl (12000, 0.0f), zr (12000, 0.0f);
    rig.run (zl, zr);
    bool silent = true;
    for (float v : zl) silent = silent && std::fabs (v) < 1e-9f;
    CHECK (silent);
}
}

int main()
{
    testEchoTimingAndLevel();
    testNoDryAtFullWet();
    testMixZeroIsTransparent();
    testFeedbackDecay();
    testDamp();
    testPan();
    testInputSpread();
    testSyncMaths();
    testDelayIsClamped();
    testStability();
    testResetClearsTails();

    if (failures == 0)
        std::printf ("All delay engine tests passed.\n");
    else
        std::printf ("%d check(s) failed.\n", failures);
    return failures;
}
