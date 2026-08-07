// GATES 70-74: Whomp's Fortress' level-7 enemies and platforms (ov084, ov091).
//
// Same law as hal/actor_classes_bob_enemy.cpp and hal/actor_classes_wf.cpp --
// ROM slot order, __fastcall thunks that call QUALIFIED, unhosted slots trap by
// name -- and the classes here are Actor subclasses through Enemy (the two
// piranhas) or through Platform (the thwomp and the two platforms), so their
// vtables run past ActorBase's eighteen: the piranhas are 31-slot Enemy tables,
// the platforms 32-slot Platform tables whose slot 31 is Platform::Kill.
//
// ov084 carries GOOMBA and BOB_OMB_BUDDY already (gate 32); the two piranhas
// share the same overlay and cost no new mount. ov091 was mounted at gate 32
// for the chomp's post (STUMP, actor 27); the thwomp and the two platforms
// share it. Both overlays' per-symbol mounts are already in PORT_ACTOR_OVERLAYS
// and both SpawnInfo runs are already in ov084_syms.txt / ov091_syms.txt.
//
// Every id here was resolved id->class->overlay from the arm9 reloc table: each
// SpawnInfo's +4 halfword is the id (the registry cross-checks it), and each
// factory's own vtable-store site names the table the class runs, read out of
// relocs.txt rather than the class name the config carries (the ov015 name
// shift shows up twice more below: FIRE_PIRANHA_PLANT runs the
// FirePiranhaPlantBig table, ROTATING_UP_DOWN_PLATFORM the
// RotatingUpDownPlatformUtm one).
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
/* the ten shared lifecycle halves, the same functions every enemy/platform
   fill in this port writes */
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
void hal_fill_platform_vtable(void);              /* the dBgActor_c base table */
void port_enemy_death_states_seat(void);          /* the Enemy death table */
}

// ---- the trap --------------------------------------------------------------
static void l7_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "FATAL: vtable slot %d is not hosted (actor id %u %s, "
                 "phase %d, spawn step %d)\n",
                 slot, id, port_actor_class_name(id), data_02099f24[0],
                 (int)data_020a4b4c);
    std::abort();
}
#define L7_TRAP(n) \
    static int __fastcall l7_trap##n(void *s, void *) \
    { l7_trap_report(s, n); return 0; }
L7_TRAP(13) L7_TRAP(14) L7_TRAP(16) L7_TRAP(17) L7_TRAP(19) L7_TRAP(30)
#undef L7_TRAP

static int __fastcall l7_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall l7_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall l7_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall l7_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall l7_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall l7_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall l7_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall l7_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall l7_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall l7_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall l7_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static int __fastcall l7_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall l7_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall l7_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall l7_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall l7_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall l7_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall l7_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall l7_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall l7_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }

/* The shared half of a 31/32-slot Actor table: Actor's four Before/After pairs,
   ActorBase::OnHeapCreated, Actor::OnYoshiTryEat, Virtual50 and the eight combat
   hooks, plus the traps. A caller writes its own 0/3/6/9/16/17 and whichever of
   12/18/19/27/29/31 it overrides. */
static void l7_fill_shared(void **vt)
{
    vt[1] = (void *)l7_binit;
    vt[2] = (void *)l7_ainit;
    vt[4] = (void *)l7_bclean;
    vt[5] = (void *)l7_aclean;
    vt[7] = (void *)l7_bbeh;
    vt[8] = (void *)l7_abeh;
    vt[10] = (void *)l7_bren;
    vt[11] = (void *)l7_aren;
    vt[12] = (void *)l7_pdes;
    vt[13] = (void *)l7_trap13;
    vt[14] = (void *)l7_trap14;
    vt[15] = (void *)l7_heap;
    vt[16] = (void *)l7_trap16;
    vt[17] = (void *)l7_trap17;
    vt[18] = (void *)l7_yoshi;
    vt[19] = (void *)l7_trap19;
    vt[20] = (void *)l7_v50;
    vt[21] = (void *)l7_pounded;
    vt[22] = (void *)l7_atk1;
    vt[23] = (void *)l7_atk2;
    vt[24] = (void *)l7_kicked;
    vt[25] = (void *)l7_pushed;
    vt[26] = (void *)l7_cannon;
    vt[27] = (void *)l7_mega;
    vt[28] = (void *)l7_under;
    vt[29] = (void *)l7_v50;    /* overwritten where a class has its own */
    vt[30] = (void *)l7_trap30;
}

