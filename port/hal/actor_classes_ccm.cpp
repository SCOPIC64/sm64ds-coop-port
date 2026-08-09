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

// ============================================================================
// GATE 190: CCM slice A -- ICE_SHEET (295) + POWER_STAR_CREATE (355), ov018's
// own overlay (per-symbol mount, port/ov018_syms.txt), plus ONE_UP_LOGO (331)
// on the already-mounted ov002. Same law as every fill above -- ROM slot
// order, __fastcall thunks that call QUALIFIED or the class's own C body,
// unhosted slots trap by name.
//
// ---- ICE_SHEET (295) -- a Platform (daObjIceBoard_c), 32 slots -----------
//
// _ZTV8IceSheet (ov018 0x02113b34) read with its relocations applied:
//   0  InitResources    0x021129c0  own (_ZN8IceSheet13InitResourcesEv)
//   3  CleanupResources 0x02112924  own
//   6  Behavior         0x02112990  own
//   9  Render           0x02112968  own -- NOT a slot-5 shadow-class
//                                    collision (calls Model::Render by
//                                    C name the way JRB's Unagi does),
//                                    verified per the task's trap list
//  12  OnPendingDestroy 0x02043ac0  ActorBase's base body (shared)
//  16  D1               0x021127bc  own (_ZN8IceSheetD1Ev)
//  17  D0               0x02112800  own (_ZN8IceSheetD0Ev)
//  21  OnGroundPounded  0x021128e0  own (func_ov018_021128e0) -- dispatches
//                                    slot 31 (Kill) VIRTUALLY through a local
//                                    shadow struct (the gate-172 Crate shape:
//                                    "this only" signature, calls c->f7c());
//                                    works unmodified once slot 31 is filled,
//                                    because the shadow struct is only used
//                                    for its ABI shape -- the real installed
//                                    vtable pointer is what actually dispatches.
//  27  OnHitByMegaChar  0x02112858  own (func_ov018_02112858) -- same shadow-
//                                    vtable-call shape, dispatches its OWN
//                                    slot 31 too (c->m(), the 31st shadow
//                                    virtual after v0..v30).
//  31  Kill             0x02112880  own (func_ov018_02112880) -- plays a
//                                    sound + 3 particles, then
//                                    MarkForDestruction. Platform's own tail
//                                    slot past Actor's 31.
// Every other slot (1/2/4/5/7/8/10/11/13/14/15/18/19/20/22/23/24/25/26/28/
// 29/30) is the shared Actor/ActorBase half ccm190_fill_shared (below, this
// TU's own copy of the JRB Platform recipe) writes.
//
// D1/D0 spell _ZTV8IceSheet then _ZTV8Platform (VT0/VT1, both REAL base
// tables, NOT the ov016 dBgActor_c/daObjKi_* SHARED PLACEHOLDER -- verified
// against src: main's post-#1083 rename replaced the stale G0/G1 spellings
// with IceSheet_ModelFile/IceSheet_ClsnFile, a per-class bss pair, not a
// cross-class alias). So BOTH stay in the slice; no host thunk needed, the
// Unagi/RotatingFirebar/LavaBubble treatment.
//
// InitResources reads the raw data_ov018_02113c84/c7c names (mounted in
// port/ov018_syms.txt); CleanupResources (carried from main post-#1083)
// reads the SAME bytes through the IceSheet_ModelFile/IceSheet_ClsnFile
// aliases -- one bss pair, two C spellings, aliased below.
//
// ---- POWER_STAR_CREATE (355) -- an ActorBase-only shape, 18 slots --------
//
// data_ov018_02113a74 (PowerStarCreate_SpawnInfo+0x24, ov018) read with its
// relocations applied:
//   0  InitResources    0x02043c80  ActorBase::InitResources, the SHARED
//                                    base default (does nothing) -- the
//                                    class overrides no Init at all
//   3  CleanupResources 0x02043bf0  ActorBase's base body (shared)
//   6  Behavior         0x02112730  own (func_ov018_02112730)
//   9  Render           0x02043af0  ActorBase::Render, a no-op base body
//  12  OnPendingDestroy 0x02043ac0  ActorBase's base body (shared)
//  16  D1               0x021126d4  own (func_ov018_021126d4)
//  17  D0               0x021126f8  own (func_ov018_021126f8) -- main's
//                                    "recovered name: daSCre_c_OnYoshiTryEat"
//                                    comment on this file is a NAME DECOY:
//                                    the reloc slot is 17 (D0's ROM position)
//                                    and the body's shape (store the class's
//                                    OWN vtable, Actor::D2, then
//                                    Memory::Deallocate) is D0's, byte-
//                                    identical to IceSheet's own D0 shape.
//                                    The table is only 18 words (0x02113a74
//                                    .. 0x02113ab8 inclusive; 0x02113abc is
//                                    _ZTI15daObjIceBoard_c, IceSheet's own
//                                    RTTI, a disjoint record) -- there is no
//                                    slot 18 for OnYoshiTryEat to occupy, so
//                                    the class does not override it. The
//                                    gate-178 Amilift check: the installed
//                                    +0x24 vtable is verified against the
//                                    factory's own p[0] write (below), not
//                                    trusted from the src comment alone.
//
// PowerStarCreate_Spawn's own body confirms the vtable base: it allocates
// 212 bytes, runs Actor::C2 (an Actor, not a bare ActorBase -- MarkFor-
// Destruction and the 212-byte size both fit ActorBase::new's accounting)
// and writes `p[0] = (int)data_ov018_02113a74` -- exactly the address this
// fill installs. No reseat wrapper needed.
//
// Its Behavior (func_ov018_02112730.cpp) spawns actor 0xb2 (178, POWER_STAR)
// within 0x64000 of the camera-tracked player then calls
// ActorBase::MarkForDestruction on itself -- so a spawned PowerStarCreate is
// expected to vanish from the next frame's live-actor census after handing
// off to its star, the census delta the verify pass checks for.
//
// ---- ONE_UP_LOGO (331) -- a plain Actor, 31 slots (no Kill) ---------------
//
// _ZTV9OneUpLogo (ov002 0x0210b1ac) read with its relocations applied:
//   0  InitResources    0x020f107c  own
//   3  CleanupResources 0x020f0e78  own
//   6  Behavior         0x020f0f08  own
//   9  Render           0x020f0ea8  own -- NOT a slot-5 collision (no local
//                                    shadow class in src; TextureSequence
//                                    Prepare/SetFile own the model directly)
//  12  OnPendingDestroy 0x02043ac0  ActorBase's base body (shared)
//  16  D1               0x020f0dd0  own -- real MSVC-synthesised member dtor
//                                    chain (TextureSequence + Model) over a
//                                    LOCAL shadow class, not a placeholder
//                                    write -- NOT compiled (the gate-31
//                                    PeachPainting D1 recipe); the fill
//                                    spells the chain directly instead
//  17  D0               0x020f0e08  own -- spells _ZTV14daObj1UpLogo_c, an
//                                    RTTI alias with exactly ONE speller (not
//                                    the ov016 196-speller _ZTV10dBgActor_c
//                                    shape), so it is this class's own real
//                                    table under another name, not a shared
//                                    placeholder -- stays in the slice
// All Actor-tail slots (18 OnYoshiTryEat .. 29 OnAimedAtWithEgg) are the
// plain shared defaults, confirmed by relocs -- OneUpLogo overrides none of
// them (Mario eating/attacking a 1-Up logo behaves like any undecorated
// Actor). No Kill (31 slots, not 32 -- OneUpLogo is Actor-derived, not
// Platform-derived; the 0x0209a764 word right past slot 30 is the START of
// the next record, RTTI data, confirmed against relocs.txt).
extern "C" {
void port_actor_render_probe(const char *cls, void *model); /* hal/actor_classes.cpp */
/* the Actor-tail shared halves ccm190_fill_shared (below) binds at 18..29 for
   both ICE_SHEET and ONE_UP_LOGO -- this TU's first declaration of the
   31-slot Actor tail (IceSlideManager, above, is a plain 20-slot ActorBase
   shape and never reaches past 19). */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                        /* 18 */
void _ZN5Actor13OnTurnIntoEggER6Player(void *self, void *p);       /* 19 */
int _ZN5Actor9Virtual50Ev(void *self);                             /* 20 */
void _ZN5Actor15OnGroundPoundedERS_(void *self, void *o);          /* 21 */
void _ZN5Actor11OnAttacked1ERS_(void *self, void *o);              /* 22 */
void _ZN5Actor11OnAttacked2ERS_(void *self, void *o);              /* 23 */
void _ZN5Actor8OnKickedERS_(void *self, void *o);                  /* 24 */
void _ZN5Actor8OnPushedERS_(void *self, void *o);                  /* 25 */
void _ZN5Actor24OnHitByCannonBlastedCharERS_(void *self, void *o); /* 26 */
void _ZN5Actor15OnHitByMegaCharER6Player(void *self, void *p);     /* 27 */
void _ZN5Actor19OnHitFromUnderneathERS_(void *self, void *o);      /* 28 */
int _ZN5Actor16OnAimedAtWithEggEv(void *self);                     /* 29 */

/* ---- ICE_SHEET's own bodies (all matched src) ---------------------------- */
int _ZN8IceSheet13InitResourcesEv(void *self);
int _ZN8IceSheet16CleanupResourcesEv(void *self);
int _ZN8IceSheet8BehaviorEv(void *self);
int _ZN8IceSheet6RenderEv(void *self);
int *_ZN8IceSheetD1Ev(void *self);
int *_ZN8IceSheetD0Ev(void *self);
void func_ov018_021128e0(void *self, void *a);   /* slot 21, OnGroundPounded */
void func_ov018_02112858(void *self, int a);     /* slot 27, OnHitByMegaChar */
void func_ov018_02112880(void *self);            /* slot 31, Kill */
void *IceSheet_Spawn(void);                      /* installs _ZTV8IceSheet itself */
int _ZTV8IceSheet[32];
/* Platform's OWN base table -- IceSheet's carried D1/D0 (src/_ZN8IceSheetD1Ev.c,
   src/_ZN8IceSheetD0Ev.c) each `extern int _ZTV8Platform[]` and store it as an
   intermediate vptr write. Already defined as a real host array in
   hal/actor_vtables.cpp (`void *_ZTV8Platform[20]`) -- declared extern here
   (not redefined) so this TU only REFERENCES it, the usual second-write a
   Platform-derived class's D1/D0 takes. */
extern int _ZTV8Platform[];

/* ---- POWER_STAR_CREATE's own bodies (all matched src) -------------------- */
int func_ov018_02112730(void *self);         /* slot 6, Behavior */
int func_ov018_021126d4(void *self);         /* slot 16, D1 */
int *func_ov018_021126f8(void *self);        /* slot 17, D0 (the name decoy) */
int *PowerStarCreate_Spawn(void);            /* installs data_ov018_02113a74 */
int data_ov018_02113a74[18];                 /* the unnamed vtable, host array */
}
/* func_ov018_02112730.cpp (Behavior) declares its OWN local `struct Actor {
   static int Spawn(unsigned, unsigned, const Vector3&, const Vector3_16*,
   signed char, short); };` -- a DIFFERENT overload from the (unsigned,
   unsigned, Vector3 const&, Vector3_16 const*, int, int) one
   hal/cxx_aliases.cpp / hal/actor_faces_bob.cpp already alias (mangled ...HH@Z,
   this one mangles ...CF@Z for the signed-char/short tail -- confirmed
   against the exact LNK2019 text, not guessed). Same real ROM function
   (Actor::Spawn is cdecl static, no `this` to lose), so a second alias onto
   the SAME extern-C body, the Enemy::SpawnCoin precedent applied to a second
   parameter-type spelling instead of a second declaring TU. */
