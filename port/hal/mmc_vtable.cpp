// Gate 16: the MovingMeshCollider vtable.
//
// hal/clsn_vtable.cpp seeds this table with MeshCollider's, which is right for
// every binary whose only collider is the level's. Gate 16 puts real moving
// colliders on the list -- SIGN_POST, BLACK_BRICK_BLOCK and CASTLE_WATER each
// Enable one, and every Player probe walks data_020a0c80 -- so the six
// overrides go in, all matched src:
//
//   3/4/5  GetNormal, GetTriangleOrigin: func_02039db8 / func_02039e30 rotate
//          the KCL's own normal and origin out by the collider's matrix
//   6/7/8  the DetectClsn triple: transform the incoming ray or sphere INTO
//          the collider's space (func_02039e48), run MeshCollider's own walk,
//          scale the answer back out by the collider's scale
//  10/11/12 TransformPos, GetAngularVelY, GetVelocity: what a riding actor
//          reads to be carried
//
// Without them a moving collider is walked as if its matrix were the identity,
// which puts the sign post's collision at the world origin.
//
// SLOT NUMBERING is the seed table's existing mixture, not a new decision.
// 0..5 are MSVC's -- the ITCM walk is compiled as a real method and reaches
// GetNormal through slot 3 -- and 6..12 are the ROM's, because every
// DetectClsn caller in src is a transcribed shadow class that counts two
// destructor slots (SphereClsn::DetectClsn dispatches v8, RaycastGround's v6,
// MovingMeshCollider::Transform's v12). Index 5 takes GetTriangleOrigin as
// well as index 4, the same fill-the-table-twice the Model render seam uses.
#include "MovingMeshCollider.h"
#include <cstdio>

extern "C" {
extern void *_ZTV18MovingMeshCollider[16];   /* storage: hal/clsn_vtable.cpp */
int _ZN18MovingMeshCollider10DetectClsnER13RaycastGround(void *self, void *g);
int _ZN18MovingMeshCollider10DetectClsnER11RaycastLine(void *self, void *r);
int _ZN18MovingMeshCollider10DetectClsnER10SphereClsn(void *self, void *s);
extern unsigned char data_020a0d0c[], data_020a0d1c[], data_020a0d60[];
}

typedef MovingMeshCollider MMC;

static void __fastcall mmc_v08(void *s, void *)
{ ((MMC *)s)->MMC::Virtual08(); }
static void __fastcall mmc_norm(void *s, void *, s16 tri, Vector3 *res)
{ ((MMC *)s)->MMC::GetNormal(tri, *res); }
static void __fastcall mmc_orig(void *s, void *, s16 tri, Vector3 *res)
{ ((MMC *)s)->MMC::GetTriangleOrigin(tri, *res); }
static int __fastcall mmc_ground(void *s, void *, void *g)
{ return _ZN18MovingMeshCollider10DetectClsnER13RaycastGround(s, g); }
static int __fastcall mmc_line(void *s, void *, void *r)
{ return _ZN18MovingMeshCollider10DetectClsnER11RaycastLine(s, r); }
static int __fastcall mmc_sphere(void *s, void *, void *sp)
{ return _ZN18MovingMeshCollider10DetectClsnER10SphereClsn(s, sp); }
static int __fastcall mmc_tpos(void *s, void *, const Vector3 *p, Vector3 *r)
{ return ((MMC *)s)->MMC::TransformPos(*p, *r); }
static s16 __fastcall mmc_angvel(void *s, void *)
{ return ((MMC *)s)->MMC::GetAngularVelY(); }
static void __fastcall mmc_vel(void *s, void *, Vector3 *r)
{ ((MMC *)s)->MMC::GetVelocity(*r); }

extern "C" void hal_fill_moving_mesh_collider_vtable(void)
{
    _ZTV18MovingMeshCollider[2] = (void *)mmc_v08;
    _ZTV18MovingMeshCollider[3] = (void *)mmc_norm;
    _ZTV18MovingMeshCollider[4] = (void *)mmc_orig;
    _ZTV18MovingMeshCollider[5] = (void *)mmc_orig;
    _ZTV18MovingMeshCollider[6] = (void *)mmc_ground;
    _ZTV18MovingMeshCollider[7] = (void *)mmc_line;
    _ZTV18MovingMeshCollider[8] = (void *)mmc_sphere;
    _ZTV18MovingMeshCollider[10] = (void *)mmc_tpos;
    _ZTV18MovingMeshCollider[11] = (void *)mmc_angvel;
    _ZTV18MovingMeshCollider[12] = (void *)mmc_vel;
    /* The three moving-collider DetectClsn bodies fill ONE static
       RaycastLine at arm9 0x020a0d0c and read the answer back out of two
       symbols dsd split off its interior. Say so once if the linker ever
       stops packing them. */
    if (data_020a0d1c - data_020a0d0c != 0x10 ||
        data_020a0d60 - data_020a0d0c != 0x54)
        std::fprintf(stderr, "  [clsn] MOVING-COLLIDER SCRATCH NOT "
                     "CONTIGUOUS: +%d +%d\n",
                     (int)(data_020a0d1c - data_020a0d0c),
                     (int)(data_020a0d60 - data_020a0d0c));
}
