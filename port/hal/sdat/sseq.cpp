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
#include <stdlib.h>
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
    int volDb10;            // PLAYER_PARAM 6: distance/fade attenuation
    sd_s16 var[SD_VARS];
};

Player g_pl[SD_PLAYERS];

// One live note per mixer channel.
struct NoteSlot {
    int active;
    int player, track;
    int ticks;              // -1 = tied, released explicitly
    int basePan;            // pan before the player's own bias
    int baseDb10;           // volume before the player's own attenuation
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

void note_off(int ch, int release, const char *why)
{
    if (ch < 0 || ch >= SD_CHANNELS) return;
    g_note[ch].active = 0;
    if (release) sd_mix_release(ch, why); else sd_mix_kill(ch, why);
}

// Kill everything a player owns.
void player_silence(int p)
{
    for (int i = 0; i < SD_CHANNELS; i++)
        if (g_note[i].active && g_note[i].player == p)
            note_off(i, 1, "player stopped out from under the note");
}

// Kill what ONE TRACK owns: the abnormal-death path. Tied notes outlive
// their track on purpose (every SEQARC one-shot is "program, note, end"
// and rings past 0xff until its sample ends), so a track that dies from a
// desynced stream has to take its voices with it, or the garbage note it
// keyed on the way out rings forever -- which is what the loud static in
// the 2026-08-05 play session was.
void track_silence(int p, int t)
{
    for (int i = 0; i < SD_CHANNELS; i++)
        if (g_note[i].active && g_note[i].player == p && g_note[i].track == t)
            note_off(i, 1, "track died and took its voices with it");
}

void start_note(Player &pl, int pi, int ti, Track &tk, int note, int vel,
                int ticks)
{
    if (!pl.sbnk) {
        SD_VT("note p%d t%d key %d DROPPED: player has no bank\n", pi, ti,
              note);
        return;
    }
    SdatNote n;
    const sd_u8 *swar = 0;
    int key = note + tk.transpose;
    if (key < 0) key = 0;
    if (key > 127) key = 127;
    if (!sdat_bank_note(pl.sbnk, tk.prog, key, &n, &swar)) {
        SD_VT("note p%d t%d key %d DROPPED: program %d has no note there\n",
              pi, ti, key, tk.prog);
        return;
    }

    SdatWave w;
    if (!sdat_swar_wave(swar, n.swav, &w)) {
        SD_VT("note p%d t%d key %d DROPPED: wave %d unresolvable\n", pi, ti,
              key, n.swav);
        return;
    }

    int baseDb10 = sd_cnv_vol(vel) + sd_cnv_vol(tk.volume)
                 + sd_cnv_vol(tk.expression) + sd_cnv_vol(pl.volume);
    if (baseDb10 < -723) baseDb10 = -723;
    int db10 = baseDb10 + pl.volDb10;
    if (db10 < -723) db10 = -723;

    int basePan = tk.panSet ? tk.pan : n.pan;
    // The player's own pan biases the track's, centred at 64.
    int pan = basePan + (pl.pan - 64);
    if (pan < 0) pan = 0;
    if (pan > 127) pan = 127;

    /* /128, not /64. The DS pitch domain is 1/64 of a semitone (768 units to
       the octave) and a full-scale bend has to come out at exactly
       +-bendRange semitones. pitchBend is an s8, so full scale is +-128:

           units     = bend * bendRange * 64 / 128 = bend * bendRange / 2
           semitones = units / 64                  = bend * bendRange / 128

       which is the same bend * bendRange / 2 units NitroSDK computes. At /64
       every bend in the game was twice as deep as the DS plays it. The ground
       loop re-randomises its bend every iteration with bendRange 2, so
       walking warbled +-1.5 semitones instead of +-0.75. */
    double semis = (double)(key - n.baseNote)
                 + (double)tk.bend * tk.bendRange / 128.0;
    double rate = (double)w.sampleRate * pow(2.0, semis / 12.0) / SD_MIX_RATE;
    // Everything that can refuse the note is settled BEFORE a channel is
    // taken. Allocating first and then bailing on the rate left a stolen
    // channel dead with the previous note's owner still recorded against it,
    // which is a voice lost for nothing.
    if (rate <= 0.0 || rate > 64.0) {
        SD_VT("note p%d t%d key %d DROPPED: playback rate %.3f out of "
              "range\n", pi, ti, key, rate);
        return;
    }
    // The inputs the rate is made of, printed whenever the note lands more
    // than a semitone from the wave's own pitch. A sample that comes out low
    // and long is one of these being wrong, and which one cannot be read
    // back out of the rate alone.
    if (semis < -1.0 || semis > 1.0)
        SD_VT("note p%d t%d key %d: %+.2f semitones off base %d "
              "(transpose %d, bend %d, range %d) -> rate %.4f\n",
              pi, ti, key, semis, n.baseNote, tk.transpose, tk.bend,
              tk.bendRange, rate);

    int ch = sd_mix_alloc(tk.priority);
    if (ch < 0) {
        SD_VT("note p%d t%d key %d DROPPED: no mixer channel free\n", pi, ti,
              key);
        return;
    }
    if (g_note[ch].active)
        SD_VT("chan %2d taken from player %d track %d\n", ch,
              g_note[ch].player, g_note[ch].track);

    sd_mix_start(ch, &w, &n, db10, pan, rate, tk.priority);
    g_note[ch].active = 1;
    g_note[ch].player = pi;
    g_note[ch].track = ti;
    g_note[ch].basePan = basePan;
    g_note[ch].baseDb10 = baseDb10;
    // Duration 0 means "no scheduled note-off" -- the note runs until its
    // envelope or its sample ends. Every sound effect in the SEQARCs is
    // written that way (a lone "program change, note, end of track"), so
    // treating 0 as a one-tick note cut all of them to a click.
    g_note[ch].ticks = (tk.tie || ticks <= 0) ? -1 : ticks;
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
        // The VARLEN arguments (note duration, rest, program) take the
        // prefixes too. A 0xA0-prefixed rest stores lo,hi as two s16s where
        // the varlen would sit, and reading them as a varlen would walk the
        // stream off by the difference. Spec correctness, not a live bug: a
        // static walk of every SSEQ and SEQARC entry in the EU SDAT (calls
        // and prefixes modelled) found no 0xA0 in front of a varlen anywhere,
        // so the 2026-08-05 pc-6664 track death was NOT this -- see the
        // unimplemented-opcode handler below for where that hunt points.
        auto argVarlen = [&](void) -> sd_u32 {
            if (useVar) {
                int v = pl.var[s[tk.pc++] & (SD_VARS - 1)];
                return (sd_u32)(v < 0 ? 0 : v);
            }
            if (useRandom) {
                int lo = rd16s(s + tk.pc); tk.pc += 2;
                int hi = rd16s(s + tk.pc); tk.pc += 2;
                int v = rnd(lo, hi);
                return (sd_u32)(v < 0 ? 0 : v);
            }
            return read_varlen(s, tk.pc);
        };

        if (op < 0x80) {                        // note on
            int vel = s[tk.pc++];
            sd_u32 dur = argVarlen();
            if (condition) start_note(pl, pi, ti, tk, op, vel, (int)dur);
            if (condition && tk.noteWait) tk.wait = (int)dur;
            continue;
        }

        switch (op) {
        case 0x80: {                            // rest
            sd_u32 d = argVarlen();
            if (condition) tk.wait = (int)d;
            break;
        }
        case 0x81: {                            // program / bank change
            sd_u32 v = argVarlen();
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
        case 0xfd:                              // return, or end of track
            // A return with an EMPTY call stack ends the track, exactly like
            // 0xff. 27 of the EU SDAT's SEQARC entries end with 0xfd rather
            // than 0xff, and treating it as a no-op walked the pc off the
            // end of the entry into the neighbouring entries and then out of
            // the archive: garbage notes tying up mixer channels until only
            // high-priority voices could still play, then a track death at
            // the first invalid byte. That was the whole static-then-silence
            // arc of the 2026-08-05 evening session; all four of its logged
            // track deaths started at legit SEQARC entries.
            if (condition) {
                if (tk.sp > 0) tk.pc = tk.stack[--tk.sp];
                else { tk.active = 0; return 0; }
            }
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
                    /* Every stream in the EU SDAT parses clean end to end
                       under this dispatcher (statically walked, calls and
                       prefixes modelled), so landing here means the pc or
                       the seq POINTER went somewhere no stream reaches --
                       state corruption, not a missing opcode. Name the
                       stream so the playlog says which one died: the
                       sdat-relative offset identifies it. */
                    long rel = (pl.seq >= g_sdat.base &&
                                pl.seq < g_sdat.base + g_sdat.size)
                               ? (long)(pl.seq - g_sdat.base) : -1;
                    fprintf(stderr, "[sseq] unimplemented opcode 0x%02x "
                            "(player %d track %d, pc %u, seq %p = "
                            "sdat+0x%lx) -- track ended; no stream parses "
                            "here, suspect a stomped pc or seq pointer\n",
                            op, pi, ti, (unsigned)(tk.pc - 1),
                            (const void *)pl.seq, rel);
                }
                track_silence(pi, ti);
                tk.active = 0;
                return 0;
            }
        }
    }
    // A track that never yields is malformed; stop it rather than hang.
    fprintf(stderr, "[sseq] player %d track %d ran 4096 commands without a "
            "wait -- stopped\n", pi, ti);
    track_silence(pi, ti);
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

