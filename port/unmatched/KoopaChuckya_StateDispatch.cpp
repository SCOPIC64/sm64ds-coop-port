/* HOST COPIES of CHUCKYA's two pointer-to-member dispatchers (gate 182,
 * ov062), the seat of the twenty state records they read, and the one
 * name-and-abort stub KOOPA's link needs.
 *
 * CHUCKYA (190, daHolhei_c) drives a TWENTY-state machine through ten 16-byte
 * State objects in ov062 bss (0x0211de70..0x0211df00), each two mwcc
 * {function, delta} records: the ENTER half at +0 and the MAIN half at +8.
 * Chuckya_ChangeState stores a State pointer at this+0x364 and dispatches the
 * ENTER half once; Chuckya::Behavior dispatches the MAIN half every frame. Both
 * matched TUs form the pointer-to-member over a FORWARD-DECLARED struct
 * (`struct C; typedef int (C::*PMF)();` / `struct Klass;`), so MSVC hands them
 * the general (worst-case-inheritance) PMF representation, a multi-word value
 * plus a this-adjust thunk, not the single code pointer a complete
 * single-inheritance class gets -- the KnockDownPlank/MontyMole/Scuttlebug
 * case exactly. Each body below is the matched source's control flow line for
 * line; only the dispatch is read as a plain { fn, delta } and the fn called
 * with `this`.
 *
 * RECORD GEOMETRY, READ OFF THE ROM'S OWN DISPATCH MATH (not the source):
 * Behavior at 0x021170c8: `ldr r1,[r4,#0x364]; ldr r0,[r1,#8]` -- the MAIN
 * half IS the record at State+8, fn at +0 of the record, delta at +4
 * (`add r3,r1,#8; ldr r1,[r3,#4]`), the virtual-bit arithmetic inline.
 * ChangeState at 0x02116ce4: `ldr r3,[r0,#0x364]; ldr r2,[r3]` -- the ENTER
 * half is the record at State+0. No table indexing anywhere -- the dispatchers
 * take record POINTERS, so there is no stride to misread.
 *
 * THE SEAT (the Scuttlebug/MontyMole "seat the source before the copy" order)
 * --------------------------------------------------------------------------
 * The twenty SOURCE statics live at ov062 0x0211d900..0x0211d998 (8 bytes
 * each); __sinit_ov062_0211cf30 -- which gate 32 already runs -- copies them
 * into the ten bss State objects. The ovdata mount lays the source blocks down
 * with their fn words as DS CODE ADDRESSES (the mount's pointer pass rebases
 * only pointers into other mounted DATA). port_chuckya_states_seat() rewrites
 * each source fn word with its host body BEFORE that sinit runs, validating
 * each mounted word against the ROM address first (WRONG-BYTES abort), so a
 * mount pointing at the wrong bytes says so instead of copying an
 * overlay-image address into the live objects. All twenty .delta halves are 0
 * (verified against the overlay image; no reloc at any +4). All twenty state
 * bodies are matched src (slice_gate182.txt). Called from
 * hal/actor_overlays.cpp between port_ov062_syms_patch() and
 * __sinit_ov062_0211cf30().
 *
 * KOOPA (203, daNknk_c) NEEDS NO SEAT AND NO BEHAVIOR COPY: its matched
 * Behavior is a plain switch machine, no pointer-to-member anywhere. Its
 * Render is the ModelAnim slot-5 collision -- port/unmatched/Koopa_Render.cpp.
 *
 * THE STUB
 * --------
 * func_ov062_02117724 (0x270 bytes) is the shell-dust Particle helper three
 * matched Koopa Behavior helpers (func_ov062_02118718/02118b4c/02118cdc) bl
 * into when the actor's state byte at +0x398 reads 3. It is UNMATCHED -- no C
 * in src/ -- and a direct bl edge cannot be trapped in data, so the stub below
 * satisfies the link and names the hole if it is ever reached (the
 * Rabbit-0x0212b8dc / Painting-0x021261f4 precedent). The closest draft is in
 * nearmiss/db.jsonl (divergences=5, source fanout-glm-5.2): a
 * Particle::System::New(0xf9) burst over the shell, scaled by actor id
 * (0xcb = KOOPA). CHUCKYA never calls it.
 */
