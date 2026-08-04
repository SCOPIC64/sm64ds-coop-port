// 16 DS-style mixer channels.
//
// The envelope lives in the DS's own domain: a s32 amplitude in units of
// 1/128 of a tenth of a decibel, running from -92544 (= -723 * 128, i.e.
// -72.3 dB, the DS's silence floor) up to 0. Attack is multiplicative in
// that log domain, decay and release are linear in it, and the rate
// conversion Cnv_Fall below is the ARM7's. The one place this departs from
// hardware is the 0..127 -> decibel table (see sd_cnv_vol in sdat.cpp): the
// ROM's table is in an ARM7 binary nobody has decompiled, so a log curve
// stands in for it.
//
// Resampling is linear interpolation. The DS does the same thing in
// hardware, so this is not a shortcut.
//
// This file also owns the 192 Hz sequencer clock: sd_mix_render slices its
// output at 192 Hz boundaries and ticks the sequencer and every envelope
// there. Driving the sequencer off the AUDIO clock rather than the video
// frame is what the ARM7 does, and it keeps tempo correct even when the
// window's frame rate is not 60.
#include "sdat.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

enum EnvState { ENV_OFF = 0, ENV_ATTACK, ENV_DECAY, ENV_SUSTAIN, ENV_RELEASE };

const sd_s32 AMPL_MIN = -723 * 128;     // -72.3 dB, in 1/128 of 0.1 dB

struct Channel {
    int active;
    const sd_s16 *pcm;
    sd_u32 total, loopStart;
    int loop;
    double pos, step;

    sd_s32 ampl;
    int state;
    int attackCoef, decayRate, sustainLevel, releaseRate;

    int volDb10;        // combined external volume, tenths of a dB (<= 0)
    int pan;            // 0..127
    int priority;
    unsigned seq;       // bumped on every start, so stale handles are inert
};

Channel g_ch[SD_CHANNELS];

// Frames of 32768 Hz output per 192 Hz sequencer tick, as a fraction.
int g_tickAcc;          // counts output frames * 192

int cnv_attack(int a)
{
    static const sd_u8 lut[19] = {
        0, 1, 5, 14, 26, 38, 51, 63, 73, 84, 92, 100, 109, 116, 123, 127,
        132, 137, 143
    };
    if (a >= 0x6d) return lut[0x7f - a];
    return 255 - a;
}

int cnv_fall(int f)
{
    if (f == 0x7f) return 0xffff;       // instant
    if (f == 0x7e) return 0x3c00;
    if (f < 0x32) return f * 2 + 1;
    return 0x1e00 / (0x7e - f);
}

double db10_to_gain(int db10)
{
    if (db10 <= -723) return 0.0;
    if (db10 > 0) db10 = 0;
    return pow(10.0, db10 / 200.0);
}

}  // namespace

void sd_mix_reset(void)
{
    memset(g_ch, 0, sizeof g_ch);
    g_tickAcc = 0;
}

int sd_mix_alloc(int priority)
{
    for (int i = 0; i < SD_CHANNELS; i++)
        if (!g_ch[i].active) return i;
    // Steal the lowest-priority voice, oldest first among equals.
    int best = -1;
    for (int i = 0; i < SD_CHANNELS; i++) {
        if (g_ch[i].priority > priority) continue;
        if (best < 0 || g_ch[i].priority < g_ch[best].priority) best = i;
    }
    if (best >= 0) g_ch[best].active = 0;
    return best;
}

void sd_mix_start(int ch, const SdatWave *w, const SdatNote *n,
                  int volume_db10, int pan, double rate, int priority)
{
    if (ch < 0 || ch >= SD_CHANNELS || !w || !w->pcm || !w->totalSamples) return;
    Channel &c = g_ch[ch];
    unsigned s = c.seq + 1;
    memset(&c, 0, sizeof c);
    c.seq = s;
    c.active = 1;
    c.pcm = w->pcm;
    c.total = w->totalSamples;
    c.loop = w->loop;
    c.loopStart = w->loopStart;
    c.pos = 0.0;
    c.step = rate;
    c.volDb10 = volume_db10;
    c.pan = pan < 0 ? 0 : (pan > 127 ? 127 : pan);
    c.priority = priority;

    c.ampl = AMPL_MIN;
    c.state = ENV_ATTACK;
    c.attackCoef  = cnv_attack(n ? n->attack : 127);
    c.decayRate   = cnv_fall(n ? n->decay : 127);
    c.releaseRate = cnv_fall(n ? n->release : 127);
    c.sustainLevel = sd_cnv_vol(n ? n->sustain : 127) * 128;
}

