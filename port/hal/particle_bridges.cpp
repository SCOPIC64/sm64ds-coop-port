// Gate-29 seams: C-linkage faces for the particle callbacks that are METHODS,
// and host bodies for the two fx-math asm primitives the effect VM reaches.
//
// FACES, and why an alias would not do. Four of the thirteen callback bodies
// are compiled from .cpp files that declare a real class, so MSVC emits them
// __thiscall (`this` in ECX). Everything that calls them -- the vtables in
// hal/particle_vtable.cpp and the tracker's own walk in func_02021bec --
// reaches them by Itanium C name through a plain cdecl function pointer, with
// self pushed on the stack. A /alternatename: alias would link cleanly and
// then read `this` out of ECX on every call, so each one gets a real
// forwarding face instead. The shadow classes below are copied from the
// defining TUs; only the layout that the forwarded call needs is spelled.
#include "Model.h"

namespace Memory {
void *Allocate(unsigned size);   /* defined in src/_ZN6Memory8AllocateEj.cpp */
}

namespace Particle {
struct System;
/* Particle::CheckLavaCallback::SpawnParticles chains to its SimpleCallback
   base as a C++ method, and the src file that used to define that method is
   gone (an ARM ride-through -- see port/unmatched/Particle_RideThroughs.cpp).
   So the METHOD face forwards into the C-named host copy: the one direction
   this file does not otherwise go. */
struct SimpleCallback {
    char pad_00[4];
    short field_4;
    void SpawnParticles(System &sys);
};
struct CheckLavaCallback {
    void SpawnParticles(System &sys);
};
struct CleanParticleCallback {
    int OnUpdate(System &sys, bool done);
};
struct CheckWaterRippleCallback {
    bool OnUpdate(System &sys, bool b);
};
}  // namespace Particle

extern "C" {

void _ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE(void *, void *);

/* NO C-NAMED FACE for Particle::SimpleCallback::SpawnParticles -- slot 0 of the
   SimpleCallback, SplashCallback, BubbleCallback and FitWaterSimpleCallback
   vtables, the one the ROM shares across all four. Its src body passes an
   argument it never names (an ARM r1 ride-through), so it is a host copy in
   port/unmatched/Particle_RideThroughs.cpp, already C-named and cdecl. The
   METHOD face for it is below, outside this extern "C" block. */

/* slot 0 of the CheckLavaCallback vtable */
void _ZN8Particle17CheckLavaCallback14SpawnParticlesERNS_6SystemE(void *self,
                                                                  void *sys)
{
    ((Particle::CheckLavaCallback *)self)
        ->SpawnParticles(*(Particle::System *)sys);
}

/* slot 1 of the CleanParticleCallback vtable */
int _ZN8Particle21CleanParticleCallback8OnUpdateERNS_6SystemEb(void *self,
                                                               void *sys,
                                                               int done)
{
    return ((Particle::CleanParticleCallback *)self)
        ->OnUpdate(*(Particle::System *)sys, done != 0);
}

/* slot 1 of the CheckWaterRippleCallback vtable */
int _ZN8Particle24CheckWaterRippleCallback8OnUpdateERNS_6SystemEb(void *self,
                                                                  void *sys,
                                                                  int b)
{
    return ((Particle::CheckWaterRippleCallback *)self)
               ->OnUpdate(*(Particle::System *)sys, b != 0)
               ? 1
               : 0;
}

/* Both are static/namespace-scope and therefore already cdecl, but the
   defining TUs are C++ so the Itanium name the particle sources spell has
   nothing behind it. Two-line faces beat guessing a decorated name. */
unsigned _ZN5Model13GetVramOffsetEj(unsigned size)
{
    return Model::GetVramOffset(size);
}

void *_ZN6Memory8AllocateEj(unsigned size) { return Memory::Allocate(size); }

/* cstd::div wears its pre-naming address in func_0204dab4, the effect VM's
   scale ramp: `0x1000 - func_02052f4c(f2e << 12, f2c)`. 0x02052f4c IS
   cstd::div -- symbols.txt names it, that source file just predates the
   naming. The body is src/_ZN4cstd3divEii.c in this gate's slice (it drives
   the DS hardware divider through raw MMIO, so it needs the hostgen
   routing); this is only the name the caller spells. */
int _ZN4cstd3divEii(int a, int b);
int func_02052f4c(int a, int b) { return _ZN4cstd3divEii(a, b); }

/* ---- two fx-math asm primitives ------------------------------------------
   Both are Thumb `asm` blocks in src/ (asm-primitive policy: Nintendo shipped
   them as assembly, so there is no C to recover and no match to chase), which
   makes them exactly the thing a host compiler can never consume. They build
   a 3x3 fixed-point rotation matrix, 4096 = 1.0, and the particle render
   family calls them per billboard to face the camera. Transcribed from the
   register writes in the asm blocks, offset for offset. */

/* Matrix3x3_SetRotationZ @ 0x02052588: {c, s, 0, -s, c, 0, 0, 0, 1} */
void Matrix3x3_SetRotationZ(int *m, int s, int c)
{
    m[0] = c;  m[1] = s;  m[2] = 0;
    m[3] = -s; m[4] = c;  m[5] = 0;
    m[6] = 0;  m[7] = 0;  m[8] = 0x1000;
}

/* func_0205256c @ 0x0205256c: the Y sibling, {c, 0, -s, 0, 1, 0, s, 0, c}.
   The asm writes by byte offset (m[8] is [r0,#32], m[6] is [r0,#24], m[2] is
   [r0,#8], m[4] is [r0,#16]); this is that, in order. */
void func_0205256c(int *m, int s, int c)
{
    m[0] = c;  m[1] = 0;      m[2] = -s;
    m[3] = 0;  m[4] = 0x1000; m[5] = 0;
    m[6] = s;  m[7] = 0;      m[8] = c;
}

}  /* extern "C" */