#include <cstdio>
#include <cstdlib>

#include "ModelAnim.h"

extern "C" {

struct PortPmf { unsigned fn; int delta; };

/* ---- Chuckya Behavior's C-linkage helpers, all matched src or hosted ---- */
int _ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
    void *self, void *wm, void *anim, unsigned int n);
unsigned short DecIfAbove0_Short(unsigned short *p);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *cc);
int _ZN5Enemy15IsGoingOffCliffER12WithMeshClsn5Fix12IiEsbbS3_(
    void *self, void *wm, int a, short b, int c, int d, void *e);
void _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *self, void *wm,
                                              unsigned int j);
void func_ov062_02116010(void *self);   /* the held/thrown-player step */
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN12CylinderClsn6UpdateEv(void *self);
void _ZN9Animation7AdvanceEv(void *self);
void func_ov062_02116d28(void *c);      /* the held-by-player short-circuit */
void func_ov062_02116e80(void *c);      /* the post step */
void func_ov062_02116dbc(void *c);      /* the shadow drop */
extern signed char data_0209f2f8;       /* the current level id */

/* func_ov062_02117724's own callees (all matched arm9 TUs) + the sin/cos table */
void func_0201267c(int a, char *b);      /* the shell-scrape sound */
int _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
    int a, int b, int x, int y, int z, int v, int cb);
void _ZN8Particle19SetSelfDestructFlagEj(int id);
char *_ZN8Particle6System12FromUniqueIDEj(int id);
extern short data_02082214[];            /* the shared sin/cos fixed-point table */

/* the State objects Behavior compares this+0x364 against (mounted bss) */
extern unsigned char data_ov062_0211de70[], data_ov062_0211de90[],
    data_ov062_0211dea0[], data_ov062_0211dec0[], data_ov062_0211ded0[],
    data_ov062_0211dee0[], data_ov062_0211df00[];

/* the twenty state bodies, all matched src (called by host pointer with
   `this` as arg 0, which is what they expect) */
int func_ov062_021161a8(void *c); int func_ov062_02116238(void *c);
int func_ov062_02116274(void *c); int func_ov062_021162b8(void *c);
int func_ov062_0211632c(void *c); int func_ov062_02116368(void *c);
int func_ov062_021163b0(void *c); int func_ov062_02116498(void *c);
int func_ov062_021164e8(void *c); int func_ov062_021165e0(void *c);
int func_ov062_021165e8(void *c); int func_ov062_02116784(void *c);
int func_ov062_021167c0(void *c); int func_ov062_02116850(void *c);
int func_ov062_02116894(void *c); int func_ov062_02116980(void *c);
int func_ov062_02116a08(void *c); int func_ov062_02116b80(void *c);
int func_ov062_02116bf8(void *c); int func_ov062_02116c78(void *c);

/* the twenty SOURCE records the sinit copies from (data, {fn, 0}) */
extern PortPmf data_ov062_0211d900, data_ov062_0211d908, data_ov062_0211d910,
    data_ov062_0211d918, data_ov062_0211d920, data_ov062_0211d928,
    data_ov062_0211d930, data_ov062_0211d938, data_ov062_0211d940,
    data_ov062_0211d948, data_ov062_0211d950, data_ov062_0211d958,
    data_ov062_0211d960, data_ov062_0211d968, data_ov062_0211d970,
    data_ov062_0211d978, data_ov062_0211d980, data_ov062_0211d988,
    data_ov062_0211d990, data_ov062_0211d998;

}  /* extern "C" */

/* PORT_HOST_ABI: mwcc pointer-to-member on the forward-declared struct C. The
   matched Chuckya_ChangeState stores the State pointer at this+0x364, reads
   the ENTER record (State+0) and, if non-null, calls it with `this`. Read as a
   plain { fn, 0 }. */
extern "C" int Chuckya_ChangeState(void *cv, void *rec)
{
    char *c = (char *)cv;
    *(void **)(c + 0x364) = rec;               /* c->pp = p */
    {
        PortPmf *q = *(PortPmf **)(c + 0x364); /* q = c->pp */
        if (q[0].fn == 0)
            return 1;
        return ((int (*)(void *))(size_t)q[0].fn)(c);
    }
}

