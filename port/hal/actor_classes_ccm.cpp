// GATE 143: Cool Cool Mountain's slide (level 11, ov019), IceSlideManager's
// vtable.
//
// Same law as hal/actor_classes.cpp and hal/actor_classes_wf.cpp -- ROM slot
// order, __fastcall thunks that call QUALIFIED, unhosted slots trap by name --
// and TWENTY SLOTS, the plain ActorBase shape (IceSlideManager is a direct
// Actor subclass, not a Platform).
//
// IceSlideManager (actor 356) is the one class level 11 spawns that lives in the
// level's own overlay rather than a shared one, so it needs the per-symbol ov019
// mount (port/ov019_syms.txt) and this host vtable, the ov015 treatment for a
// level overlay's own class. The vtable _ZTV15IceSlideManager (ov019 0x021133cc)
// is a host array the registry fills; a mounted vtable would hand the factory DS
// code addresses (the ov080/ov015 rule).
//
// Every slot below was read out of _ZTV15IceSlideManager with its relocations
// applied:
//
//   slot  0  InitResources    0x0211271c  ov019, hosted (ism_init)
//   slot  3  CleanupResources 0x02043bf0  ActorBase's base body (ism_clean_base)
//   slot  6  Behavior         0x02112678  ov019, hosted (ism_behavior)
//   slot  9  Render           0x02043af0  ActorBase::Render, a no-op base body
//   slot 12  OnPendingDestroy 0x02043ac0  ActorBase's base body (ac_pdes)
//   slot 16  D1               0x0211261c  ov019; the ROM body is an empty
//                                         ~IceSlideManager over ~Actor, i.e.
//                                         Actor::D2 alone (ism_d1)
//   slot 17  D0               0x02112640  ov019; kept trapped -- the ROM
//                                         teardown dispatches 16 and does the
//                                         Memory::Deallocate itself, so a call
//                                         landing on 17 is worth an abort
//
// The other thirteen slots are the shared Actor/ActorBase halves. IceSlideManager
// has no Render, CleanupResources or OnPendingDestroy of its own; its slots 3/9/
// 12 point at ActorBase's base do-nothing bodies, so those take base faces here.
//
// The class body is matched src (slice_gate142.txt): InitResources copies the
// three words __sinit_ov019_02112b14 wrote into data_ov019_021135d8 into
// unk_05c/060/064 and arms a 0x78-frame timer; Behavior waits for the player
// within 0x180000, plays a sound, counts the timer down and kills the actor.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
int _ZN5Actor19BeforeInitResourcesEv(void *self);            /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a); /* slot 2  */
int _ZN5Actor14BeforeBehaviorEv(void *self);                 /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);                   /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                  /* slot 18 */
void *_ZN5ActorD2Ev(void *self);                             /* slot 16 tail */

extern int data_02099f24[];          /* the frame phase the lists are in */
extern unsigned char data_020a4b4c;  /* the spawn spine's own step */
const char *port_actor_class_name(unsigned id);   /* hal/actor_registry */
  void port_actor_slot_decline(const char *what);  /* func_02043fdc.cpp: per-actor decline */

/* The class's own two matched methods (ov019). */
int _ZN15IceSlideManager13InitResourcesEv(void *self);       /* 0x0211271c */
int _ZN15IceSlideManager8BehaviorEv(void *self);             /* 0x02112678 */
}

// ---- the trap --------------------------------------------------------------
static void ccm_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "UNHOSTED: vtable slot %d is not hosted (actor id %u %s, "
                 "phase %d, spawn step %d)\n",
                 slot, id, port_actor_class_name(id), data_02099f24[0],
                 (int)data_020a4b4c);
    { static char _m[128];
      std::snprintf(_m, sizeof _m, "unhosted vtable slot %d on id %u %s",
                    slot, id, port_actor_class_name(id));
      port_actor_slot_decline(_m); }
}
static int __fastcall ccm_trap13(void *s, void *) { ccm_trap_report(s, 13); return 0; }
static int __fastcall ccm_trap14(void *s, void *) { ccm_trap_report(s, 14); return 0; }
static int __fastcall ccm_trap17(void *s, void *) { ccm_trap_report(s, 17); return 0; }
static int __fastcall ccm_trap19(void *s, void *) { ccm_trap_report(s, 19); return 0; }