// SNDSharedWork.playerStatus: one bit per player, set while that player is
// still holding the sequence it was started with. `active` and not merely
// "a track is running" is the right test -- sd_seq_frame keeps the player
// marked active after its last track ends until the release tails finish,
// which is exactly the window the DS's own player occupies.
sd_u32 sd_seq_player_mask(void)
{
    sd_u32 m = 0;
    for (int p = 0; p < SD_PLAYERS; p++)
        if (g_pl[p].active) m |= 1u << p;
    return m;
}

// startOff is the entry's offset inside seqBase, and it stays SEPARATE from
// the base on purpose.
//
// Every pc in this player -- the initial one, every 0x94 jump, every 0x95
// call, every 0x93 open-track -- is an offset from seqBase. In a SEQARC that
// base is the archive's whole DATA block and the entry is somewhere inside
// it, so folding the entry offset into the base (seqBase = base + entry, pc
// = 0) reads correctly for exactly as long as the stream runs straight: the
// first branch then lands at base + entry + target instead of base + target,
// short by nothing on entry 0 and by the entry's own offset on every other
// one.
//
// What that sounds like is the second half of the sliding-effect report.
// SEQARC entry 67 at archive offset 0xd01 is a looping effect -- random
// detune, random volume, one note, rest, jump back to 0xd23 -- and folded,
// its jump landed 0xd01 further on, in the middle of another entry, on a
// byte that reads as note 0. Note 0 under that track's transpose of -24 is
// 60 semitones below the instrument's base note, so a 792-sample 16 kHz wave
// came out five octaves down and ran 1669 ms instead of 49:
//
//   [vt] note p2 t0 key 0: -60.91 semitones off base 60 (transpose -24,
//        bend -29, range 2) -> rate 0.0145
//   [vt] chan  0 start: 792 samples, -17 dB10, pan 64, prio 64,
//        rate 0.0145 (16000 Hz wave, 1669 ms)
//
// and a pc walking through data rather than code rarely meets an end-of-track
// with an empty stack, so the track does not die either. Pitched down,
// stretched out, and dragging on: one offset applied twice.
int sd_seq_start(int p, const sd_u8 *seqBase, sd_u32 startOff,
                 const sd_u8 *sbnk)
{
    if (p < 0 || p >= SD_PLAYERS || !seqBase) return 0;
    /* PORT_SSEQ_TRACE=1: every start with the stream's identity, so a later
       abnormal track death can be matched to what was actually started on
       that player. The 2026-08-05 session died at pc 6664 on a player whose
       stream could not be named after the fact. */
    {
        static int trace = -1;
        if (trace < 0) trace = getenv("PORT_SSEQ_TRACE") != 0;
        if (trace) {
            const sd_u8 *entry = seqBase + startOff;
            long rel = (entry >= g_sdat.base &&
                        entry < g_sdat.base + g_sdat.size)
                       ? (long)(entry - g_sdat.base) : -1;
            fprintf(stderr, "[sseq] start player %d seq %p + 0x%lx "
                    "(sdat+0x%lx)\n", p, (const void *)seqBase,
                    (unsigned long)startOff, rel);
        }
    }
    SD_VT("play %2d start\n", p);
    sd_seq_stop(p);

    Player &pl = g_pl[p];
    memset(&pl, 0, sizeof pl);
    pl.active = 1;
    pl.seq = seqBase;
    pl.sbnk = sbnk;
    pl.tempo = 120;
    pl.tempoCount = 0;
    pl.volume = 127;
    pl.pan = 64;

    Track &t0 = pl.tr[0];
    t0.active = 1;
    t0.pc = startOff;
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
    if (g_pl[p].active) SD_VT("play %2d stop\n", p);
    player_silence(p);
    memset(&g_pl[p], 0, sizeof g_pl[p]);
}

