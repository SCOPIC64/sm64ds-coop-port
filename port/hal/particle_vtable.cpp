// The thirteen Particle::Callback vtables, as host storage.
//
// Particle::SysTracker's constructor builds fourteen callback sub-objects
// inside itself and points each one's vptr at one of these:
//
//     func_020226a4(p)  ->  *(void **)p = &data_0208f3b4;   /* then f3c4 */
//     func_020225fc(p)  ->  func_020226a4(p); p->vt = &data_0208f414;
//
// so the vptr is the ADDRESS of the symbol, and the two words at and just
// after it are the class's two virtual functions. The dispatch that matters
// is in func_02021bec, the tracker's per-frame walk over its 0x40 system
// slots:
//
//     e->f10->vt->m[1](e->f10, e->fc, m)
//
// -- plain cdecl through slot 1. Slot 0 (SpawnParticles) is reached the same
// way from the spawn path.
//
// WHY THIS FILE EXISTS AT ALL. On the ROM these are .data words in arm9 at
// 0x0208f3a4..0x0208f464 holding DS code addresses, and the port has been
// bitten more than once by exactly that shape -- a table of raw DS addresses
// that must never be called directly. Here the fix is the simplest form of
// the seat: the port never loads that region (romdata.py's NAMED list does
// not carry it, and hal/auto_bss.cpp would otherwise hand out zeroed storage,
// which is a null call on the first frame), so instead of patching ROM words
// after the fact this file DEFINES the thirteen vtables outright, with host
// function pointers, in the ROM's own layout.
//
// THE LAYOUT IS THE ROM'S, NOT A GUESS. Each vtable is Itanium-shaped --
// [offset-to-top][typeinfo][slot0][slot1] -- and the vptr the constructors
// install points at slot0, four words in. Only slot0 and slot1 are ever
// read, so each vtable is self-contained in its own symbol as a two-element
// array and no adjacency between symbols is required (which is why this
// needs no romdata.py CONTIG run).
//
// EVERY PAIR BELOW IS THE ROM'S OWN RELOCATION. config/arm9/relocs.txt gives
// the destination of all 26 words; the comment on each line is the exact
// from:/to: pair, so this table can be re-derived and re-checked against the
// ROM at any time. port_particle_vtables_check() re-asserts the shape at
// runtime before the first frame.
//
// Slot 0 is SpawnParticles, slot 1 is OnUpdate. Note that slot 0 is SHARED:
// nine of the thirteen classes never override SpawnParticles and take either
// Particle::Callback's (a bare `return`) or Particle::SimpleCallback's. That
// sharing is the ROM's, not a simplification -- see the to: addresses.
#include <cstdio>
#include <cstdlib>
#include "dsstate_seg.h"

extern "C" {

/* the two base implementations, shared by most of the table */
void _ZN8Particle8Callback14SpawnParticlesERNS_6SystemE(void *, void *);
void _ZN8Particle8Callback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE(void *, void *);
void _ZN8Particle14SimpleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);

/* the eleven overriding OnUpdates */
void _ZN8Particle14BubbleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle25EndingStarGlitterCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle25EndingStarGlitterCallback14SpawnParticlesERNS_6SystemE(void *, void *);
void _ZN8Particle14SplashCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle18CheckWaterCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle17CheckLavaCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle17CheckLavaCallback14SpawnParticlesERNS_6SystemE(void *, void *);
void _ZN8Particle13ScaleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle13ScaleCallback14SpawnParticlesERNS_6SystemE(void *, void *);
void _ZN8Particle24CheckWaterRippleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle12ClipCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle22FitWaterSimpleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle16FitWaterCallback8OnUpdateERNS_6SystemEb(void *, void *, int);
void _ZN8Particle21CleanParticleCallback8OnUpdateERNS_6SystemEb(void *, void *, int);

#define SPAWN(f) ((void *)&f)
#define UPDATE(f) ((void *)&f)