// ---- the shared half -------------------------------------------------------
static int __fastcall ccm_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ccm_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ccm_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall ccm_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall ccm_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ccm_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall ccm_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall ccm_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall ccm_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall ccm_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static int __fastcall ccm_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }

/* Slots 3 and 9 are ActorBase's own base bodies in the ROM vtable
   (CleanupResources 0x02043bf0, Render 0x02043af0), both empty do-nothing
   forms; IceSlideManager overrides neither. Rather than trap them (they are
   reachable: the cleanup Process dispatches slot 3), they call the base member
   qualified, the same reading ac_bclean/ac_aclean take for the shared pairs. */
static int __fastcall ccm_clean_base(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::CleanupResources(); }
static int __fastcall ccm_render_base(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::Render(); }

/* Slot 16, D1. The ROM body is an empty ~IceSlideManager: no member sub-objects
   (the header is plain u8 fields), so it is Actor::D2 alone, the ac_d1_actor_only
   reading hal/actor_classes.cpp already documents. */
static int __fastcall ism_d1(void *s, void *)
{ return (int)(size_t)_ZN5ActorD2Ev(s); }

// ---- the class's own two slots ---------------------------------------------
static int __fastcall ism_init(void *s, void *)
{ return _ZN15IceSlideManager13InitResourcesEv(s); }
static int __fastcall ism_behavior(void *s, void *)
{ return _ZN15IceSlideManager8BehaviorEv(s); }

/* The one array the ROM factory installs (IceSlideManager_Spawn does
   `p[0] = (int)_ZTV15IceSlideManager`); twenty slots like every ActorBase
   actor. Defined here, not just declared: the `int` type and C linkage match
   the `extern int _ZTV15IceSlideManager[]` in decl_common.h that the factory
   TU sees. */
extern "C" { int _ZTV15IceSlideManager[20]; }

/* IceSlideManager::InitResources (src, compiled C++) reads its construction data
   through `extern struct S3 data_ov019_021135d8;`, which MSVC decorates as a C++
   symbol (?data_ov019_021135d8@@3US3@@A). The ov019 per-symbol mount and this
   class's sinit both emit the SAME bytes at C linkage (_data_ov019_021135d8), so
   the two names are one object -- alias the C++ spelling onto the C one rather
   than let the src TU's declaration go unresolved. src is byte-matched decomp
   and is not edited; the alias is the same reading the ov019 chomp code alias in
   hal/bob_enemy_bridges.cpp takes. */
#pragma comment(linker, \
    "/alternatename:?data_ov019_021135d8@@3US3@@A=_data_ov019_021135d8")

extern "C" void hal_fill_ice_slide_manager_vtable(void)
{
    void **vt = (void **)_ZTV15IceSlideManager;
    vt[1]  = (void *)ccm_binit;
    vt[2]  = (void *)ccm_ainit;
    vt[4]  = (void *)ccm_bclean;
    vt[5]  = (void *)ccm_aclean;
    vt[7]  = (void *)ccm_bbeh;
    vt[8]  = (void *)ccm_abeh;
    vt[10] = (void *)ccm_bren;
    vt[11] = (void *)ccm_aren;
    vt[13] = (void *)ccm_trap13;
    vt[14] = (void *)ccm_trap14;
    vt[15] = (void *)ccm_heap;
    vt[18] = (void *)ccm_yoshi;
    vt[19] = (void *)ccm_trap19;
    /* the class's own and the base-body slots */
    vt[0]  = (void *)ism_init;
    vt[3]  = (void *)ccm_clean_base;
    vt[6]  = (void *)ism_behavior;
    vt[9]  = (void *)ccm_render_base;
    vt[12] = (void *)ccm_pdes;
    vt[16] = (void *)ism_d1;
    vt[17] = (void *)ccm_trap17;
}
