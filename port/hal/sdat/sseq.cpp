// SSEQ bytecode sequencer.
//
// 16 players (the DS has 16 too), 16 tracks each, driven at 192 Hz by
// sd_mix_render. Tempo is BPM against 48 ticks per quarter note: the tick
// counter takes +tempo per 192 Hz frame and fires a tick at 240, which is
// 192*tempo/240 = tempo*48/60 ticks per second. That is the DS's timing.
//
// SM64DS's sound effects are sequences too -- SDAT has no "raw sample"
// concept -- so this file is on the critical path for SFX, not just music.
//
// Unimplemented opcodes print once by id and end the track that hit one.
// Ending is deliberate: an unknown opcode has an unknown argument length, so
// skipping it would desync the stream and play garbage. Silence that says
// why beats noise that lies.
#include "sdat.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace {

enum { SD_PLAYERS = 16, SD_TRACKS = 16, SD_STACK = 4, SD_VARS = 32 };

struct Track {
    int active;
    sd_u32 pc;
    sd_u32 stack[SD_STACK];
    int sp;
    sd_u32 loopPc[SD_STACK];
    int loopCount[SD_STACK];
    int loopSp;

    int wait;               // ticks still to burn before the next command
    int prog;               // program (instrument) number
    int volume, expression, pan, panSet;
    int transpose, bend, bendRange;
    int priority;
    int noteWait;           // C7: notes block the track for their duration
    int tie;
    int cmpFlag;
    int mono;
};

struct Player {
    int active;
    const sd_u8 *seq;       // bytecode base (SSEQ base + *(u32*)(base+0x18))
    const sd_u8 *sbnk;
    Track tr[SD_TRACKS];
    int tempo, tempoCount;
    int volume, pan;
    sd_s16 var[SD_VARS];
};

Player g_pl[SD_PLAYERS];

// One live note per mixer channel.
struct NoteSlot {
    int active;
    int player, track;
    int ticks;              // -1 = tied, released explicitly
};
NoteSlot g_note[SD_CHANNELS];

unsigned g_rand = 0x12345678u;
int rnd(int lo, int hi)
{
    g_rand = g_rand * 1103515245u + 12345u;
    int span = hi - lo + 1;
    if (span <= 0) return lo;
    return lo + (int)((g_rand >> 16) % (unsigned)span);
}

inline sd_u8 rd8(const sd_u8 *p) { return *p; }
inline sd_s16 rd16s(const sd_u8 *p) { return (sd_s16)(p[0] | (p[1] << 8)); }
inline sd_u32 rd24(const sd_u8 *p)
{ return (sd_u32)p[0] | ((sd_u32)p[1] << 8) | ((sd_u32)p[2] << 16); }

sd_u32 read_varlen(const sd_u8 *seq, sd_u32 &pc)
{
    sd_u32 v = 0;
    for (int i = 0; i < 4; i++) {
        sd_u8 b = seq[pc++];
        v = (v << 7) | (b & 0x7f);
        if (!(b & 0x80)) break;
    }
    return v;
}

void note_off(int ch, int release)
{
    if (ch < 0 || ch >= SD_CHANNELS) return;
    g_note[ch].active = 0;
    if (release) sd_mix_release(ch); else sd_mix_kill(ch);
}

// Kill everything a player owns.
void player_silence(int p)
{
    for (int i = 0; i < SD_CHANNELS; i++)
        if (g_note[i].active && g_note[i].player == p) note_off(i, 1);
}