DSSTATE_BEGIN
/* BubbleCallback.  from:0x0208f3a4 to:0x02022640 | 0x0208f3a8 to:0x02022464 */
void *data_0208f3a4[2] = {
    SPAWN(_ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle14BubbleCallback8OnUpdateERNS_6SystemEb),
};
/* Callback (the base).
   from:0x0208f3b4 to:0x020226d0 | 0x0208f3b8 to:0x020226c8 */
void *data_0208f3b4[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle8Callback8OnUpdateERNS_6SystemEb),
};
/* SimpleCallback.  from:0x0208f3c4 to:0x02022640 | 0x0208f3c8 to:0x02022630 */
void *data_0208f3c4[2] = {
    SPAWN(_ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle14SimpleCallback8OnUpdateERNS_6SystemEb),
};
/* EndingStarGlitterCallback.
   from:0x0208f3d4 to:0x0202222c | 0x0208f3d8 to:0x020221dc */
void *data_0208f3d4[2] = {
    SPAWN(_ZN8Particle25EndingStarGlitterCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle25EndingStarGlitterCallback8OnUpdateERNS_6SystemEb),
};
/* SplashCallback.  from:0x0208f3e4 to:0x02022640 | 0x0208f3e8 to:0x020224fc */
void *data_0208f3e4[2] = {
    SPAWN(_ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle14SplashCallback8OnUpdateERNS_6SystemEb),
};
/* CheckWaterCallback.
   from:0x0208f3f4 to:0x020226d0 | 0x0208f3f8 to:0x02022160 */
void *data_0208f3f4[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle18CheckWaterCallback8OnUpdateERNS_6SystemEb),
};
/* CheckLavaCallback.
   from:0x0208f404 to:0x02022328 | 0x0208f408 to:0x020222f0 */
void *data_0208f404[2] = {
    SPAWN(_ZN8Particle17CheckLavaCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle17CheckLavaCallback8OnUpdateERNS_6SystemEb),
};
/* ScaleCallback.  from:0x0208f414 to:0x020225d0 | 0x0208f418 to:0x020225a8 */
void *data_0208f414[2] = {
    SPAWN(_ZN8Particle13ScaleCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle13ScaleCallback8OnUpdateERNS_6SystemEb),
};
/* CheckWaterRippleCallback.
   from:0x0208f424 to:0x020226d0 | 0x0208f428 to:0x020220a4 */
void *data_0208f424[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle24CheckWaterRippleCallback8OnUpdateERNS_6SystemEb),
};
/* ClipCallback.  from:0x0208f434 to:0x020226d0 | 0x0208f438 to:0x02021e70 */
void *data_0208f434[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle12ClipCallback8OnUpdateERNS_6SystemEb),
};
/* FitWaterSimpleCallback.
   from:0x0208f444 to:0x02022640 | 0x0208f448 to:0x02022418 */
void *data_0208f444[2] = {
    SPAWN(_ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle22FitWaterSimpleCallback8OnUpdateERNS_6SystemEb),
};
/* FitWaterCallback.
   from:0x0208f454 to:0x020226d0 | 0x0208f458 to:0x0202202c */
void *data_0208f454[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle16FitWaterCallback8OnUpdateERNS_6SystemEb),
};
/* CleanParticleCallback.
   from:0x0208f464 to:0x020226d0 | 0x0208f468 to:0x02021e40 */
void *data_0208f464[2] = {
    SPAWN(_ZN8Particle8Callback14SpawnParticlesERNS_6SystemE),
    UPDATE(_ZN8Particle21CleanParticleCallback8OnUpdateERNS_6SystemEb),
};

/* ---- the two dispatch tables func_0204b028 / func_0204b244 index ---------
   THE SAME BUG SHAPE ONE MORE TIME, and this one crashed a play session
   (2026-08-05, ~frame 9750, warping into Bob-omb Battlefield): a table of raw
   DS code words, called directly.

   romdata.py carried data_02099fb4 and data_02099fbc in its NAMED list as
   "8-byte constants", so the port got the ROM's own bytes -- four DS
   addresses -- and the billboard builder called straight through them:

       data_02099fbc[g->axis](rotX, rotY, rot);   -- the axis rotation
       data_02099fb4[g->flag](fa, fb);            -- the blend/cull pair

   port_particle_render -> Particle::RenderAll -> func_02049ee8 ->
   func_0204a5c8 -> func_0204af3c -> func_0204b244 -> 0x0204c0a8, which on the
   host is unmapped memory. It survived this long because the four callees are
   only ever named by these two tables, so the linker discarded three of them
   as unreferenced and nothing pointed at the gap.

   Defined here with host pointers, in the ROM's own order. Every entry is the
   ROM's own relocation out of config/arm9/relocs.txt, quoted per line, and
   removed from romdata.py's list in exchange. */