/* PORT_HOST_ABI: mwcc pointer-to-member on the forward-declared struct Klass
   (the MAIN half, State+8), plus one raw vptr+0xc model dispatch. The matched
   Behavior's control flow line for line. */
extern "C" int _ZN7Chuckya8BehaviorEv(void *cv)
{
    char *c = (char *)cv;

    if (_ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
            c, c + 0x144, c + 0x300, 3))
        return 1;

    DecIfAbove0_Short((unsigned short *)(c + 0x100));
    DecIfAbove0_Short((unsigned short *)(c + 0x3e6));
    DecIfAbove0_Short((unsigned short *)(c + 0x3e8));

    {
        /* (((Klass*)c)->*(m->pmf))() -- m = State at +0x364, pmf = the record
           at m+8; read as a plain { fn, 0 } and fn called with `this`. */
        PortPmf *m = *(PortPmf **)(c + 0x364);
        if (m[1].fn != 0)
            ((void (*)(void *))(size_t)m[1].fn)(c);
    }

    *(short *)(c + 0x8c) = *(short *)(c + 0x92);
    *(short *)(c + 0x8e) = *(short *)(c + 0x94);
    *(short *)(c + 0x90) = *(short *)(c + 0x96);
    _ZN5Actor9UpdatePosEP12CylinderClsn(c, c + 0x110);

    if (*(void **)(c + 0x364) != (void *)data_ov062_0211dea0) {
        if (*(void **)(c + 0x364) != (void *)data_ov062_0211dec0
            && *(void **)(c + 0x364) != (void *)data_ov062_0211de70) {
            int r2 = 0;
            signed char t = data_0209f2f8;
            if (t == 0x16) {
                if (*(int *)(c + 0x5c) > (int)0xff95c000)
                    r2 = 1;
            } else if (t == 0x15) {
                if (*(int *)(c + 0x64) < (int)0xff2f4000)
                    r2 = 1;
            }
            if (r2 != 0
                || (*(int *)(c + 0x98) != 0
                    && _ZN5Enemy15IsGoingOffCliffER12WithMeshClsn5Fix12IiEsbbS3_(
                           c, c + 0x144, 0x3c000, (short)0x2888, 0, 1,
                           (void *)0x32000))) {
                *(int *)(c + 0x98) = 0;
                *(char *)(c + 0x3e4) = 1;
                *(int *)(c + 0x5c) = *(int *)(c + 0x3d8);
                *(int *)(c + 0x60) = *(int *)(c + 0x3dc);
                *(int *)(c + 0x64) = *(int *)(c + 0x3e0);
            } else {
                if (*(unsigned char *)(c + 0x3e4) == 1)
                    *(char *)(c + 0x3e4) = 0;
            }
        }
        *(int *)(c + 0x3d8) = *(int *)(c + 0x5c);
        *(int *)(c + 0x3dc) = *(int *)(c + 0x60);
        *(int *)(c + 0x3e0) = *(int *)(c + 0x64);
        _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(c, c + 0x144, 3);
    }

    if (*(void **)(c + 0x364) == (void *)data_ov062_0211ded0
        || *(void **)(c + 0x364) == (void *)data_ov062_0211dee0
        || *(void **)(c + 0x364) == (void *)data_ov062_0211de90
        || *(void **)(c + 0x364) == (void *)data_ov062_0211df00) {
        func_ov062_02116010(c);
    }

    _ZN12CylinderClsn5ClearEv(c + 0x110);
    _ZN9Animation7AdvanceEv(c + 0x350);

    /* PORT_HOST_ABI: the matched TU reads the ModelAnim's vptr as an int and
       dispatches *(vptr + 0xc) -- ROM slot 3, UpdateVerts (include/ModelAnim.h's
       own slot map). The host _ZTV9ModelAnim is MSVC-ordered (one dtor slot
       where Itanium spends two), so a raw +0xc would land on Virtual10 -- the
       same shift as the Render collisions, one slot lower. Spell the qualified
       method the ROM means (the cxxname_bridge reading). */
    ((ModelAnim *)(c + 0x300))->ModelAnim::UpdateVerts();

    {
        char *p3f8 = *(char **)(c + 0x3f8);
        if (p3f8 != 0) {
            int flag = (*(int *)(c + 0xb0) & 0x4000) != 0;
            if (flag) {
                if (*(int *)(p3f8 + 0xc8) != 0) {
                    func_ov062_02116d28(c);
                    return 1;   /* the matched source's `goto ret` */
                }
            }
        }
    }

    _ZN12CylinderClsn6UpdateEv(c + 0x110);
    *(int *)(c + 0x42c) = 0;
    *(int *)(c + 0x430) = 0;
    *(int *)(c + 0x434) = 0;
    func_ov062_02116e80(c);
    func_ov062_02116dbc(c);
    return 1;
}