#pragma comment(linker, "/alternatename:?Spawn@Actor@@SAHIIABUVector3@@PBUVector3_16@@CF@Z=__ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii")
extern "C" {

/* ---- ONE_UP_LOGO's own bodies (all matched src) --------------------------- */
int _ZN9OneUpLogo13InitResourcesEv(void *self);
int _ZN9OneUpLogo16CleanupResourcesEv(void *self);
int _ZN9OneUpLogo8BehaviorEv(void *self);
int _ZN9OneUpLogo6RenderEv(void *self);
int *_ZN9OneUpLogoD0Ev(void *self);
void _ZN5ModelD1Ev(void *self);              /* Model member at +0xd4 */
void _ZN15TextureSequenceD1Ev(void *self);   /* TextureSequence member at +0x124 */
void *OneUpLogo_Spawn(void);                 /* installs _ZTV9OneUpLogo itself */
int _ZTV9OneUpLogo[31];
int _ZTV14daObj1UpLogo_c[];   /* RTTI alias, ONE speller (_ZN9OneUpLogoD0Ev),
                                  not a shared placeholder -- aliased below. */
}

/* IceSheet::CleanupResources (carried from main post-#1083) reads the same
   bss pair InitResources does (data_ov018_02113c84/c7c, mounted in
   port/ov018_syms.txt) through the renamed aliases. One object, two C names;
   alias rather than edit the matched src, the OneUpLogo/daObj1UpLogo_c
   treatment applied to a bss pair instead of a vtable. */