void start_note(Player &pl, int pi, int ti, Track &tk, int note, int vel,
                int ticks)
{
    if (!pl.sbnk) return;
    SdatNote n;
    const sd_u8 *swar = 0;
    int key = note + tk.transpose;
    if (key < 0) key = 0;
    if (key > 127) key = 127;
    if (!sdat_bank_note(pl.sbnk, tk.prog, key, &n, &swar)) return;

    SdatWave w;
    if (!sdat_swar_wave(swar, n.swav, &w)) return;

    int ch = sd_mix_alloc(tk.priority);
    if (ch < 0) return;

    int db10 = sd_cnv_vol(vel) + sd_cnv_vol(tk.volume)
             + sd_cnv_vol(tk.expression) + sd_cnv_vol(pl.volume);
    if (db10 < -723) db10 = -723;

    int pan = tk.panSet ? tk.pan : n.pan;
    // The player's own pan biases the track's, centred at 64.
    pan += (pl.pan - 64);
    if (pan < 0) pan = 0;
    if (pan > 127) pan = 127;

    double semis = (double)(key - n.baseNote)
                 + (double)tk.bend * tk.bendRange / 64.0;
    double rate = (double)w.sampleRate * pow(2.0, semis / 12.0) / SD_MIX_RATE;
    if (rate <= 0.0 || rate > 64.0) return;

    sd_mix_start(ch, &w, &n, db10, pan, rate, tk.priority);
    g_note[ch].active = 1;
    g_note[ch].player = pi;
    g_note[ch].track = ti;
    g_note[ch].ticks = tk.tie ? -1 : ticks;
}