/* ---- THE SEAT --------------------------------------------------------------
   The twenty source fn words in FEEDER ORDER (0x0211d900..0x0211d998), each
   {slot, rom, host} triple matched against its own reloc destination in
   config/arm9/overlays/ov062/relocs.txt (from:0x0211d900 -> 0x02116784 and so
   on down the run). Rewritten BEFORE __sinit_ov062_0211cf30 copies them into
   the ten bss State objects. */
static const struct { PortPmf *slot; unsigned rom; int (*host)(void *); }
g_chuckya_states[] = {
    {&data_ov062_0211d900, 0x02116784, func_ov062_02116784},
    {&data_ov062_0211d908, 0x021167c0, func_ov062_021167c0},
    {&data_ov062_0211d910, 0x0211632c, func_ov062_0211632c},
    {&data_ov062_0211d918, 0x021165e8, func_ov062_021165e8},
    {&data_ov062_0211d920, 0x02116850, func_ov062_02116850},
    {&data_ov062_0211d928, 0x02116274, func_ov062_02116274},
    {&data_ov062_0211d930, 0x021163b0, func_ov062_021163b0},
    {&data_ov062_0211d938, 0x02116bf8, func_ov062_02116bf8},
    {&data_ov062_0211d940, 0x02116c78, func_ov062_02116c78},
    {&data_ov062_0211d948, 0x02116238, func_ov062_02116238},
    {&data_ov062_0211d950, 0x02116a08, func_ov062_02116a08},
    {&data_ov062_0211d958, 0x02116980, func_ov062_02116980},
    {&data_ov062_0211d960, 0x02116894, func_ov062_02116894},
    {&data_ov062_0211d968, 0x021162b8, func_ov062_021162b8},
    {&data_ov062_0211d970, 0x021161a8, func_ov062_021161a8},
    {&data_ov062_0211d978, 0x02116368, func_ov062_02116368},
    {&data_ov062_0211d980, 0x021165e0, func_ov062_021165e0},
    {&data_ov062_0211d988, 0x02116498, func_ov062_02116498},
    {&data_ov062_0211d990, 0x021164e8, func_ov062_021164e8},
    {&data_ov062_0211d998, 0x02116b80, func_ov062_02116b80},
};

extern "C" void port_chuckya_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_chuckya_states /
                             sizeof g_chuckya_states[0]; ++i) {
        PortPmf *p = g_chuckya_states[i].slot;
        if (p->fn != g_chuckya_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: Chuckya state source %u: the mount "
                         "holds %08x/%d, the ROM's own table says %08x/0 -- "
                         "WRONG BYTES\n", i, p->fn, p->delta,
                         g_chuckya_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_chuckya_states[i].host;
    }
}