void sd_seq_set_volume(int p, int v)
{
    if (p < 0 || p >= SD_PLAYERS || !g_pl[p].active) return;
    g_pl[p].volume = v < 0 ? 0 : (v > 127 ? 127 : v);
}

// PLAYER_PARAM 6, the attenuation func_0204fafc recomputes every frame from
// the voice's distance and its fade ramp. It arrives WHILE the sound plays --
// that is the whole point of it -- so it reaches the notes already sounding,
// the same way the positional pan does below.
void sd_seq_set_volume_db10(int p, int db10)
{
    if (p < 0 || p >= SD_PLAYERS || !g_pl[p].active) return;
    if (db10 > 0) db10 = 0;
    if (db10 < -723) db10 = -723;
    g_pl[p].volDb10 = db10;
    for (int i = 0; i < SD_CHANNELS; i++) {
        if (!g_note[i].active || g_note[i].player != p) continue;
        sd_mix_set_vol(i, g_note[i].baseDb10 + db10);
    }
}

void sd_seq_set_pan(int p, int v)
{
    if (p < 0 || p >= SD_PLAYERS || !g_pl[p].active) return;
    g_pl[p].pan = v < 0 ? 0 : (v > 127 ? 127 : v);
    // Positional pan arrives AFTER the note that needs it: Sound::Play calls
    // Player_PlaySoundEffect (which sends START) and only then func_02048d80
    // (which sends the pan). Applying it to the player alone would leave the
    // sound it was computed for playing dead centre, so retune the voices
    // this player already has ringing.
    for (int i = 0; i < SD_CHANNELS; i++) {
        if (!g_note[i].active || g_note[i].player != p) continue;
        int pan = g_note[i].basePan + (g_pl[p].pan - 64);
        sd_mix_set_pan(i, pan < 0 ? 0 : (pan > 127 ? 127 : pan));
    }
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
                if (--g_note[i].ticks <= 0)
                    note_off(i, 1, "note duration expired");
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
                if (!ringing) {
                    SD_VT("play %2d finished: sequence ended and its tails "
                          "are silent\n", p);
                    pl.active = 0;
                    break;
                }
            }
        }
    }
}