/* IceSheet_ModelFile/IceSheet_ClsnFile are declared at FILE SCOPE in
   src/_ZN8IceSheet16CleanupResourcesEv.cpp (`extern int IceSheet_ClsnFile[];`),
   not through decl_common.h's extern "C" umbrella, so MSVC C++-mangles them
   (?IceSheet_ModelFile@@3PAHA / ?IceSheet_ClsnFile@@3PAHA) -- confirmed
   against the exact LNK2019 text, not guessed. */
#pragma comment(linker, "/alternatename:?IceSheet_ModelFile@@3PAHA=_data_ov018_02113c84")
#pragma comment(linker, "/alternatename:?IceSheet_ClsnFile@@3PAHA=_data_ov018_02113c7c")
/* OneUpLogo's own D0 spells its table by this RTTI name (one speller only). */
#pragma comment(linker, "/alternatename:__ZTV14daObj1UpLogo_c=__ZTV9OneUpLogo")

// ---- the shared Actor-tail half (18..30), reused by ICE_SHEET and
// ONE_UP_LOGO -- this TU's own copy (hal/actor_classes_jrb.cpp's
// jrb_fill_shared_0_30 is `static`, a different TU, not linkable here). -----
static int __fastcall ccm190_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static int __fastcall ccm190_egg(void *s, void *, void *p)
{ _ZN5Actor13OnTurnIntoEggER6Player(s, p); return 0; }
static int __fastcall ccm190_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall ccm190_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall ccm190_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall ccm190_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall ccm190_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall ccm190_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall ccm190_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall ccm190_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall ccm190_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }
static int __fastcall ccm190_aimed(void *s, void *)
{ return _ZN5Actor16OnAimedAtWithEggEv(s); }
static int __fastcall ccm190_trap30(void *s, void *)
{ ccm_trap_report(s, 30); return 0; }

