// The synthetic MeshCollider vtable (gate 8) -- the gate-3a mechanism at
// its second use, and the first with REAL slot fillers throughout the hot
// path: GetSurfaceInfo (matched, ITCM) calls GetNormal through the vtable
// (notes/itcm.md, "the one lever"), so slot 4 must dispatch for the octree
// walk to survive. Slots are __fastcall shims (ecx carries `this` exactly
// as __thiscall does; the dummy edx absorbs fastcall's second register),
// slot order per include/MeshCollider.h's ROM-read map: dtor 0/1,
// Virtual08 2, surface queries 3-5, the DetectClsn overloads 6-8.
// Unevidenced slots trap loudly.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MeshCollider.h"

static void __fastcall slot_v08(void *self, void *)
{ ((MeshCollider *)self)->MeshCollider::Virtual08(); }
static void __fastcall slot_surf(void *self, void *, s16 tri, SurfaceInfo *res)
{ ((MeshCollider *)self)->MeshCollider::GetSurfaceInfo(tri, *res); }
static void __fastcall slot_norm(void *self, void *, s16 tri, Vector3 *res)
{ ((MeshCollider *)self)->MeshCollider::GetNormal(tri, *res); }
static void __fastcall slot_orig(void *self, void *, s16 tri, Vector3 *res)
{ ((MeshCollider *)self)->MeshCollider::GetTriangleOrigin(tri, *res); }
static int __fastcall slot_ray(void *self, void *, RaycastLine *ray)
{ return ((MeshCollider *)self)->MeshCollider::DetectClsn(*ray); }

/* Ground overload (ROM slot 6): the vertical specialization the decomp has
   not matched. Adapter: a stack RaycastLine straight down from the ground
   ray's position, walked by the hosted line walk, result copied back.
   Ground layout evidence: BgCh head 0x10, ClsnResult 0x10, pos Vec3 0x38,
   reach 0x4c (InitResources writes it). */
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
    int hit = ((MeshCollider *)self)->MeshCollider::DetectClsn(
        *(RaycastLine *)line);
    if (getenv("PORT_TRACE_CLSN"))
        fprintf(stderr, "  [ground] start(%d,%d,%d) reach=%d head4=%d "
                        "-> hit=%d y=%d\n",
                ((int *)(line + 0x38))[0], ((int *)(line + 0x38))[1],
                ((int *)(line + 0x38))[2], reach, line[4], hit,
                ((int *)(line + 0x54))[1]);
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
    int hit = ((MeshCollider *)mc)->MeshCollider::DetectClsn(
        *(RaycastLine *)line);
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
    int hit = ((MeshCollider *)mc)->MeshCollider::DetectClsn(
        *(RaycastLine *)line);
    if (hit && out)
        memcpy(out, line + 0x54, 12);            /* clsnPos */
    return hit;
}

static int __fastcall slot_sphere(void *self, void *, void *sph)
{
    /* sphere push-out is unmatched; no wall contact for now (once only) */
    static int warned;
    if (!warned) {
        warned = 1;
        fprintf(stderr, "  [clsn] sphere DetectClsn stubbed (no wall "
                        "push-out yet)\n");
    }
    (void)self; (void)sph;
    return 0;
}

#define TRAP(n) \
    static void __fastcall slot_trap##n(void *, void *) { \
        fprintf(stderr, "FATAL: MeshCollider vtable slot %d dispatched " \
                        "with no filler (clsn_vtable.cpp)\n", n); \
        abort(); }
TRAP(0) TRAP(1) TRAP(6) TRAP(8) TRAP(9) TRAP(10) TRAP(11) TRAP(12)

// SLOT ORDER IS MSVC'S, NOT THE ROM'S. The dispatching code here is
// MSVC-compiled against include/MeshCollider.h, and MSVC lays the table
// with a ONE-slot destructor (the ROM's Itanium layout spends two). Filling
// the array in ROM order put GetSurfaceInfo where MSVC reads GetNormal, and
// its internal virtual call recursed into itself until the stack died --
// the exact D1/D0-vs-scalar-dtor skew the earlier gates dodged by never
// dispatching. One more MSVC quirk pinned here: adjacent overloads
// (the DetectClsn trio) are emitted in REVERSE declaration order.
extern "C" void *_ZTV12MeshCollider[13] = {
    (void *)slot_trap0,         /* 0: scalar deleting dtor */
    (void *)slot_v08,           /* 1: Virtual08 */
    (void *)slot_surf,          /* 2: GetSurfaceInfo */
    (void *)slot_norm,          /* 3: GetNormal -- the walk's hot slot */
    (void *)slot_orig,          /* 4: GetTriangleOrigin */
    (void *)slot_trap1,         /* 5: GetTriangleOrigin (ROM numbering) */
    (void *)slot_ground,        /* 6: DetectClsn(RaycastGround) - adapter */
    (void *)slot_ray,           /* 7: DetectClsn(RaycastLine) */
    (void *)slot_sphere,        /* 8: DetectClsn(SphereClsn) - stub */
    (void *)slot_trap9, (void *)slot_trap10,
    (void *)slot_trap11, (void *)slot_trap12,
};

// MovingMeshCollider inherits the surface queries; its table gets the same
// shims at runtime (its own DetectClsn overloads stay trapped until a
// consumer needs them).
extern "C" {
void *_ZTV18MovingMeshCollider[16];
void hal_fill_mmc_vtable(void)
{
    for (int i = 0; i < 13; ++i)
        _ZTV18MovingMeshCollider[i] = _ZTV12MeshCollider[i];
}
}