/* the method face named above: CheckLavaCallback's own SpawnParticles calls
   this on its base, and the host copy is what actually runs */
void Particle::SimpleCallback::SpawnParticles(System &sys)
{
    _ZN8Particle14SimpleCallback14SpawnParticlesERNS_6SystemE(this, &sys);
}

// ---- the lifecycle seams ---------------------------------------------------
//
// WHERE THE ROM PUTS THEM, read out of the relocation table rather than
// guessed. Three call sites, all on the Stage:
//
//   0x0202d3dc  Stage::InitResources    -> Particle::SysTracker::Initialise
//   0x0202b930  Stage::Render           -> Particle::SysTracker::Update
//   0x02029840  Stage::GraphCallback1   -> Particle::RenderAll
//
// The port drives the frame by hand, so the two per-frame calls are made
// where those two Stage methods would have run: both inside the render pass,
// Update before RenderAll. Initialise goes at the end of port_stage_a_boot,
// which is where Stage::InitResources calls it -- after the level's model,
// collision, objects, fog and skybox are all loaded, because the particle
// archive's textures are uploaded into VRAM the level has already banked.
#include <cstdio>
#include <cstdlib>

#include "ntr/gx.h"

extern "C" {
void _ZN8Particle10SysTracker10InitialiseEv(void *self);
void _ZN8Particle10SysTracker6UpdateEv(void *self);
void _ZN8Particle9RenderAllEv(void);
void port_particle_vtables_check(void);
void *port_stage_object(void);
extern int data_0209b3ec[];    /* the engine view matrix, ROM scene units */
extern void *data_0209ee74;    /* the tracker, set by SysTracker::SysTracker */
}

/* Stage+0x50 is the SysTracker sub-object; Stage::InitResources spells the
   same `(char *)this + 0x50`. */
static char *tracker(void)
{
    char *stage = (char *)port_stage_object();
    return stage ? stage + 0x50 : 0;
}

/* SM64DS_FX_TRACE=1 prints the live counts, =2 adds a per-system dump of the
   fields the emitter reads (the pair that found the ride-through). */
static int fx_trace(void)
{
    static int lvl = -1;
    if (lvl < 0) {
        const char *s = std::getenv("SM64DS_FX_TRACE");
        lvl = s ? std::atoi(s) : 0;
        if (s && lvl < 1) lvl = 1;
    }
    return lvl;
}

