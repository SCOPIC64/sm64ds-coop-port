// Gate 16: the MovingMeshCollider vtable, now dBgW_KcMbg.
//
// The KcMbg triple (Lin/Gnd/SphCrr DetectClsn) is matched upstream; the
// table COMES UP as a copy of the base dBgW_Kc table (upstream
// hal_fill_mmc_vtable) and this fill installs the moving overrides plus
// the ride slots, same numbering as the base table:
//
//   3/4/5  GetNormal, GetTriangleOrigin: KcMbg's own, rotating the KCL's
//          own normal and origin out by the collider's matrix
//   6/7/8  the DetectClsn triple: transform the incoming ray or sphere INTO
//          the collider's space, run the base walk, scale the answer back
//  10/11/12 TransformPos, GetAngularVelY, GetVelocity: what a riding actor
//          reads to be carried
//
// Without them a moving collider is walked as if its matrix were the
// identity, which puts the sign post's collision at the world origin.
#include "dBgW_KcMbg.h"
#include "dBgCh_Gnd.h"
#include "dBgCh_Lin.h"
#include "dBgCh_SphCrr.h"
#include <cstdio>

extern "C" {
extern void *_ZTV10dBgW_KcMbg[16];   /* storage: hal/clsn_vtable.cpp */
}

typedef dBgW_KcMbg MMC;

static void __fastcall mmc_v08(void *s, void *)
{ ((MMC *)s)->MMC::Virtual08(); }
static void __fastcall mmc_norm(void *s, void *, s16 tri, Vector3 *res)
{ ((MMC *)s)->MMC::GetNormal(tri, *res); }
static void __fastcall mmc_orig(void *s, void *, s16 tri, Vector3 *res)
{ ((MMC *)s)->MMC::GetTriangleOrigin(tri, *res); }
static int __fastcall mmc_ground(void *s, void *, void *g)
{ return ((MMC *)s)->MMC::DetectClsn(*(dBgCh_Gnd *)g); }
static int __fastcall mmc_line(void *s, void *, void *r)
{ return ((MMC *)s)->MMC::DetectClsn(*(dBgCh_Lin *)r); }
static int __fastcall mmc_sphere(void *s, void *, void *sp)
{ return ((MMC *)s)->MMC::DetectClsn(*(dBgCh_SphCrr *)sp); }
static int __fastcall mmc_tpos(void *s, void *, const Vector3 *p, Vector3 *r)
{ return ((MMC *)s)->MMC::TransformPos(*p, *r); }
static s16 __fastcall mmc_angvel(void *s, void *)
{ return ((MMC *)s)->MMC::GetAngularVelY(); }
static void __fastcall mmc_vel(void *s, void *, Vector3 *r)
{ ((MMC *)s)->MMC::GetVelocity(*r); }

extern "C" void hal_fill_moving_mesh_collider_vtable(void)
{
    /* MSVC slot order (one dtor slot, DetectClsn trio reversed): v08=1,
       norm=3, orig=4, sphere=5, line=6, ground=7, then the KcMbg's own
       ride slots 8/9/10. */
    _ZTV10dBgW_KcMbg[1] = (void *)mmc_v08;
    _ZTV10dBgW_KcMbg[3] = (void *)mmc_norm;
    _ZTV10dBgW_KcMbg[4] = (void *)mmc_orig;
    _ZTV10dBgW_KcMbg[5] = (void *)mmc_sphere;
    _ZTV10dBgW_KcMbg[6] = (void *)mmc_line;
    _ZTV10dBgW_KcMbg[7] = (void *)mmc_ground;
    _ZTV10dBgW_KcMbg[8] = (void *)mmc_tpos;
    _ZTV10dBgW_KcMbg[9] = (void *)mmc_angvel;
    _ZTV10dBgW_KcMbg[10] = (void *)mmc_vel;
}