/* Fills slots 1..15 and 18..30 with the shared Actor/ActorBase halves; the
   caller writes its own 0/3/6/9/12/16/17 and (Platform-derived) 31. Slots
   21/27/29 are overwritten again by a caller that overrides them (IceSheet
   does 21/27; neither class here overrides 29). */
static void ccm190_fill_shared(void **vt)
{
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
    vt[18] = (void *)ccm190_yoshi;
    vt[19] = (void *)ccm190_egg;
    vt[20] = (void *)ccm190_v50;
    vt[21] = (void *)ccm190_pounded;
    vt[22] = (void *)ccm190_atk1;
    vt[23] = (void *)ccm190_atk2;
    vt[24] = (void *)ccm190_kicked;
    vt[25] = (void *)ccm190_pushed;
    vt[26] = (void *)ccm190_cannon;
    vt[27] = (void *)ccm190_mega;
    vt[28] = (void *)ccm190_under;
    vt[29] = (void *)ccm190_aimed;
    vt[30] = (void *)ccm190_trap30;
}

// ---- ICE_SHEET fill (Platform, 32 slots) -----------------------------------
static int __fastcall ics_init(void *s, void *)
{ return _ZN8IceSheet13InitResourcesEv(s); }
static int __fastcall ics_clean(void *s, void *)
{ return _ZN8IceSheet16CleanupResourcesEv(s); }
static int __fastcall ics_behavior(void *s, void *)
{ return _ZN8IceSheet8BehaviorEv(s); }
static int __fastcall ics_render(void *s, void *)
{ port_actor_render_probe("ICE_SHEET", (char *)s + 0xd4);
  return _ZN8IceSheet6RenderEv(s); }