extern "C" void port_particle_boot(void)
{
    char *t = tracker();
    if (!t) {
        std::fprintf(stderr, "  [fx] NO STAGE: the particle tracker has no "
                     "home, subsystem left down\n");
        return;
    }
    /* the vtables the constructor already installed, checked before the
       first dispatch can reach one */
    port_particle_vtables_check();
    if ((char *)data_0209ee74 != t)
        std::fprintf(stderr, "  [fx] TRACKER GLOBAL MISMATCH: data_0209ee74 "
                     "is %p, Stage+0x50 is %p\n", data_0209ee74, (void *)t);

    _ZN8Particle10SysTracker10InitialiseEv(t);

    char *engine = *(char **)(t + 4);
    if (!engine) {
        std::fprintf(stderr, "  [fx] Initialise left no engine at +4; the "
                     "particle subsystem is down\n");
        return;
    }
    /* func_0204a17c parsed the blob into these: the PEntry array at +0x1c
       (stride 0x20, the stride Manager::AddSystem indexes by) with its count
       at +0x24, and the secondary array at +0x20 counted at +0x26. Every
       effect id the game spawns is an index into the first array, so the
       count is the one number that says whether the parse worked: the
       Player's running/sliding dust is id 0xda, and the ground pound's are
       0xd3..0xd5. */
    const unsigned numA = *(unsigned short *)(engine + 0x24);
    const unsigned numB = *(unsigned short *)(engine + 0x26);
    std::printf("  [fx] particle subsystem up: engine %p, %d systems, %d "
                "particles, defs %p (%u effects, %u secondary)\n",
                (void *)engine, (int)*(short *)(engine + 0x28),
                (int)*(short *)(engine + 0x2a), *(void **)t, numA, numB);
    if (numA <= 0xda)
        std::fprintf(stderr, "  [fx] ONLY %u EFFECTS PARSED: the Player's "
                     "dust is id 0xda, so the blob did not parse\n", numA);

    if (fx_trace()) {
        /* byte 0x30 of a definition record is the emission interval that
           func_0204ae2c copies into a live system's +0x58, and +0x58 is the
           divisor in func_0204a730's `counter % interval` gate. Count how
           many of the ROM's own definitions ship a zero there. */
        char *base = *(char **)(engine + 0x1c);
        unsigned zero = 0, live = 0;
        for (unsigned i = 0; i < numA; ++i) {
            char *rec = *(char **)(base + i * 0x20);
            if (!rec) continue;
            ++live;
            if (*(unsigned char *)(rec + 0x30) == 0) ++zero;
        }
        char *dust = *(char **)(base + 0xda * 0x20);
        std::printf("  [fx] definitions: %u with a record, %u of them with a "
                    "ZERO emission interval; id 0xda (the Player's dust) "
                    "interval=%u\n", live, zero,
                    dust ? (unsigned)*(unsigned char *)(dust + 0x30) : 0u);
    }
}

/* Live counts straight out of the engine's own free lists, which is why this
   costs nothing: func_0204a4c8 threads every system node onto the free list
   at +0xc and every particle node onto the one at +0x14, and each list keeps
   its length in the word after the head. Live = capacity - free, and the
   active system list at +4 keeps its own count at +8. */
extern "C" void port_particle_counts(int *systems, int *particles)
{
    char *t = tracker();
    char *engine = t ? *(char **)(t + 4) : 0;
    if (!engine) { if (systems) *systems = 0; if (particles) *particles = 0; return; }
    if (systems) *systems = *(int *)(engine + 8);
    if (particles)
        *particles = (int)*(unsigned short *)(engine + 0x2a) -
                     *(int *)(engine + 0x18);
}

