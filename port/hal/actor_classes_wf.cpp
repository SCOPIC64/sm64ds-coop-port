// GATE 60-63: the vtables of Whomp's Fortress' mechanical platforms (ov015).
//
// Same law as hal/actor_classes.cpp and hal/actor_classes_bob_enemy.cpp -- ROM
// slot order, __fastcall thunks that call QUALIFIED, unhosted slots trap by
// name -- and THIRTY-ONE SLOTS, because these are Actor subclasses through
// Platform (dBgActor_c), not the twenty-slot ActorBase shape.
//
// ---- THE ov015 NAME SHIFT --------------------------------------------------
//
// The config's class-to-vtable naming is shifted by one all through this
// overlay, the gate-20 shift again: PoleBillboard_Spawn installs the billboard
// BASE table (data_ov015_02114360), while _ZTV13PoleBillboard (0x02114420)
// carries the _ZN13PoleBillboard* methods and is what KnockDownPlank_Spawn
// installs, and so on down the file. So every row here is wired BY THE VTABLE
// ITS FACTORY ACTUALLY INSTALLS -- read out of relocs.txt at the factory's own
// vtable-store site -- never by the class name the SpawnInfo carries. The seven
// vtables and what each id's factory installs:
//
//   id 42 POLE_BILLBOARD       -> 0x02114360  (billboard base; slot 6 is the
//                                 arm9 Actor no-op, this platform does not tick)
//   id 44 KNOCK_DOWN_PLANK     -> 0x02114420  _ZTV13PoleBillboard / Botaosi_c
//   id 53 MOVING_BAR_BIG       -> 0x0211458c  _ZTV14KnockDownPlank / Dossunbar_c
//   id 54 MOVING_BAR_SMALL     -> 0x0211458c  (same table as BIG)
//   id 52 TOWER_STEP           -> 0x02114650  _ZTV14MovingBarSmall / Lift_c
//   id 51 ROTATING_BRIDGE      -> 0x02114714  _ZTV9TowerStep / Rotebar_c
//   id 50 ROTATING_PLATFORM_WF -> 0x021147e8  (unnamed; slots 6/9 are ov002's
//                                 shared Platform Behavior/Render)
//   id 45 FALL_BLOCK_WF        -> 0x021148dc  _ZTV11FallBlockWf (slots 6/9 are
//                                 ov098 cannon code -- see the gate-63 header)
//
// ---- THE VTABLES ARE HOST STORAGE ------------------------------------------
//
// All seven are excluded from the ov015 per-symbol mount (ov015_syms.txt) and
// declared here as host arrays the registry fills, the ov080 painting rule: a
// mounted vtable would hand a factory DS code addresses. Two are unnamed data
// symbols dsd left as plain data (0x02114360 and 0x021147e8); the rest carry a
// _ZTV name. Each class's own D1/D0 restores its vtable by RTTI name mid-
// teardown, so every _ZTV name that address answers to is aliased onto the one
// host array below.
//
// SLOT 30 does not trap here the way it does for the enemies: these are
// Platform subclasses whose Actor tail past slot 17 is entirely the arm9 base
// bodies (no OnAimedAtWithEggReturnVec override), and the mount leaves those
// pointing at arm9 anyway. The shared fill writes the ten ActorBase halves plus
// Actor's own tail from the same host functions the enemies use; slot 12 is
// ActorBase::OnPendingDestroy for all seven (their vtables' slot 12 is the arm9
// 0x02043ac0), which the shared fill already installs.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
int _ZN5Actor19BeforeInitResourcesEv(void *self);          /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a); /* slot 2 */
int _ZN5Actor14BeforeBehaviorEv(void *self);               /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);                 /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                /* slot 18 */
int _ZN5Actor9Virtual50Ev(void *self);                     /* slot 20 */
void _ZN5Actor15OnGroundPoundedERS_(void *self, void *o);  /* slot 21 */
void _ZN5Actor11OnAttacked1ERS_(void *self, void *o);      /* slot 22 */
void _ZN5Actor11OnAttacked2ERS_(void *self, void *o);      /* slot 23 */
void _ZN5Actor8OnKickedERS_(void *self, void *o);          /* slot 24 */
void _ZN5Actor8OnPushedERS_(void *self, void *o);          /* slot 25 */
void _ZN5Actor24OnHitByCannonBlastedCharERS_(void *self, void *o); /* 26 */
void _ZN5Actor15OnHitByMegaCharER6Player(void *self, void *p);     /* 27 */
void _ZN5Actor19OnHitFromUnderneathERS_(void *self, void *o);      /* 28 */