void sd_mix_set(int ch, int volume_db10, int pan, double rate)
{
    if (ch < 0 || ch >= SD_CHANNELS || !g_ch[ch].active) return;
    g_ch[ch].volDb10 = volume_db10;
    g_ch[ch].pan = pan < 0 ? 0 : (pan > 127 ? 127 : pan);
    if (rate > 0.0) g_ch[ch].step = rate;
}

void sd_mix_set_pan(int ch, int pan)
{
    if (ch < 0 || ch >= SD_CHANNELS || !g_ch[ch].active) return;
    g_ch[ch].pan = pan < 0 ? 0 : (pan > 127 ? 127 : pan);
}

void sd_mix_release(int ch)
{
    if (ch < 0 || ch >= SD_CHANNELS || !g_ch[ch].active) return;
    g_ch[ch].state = ENV_RELEASE;
}

void sd_mix_kill(int ch)
{
    if (ch < 0 || ch >= SD_CHANNELS) return;
    g_ch[ch].active = 0;
    g_ch[ch].state = ENV_OFF;
}

int sd_mix_active(int ch)
{
    return (ch >= 0 && ch < SD_CHANNELS) ? g_ch[ch].active : 0;
}

void sd_mix_frame(void)
{
    for (int i = 0; i < SD_CHANNELS; i++) {
        Channel &c = g_ch[i];
        if (!c.active) continue;
        switch (c.state) {
        case ENV_ATTACK:
            // ampl is negative and climbs toward 0 multiplicatively.
            c.ampl = (sd_s32)(((long long)c.attackCoef * c.ampl) / 255);
            if (c.ampl >= 0) { c.ampl = 0; c.state = ENV_DECAY; }
            break;
        case ENV_DECAY:
            c.ampl -= c.decayRate;
            if (c.ampl <= c.sustainLevel) {
                c.ampl = c.sustainLevel;
                c.state = ENV_SUSTAIN;
            }
            break;
        case ENV_SUSTAIN:
            break;
        case ENV_RELEASE:
            c.ampl -= c.releaseRate;
            if (c.ampl <= AMPL_MIN) { c.active = 0; c.state = ENV_OFF; }
            break;
        default:
            break;
        }
    }
}

void sd_seq_frame(void);   // forward: the sequencer shares this clock

void sd_mix_render(sd_s16 *dst, int frames)
{
    memset(dst, 0, (size_t)frames * 2 * sizeof(sd_s16));
    int done = 0;
    while (done < frames) {
        // Frames remaining before the next 192 Hz boundary.
        int need = (SD_MIX_RATE - g_tickAcc + 191) / 192;
        if (need <= 0) need = 1;
        int n = frames - done;
        if (n > need) n = need;

        for (int i = 0; i < SD_CHANNELS; i++) {
            Channel &c = g_ch[i];
            if (!c.active || !c.pcm) continue;
            int envDb10 = c.ampl / 128;
            int total = envDb10 + c.volDb10;
            if (total < -723) total = -723;
            double g = db10_to_gain(total);
            if (g <= 0.0) continue;
            double gl = g * (127 - c.pan) / 127.0;
            double gr = g * c.pan / 127.0;

            sd_s16 *o = dst + (done * 2);
            for (int k = 0; k < n; k++, o += 2) {
                sd_u32 idx = (sd_u32)c.pos;
                if (idx >= c.total) {
                    if (c.loop && c.total > c.loopStart) {
                        double span = (double)(c.total - c.loopStart);
                        c.pos = c.loopStart + fmod(c.pos - c.loopStart, span);
                        idx = (sd_u32)c.pos;
                    } else {
                        c.active = 0;
                        break;
                    }
                }
                double frac = c.pos - (double)idx;
                sd_s32 a = c.pcm[idx];
                sd_s32 b = (idx + 1 < c.total) ? c.pcm[idx + 1]
                         : (c.loop ? c.pcm[c.loopStart] : a);
                double s = a + (b - a) * frac;
                sd_s32 l = (sd_s32)(o[0] + s * gl);
                sd_s32 r = (sd_s32)(o[1] + s * gr);
                o[0] = (sd_s16)(l < -32768 ? -32768 : (l > 32767 ? 32767 : l));
                o[1] = (sd_s16)(r < -32768 ? -32768 : (r > 32767 ? 32767 : r));
                c.pos += c.step;
            }
        }

        done += n;
        g_tickAcc += n * 192;
        while (g_tickAcc >= SD_MIX_RATE) {
            g_tickAcc -= SD_MIX_RATE;
            sd_seq_frame();
            sd_mix_frame();
        }
    }
}