extern "C" void port_particle_frame(void)
{
    char *t = tracker();
    if (!t || !*(char **)(t + 4))
        return;                       /* subsystem down; say nothing per frame */

    if (fx_trace() >= 2) {
        /* the view matrix Particle::RenderAll hands the engine, which every
           particle's world position is multiplied by before the billboard
           matrix is loaded (func_0204be40: MulVec3Mat4x3(&trans, mtx, &trans)) */
        static int said_mtx;
        if (!said_mtx) {
            said_mtx = 1;
            extern int data_0209b3ec[];
            std::printf("[fx] view matrix data_0209b3ec:");
            for (int i = 0; i < 12; ++i)
                std::printf(" %d", data_0209b3ec[i]);
            std::printf("\n");
        }
    }
    if (fx_trace() >= 2) {
        /* walk the engine's active system list before the update: head at
           engine+4, count at +8, each node's prev in word 0 (func_0204d9a0
           writes node->prev = old, node->next = 0) */
        char *engine = *(char **)(t + 4);
        char *node = *(char **)(engine + 4);
        for (int guard = 0; node && guard < 64; ++guard) {
            char *pentry = *(char **)(node + 0x18);
            char *base = *(char **)(engine + 0x1c);
            /* Particle::System::New stores the spawn point already divided by
               8 (its `args.a = a >> 3` trio) into the param block at +0x20.
               Print it against Mario so the units question is answered by the
               stored number rather than by reading the arithmetic. */
            {
                char *pb = *(char **)(node + 0x0c);
                if (pb)
                    std::printf("[fx]   paramblk pos %.1f %.1f %.1f (scene) "
                                "= %.1f %.1f %.1f (world)\n",
                                *(int *)(pb + 0x20) / 4096.0,
                                *(int *)(pb + 0x24) / 4096.0,
                                *(int *)(pb + 0x28) / 4096.0,
                                *(int *)(pb + 0x20) / 512.0,
                                *(int *)(pb + 0x24) / 512.0,
                                *(int *)(pb + 0x28) / 512.0);
            }
            std::printf("[fx]   system %p interval=%u counter=%u pentry=%p "
                        "(id %d) def=%p\n",
                        (void *)node, (unsigned)*(unsigned char *)(node + 0x58),
                        (unsigned)*(unsigned short *)(node + 0x38),
                        (void *)pentry,
                        pentry ? (int)((pentry - base) / 0x20) : -1,
                        pentry ? *(void **)pentry : 0);
            node = *(char **)node;
        }
    }
    _ZN8Particle10SysTracker6UpdateEv(t);   /* Stage::Render's call */
    /* SM64DS_NO_FX_RENDER=1 keeps the simulation and drops the submission,
       for A/B-ing what the particles actually put on the screen */
    static int no_render = -1;
    if (no_render < 0)
        no_render = std::getenv("SM64DS_NO_FX_RENDER") ? 1 : 0;
    /* SM64DS_FX_TRACE=3: what RenderAll actually put in the triangle buffer.
       The subsystem simulates correctly and submits geometry, so anything
       invisible is being lost between submission and the raster: a degenerate
       billboard, an off-screen transform, a zero alpha, or a texture that
       decoded to nothing. Print the triangles it added so the answer is read
       off the numbers instead of guessed. */
    size_t tris_before = 0;
    if (fx_trace() >= 3) ntr::gx_polygons(tris_before);
    /* SM64DS_FX_SCALE=1: an EXPERIMENT, not a fix. func_0204be40 multiplies a
       particle's stored position straight through data_0209b3ec and loads the
       result as the billboard, with no shift anywhere in between, so the
       position it holds must already be in the same units as that matrix.
       Dividing the matrix's 3x3 by 8 is the same arithmetic as converting the
       position from world units to scene units, so if the particles are being
       fed world positions this lands them where they belong and the real fix
       belongs at whatever seats the position. */
    int saved[9];
    static int scale_fix = -1;
    if (scale_fix < 0) scale_fix = std::getenv("SM64DS_FX_SCALE") ? 1 : 0;
    if (scale_fix) {
        for (int i = 0; i < 9; ++i) { saved[i] = data_0209b3ec[i]; data_0209b3ec[i] /= 8; }
    }
    if (!no_render)
        _ZN8Particle9RenderAllEv();         /* Stage::GraphCallback1's call */
    if (scale_fix)
        for (int i = 0; i < 9; ++i) data_0209b3ec[i] = saved[i];
    if (fx_trace() >= 3) {
        size_t tn = 0;
        const ntr::GxTriangle *ta = ntr::gx_polygons(tn);
        static int shown;
        if (tn > tris_before && shown < 6) {
            ++shown;
            std::printf("[fx] RenderAll added %u triangles:\n",
                        (unsigned)(tn - tris_before));
            for (size_t i = tris_before; i < tn && i < tris_before + 3; ++i) {
                const ntr::GxTriangle &t = ta[i];
                std::printf("[fx]   tri a(%.1f,%.1f,%.3f w%.2f) b(%.1f,%.1f,%.3f) "
                            "c(%.1f,%.1f,%.3f) col %08X alpha %u cull %u tex %s %dx%d\n",
                            t.v[0].x, t.v[0].y, t.v[0].z, t.v[0].w,
                            t.v[1].x, t.v[1].y, t.v[1].z,
                            t.v[2].x, t.v[2].y, t.v[2].z,
                            t.v[0].color, (unsigned)t.alpha, (unsigned)t.cull,
                            t.tex ? "yes" : "NULL", t.tw, t.th);
                const float area =
                    (t.v[1].x - t.v[0].x) * (t.v[2].y - t.v[0].y) -
                    (t.v[2].x - t.v[0].x) * (t.v[1].y - t.v[0].y);
                std::printf("[fx]     signed area %.3f%s\n", area,
                            area == 0.0f ? "  <-- DEGENERATE" : "");
            }
        }
    }

    if (fx_trace()) {
        /* one line a second at 30fps, plus every frame the count changes, so
           a dust puff that lives twelve frames still shows up */
        static int n, last = -1, peak;
        int sys = 0, par = 0;
        port_particle_counts(&sys, &par);
        if (par > peak) peak = par;
        if (par != last || (n % 30) == 0) {
            std::printf("[fx] frame %d: %d systems live, %d particles live "
                        "(peak %d)\n", n, sys, par, peak);
            last = par;
        }
        ++n;
    }
}