// ============================================================================
// THWOMP (actor 161, ov091) -- vtable _ZTV6Thwomp (ov091 0x02135174)
// ============================================================================
//
// _ZTV6Thwomp / _ZTV7daDsn_c, a THIRTY-TWO slot Platform table: slot 31 is
// _ZN8Platform4KillEv (ov002 0x020ee55c, already in the build). Thwomp_Spawn
// (ov091 0x02132cb8) installs it, and its own +4 halfword reads 161. A 932-byte
// object with a Model at +0xd4, a MovingMeshCollider at +0x124, a
// TextureSequence at +0x324 and a ShadowModel at +0x338. Whomp's Fortress names
// two thwomps in its default object table.
//
// THE FACTORY LEAVES THE VPTR ON A PLACEHOLDER. Thwomp_Spawn's last line stores
// VT1 -- the recovered source's byte-matched wildcard for the relocated word
// the ROM points at _ZTV6Thwomp -- and VT1 is auto_bss's zeroed [8] array, so a
// raw spawn dispatches through nulls. The host factory calls the real spawn and
// reseats slot 0 onto the class's host table, the RotatingPlatformWf treatment.
// Its earlier `_ZTV7daDsn_c` store is the daDsnBase_c base table
// data_ov091_021351fc, which the ov091 mount carries; the alias below points the
// wildcard name at it.
//
// Its own slots past 17 are slot 27 (func_ov091_02132a14, OnHitByMegaChar) and
// slot 29 (func_ov091_02132a0c, OnAimedAtWithEgg). Init/Clean/Behavior/Render
// are real C++ methods; D1/D0 are plain C.
#include "Thwomp.h"
extern "C" {
int _ZN6Thwomp13InitResourcesEv(void *self);       /* face: below */
int _ZN6Thwomp16CleanupResourcesEv(void *self);    /* face: below */
int _ZN6Thwomp8BehaviorEv(void *self);             /* face: below */
int _ZN6Thwomp6RenderEv(void *self);               /* face: below */
int *_ZN6ThwompD1Ev(int *self);                    /* .c, C linkage */
int *_ZN6ThwompD0Ev(int *self);                    /* .c, C linkage */
void func_ov091_02132a14(void *self, void *o);     /* slot 27, its own */
int func_ov091_02132a0c(void *self);               /* slot 29, its own */
void _ZN8Platform4KillEv(void *self);              /* slot 31 */
void *_ZTV6Thwomp[32];
extern unsigned char data_ov091_021351fc[];        /* daDsnBase_c base table */
int *Thwomp_Spawn(void);
}
/* The class D1/D0 spell the base table by the recovered wildcard name; point it
   at the mounted daDsnBase_c table. */
#pragma comment(linker, "/alternatename:__ZTV7daDsn_c=_data_ov091_021351fc")

/* Thwomp_Spawn leaves the vptr on VT1; reseat it onto the host table. */
extern "C" void *port_factory_thwomp(void)
{
    void *p = Thwomp_Spawn();
    if (p)
        *(void **)p = (void *)_ZTV6Thwomp;
    return p;
}

static int __fastcall thw_init(void *s, void *)
{ return _ZN6Thwomp13InitResourcesEv(s); }
static int __fastcall thw_clean(void *s, void *)
{ return _ZN6Thwomp16CleanupResourcesEv(s); }
static int __fastcall thw_behavior(void *s, void *)
{ return _ZN6Thwomp8BehaviorEv(s); }
static int __fastcall thw_render(void *s, void *)
{ port_actor_render_probe("THWOMP", (char *)s + 0xd4);
  return _ZN6Thwomp6RenderEv(s); }
/* SLOT 16 IS LIVE: a thwomp does not destroy itself on this level, but the
   cleanup pass dispatches D1 when the level tears down, so both are the class's
   own. */
static int __fastcall thw_d1(void *s, void *)
{ return (int)(size_t)_ZN6ThwompD1Ev((int *)s); }
static int __fastcall thw_d0(void *s, void *)
{ return (int)(size_t)_ZN6ThwompD0Ev((int *)s); }
static int __fastcall thw_mega(void *s, void *, void *o)
{ func_ov091_02132a14(s, o); return 0; }
static int __fastcall thw_aimed(void *s, void *)
{ return func_ov091_02132a0c(s); }
static int __fastcall thw_kill(void *s, void *)
{ _ZN8Platform4KillEv(s); return 0; }

extern "C" void hal_fill_thwomp_vtable(void)
{
    void **vt = _ZTV6Thwomp;
    hal_fill_platform_vtable();
    l7_fill_shared(vt);
    vt[0] = (void *)thw_init;
    vt[3] = (void *)thw_clean;
    vt[6] = (void *)thw_behavior;
    vt[9] = (void *)thw_render;
    vt[16] = (void *)thw_d1;
    vt[17] = (void *)thw_d0;
    vt[27] = (void *)thw_mega;
    vt[29] = (void *)thw_aimed;
    vt[31] = (void *)thw_kill;
}