/* ---- HOST COPY (PORT_HOST_ABI) ---------------------------------------------
   ov062 0x02117724 is KOOPA's state-3 shell-dust Particle helper, bl'd from
   three matched Koopa helpers (func_ov062_02118718/02118b4c gate on
   `if (state==3)`, func_ov062_02118cdc on `if (state==1)`). It was the one hole in KOOPA: an abort stub
   that killed BoB star groups 4-7 at ~frame 9 the instant KOOPA reached state 3.

   The byte-match is a TRUE FLOOR at 5 divergences (nearmiss/db.jsonl,
   fanout-glm-5.2). The residue is two pure instruction-scheduler reorders in the
   two coordinate-update branches -- an identical-multiset shuffle where the ROM
   fills the pos.x store's delay slot with the second coordinate's multiply/load
   (the one-slot load-delay-slot fill of notes/mwccarm-codegen.md 6o, the
   func_ov060_02113740 family). Every source spelling that touches the two
   branches (~12 tried) perturbs the upstream idx*2 array-index allocation and
   regresses; 4500 whole-function + 1300 PERM_RANDOMIZE-scoped decomp-permuter
   iterations found no size-correct improvement below 5 (all its "better" scores
   are size-inflated dead-store candidates). So no bannered hand-asm (game logic)
   -- this host copy reproduces the ROM's SEMANTICS line for line instead.

   ROM 0x02117724 (disassembled): if the actor's shell-facing angle bucket at
   +0x358 (h = (u16)((s32<<4)>>16)) falls in [a1,a2] or [a3,a4], and the
   once-only latch +0x3cd is clear, latch it, run func_0201267c(0xe4, self+0x74)
   (the shell-scrape sound), and -- unless the held/thrown flag +0x398 == 1 --
   spawn one Particle::System::New(id 0xf9) dust burst at the actor's world
   position (+0x5c/+0x60/+0x64), nudged k=(id==0xcb?0xf:0xa) units up in Y and k
   units along the shell-facing normal read from the sin/cos table
   data_02082214[ (ang+0x4000) bucket ]. On the low-side arm (h<=a2) the normal
   is subtracted, on the high-side arm added. The spawned particle's field +0x50
   (its own scale) is then rescaled by 0x800/4096 for KOOPA (id 0xcb) or
   0x500/4096 otherwise, both with round-to-nearest (+0x800 >> 12) and a s16
   clamp. Out of both bands: clear the +0x3cd latch. CHUCKYA (id != 0xcb) never
   reaches it. */
extern "C" void func_ov062_02117724(void *c, int a, int b, int d, int e)
{
    char *t = (char *)c;
    unsigned int a1 = (unsigned int)a;
    unsigned int a2 = (unsigned int)b;
    unsigned int a3 = (unsigned int)d;
    unsigned short a4 = (unsigned short)e;

    unsigned int h = (unsigned int)(*(int *)(t + 0x358) << 4) >> 16;
    if ((h >= a1 && h <= a2) || (h >= a3 && h <= a4)) {
        int k, idx, uid;
        short ang;
        int px, py, pz;
        char *ps;

        if (*(unsigned char *)(t + 0x3cd) != 0) return;
        *(unsigned char *)(t + 0x3cd) = 1;
        func_0201267c(0xe4, t + 0x74);
        if (*(unsigned char *)(t + 0x398) == 1) return;

        ang = (short)(*(short *)(t + 0x8e) + 0x4000);
        px = *(int *)(t + 0x5c);
        py = *(int *)(t + 0x60);
        pz = *(int *)(t + 0x64);
        k = (*(unsigned short *)(t + 0xc) == 0xcb) ? 0xf : 0xa;
        py += k << 12;

        idx = (unsigned short)ang >> 4;
        {
            int i2 = idx * 2;
            int i1 = i2 + 1;
            int sv = data_02082214[i2];
            int cv = data_02082214[i1];
            if (h <= a2) {
                px = px - k * sv;
                pz = pz - k * cv;
            } else {
                px = k * sv + px;
                pz = k * cv + pz;
            }
        }

        uid = _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
                  0, 0xf9, px, py, pz, 0, 0);
        _ZN8Particle19SetSelfDestructFlagEj(0xf9);
        if (uid == 0) return;
        ps = _ZN8Particle6System12FromUniqueIDEj(uid);
        if (ps == 0) return;

        if (*(unsigned short *)(t + 0xc) == 0xcb)
            *(int *)(ps + 0x50) =
                (short)(((long long)*(int *)(ps + 0x50) * 0x800 + 0x800) >> 12);
        else
            *(int *)(ps + 0x50) =
                (short)(((long long)*(int *)(ps + 0x50) * 0x500 + 0x800) >> 12);
    } else {
        *(unsigned char *)(t + 0x3cd) = 0;
    }
}