static int __fastcall ics_d1(void *s, void *)
{ return (int)(size_t)_ZN8IceSheetD1Ev(s); }
static int __fastcall ics_d0(void *s, void *)
{ return (int)(size_t)_ZN8IceSheetD0Ev(s); }
static int __fastcall ics_pounded(void *s, void *, void *o)
{ func_ov018_021128e0(s, o); return 0; }
static int __fastcall ics_mega(void *s, void *, void *p)
{ func_ov018_02112858(s, (int)(size_t)p); return 0; }
static int __fastcall ics_kill(void *s, void *)
{ func_ov018_02112880(s); return 0; }

extern "C" void hal_fill_ice_sheet_vtable(void)
{
    void **vt = (void **)_ZTV8IceSheet;
    ccm190_fill_shared(vt);
    vt[0]  = (void *)ics_init;
    vt[3]  = (void *)ics_clean;
    vt[6]  = (void *)ics_behavior;
    vt[9]  = (void *)ics_render;
    vt[16] = (void *)ics_d1;
    vt[17] = (void *)ics_d0;
    vt[21] = (void *)ics_pounded;   /* own OnGroundPounded, overrides the shared default */
    vt[27] = (void *)ics_mega;      /* own OnHitByMegaChar, overrides the shared default */
    vt[31] = (void *)ics_kill;      /* own Kill (Platform's tail slot) */
}

// ---- POWER_STAR_CREATE fill (ActorBase-only, 18 slots) --------------------
//
// Slots 0/3/9/12 take the SHARED base bodies directly (ActorBase::
// InitResources/CleanupResources/Render/OnPendingDestroy) since the class
// overrides none of them -- confirmed by reloc target address, not assumed.
/* ActorBase::InitResources is deliberately NOT a real qualified method
   anywhere in this build (src/_ZN9ActorBase13InitResourcesEv.cpp's own header
   comment: it is ActorBase's KEY FUNCTION -- the first virtual CW would emit
   the class's vtable into, and the ROM already supplies that vtable as data,
   so a real method definition would multiply-define it at ROM link time). It
   is a plain extern-C `s32(void)` with no `this` at all -- takes no args,
   confirmed against src (`return 1;`, VS_FAIL, unconditionally). */
extern "C" int _ZN9ActorBase13InitResourcesEv(void);
static int __fastcall psc_init_base(void *, void *)
{ return _ZN9ActorBase13InitResourcesEv(); }
static int __fastcall psc_clean_base(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::CleanupResources(); }
static int __fastcall psc_render_base(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::Render(); }
static int __fastcall psc_behavior(void *s, void *)
{ return func_ov018_02112730(s); }
static int __fastcall psc_d1(void *s, void *)
{ return func_ov018_021126d4(s); }
static int __fastcall psc_d0(void *s, void *)
{ return (int)(size_t)func_ov018_021126f8(s); }

extern "C" void hal_fill_power_star_create_vtable(void)
{
    void **vt = (void **)data_ov018_02113a74;
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
    vt[0]  = (void *)psc_init_base;
    vt[3]  = (void *)psc_clean_base;
    vt[6]  = (void *)psc_behavior;
    vt[9]  = (void *)psc_render_base;
    vt[12] = (void *)ccm_pdes;
    vt[16] = (void *)psc_d1;
    vt[17] = (void *)psc_d0;
    /* no slot 18+: the table is 18 words, ends here */
}

// ---- ONE_UP_LOGO fill (Actor, 31 slots, no Kill) ---------------------------
static int __fastcall oul_init(void *s, void *)
{ return _ZN9OneUpLogo13InitResourcesEv(s); }
static int __fastcall oul_clean(void *s, void *)
{ return _ZN9OneUpLogo16CleanupResourcesEv(s); }
static int __fastcall oul_behavior(void *s, void *)
{ return _ZN9OneUpLogo8BehaviorEv(s); }
static int __fastcall oul_render(void *s, void *)
{ port_actor_render_probe("ONE_UP_LOGO", (char *)s + 0xd4);
  return _ZN9OneUpLogo6RenderEv(s); }
