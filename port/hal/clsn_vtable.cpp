// The synthetic dBgW_Kc vtable (gate 8) -- the gate-3a mechanism at
// its second use, and the first with REAL slot fillers throughout the hot
// path: GetSurfaceInfo (matched, ITCM) calls GetNormal through the vtable
// (notes/itcm.md, "the one lever"), so slot 4 must dispatch for the octree
// walk to survive. Slots are __fastcall shims (ecx carries `this` exactly
// as __thiscall does; the dummy edx absorbs fastcall's second register),
// slot order per include/dBgW_Kc.h's ROM-read map: dtor 0/1,
// Virtual08 2, surface queries 3-5, the DetectClsn overloads 6-8.
// Unevidenced slots trap loudly.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dBgW_Kc.h"
#include "dBgCh.h"
#include "dBgCh_Lin.h"
#include "dBgCh_Gnd.h"
#include "dBgCh_SphCrr.h"
#include "SurfaceInfo.h"

namespace cstd {
int sqrt(unsigned long long);
int fdiv_result();
}
extern "C" {
int _ZN4cstd4sqrtEy(unsigned long long value) { return cstd::sqrt(value); }
int _ZN4cstd11fdiv_resultEv() { return cstd::fdiv_result(); }
bool _ZN5dBgCh21ShouldPassThroughImplEPvRK4CLPSRKS_b(
    void *p, const CLPS &clps, const dBgCh &bg, bool flag)
{ return dBgCh::ShouldPassThroughImpl(p, clps, bg, flag); }
void _ZNK11SurfaceInfo12CopyNormalToER7Vector3(
    const SurfaceInfo *self, Vector3 *out)
{ self->SurfaceInfo::CopyNormalTo(*out); }
}

static void __fastcall slot_v08(void *self, void *)
{ ((dBgW_Kc *)self)->dBgW_Kc::Virtual08(); }
static void __fastcall slot_surf(void *self, void *, s16 tri, SurfaceInfo *res)
{ ((dBgW_Kc *)self)->dBgW_Kc::GetSurfaceInfo(tri, *res); }
static void __fastcall slot_norm(void *self, void *, s16 tri, Vector3 *res)
{ ((dBgW_Kc *)self)->dBgW_Kc::GetNormal(tri, *res); }
static void __fastcall slot_orig(void *self, void *, s16 tri, Vector3 *res)
{ ((dBgW_Kc *)self)->dBgW_Kc::GetTriangleOrigin(tri, *res); }
static int __fastcall slot_ray(void *self, void *, dBgCh_Lin *ray)
{ return ((dBgW_Kc *)self)->dBgW_Kc::DetectClsn(*ray); }

#define TRAP(n) \
    static void __fastcall slot_trap##n(void *, void *) { \
        fprintf(stderr, "FATAL: dBgW_Kc vtable slot %d dispatched " \
                        "with no filler (clsn_vtable.cpp)\n", n); \
        abort(); }
TRAP(0) TRAP(1) TRAP(6) TRAP(8) TRAP(9) TRAP(10) TRAP(11) TRAP(12)

/* hosted passes, defined below the table */
static int __fastcall slot_ground(void *self, void *, unsigned char *g);
static int __fastcall slot_sphere(void *self, void *, void *sph);

// SLOT ORDER IS MSVC'S, NOT THE ROM'S. The dispatching code here is
// MSVC-compiled against include/dBgW_Kc.h, and MSVC lays the table
// with a ONE-slot destructor (the ROM's Itanium layout spends two). Filling
// the array in ROM order put GetSurfaceInfo where MSVC reads GetNormal, and
// its internal virtual call recursed into itself until the stack died --
// the exact D1/D0-vs-scalar-dtor skew the earlier gates dodged by never
// dispatching. One more MSVC quirk pinned here: adjacent overloads
// (the DetectClsn trio) are emitted in REVERSE declaration order.
extern "C" void *_ZTV7dBgW_Kc[13] = {
    (void *)slot_trap0,         /* 0: scalar deleting dtor */
    (void *)slot_v08,           /* 1: Virtual08 */
    (void *)slot_surf,          /* 2: GetSurfaceInfo */
    (void *)slot_norm,          /* 3: GetNormal -- the walk's hot slot */
    (void *)slot_orig,          /* 4: GetTriangleOrigin */
    (void *)slot_trap1,         /* 5: DetectClsn(dBgCh_SphCrr) - hosted below */
    (void *)slot_ray,           /* 6: DetectClsn(dBgCh_Lin) */
    (void *)slot_ground,        /* 7: DetectClsn(dBgCh_Gnd) - adapter below */
    (void *)slot_trap8,
    (void *)slot_trap9, (void *)slot_trap10,
    (void *)slot_trap11, (void *)slot_trap12,
};