extern int data_02099f24[];          /* the frame phase the lists are in */
extern unsigned char data_020a4b4c;  /* the spawn spine's own step */
const char *port_actor_class_name(unsigned id);   /* hal/actor_registry */
void port_actor_render_probe(const char *cls, void *model); /* actor_classes */
}

// ---- the trap --------------------------------------------------------------
static void wf_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "FATAL: vtable slot %d is not hosted (actor id %u %s, "
                 "phase %d, spawn step %d)\n",
                 slot, id, port_actor_class_name(id), data_02099f24[0],
                 (int)data_020a4b4c);
    std::abort();
}
#define WF_TRAP(n) \
    static int __fastcall wf_trap##n(void *s, void *) \
    { wf_trap_report(s, n); return 0; }
WF_TRAP(13) WF_TRAP(14) WF_TRAP(17) WF_TRAP(19) WF_TRAP(30)
#undef WF_TRAP

static int __fastcall wf_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall wf_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall wf_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall wf_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall wf_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall wf_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall wf_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall wf_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall wf_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall wf_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall wf_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static int __fastcall wf_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall wf_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall wf_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall wf_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall wf_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall wf_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall wf_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall wf_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall wf_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }

/* The shared half of a 31-slot Platform table. A caller writes its own
   0/3/6/9/16, and slot 12 stays ActorBase::OnPendingDestroy (the ROM's own
   slot-12 target for all seven). Slot 17 traps: the ROM teardown dispatches
   slot 16 and Memory::Deallocate is done by ActorBase::AfterCleanupResources,
   so a call landing on 17 is worth an abort. */
static void wf_fill_shared(void **vt)
{
    vt[1] = (void *)wf_binit;
    vt[2] = (void *)wf_ainit;
    vt[4] = (void *)wf_bclean;
    vt[5] = (void *)wf_aclean;
    vt[7] = (void *)wf_bbeh;
    vt[8] = (void *)wf_abeh;
    vt[10] = (void *)wf_bren;
    vt[11] = (void *)wf_aren;
    vt[12] = (void *)wf_pdes;
    vt[13] = (void *)wf_trap13;
    vt[14] = (void *)wf_trap14;
    vt[15] = (void *)wf_heap;
    vt[17] = (void *)wf_trap17;
    vt[18] = (void *)wf_yoshi;
    vt[19] = (void *)wf_trap19;
    vt[20] = (void *)wf_v50;
    vt[21] = (void *)wf_pounded;
    vt[22] = (void *)wf_atk1;
    vt[23] = (void *)wf_atk2;
    vt[24] = (void *)wf_kicked;
    vt[25] = (void *)wf_pushed;
    vt[26] = (void *)wf_cannon;
    vt[27] = (void *)wf_mega;
    vt[28] = (void *)wf_under;
    vt[29] = (void *)wf_v50;    /* the Platform tail's own, per class below */
    vt[30] = (void *)wf_trap30;
}

// ============================================================================
// TOWER_STEP (id 52) -- vtable 0x02114650, the _ZN14MovingBarSmall* methods
// ============================================================================
//
// TowerStep_Spawn installs 0x02114650 (config _ZTV14MovingBarSmall /
// _ZTV14daObjBk_Lift_c), so an id-52 object runs MovingBarSmall's lifecycle.
// 916-byte object: Model at +0xd4, MovingMeshCollider at +0x124, ShadowModel at
// +0x320. Whomp's Fortress names several tower steps.
/* Init/Clean/Render are real C++ methods (the .cpp files define
   MovingBarSmall::<name>); Behavior and D1 are plain C in src. The thunks call
   each the way it is defined, the Tree/ArrowSign shape. */