/* slot 16: src/_ZN9OneUpLogoD1Ev.cpp is a real MSVC-synthesised destructor
   (`OneUpLogo::~OneUpLogo(){}`) over a LOCAL shadow class (Actor/Model/
   TextureSequence declared with no bodies of their own), so it is not
   compiled -- the gate-31 recipe hal/actor_classes.cpp's PeachPainting D1
   (pt_d1) already takes for the identical shape. The chain is spelled here
   instead: TextureSequence at +0x124, Model at +0xd4 (HIGH ADDRESS FIRST,
   the destructor-order convention every other fill in this file follows),
   then Actor's own D2. No vtable-store write -- OneUpLogo's D1/D0 never
   overwrite the vptr with a placeholder (confirmed against src: D0 stores
   _ZTV14daObj1UpLogo_c, its OWN table, not a shared one), so nothing to
   redo here that D0's real body does not already do itself. */
static int __fastcall oul_d1(void *s, void *)
{
    char *t = (char *)s;
    _ZN15TextureSequenceD1Ev(t + 0x124);
    _ZN5ModelD1Ev(t + 0xd4);
    _ZN5ActorD2Ev(t);
    return (int)(size_t)s;
}
static int __fastcall oul_d0(void *s, void *)
{ return (int)(size_t)_ZN9OneUpLogoD0Ev(s); }

/* OneUpLogo overrides none of the Actor-tail slots (18..30) -- confirmed by
   reloc target address, every one of them the plain shared default -- so
   ccm190_fill_shared covers 18..30 unmodified, no per-class override after. */
extern "C" void hal_fill_one_up_logo_vtable(void)
{
    void **vt = (void **)_ZTV9OneUpLogo;
    ccm190_fill_shared(vt);
    vt[0]  = (void *)oul_init;
    vt[3]  = (void *)oul_clean;
    vt[6]  = (void *)oul_behavior;
    vt[9]  = (void *)oul_render;
    vt[12] = (void *)ccm_pdes;
    vt[16] = (void *)oul_d1;
    vt[17] = (void *)oul_d0;
    /* no slot 31: OneUpLogo is Actor-derived, not Platform-derived -- 31 slots total */
}

// ---- method faces -----------------------------------------------------------
// The C-named references the thunks above take onto the real MSVC methods
// against include/, the hal/actor_classes_jrb.cpp recipe. IceSheet's four
// (Init/Cleanup/Behavior/Render) and OneUpLogo's two (Init/Render) are
// `Class::Method` .cpp definitions (MSVC mangles them off the Itanium name:
// confirmed against dumpbin -- the .obj exports ?InitResources@IceSheet@@...,
// not __ZN8IceSheet13InitResourcesEv), so they are faced here. IceSheet's
// D1/D0 and OneUpLogo's Cleanup/Behavior/D0 are already C-linkage bodies (.c
// files, or a .cpp inside an unqualified extern "C" block) -- no face.
// PowerStarCreate has no real-method file at all (every own slot is a plain
// C function, func_ov018_*).
#include "IceSheet.h"
#include "OneUpLogo.h"
extern "C" {
int _ZN8IceSheet13InitResourcesEv(void *self)
{ return ((IceSheet *)self)->IceSheet::InitResources(); }
int _ZN8IceSheet16CleanupResourcesEv(void *self)
{ return ((IceSheet *)self)->IceSheet::CleanupResources(); }
int _ZN8IceSheet8BehaviorEv(void *self)
{ return ((IceSheet *)self)->IceSheet::Behavior(); }
int _ZN8IceSheet6RenderEv(void *self)
{ return ((IceSheet *)self)->IceSheet::Render(); }
int _ZN9OneUpLogo13InitResourcesEv(void *self)
{ return ((OneUpLogo *)self)->OneUpLogo::InitResources(); }
int _ZN9OneUpLogo6RenderEv(void *self)
{ return ((OneUpLogo *)self)->OneUpLogo::Render(); }
}