// Execute one track until it must wait. Returns 0 if the track ended.
int run_track(Player &pl, int pi, int ti)
{
    Track &tk = pl.tr[ti];
    const sd_u8 *s = pl.seq;

    for (int guard = 0; guard < 4096; guard++) {
        if (!tk.active) return 0;
        if (tk.wait > 0) return 1;

        // Prefix state for 0xA0/0xA1/0xA2.
        int useRandom = 0, useVar = 0, condition = 1;
        sd_u8 op;
        for (;;) {
            op = s[tk.pc++];
            if (op == 0xa0) { useRandom = 1; continue; }
            if (op == 0xa1) { useVar = 1; continue; }
            if (op == 0xa2) { condition = tk.cmpFlag; continue; }
            break;
        }

        // Helper lambdas read the "last argument" honouring the prefixes.
        auto argU8 = [&](void) -> int {
            if (useVar) { int v = pl.var[s[tk.pc++] & (SD_VARS - 1)]; return v; }
            if (useRandom) {
                int lo = rd16s(s + tk.pc); tk.pc += 2;
                int hi = rd16s(s + tk.pc); tk.pc += 2;
                return rnd(lo, hi);
            }
            return s[tk.pc++];
        };
        auto argS16 = [&](void) -> int {
            if (useVar) { int v = pl.var[s[tk.pc++] & (SD_VARS - 1)]; return v; }
            if (useRandom) {
                int lo = rd16s(s + tk.pc); tk.pc += 2;
                int hi = rd16s(s + tk.pc); tk.pc += 2;
                return rnd(lo, hi);
            }
            int v = rd16s(s + tk.pc); tk.pc += 2; return v;
        };

        if (op < 0x80) {                        // note on
            int vel = s[tk.pc++];
            sd_u32 dur = useVar ? (sd_u32)pl.var[s[tk.pc++] & (SD_VARS - 1)]
                                : read_varlen(s, tk.pc);
            if (condition) start_note(pl, pi, ti, tk, op, vel, (int)dur);
            if (condition && tk.noteWait) tk.wait = (int)dur;
            continue;
        }

        switch (op) {
        case 0x80: {                            // rest
            sd_u32 d = useVar ? (sd_u32)pl.var[s[tk.pc++] & (SD_VARS - 1)]
                              : read_varlen(s, tk.pc);
            if (condition) tk.wait = (int)d;
            break;
        }
        case 0x81: {                            // program / bank change
            sd_u32 v = useVar ? (sd_u32)pl.var[s[tk.pc++] & (SD_VARS - 1)]
                              : read_varlen(s, tk.pc);
            if (condition) tk.prog = (int)(v & 0x7f);
            break;
        }
        case 0x93: {                            // open track
            int n = s[tk.pc++];
            sd_u32 off = rd24(s + tk.pc); tk.pc += 3;
            if (condition && n > 0 && n < SD_TRACKS && !pl.tr[n].active) {
                Track &t2 = pl.tr[n];
                int keepActive = 1;
                memset(&t2, 0, sizeof t2);
                t2.active = keepActive;
                t2.pc = off;
                t2.volume = 127; t2.expression = 127; t2.pan = 64;
                t2.bendRange = 2; t2.priority = 64; t2.noteWait = 1;
                t2.prog = 0;
            }
            break;
        }
        case 0x94:                              // jump
            { sd_u32 off = rd24(s + tk.pc); tk.pc += 3;
              if (condition) tk.pc = off; }
            break;
        case 0x95:                              // call
            { sd_u32 off = rd24(s + tk.pc); tk.pc += 3;
              if (condition && tk.sp < SD_STACK) {
                  tk.stack[tk.sp++] = tk.pc; tk.pc = off;
              } }
            break;
        case 0xfd:                              // return
            if (condition && tk.sp > 0) tk.pc = tk.stack[--tk.sp];
            break;
        case 0xd4:                              // loop start
            { int n = argU8();
              if (condition && tk.loopSp < SD_STACK) {
                  tk.loopPc[tk.loopSp] = tk.pc;
                  tk.loopCount[tk.loopSp] = n;   // 0 == infinite
                  tk.loopSp++;
              } }
            break;
        case 0xfc:                              // loop end
            if (condition && tk.loopSp > 0) {
                int i = tk.loopSp - 1;
                if (tk.loopCount[i] == 0) { tk.pc = tk.loopPc[i]; }
                else if (--tk.loopCount[i] > 0) { tk.pc = tk.loopPc[i]; }
                else tk.loopSp--;
            }
            break;
        case 0xff:                              // end of track
            tk.active = 0;
            return 0;
        case 0xfe:                              // alloc tracks (bitmask)
            tk.pc += 2;
            break;

        case 0xc0: { int v = argU8(); if (condition) { tk.pan = v; tk.panSet = 1; } break; }
        case 0xc1: { int v = argU8(); if (condition) tk.volume = v; break; }
        case 0xc2: { int v = argU8(); if (condition) pl.volume = v; break; }
        case 0xc3: { int v = (sd_s8)argU8(); if (condition) tk.transpose = v; break; }
        case 0xc4: { int v = argU8(); if (condition) tk.bend = (sd_s8)v; break; }
        case 0xc5: { int v = argU8(); if (condition) tk.bendRange = v; break; }
        case 0xc6: { int v = argU8(); if (condition) tk.priority = v; break; }
        case 0xc7: { int v = argU8(); if (condition) tk.noteWait = v; break; }
        case 0xc8: { int v = argU8(); if (condition) tk.tie = v; break; }
        case 0xd5: { int v = argU8(); if (condition) tk.expression = v; break; }

        // Accepted and parsed, but not rendered: portamento, modulation and
        // the per-track ADSR override. Argument lengths are correct so the
        // stream stays in sync; the effect is simply not applied yet.
        case 0xc9: case 0xca: case 0xcb: case 0xcc: case 0xcd:
        case 0xce: case 0xcf: case 0xd0: case 0xd1: case 0xd2:
        case 0xd3: case 0xd6:
            argU8();
            break;
        case 0xe0:                              // modulation delay
        case 0xe3:                              // sweep pitch
            argS16();
            break;
        case 0xe1:                              // tempo
            { int v = argS16(); if (condition && v > 0) pl.tempo = v; }
            break;

        default:
            if (op >= 0xb0 && op <= 0xbd) {     // variable ops
                int vn = s[tk.pc++] & (SD_VARS - 1);
                int v = argS16();
                sd_s16 &V = pl.var[vn];
                if (condition) switch (op) {
                case 0xb0: V = (sd_s16)v; break;
                case 0xb1: V = (sd_s16)(V + v); break;
                case 0xb2: V = (sd_s16)(V - v); break;
                case 0xb3: V = (sd_s16)(V * v); break;
                case 0xb4: if (v) V = (sd_s16)(V / v); break;
                case 0xb5: V = (sd_s16)(v >= 0 ? (V << v) : (V >> -v)); break;
                case 0xb6: V = (sd_s16)rnd(v < 0 ? v : 0, v < 0 ? 0 : v); break;
                case 0xb8: tk.cmpFlag = (V == v); break;
                case 0xb9: tk.cmpFlag = (V >= v); break;
                case 0xba: tk.cmpFlag = (V >  v); break;
                case 0xbb: tk.cmpFlag = (V <= v); break;
                case 0xbc: tk.cmpFlag = (V <  v); break;
                case 0xbd: tk.cmpFlag = (V != v); break;
                }
                break;
            }
            {
                static sd_u8 seen[256];
                if (!seen[op]) {
                    seen[op] = 1;
                    fprintf(stderr, "[sseq] unimplemented opcode 0x%02x "
                            "(player %d track %d, pc %u) -- track ended\n",
                            op, pi, ti, (unsigned)(tk.pc - 1));
                }
                tk.active = 0;
                return 0;
            }
        }
    }
    // A track that never yields is malformed; stop it rather than hang.
    fprintf(stderr, "[sseq] player %d track %d ran 4096 commands without a "
            "wait -- stopped\n", pi, ti);
    tk.active = 0;
    return 0;
}

}  // namespace