#include "MovingBarSmall.h"
extern "C" {
int _ZN14MovingBarSmall13InitResourcesEv(char *self);  /* .cpp, but extern "C" */
int _ZN14MovingBarSmall8BehaviorEv(char *self);   /* .c, C linkage */
int *_ZN14MovingBarSmallD1Ev(int *self);          /* .c, C linkage */
void *_ZTV14MovingBarSmall[31];
}
/* The address 0x02114650 answers to both _ZTV names; the class D1 restores it
   by its RTTI name. */
#pragma comment(linker, "/alternatename:__ZTV14daObjBk_Lift_c=__ZTV14MovingBarSmall")
static int __fastcall ts_init(void *s, void *)
{ return _ZN14MovingBarSmall13InitResourcesEv((char *)s); }
static int __fastcall ts_clean(void *s, void *)
{ return ((MovingBarSmall *)s)->MovingBarSmall::CleanupResources(); }
static int __fastcall ts_behavior(void *s, void *)
{ return _ZN14MovingBarSmall8BehaviorEv((char *)s); }
static int __fastcall ts_render(void *s, void *)
{
    port_actor_render_probe("TOWER_STEP", (char *)s + 0xd4);
    return ((MovingBarSmall *)s)->MovingBarSmall::Render();
}
static int __fastcall ts_d1(void *s, void *)
{ return (int)(size_t)_ZN14MovingBarSmallD1Ev((int *)s); }
extern "C" void hal_fill_tower_step_vtable(void)
{
    void **vt = _ZTV14MovingBarSmall;
    wf_fill_shared(vt);
    vt[0] = (void *)ts_init;
    vt[3] = (void *)ts_clean;
    vt[6] = (void *)ts_behavior;
    vt[9] = (void *)ts_render;
    vt[16] = (void *)ts_d1;
}

// ============================================================================
// ROTATING_BRIDGE (id 51) -- vtable 0x02114714, the _ZN9TowerStep* methods
// ============================================================================
//
// RotatingBridge_Spawn installs 0x02114714 (config _ZTV9TowerStep /
// _ZTV17daObjBk_Rotebar_c), so an id-51 object runs TowerStep's lifecycle.
// 804-byte object: Model at +0xd4, MovingMeshCollider at +0x124. Whomp's
// Fortress names the rotating bridge.
/* TowerStep's Init/Clean/Behavior/Render are real C++ methods; only D1 is
   plain C in src. */
#include "TowerStep.h"
extern "C" {
int *_ZN9TowerStepD1Ev(int *self);                /* .c, C linkage */
void *_ZTV9TowerStep[31];
}
#pragma comment(linker, "/alternatename:__ZTV17daObjBk_Rotebar_c=__ZTV9TowerStep")
static int __fastcall rb_init(void *s, void *)
{ return ((TowerStep *)s)->TowerStep::InitResources(); }
static int __fastcall rb_clean(void *s, void *)
{ return ((TowerStep *)s)->TowerStep::CleanupResources(); }
static int __fastcall rb_behavior(void *s, void *)
{ return ((TowerStep *)s)->TowerStep::Behavior(); }
static int __fastcall rb_render(void *s, void *)
{
    port_actor_render_probe("ROTATING_BRIDGE", (char *)s + 0xd4);
    return ((TowerStep *)s)->TowerStep::Render();
}
static int __fastcall rb_d1(void *s, void *)
{ return (int)(size_t)_ZN9TowerStepD1Ev((int *)s); }
extern "C" void hal_fill_rotating_bridge_vtable(void)
{
    void **vt = _ZTV9TowerStep;
    wf_fill_shared(vt);
    vt[0] = (void *)rb_init;
    vt[3] = (void *)rb_clean;
    vt[6] = (void *)rb_behavior;
    vt[9] = (void *)rb_render;
    vt[16] = (void *)rb_d1;
}