/* ---- method faces ---------------------------------------------------------
   Thwomp's Init/Clean/Behavior/Render are real MSVC methods against include/. */
extern "C" {
int _ZN6Thwomp13InitResourcesEv(void *self)
{ return ((Thwomp *)self)->Thwomp::InitResources(); }
int _ZN6Thwomp16CleanupResourcesEv(void *self)
{ return ((Thwomp *)self)->Thwomp::CleanupResources(); }
int _ZN6Thwomp8BehaviorEv(void *self)
{ return ((Thwomp *)self)->Thwomp::Behavior(); }
int _ZN6Thwomp6RenderEv(void *self)
{ return ((Thwomp *)self)->Thwomp::Render(); }
}

// ============================================================================
// SLIDING_PLATFORM_WF (actor 55, ov091) -- _ZTV17SlidingPlatformWf (0x02135080)
// ============================================================================
//
// SlidingPlatformWf_Spawn (ov091 0x021327e8) installs 0x02135080 cleanly and its
// own +4 halfword reads 55. A THIRTY-TWO slot Platform table whose slot 31 is
// Platform::Kill (ov002 0x020ee55c). An 816-byte object with a Model at +0xd4
// and a MovingMeshCollider at +0x124; Whomp's Fortress names three of these.
//
// It overrides nothing past slot 17: the whole class is the six lifecycle slots
// and Kill. Init/Clean/Behavior/Render/D1/D0 are all matched src -- Init/Clean/
// Behavior/Render real C++ methods, the two destructors plain C -- and its
// Render dispatches Model slot 5 through a `void m(int)` shadow, which
// _ZTV5Model is dual-filled for, so it serves from src unchanged.
#include "SlidingPlatformWf.h"
extern "C" {
int _ZN17SlidingPlatformWf13InitResourcesEv(void *self);    /* face: below */
int _ZN17SlidingPlatformWf16CleanupResourcesEv(void *self); /* face: below */
int _ZN17SlidingPlatformWf8BehaviorEv(void *self);          /* face: below */
int _ZN17SlidingPlatformWf6RenderEv(void *self);            /* face: below */
int *_ZN17SlidingPlatformWfD1Ev(int *self);                 /* .c, C linkage */
int *_ZN17SlidingPlatformWfD0Ev(int *self);                 /* .c, C linkage */
void *_ZTV17SlidingPlatformWf[32];
}
static int __fastcall swf_init(void *s, void *)
{ return _ZN17SlidingPlatformWf13InitResourcesEv(s); }
static int __fastcall swf_clean(void *s, void *)
{ return _ZN17SlidingPlatformWf16CleanupResourcesEv(s); }
static int __fastcall swf_behavior(void *s, void *)
{ return _ZN17SlidingPlatformWf8BehaviorEv(s); }
static int __fastcall swf_render(void *s, void *)
{ port_actor_render_probe("SLIDING_PLATFORM_WF", (char *)s + 0xd4);
  return _ZN17SlidingPlatformWf6RenderEv(s); }
static int __fastcall swf_d1(void *s, void *)
{ return (int)(size_t)_ZN17SlidingPlatformWfD1Ev((int *)s); }
static int __fastcall swf_d0(void *s, void *)
{ return (int)(size_t)_ZN17SlidingPlatformWfD0Ev((int *)s); }
static int __fastcall swf_kill(void *s, void *)
{ _ZN8Platform4KillEv(s); return 0; }

extern "C" void hal_fill_sliding_platform_wf_vtable(void)
{
    void **vt = _ZTV17SlidingPlatformWf;
    hal_fill_platform_vtable();
    l7_fill_shared(vt);
    vt[0] = (void *)swf_init;
    vt[3] = (void *)swf_clean;
    vt[6] = (void *)swf_behavior;
    vt[9] = (void *)swf_render;
    vt[16] = (void *)swf_d1;
    vt[17] = (void *)swf_d0;
    vt[31] = (void *)swf_kill;
}

extern "C" {
int _ZN17SlidingPlatformWf13InitResourcesEv(void *self)
{ return ((SlidingPlatformWf *)self)->SlidingPlatformWf::InitResources(); }
int _ZN17SlidingPlatformWf16CleanupResourcesEv(void *self)
{ return ((SlidingPlatformWf *)self)->SlidingPlatformWf::CleanupResources(); }
int _ZN17SlidingPlatformWf8BehaviorEv(void *self)
{ return ((SlidingPlatformWf *)self)->SlidingPlatformWf::Behavior(); }
int _ZN17SlidingPlatformWf6RenderEv(void *self)
{ return ((SlidingPlatformWf *)self)->SlidingPlatformWf::Render(); }
}