void sd_seq_reset(void)
{
    memset(g_pl, 0, sizeof g_pl);
    memset(g_note, 0, sizeof g_note);
}

int sd_seq_active(int p)
{
    if (p < 0 || p >= SD_PLAYERS) return 0;
    if (!g_pl[p].active) return 0;
    for (int i = 0; i < SD_TRACKS; i++)
        if (g_pl[p].tr[i].active) return 1;
    return 0;
}

int sd_seq_start(int p, const sd_u8 *seqData, const sd_u8 *sbnk)
{
    if (p < 0 || p >= SD_PLAYERS || !seqData) return 0;
    sd_seq_stop(p);

    Player &pl = g_pl[p];
    memset(&pl, 0, sizeof pl);
    pl.active = 1;
    pl.seq = seqData;
    pl.sbnk = sbnk;
    pl.tempo = 120;
    pl.tempoCount = 0;
    pl.volume = 127;
    pl.pan = 64;

    Track &t0 = pl.tr[0];
    t0.active = 1;
    t0.pc = 0;
    t0.volume = 127; t0.expression = 127; t0.pan = 64;
    t0.bendRange = 2; t0.priority = 64; t0.noteWait = 1;

    // A multi-track sequence opens with 0xFE <u16 mask>; track 0's own code
    // follows the 0x93 open-track commands, so nothing special is needed
    // here -- run_track walks them.
    return 1;
}

void sd_seq_stop(int p)
{
    if (p < 0 || p >= SD_PLAYERS) return;
    player_silence(p);
    memset(&g_pl[p], 0, sizeof g_pl[p]);
}

void sd_seq_set_volume(int p, int v)
{
    if (p < 0 || p >= SD_PLAYERS || !g_pl[p].active) return;
    g_pl[p].volume = v < 0 ? 0 : (v > 127 ? 127 : v);
}

void sd_seq_set_pan(int p, int v)
{
    if (p < 0 || p >= SD_PLAYERS || !g_pl[p].active) return;
    g_pl[p].pan = v < 0 ? 0 : (v > 127 ? 127 : v);
}

void sd_seq_frame(void)
{
    for (int p = 0; p < SD_PLAYERS; p++) {
        Player &pl = g_pl[p];
        if (!pl.active) continue;

        pl.tempoCount += pl.tempo;
        while (pl.tempoCount >= 240) {
            pl.tempoCount -= 240;

            // Note durations are in sequencer TICKS, so they expire here --
            // inside the tempo loop -- not once per 192 Hz frame.
            for (int i = 0; i < SD_CHANNELS; i++) {
                if (!g_note[i].active || g_note[i].player != p) continue;
                if (g_note[i].ticks < 0) continue;      // tied
                if (--g_note[i].ticks <= 0) note_off(i, 1);
            }

            int any = 0;
            for (int t = 0; t < SD_TRACKS; t++) {
                Track &tk = pl.tr[t];
                if (!tk.active) continue;
                if (tk.wait > 0) tk.wait--;
                if (tk.wait == 0) run_track(pl, p, t);
                if (tk.active) any = 1;
            }
            if (!any) {
                // Every track ended. Leave the player marked active until
                // its tails finish so a release is not cut short.
                int ringing = 0;
                for (int i = 0; i < SD_CHANNELS; i++)
                    if (g_note[i].active && g_note[i].player == p) ringing = 1;
                if (!ringing) { pl.active = 0; break; }
            }
        }
    }
}