// dBgW_KcMbg inherits the surface queries; its table gets the same
// shims at runtime (its own DetectClsn overloads stay trapped until a
// consumer needs them).
extern "C" {
void *_ZTV10dBgW_KcMbg[16];
void hal_fill_mmc_vtable(void)
{
    for (int i = 0; i < 13; ++i)
        _ZTV10dBgW_KcMbg[i] = _ZTV7dBgW_Kc[i];
}
}

/* ---- the port's hosted collision passes (gates 8/15) --------------------
   Slot 7 (ground) is an adapter: a stack dBgCh_Lin straight down from the
   ground ray's own position by its own reach, walked by the hosted line
   walk, hit fields copied back. Slot 5 (sphere) dispatches the hosted
   transcription in port/unmatched/MeshCollider_DetectClsn_Sphere.cpp. */

extern "C" int g_sphere_dbg[16];   /* the transcription's probe array */

static int __fastcall slot_ground(void *self, void *, unsigned char *g)
{
    unsigned char line[0x64];
    memset(line, 0, sizeof line);
    memcpy(line, g, 0x10);                       /* BgCh flags ride along */
    memcpy(line + 0x38, g + 0x38, 12);           /* lineStart = pos */
    memcpy(line + 0x54, g + 0x38, 12);           /* lineEnd = pos ... */
    int reach = *(int *)(g + 0x4c);
    if (reach <= 0) reach = 0xA0000;             /* 160 units default */
    *(int *)(line + 0x58) -= reach;              /* ... straight down */
    *(int *)(line + 0x60) = 0x7FFFFFF;           /* best-dist seed */
    int hit = ((dBgW_Kc *)self)->dBgW_Kc::DetectClsn(
        *(dBgCh_Lin *)line);
    if (hit) {
        /* hit fields only: SurfaceInfo (+0x14, 20 bytes) and tri (+0x28).
           NEVER the whole result head -- the old 0x1c-byte copy from the
           un-ctor'd stack line clobbered the ground result's vtable,
           clsn slot (0x18 = empty sentinel) and objID (-1 = no actor),
           and the zeros sent UpdateExtraContinous walking slot 0xffff
           and FindWithID hunting actor 0 */
        memcpy(g + 0x14, line + 0x14, 20);
        *(unsigned short *)(g + 0x28) = *(unsigned short *)(line + 0x28);
        /* ground result fields (func_02037464 resets them: y at +0x44
           seeded 0x80000000, flag at +0x48) */
        if (((int *)(line + 0x54))[1] > *(int *)(g + 0x44))
            *(int *)(g + 0x44) = ((int *)(line + 0x54))[1];
        g[0x48] = 1;
    }
    return hit;
}

/* direct probe for harnesses: vertical line walk, returns 1 and writes
   *out_y on hit */
extern "C" int hal_ground_ray(void *mc, int x, int y, int z, int reach,
                              int *out_y)
{
    unsigned char line[0x64];
    memset(line, 0, sizeof line);
    line[4] = 1;                                 /* collide ordinary */
    ((int *)(line + 0x38))[0] = x;
    ((int *)(line + 0x38))[1] = y;
    ((int *)(line + 0x38))[2] = z;
    ((int *)(line + 0x54))[0] = x;
    ((int *)(line + 0x54))[1] = y - reach;
    ((int *)(line + 0x54))[2] = z;
    *(int *)(line + 0x60) = 0x7FFFFFF;           /* best-dist seed */
    int hit = ((dBgW_Kc *)mc)->dBgW_Kc::DetectClsn(
        *(dBgCh_Lin *)line);
    if (hit && out_y)
        *out_y = ((int *)(line + 0x54))[1];      /* clsnPos.y (walk writes
                                                    the end as the hit) */
    return hit;
}

/* arbitrary line for camera occlusion: from a toward b, returns hit and
   the clip point */
extern "C" int hal_line_ray(void *mc, const int *a, const int *b, int *out)
{
    unsigned char line[0x64];
    memset(line, 0, sizeof line);
    line[4] = 1;
    memcpy(line + 0x38, a, 12);
    memcpy(line + 0x54, b, 12);
    *(int *)(line + 0x60) = 0x7FFFFFF;
    int hit = ((dBgW_Kc *)mc)->dBgW_Kc::DetectClsn(
        *(dBgCh_Lin *)line);
    if (hit && out)
        memcpy(out, line + 0x54, 12);            /* clsnPos */
    return hit;
}

static int __fastcall slot_sphere(void *self, void *, void *sph)
{
    static int off = -1;
    if (off < 0) off = getenv("SM64DS_NO_SPHERE") ? 1 : 0;
    if (off) return 0;
    int r = ((dBgW_Kc *)self)->dBgW_Kc::DetectClsn(
        *(dBgCh_SphCrr *)sph);
    if (r && getenv("PORT_TRACE_CLSN")) {
        const unsigned char *s = (const unsigned char *)sph;
        fprintf(stderr, "  [sphere] mask=%d flags=%02x tri=%d\n",
                r, s[0x70], g_sphere_dbg[14]);
    }
    return r;
}