void func_0204c0a8(int, int, void *);
void func_0204c0e8(int, int, void *);
void func_0204c24c(int, int);
void func_0204c194(int, int);

/* from:0x02099fbc to:0x0204c0a8 / from:0x02099fc0 to:0x0204c0e8 */
void (*data_02099fbc[2])(int, int, void *) = {
    func_0204c0a8,
    func_0204c0e8,
};

/* from:0x02099fb4 to:0x0204c24c / from:0x02099fb8 to:0x0204c194 */
void (*data_02099fb4[2])(unsigned char, unsigned char) = {
    (void (*)(unsigned char, unsigned char))func_0204c24c,
    (void (*)(unsigned char, unsigned char))func_0204c194,
};
DSSTATE_END

/* Called once before the first frame. Cheap, and it catches the two ways
   this file can rot: a slot left null (the symbol got zeroed storage from
   somewhere else and this file lost the tie-break), and a slot holding
   something in the DS address range instead of a host pointer -- which is
   what a raw ROM word looks like and is the bug class this file exists to
   prevent. */
void port_particle_vtables_check(void)
{
    void **vts[13] = {
        data_0208f3a4, data_0208f3b4, data_0208f3c4, data_0208f3d4,
        data_0208f3e4, data_0208f3f4, data_0208f404, data_0208f414,
        data_0208f424, data_0208f434, data_0208f444, data_0208f454,
        data_0208f464,
    };
    static const unsigned addr[13] = {
        0x0208f3a4, 0x0208f3b4, 0x0208f3c4, 0x0208f3d4, 0x0208f3e4,
        0x0208f3f4, 0x0208f404, 0x0208f414, 0x0208f424, 0x0208f434,
        0x0208f444, 0x0208f454, 0x0208f464,
    };
    for (int i = 0; i < 13; ++i) {
        for (int s = 0; s < 2; ++s) {
            const void *p = vts[i][s];
            if (!p) {
                std::fprintf(stderr, "FATAL: particle vtable %08x slot %d is "
                             "null -- nothing seated it\n", addr[i], s);
                std::abort();
            }
            /* a DS code word would land in 0x01ff0000..0x023fffff */
            const unsigned v = (unsigned)(unsigned long long)(unsigned long)p;
            if (v >= 0x01ff0000u && v < 0x02400000u) {
                std::fprintf(stderr, "FATAL: particle vtable %08x slot %d "
                             "holds a DS address %08x, not a host pointer\n",
                             addr[i], s, v);
                std::abort();
            }
        }
    }
    /* the two billboard dispatch tables, same test. These were raw ROM words
       until 2026-08-05 and this loop is what would have said so on frame 0
       instead of ten thousand frames into a play session. */
    const void *disp[4] = {
        (const void *)data_02099fbc[0], (const void *)data_02099fbc[1],
        (const void *)data_02099fb4[0], (const void *)data_02099fb4[1],
    };
    static const unsigned dispaddr[4] = {
        0x02099fbc, 0x02099fc0, 0x02099fb4, 0x02099fb8,
    };
    for (int i = 0; i < 4; ++i) {
        const unsigned v = (unsigned)(unsigned long long)(unsigned long)disp[i];
        if (!v || (v >= 0x01ff0000u && v < 0x02400000u)) {
            std::fprintf(stderr, "FATAL: billboard dispatch word %08x holds "
                         "%08x, not a host pointer\n", dispaddr[i], v);
            std::abort();
        }
    }
}

}  /* extern "C" */
