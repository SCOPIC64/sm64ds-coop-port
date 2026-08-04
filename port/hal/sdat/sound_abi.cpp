// ARM argument ride-throughs on the sound path.
//
// Five functions in the sound stack are declared in src/ with fewer
// parameters than their callers pass. That is not a decomp error -- on ARM
// the extra arguments sit in r1..r3 across a call that never names them, and
// the callee's own call passes them straight on. mwccarm reproduces the ROM
// bytes exactly that way, so the src stays as it is and the host spells the
// arguments out instead. Same pattern as the SharedFilePtr::Construct
// veneers in hal/cxx_aliases.cpp and the gate-14 ride-throughs in
// port/unmatched/.
//
// Each of these src files is filtered out of SLICE10_CAM_SOURCES in
// port/CMakeLists.txt; the bodies below are the same code with the riders
// named.
//
//   func_0204f600  (1 -> 4)  START: the sequence pointer, entry offset and
//                            bank all ride into Snd_SendCommand(0, ...).
//   func_0204f89c  (1 -> 2)  volume rides into Snd_SendCommand(3, ...).
//   func_0204f7cc  (1 -> 3)  pan mode + value ride into Snd_SendCommand(4).
//   func_0204f86c  (1 -> 3)  two riders into Snd_SendCommand(5, ...).
//   func_0204fa2c  (1 -> 2)  the fade length rides into func_0204f5a0.
#include "sdat.h"

#include <stdio.h>

typedef unsigned char u8;

extern "C" {

void func_0205adc4(void *a, int b, int c, int d);
void func_0205ad24(int a, int b);
int  func_0205acac(int a, int b, int c);
void func_0205aaf4(void *a, int b, int c);
void func_0204f4bc(void *obj);
void *func_0205afb4(void);
void func_0204f5a0(u8 *thiz, int arg1);

// func_0204f600(thiz) on ARM; r1 = sequence data, r2 = entry offset,
// r3 = resident bank, all of which ride into func_0205adc4.
int func_0204f600(void *thiz, int seqData, int entryOff, int bank)
{
    func_0205adc4((void *)(size_t)(unsigned)*(u8 *)((char *)thiz + 0x3c),
                  seqData, entryOff, bank);
    func_0204f4bc(thiz);
    *(void **)((char *)thiz + 0x30) = func_0205afb4();
    *(u8 *)((char *)thiz + 0x2c) = 1;
    return 1;
}

// func_0204f89c(c) on ARM; r1 = volume, riding into func_0205ad24's second
// parameter (which the src file's own declaration does not name either).
void func_0204f89c(char **c, int volume)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205ad24(p[0x3c], volume);
}

// func_0204f7cc(c) on ARM; r1/r2 = the pan mode and value.
void func_0204f7cc(char **c, int mode, int pan)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205acac(p[0x3c], mode, pan);
}

// func_0204f86c(c) on ARM; r1/r2 ride into func_0205aaf4.
void func_0204f86c(char **c, int b, int d)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205aaf4((void *)(size_t)(unsigned)p[0x3c], b, d);
}

// func_0204fa2c(p) on ARM; r1 = the fade length in frames, riding into
// func_0204f5a0's second parameter. 0 means stop now, non-zero ramps down.
int func_0204fa2c(int *p, int fade)
{
    func_0204f5a0((u8 *)(size_t)(unsigned)*p, fade);
    return 0;
}

// Sound::Play. Its src declares the resolver as func_02050cdc(void) and
// calls it with no arguments -- the kind and id are already in r0/r1 from
// Play's own frame, so on ARM they ride straight through. On the host the
// callee read whatever happened to be in the argument slots, returned 0, and
// the very next line (*(u8 *)(s + 5)) faulted on address 5. That was the
// first crash the unstubbed front door produced.
//
// The null check is a host addition, not the ROM's behaviour: func_02050cdc
// legitimately returns 0 for an id whose group was never loaded, and on
// hardware the game is never in that state. Printing once and returning is
// the honest answer instead of reproducing a crash the DS would not have.
struct Vector3 { int x, y, z; };
void *func_02050cdc(int kind, int idx);
void *func_02048720(struct Vector3 *v, int kind, int id);
void  func_02048908(void *obj, int *p);
int   func_02048a1c(int *v, int kind, int id);
void  func_02048d80(void *obj, int *p);
void  Player_PlaySoundEffect(int x, unsigned a, unsigned b);
extern int data_0209b4a4[];

void _ZN5Sound4PlayEjjRK7Vector3(unsigned kind, unsigned id, struct Vector3 *v)
{
    // Self-initialise. Not every harness has a frame loop calling
    // sdat_host_tick -- smoke_player reaches Player::Behavior -> Sound::Play
    // directly -- and the table walkers below read data_020a5bb8 + 0x84
    // unconditionally, so an unseated root is a null dereference rather than
    // a quiet no-op. Idempotent and cheap after the first call.
    sd_consumer_init();
    unsigned char *s = (unsigned char *)func_02050cdc((int)kind, (int)id);
    if (s == 0) {
        static unsigned char seen[8][32];
        unsigned k = kind & 7, b = (id >> 3) & 31, m = 1u << (id & 7);
        if (!(seen[k][b] & m)) {
            seen[k][b] |= (unsigned char)m;
            fprintf(stderr, "[snd] Sound::Play(%u, %u): no SEQARC entry "
                    "(group not loaded?) -- skipped\n", kind, id);
        }
        return;
    }
    int t = s[5];
    if (t == 9 || t == 2) {
        void *r = func_02048720(v, (int)kind, (int)id);
        if (r == 0) return;
        Player_PlaySoundEffect((int)(size_t)r, kind, id);
        func_02048908(r, (int *)v);
        return;
    }
    if (func_02048a1c((int *)v, (int)kind, (int)id) == 0) return;
    Player_PlaySoundEffect((int)(size_t)data_0209b4a4, kind, id);
    func_02048d80(data_0209b4a4, (int *)v);
}

// Sound::Player::SetPlayableSeqCount. Not a ride-through -- an ALIAS. The
// src writes *(u32 *)(data_020a4d84 + id * 0x1c), and on the DS
// data_020a4d84 is data_020a4d6c + 0x18, i.e. field +0x18 of the same
// 32-entry player array. Host symbols are separate objects, so the src
// version would drop the write into a different block from the one
// func_0204f63c reads it back out of -- and func_0204f63c uses that field as
// "how many sequences may this player run", so a lost write means it thinks
// the limit is 0 and evicts a voice on every single sound. Writing through
// data_020a4d6c keeps the two views aliased.
extern unsigned char data_020a4d6c[];
void _ZN5Sound6Player19SetPlayableSeqCountEii(int playerId, int maxSeq)
{
    if (playerId < 0 || playerId >= 32) return;
    *(unsigned int *)(data_020a4d6c + playerId * 0x1c + 0x18) =
        (unsigned short)maxSeq;
}

}  // extern "C"
