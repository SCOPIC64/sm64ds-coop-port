// RUN LINKW WAVE 5, LANE w5-A: BIG BOO'S HAUNT'S OWN CAST (ov063) plus the
// stragglers level 12 names on overlays that were already mounted. The lane
// that takes level 12's census from 41 skipped toward zero.
//
// TEN ov063 ids, SIX tables. The id map was re-derived from the raw images
// (extracted/overlays/overlay_0063.bin -- ov063 is compressed:true, the dsd
// export is noise -- and ACTOR_SPAWN_TABLE at arm9 0x02090864 read from
// extracted/arm9_dec.bin at base 0x02004000, the tools/actor_names.py
// convention), and this config has NO ov045-style naming shift: every
// record's word[0] is its own name's Spawn and every +4 halfword is the id
// the spawn table reached it from. Full table in port/ov063_syms.txt.
//
//   id   class          table                       instances on L12
//   209  Boo            _ZTV3Boo 0x0211e828         x6
//   210  BigBoo         SHARES _ZTV3Boo             x2   (BigBoo_Spawn's own
//                       src stores &_ZTV3Boo; the classes differ by the id
//                       halfword their shared bodies read at +0xc)
//   211  BigBooIcon     _ZTV10BigBooIcon 0x0211e8ac runtime (boss intro)
//   212  BooCage        _ZTV7BooCage 0x0211e930     runtime (boss reward)
//   65   MansionSteps   _ZTV12MansionSteps 0x0211ea88  x1 (+2 self-spawned)
//   158  Bookshelf      SHARES _ZTV12MansionSteps   x1
//   159  MerryGoRound   SHARES _ZTV12MansionSteps   x1
//   160  TrapDoor       SHARES _ZTV12MansionSteps   x1
//   66   FallBlockBbh   _ZTV12FallBlockBbh 0x0211eb58  x9
//   249  MadPiano       _ZTV8MadPiano 0x0211ed34    x1
//
// The four furniture ids are ONE class (daTrsTrap_c): four 0x40-byte Spawns,
// one 852-byte Actor + Model(+0xd4) + MovingMeshCollider(+0x15c), mode
// resolved from the id at +0xc inside InitResources (0x41 stairs / 0xa0
// trapdoor / 0x9e bookshelf / else merry-go-round).
//
// ---- WIDTHS, by the vtspan.py rule ----------------------------------------
// Boo / BigBooIcon / BooCage / MansionSteps are 31 slots: the word past each
// slot 30 is data (0x00000000 -- the {0, typeinfo} RTTI header of the NEXT
// table; ov063 packs them back to back). FallBlockBbh is 32 with slot 31 =
// func_ov098_0213a17c -- the SAME inherited ov098 quad (6/9/27/31)
// FALL_BLOCK_WF and FALL_BLOCK_BFS seat, all four bodies already linked.
// MadPiano is 32 with slot 31 = 0x020ee55c Platform::Kill (the piano is a
// Platform), then 12 bytes of .data align padding to the bss boundary.
//
// ---- TWO RUNTIME SPAWNS THE ROWS MUST COVER -------------------------------
// MansionSteps::InitResources (host copy, its banner has the derivation)
// spawns TWO more id-65 actors (the staircase segments) and, in Bookshelf
// mode, THREE id-0xd5 = 213 BOOK_SWITCH (ov020 record 0x02114898, the
// library-puzzle books) -- and writes through both results UNCHECKED. So 65
// and 158 are only safe with 65 and 213 in the table; 213's row rides the
// ov020 straggler block below. BigBoo's boss flow spawns 211/212 the same
// way (their rows are here so the fight cannot null-spawn).
//
// ---- THE PMF TABLES ARE SEATED, THE PAINTING RECIPE -----------------------
// Two ov063 state tables are mwcc pointer-to-member records the sinits build
// by copying 8-byte .data pairs into bss (furniture: data_ov063_0211ef38
// <- e9b4/e9bc/e9c4/e9ac; piano: data_ov063_0211efbc <- ecd8/ece8/ece0/ecf0,
// deltas all zero, every target a matched TU on slice_w5a.txt). ov63_bringup
// seats the .data pair fn words with host addresses BEFORE running the
// sinits, so the matched sinit copy propagates host pointers; the three
// dispatchers that read the tables are host copies with the 8-byte stride
// spelled out (port/unmatched/Bbh_PmfDispatch.c -- the MSVC 16-byte PMF
// would stride both tables wrong, the Painting_Dispatch.cpp case).
//
// ---- LANE OWNERSHIP -------------------------------------------------------
// The mount bring-up lives here, not in hal/actor_overlays.cpp -- the ov045
// precedent: no wave-5 lane owns actor_overlays.cpp, and per-lane files merge
// clean. Same measured ordering argument as ov045's header: the registry
// writes SpawnInfo+0 then calls the fill, the generated port_ov063_syms_patch
// touches SpawnInfo+32 words only, and the SharedFilePtrs the sinits build
// are first read at spawn time. THE HANDOFF: whoever next owns
// actor_overlays.cpp should fold ov63_bringup's body into
// port_actor_overlays_sinits beside the ov013 block.
//
// ---- FOUR BODIES ARE BYTE-VERIFIED HOST COPIES ----------------------------
// _ZN3Boo8BehaviorEv, func_ov063_021166ac, func_ov063_02117cdc and
// _ZN12MansionSteps13InitResourcesEv have no matched TU anywhere (cons and
// main checked 2026-08-13). Each ships in port/unmatched with an abverify
// derivation in its banner: equal size against the ROM bytes, every
// divergence a register rename or scheduling swap. When the decomp matches
// any of them, the src TU replaces the copy via slice_w5a.txt.
#include <cstdio>
#include "dsstate_seg.h"
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"
#include "Model.h"

extern "C" {
/* the arm9 shared half, the same defaults every enemy fill writes */
int _ZN5Actor19BeforeInitResourcesEv(void *self);              /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a);  /* slot 2  */
int _ZN5Actor14BeforeBehaviorEv(void *self);                   /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);                     /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                    /* slot 18 */
void _ZN5Actor13OnTurnIntoEggER6Player(void *self, void *p);   /* slot 19 */
int _ZN5Actor9Virtual50Ev(void *self);                         /* slot 20 */
void _ZN5Actor15OnGroundPoundedERS_(void *self, void *o);      /* slot 21 */
void _ZN5Actor11OnAttacked1ERS_(void *self, void *o);          /* slot 22 */
void _ZN5Actor11OnAttacked2ERS_(void *self, void *o);          /* slot 23 */
void _ZN5Actor8OnKickedERS_(void *self, void *o);              /* slot 24 */
void _ZN5Actor8OnPushedERS_(void *self, void *o);              /* slot 25 */
void _ZN5Actor24OnHitByCannonBlastedCharERS_(void *self, void *o); /* 26  */
void _ZN5Actor15OnHitByMegaCharER6Player(void *self, void *p);     /* 27  */
void _ZN5Actor19OnHitFromUnderneathERS_(void *self, void *o);      /* 28  */
int _ZN5Actor16OnAimedAtWithEggEv(void *self);                     /* 29  */
void _ZN8Platform4KillEv(void *self);                              /* 31  */

const char *port_actor_class_name(unsigned id);   /* hal/actor_registry */
void port_actor_slot_decline(const char *what);   /* func_02043fdc.cpp */
void port_actor_render_probe(const char *cls, void *model); /* actor_classes */

/* the generated ov063 per-symbol mount (build/host-src/ov063_syms.c) */
void port_ov063_pack_check(void);
void port_ov063_syms_patch(void);
/* the four ov063 sinits, all matched TUs on slice_w5a.txt, .ctor order */
void __sinit_ov063_0211e29c(void);
void __sinit_ov063_0211e3cc(void);
void __sinit_ov063_0211e590(void);
void __sinit_ov063_0211e5fc(void);

/* the eight PMF source pairs the seat rewrites (mounted .data storage) */
extern unsigned char data_ov063_0211e9ac[], data_ov063_0211e9b4[];
extern unsigned char data_ov063_0211e9bc[], data_ov063_0211e9c4[];
extern unsigned char data_ov063_0211ecd8[], data_ov063_0211ece0[];
extern unsigned char data_ov063_0211ece8[], data_ov063_0211ecf0[];
/* ...and the eight matched bodies they must point at */
void func_ov063_0211cc18(char *c);   /* furniture mode 0, stairs   */
void func_ov063_0211cb54(char *c);   /* furniture mode 1, trapdoor */
void func_ov063_0211c89c(char *c);   /* furniture mode 2, bookshelf */
void func_ov063_0211c7b0(char *c);   /* furniture mode 3, merry-go-round */
void func_ov063_0211dd78(char *c);   /* piano [0].pmf0 */
void func_ov063_0211dbb8(char *c);   /* piano [0].pmf1 */
void func_ov063_0211dba4(char *c);   /* piano [1].pmf0 */
void func_ov063_0211d8cc(char *c);   /* piano [1].pmf1 */
}  /* extern "C" */

// ---- the trap --------------------------------------------------------------
static void ov63_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "UNHOSTED: ov063 vtable slot %d is not hosted (actor id %u "
                 "%s)\n", slot, id, port_actor_class_name(id));
    { static char _m[128];
      std::snprintf(_m, sizeof _m, "unhosted ov063 vtable slot %d on id %u %s",
                    slot, id, port_actor_class_name(id));
      port_actor_slot_decline(_m); }
}
#define OV63_TRAP(n) \
    static int __fastcall ov63_trap##n(void *s, void *) \
    { ov63_trap_report(s, n); return 0; }
OV63_TRAP(13) OV63_TRAP(14) OV63_TRAP(30)
#undef OV63_TRAP

static int __fastcall ov63_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ov63_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ov63_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall ov63_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall ov63_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ov63_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall ov63_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall ov63_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall ov63_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall ov63_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall ov63_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
/* slot 19 takes the three-parameter shape so it emits `ret 4`: the dispatch
   site pushes the Player the callee pops -- the wf_turn_egg contract. */
static int __fastcall ov63_turn_egg(void *s, void *, void *p)
{ _ZN5Actor13OnTurnIntoEggER6Player(s, p); return 0; }
static int __fastcall ov63_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall ov63_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall ov63_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall ov63_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall ov63_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall ov63_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall ov63_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall ov63_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall ov63_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }
static int __fastcall ov63_egg(void *s, void *)
{ return _ZN5Actor16OnAimedAtWithEggEv(s); }
/* slot 31 of the two Platform tables; MadPiano takes it unchanged,
   FallBlockBbh overrides it with its ov098 poof-dust body. */
static int __fastcall ov63_kill(void *s, void *)
{ _ZN8Platform4KillEv(s); return 0; }

/* The shared half, slots 1..30, every word read off the six tables'
   relocations (they agree slot for slot outside each class's own overrides).
   The caller writes its own 0/3/6/9/16/17 (and 12/18/29/31 where the ROM
   table differs). Slots 13/14 are the ActorBase Virtual34/38 traps and 30
   declines -- the wf reading (30's ROM body returns a Vector3 by value and
   the sret contract is unproved).

   THE POINTER IS VOLATILE ON PURPOSE -- the gate-200 elided-stores bug
   (proof in hal/actor_classes_ov002g200.cpp): MSVC can delete a static
   filler's stores when it is called with several distinct extern-array
   arguments, and this helper has six callers. */
static void ov63_fill_shared(void *volatile *vt)
{
    vt[1]  = (void *)ov63_binit;
    vt[2]  = (void *)ov63_ainit;
    vt[4]  = (void *)ov63_bclean;
    vt[5]  = (void *)ov63_aclean;
    vt[7]  = (void *)ov63_bbeh;
    vt[8]  = (void *)ov63_abeh;
    vt[10] = (void *)ov63_bren;
    vt[11] = (void *)ov63_aren;
    vt[12] = (void *)ov63_pdes;
    vt[13] = (void *)ov63_trap13;
    vt[14] = (void *)ov63_trap14;
    vt[15] = (void *)ov63_heap;
    vt[18] = (void *)ov63_yoshi;
    vt[19] = (void *)ov63_turn_egg;
    vt[20] = (void *)ov63_v50;
    vt[21] = (void *)ov63_pounded;
    vt[22] = (void *)ov63_atk1;
    vt[23] = (void *)ov63_atk2;
    vt[24] = (void *)ov63_kicked;
    vt[25] = (void *)ov63_pushed;
    vt[26] = (void *)ov63_cannon;
    vt[27] = (void *)ov63_mega;
    vt[28] = (void *)ov63_under;
    vt[29] = (void *)ov63_egg;
    vt[30] = (void *)ov63_trap30;
}

// ---- the mount bring-up ----------------------------------------------------
static void ov63_bringup(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    port_ov063_pack_check();
    port_ov063_syms_patch();
    /* Seat the eight PMF source pairs with host bodies BEFORE the sinits
       copy them into the two bss dispatch tables (the Painting recipe; the
       header block has the ROM pair values). The delta word stays the ROM's
       zero. */
    *(void **)data_ov063_0211e9b4 = (void *)func_ov063_0211cc18;
    *(void **)data_ov063_0211e9bc = (void *)func_ov063_0211cb54;
    *(void **)data_ov063_0211e9c4 = (void *)func_ov063_0211c89c;
    *(void **)data_ov063_0211e9ac = (void *)func_ov063_0211c7b0;
    *(void **)data_ov063_0211ecd8 = (void *)func_ov063_0211dd78;
    *(void **)data_ov063_0211ece8 = (void *)func_ov063_0211dbb8;
    *(void **)data_ov063_0211ece0 = (void *)func_ov063_0211dba4;
    *(void **)data_ov063_0211ecf0 = (void *)func_ov063_0211d8cc;
    __sinit_ov063_0211e29c();
    __sinit_ov063_0211e3cc();
    __sinit_ov063_0211e590();
    __sinit_ov063_0211e5fc();
}

// ============================================================================
// BOO (209) and BIG_BOO (210) -- _ZTV3Boo, 31 slots, shared
// ============================================================================
//
// 0x5e0-byte CapEnemy: MovingCylinderClsnWithPos +0x184, WithMeshClsn +0x1c4,
// ModelAnim +0x380, Model +0x3e4, ShadowModel +0x434/+0x45c. Both Spawns
// (matched src) store &_ZTV3Boo as the LAST vptr write, so both ids register
// the factories directly -- no reseat wrapper. Boo overrides slots 12
// (its own empty OnPendingDestroy), 18 (func_ov063_0211c480: Yoshi cannot
// eat a BigBoo -- returns 7 for id 0xd1) and 29 (func_ov063_021160c4, the
// egg-aim radius off the scaled cylinder).
extern "C" {
int _ZN3Boo13InitResourcesEv(char *c);          /* slot 0, matched .c */
int _ZN3Boo8BehaviorEv(void *c);                /* slot 6, HOST COPY  */
void _ZN3Boo16OnPendingDestroyEv(void);         /* slot 12, matched .c (empty) */
int _ZN3BooD1Ev(void *self);                    /* slot 16, matched .c */
int *_ZN3BooD0Ev(void *self);                   /* slot 17, matched .c */
int func_ov063_0211c480(void *c);               /* slot 18, matched */
int func_ov063_021160c4(void *c);               /* slot 29, matched */
void *Boo_Spawn(void);
void *BigBoo_Spawn(void);
DSSTATE_BEGIN
void *_ZTV3Boo[31];
DSSTATE_END
}
struct Boo { int CleanupResources(); int Render(); };
static int __fastcall boo_init(void *s, void *)
{ return _ZN3Boo13InitResourcesEv((char *)s); }
static int __fastcall boo_clean(void *s, void *)
{ return ((Boo *)s)->Boo::CleanupResources(); }
static int __fastcall boo_behavior(void *s, void *)
{ return _ZN3Boo8BehaviorEv(s); }
static int __fastcall boo_render(void *s, void *)
{ port_actor_render_probe("BOO", (char *)s + 0x3e4);
  return ((Boo *)s)->Boo::Render(); }
static int __fastcall boo_pdes(void *s, void *)
{ (void)s; _ZN3Boo16OnPendingDestroyEv(); return 0; }
static int __fastcall boo_d1(void *s, void *)
{ return _ZN3BooD1Ev(s); }
static int __fastcall boo_d0(void *s, void *)
{ return (int)(size_t)_ZN3BooD0Ev(s); }
static int __fastcall boo_yoshi(void *s, void *)
{ return func_ov063_0211c480(s); }
static int __fastcall boo_egg(void *s, void *)
{ return func_ov063_021160c4(s); }
extern "C" void hal_fill_boo_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV3Boo;
    ov63_fill_shared(vt);
    vt[0]  = (void *)boo_init;
    vt[3]  = (void *)boo_clean;
    vt[6]  = (void *)boo_behavior;
    vt[9]  = (void *)boo_render;
    vt[12] = (void *)boo_pdes;
    vt[16] = (void *)boo_d1;
    vt[17] = (void *)boo_d0;
    vt[18] = (void *)boo_yoshi;
    vt[29] = (void *)boo_egg;
}

// ============================================================================
// BIG_BOO_ICON (211) -- _ZTV10BigBooIcon, 31 slots
// ============================================================================
//
// 216-byte plain Actor (the boss-intro marker BigBoo spawns). Its ROM table
// overrides ONLY 0/16/17; slots 3/6/9 are the ActorBase base bodies
// (0x02043bf0/0x02043b24/0x02043af0), written below as qualified calls --
// the actor_classes_star.cpp precedent -- NOT the Actor defaults the shared
// fill would leave.
//
// Slot 16 is a HOST THUNK: src/_ZN10BigBooIconD1Ev.cpp is a real MSVC
// destructor (`BigBooIcon::~BigBooIcon() {}` over a virtual base) whose
// auto-emitted base-dtor call would resolve to an MSVC name that does not
// exist -- the HauntedChair hc_d1 case. The chain is what its matched D0
// (.c, linked) spells minus the Deallocate: restore the table, ActorD2.
extern "C" {
int _ZN10BigBooIcon13InitResourcesEv(void *c);  /* slot 0, matched extern-C */
int *_ZN10BigBooIconD0Ev(void *t);              /* slot 17, matched .c */
void *_ZN5ActorD2Ev(void *self);
void *BigBooIcon_Spawn(void);
DSSTATE_BEGIN
void *_ZTV10BigBooIcon[31];
DSSTATE_END
}
static int __fastcall bbi_init(void *s, void *)
{ return _ZN10BigBooIcon13InitResourcesEv(s); }
static int __fastcall bbi_clean(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::CleanupResources(); }
static int __fastcall bbi_behavior(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::Behavior(); }
static int __fastcall bbi_render(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::Render(); }
static int __fastcall bbi_d1(void *s, void *)
{
    *(void **)s = (void *)_ZTV10BigBooIcon;
    _ZN5ActorD2Ev(s);
    return (int)(size_t)s;
}
static int __fastcall bbi_d0(void *s, void *)
{ return (int)(size_t)_ZN10BigBooIconD0Ev(s); }
extern "C" void hal_fill_big_boo_icon_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV10BigBooIcon;
    ov63_fill_shared(vt);
    vt[0]  = (void *)bbi_init;
    vt[3]  = (void *)bbi_clean;
    vt[6]  = (void *)bbi_behavior;
    vt[9]  = (void *)bbi_render;
    vt[16] = (void *)bbi_d1;
    vt[17] = (void *)bbi_d0;
}

// ============================================================================
// BOO_CAGE (212) -- _ZTV7BooCage == _ZTV11daTBasket_c, 31 slots
// ============================================================================
//
// 896-byte Enemy (the birdcage the star rides in after the Big Boo falls):
// MovingCylinderClsn +0x110, WithMeshClsn +0x144, Model +0x300, ShadowModel
// +0x350. Spawn stores _ZTV7BooCage directly. Its matched CleanupResources
// (.c) takes NO receiver on the ROM either -- it releases the one shared
// SharedFilePtr (data_ov063_0211edec) and never reads `this`, so the void
// call below is the ROM's own shape, not a dropped receiver.
extern "C" {
int _ZN7BooCage16CleanupResourcesEv(void);      /* slot 3, matched .c */
int *_ZN7BooCageD0Ev(void *t);                  /* slot 17, matched .c */
int *_ZN7BooCageD1Ev(void *t);                  /* slot 16, matched .c */
void *BooCage_Spawn(void);
DSSTATE_BEGIN
void *_ZTV7BooCage[31];
DSSTATE_END
}
/* the RTTI base spelling; the D1/D0 restore the table by this name */
#pragma comment(linker, "/alternatename:__ZTV11daTBasket_c=__ZTV7BooCage")
struct BooCage { int InitResources(); int Behavior(); int Render(); };
static int __fastcall bc_init(void *s, void *)
{ return ((BooCage *)s)->BooCage::InitResources(); }
static int __fastcall bc_clean(void *s, void *)
{ return _ZN7BooCage16CleanupResourcesEv(); }
static int __fastcall bc_behavior(void *s, void *)
{ return ((BooCage *)s)->BooCage::Behavior(); }
static int __fastcall bc_render(void *s, void *)
{ port_actor_render_probe("BOO_CAGE", (char *)s + 0x300);
  return ((BooCage *)s)->BooCage::Render(); }
static int __fastcall bc_d1(void *s, void *)
{ return (int)(size_t)_ZN7BooCageD1Ev(s); }
static int __fastcall bc_d0(void *s, void *)
{ return (int)(size_t)_ZN7BooCageD0Ev(s); }
extern "C" void hal_fill_boo_cage_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV7BooCage;
    ov63_fill_shared(vt);
    vt[0]  = (void *)bc_init;
    vt[3]  = (void *)bc_clean;
    vt[6]  = (void *)bc_behavior;
    vt[9]  = (void *)bc_render;
    vt[16] = (void *)bc_d1;
    vt[17] = (void *)bc_d0;
}

// ============================================================================
// THE FURNITURE (65 MansionSteps / 158 Bookshelf / 159 MerryGoRound /
// 160 TrapDoor) -- _ZTV12MansionSteps == _ZTV11daTrsTrap_c, 31 slots, shared
// ============================================================================
//
// One 852-byte class, four SpawnInfo records, four registry rows. Slot 0 is
// the byte-verified host copy (port/unmatched/MansionSteps_InitResources.c);
// slot 6 is the PMF-dispatching Behavior host copy (Bbh_PmfDispatch.c); 12
// is the class's own empty OnPendingDestroy. Slot 16 is a HOST THUNK for the
// same reason as BigBooIcon's: src/_ZN12MansionStepsD1Ev.cpp is a real MSVC
// destructor. The chain is its matched D0's, minus the Deallocate: restore
// the table, MovingMeshCollider::~ (+0x15c), Model::~ (+0xd4), ActorD2.
extern "C" {
int _ZN12MansionSteps13InitResourcesEv(char *c);   /* slot 0, HOST COPY */
int _ZN12MansionSteps8BehaviorEv(char *c);         /* slot 6, HOST COPY */
void _ZN12MansionSteps16OnPendingDestroyEv(void);  /* slot 12, matched .c */
int *_ZN12MansionStepsD0Ev(void *t);               /* slot 17, matched .c */
void _ZN18MovingMeshColliderD1Ev(void *self);
void _ZN5ModelD1Ev(void *self);
void *MansionSteps_Spawn(void);
void *Bookshelf_Spawn(void);
void *MerryGoRound_Spawn(void);
void *TrapDoor_Spawn(void);
DSSTATE_BEGIN
void *_ZTV12MansionSteps[31];
DSSTATE_END
}
/* the RTTI base spelling; the matched D0 restores the table by this name */
#pragma comment(linker, "/alternatename:__ZTV11daTrsTrap_c=__ZTV12MansionSteps")
struct MansionSteps { int CleanupResources(); int Render(); };
static int __fastcall ms_init(void *s, void *)
{ return _ZN12MansionSteps13InitResourcesEv((char *)s); }
static int __fastcall ms_clean(void *s, void *)
{ return ((MansionSteps *)s)->MansionSteps::CleanupResources(); }
static int __fastcall ms_behavior(void *s, void *)
{ return _ZN12MansionSteps8BehaviorEv((char *)s); }
static int __fastcall ms_render(void *s, void *)
{ port_actor_render_probe("MANSION_STEPS", (char *)s + 0xd4);
  return ((MansionSteps *)s)->MansionSteps::Render(); }
static int __fastcall ms_pdes(void *s, void *)
{ (void)s; _ZN12MansionSteps16OnPendingDestroyEv(); return 0; }
static int __fastcall ms_d1(void *s, void *)
{
    char *t = (char *)s;
    *(void **)t = (void *)_ZTV12MansionSteps;
    _ZN18MovingMeshColliderD1Ev(t + 0x15c);
    _ZN5ModelD1Ev(t + 0xd4);
    _ZN5ActorD2Ev(t);
    return (int)(size_t)s;
}
static int __fastcall ms_d0(void *s, void *)
{ return (int)(size_t)_ZN12MansionStepsD0Ev(s); }
extern "C" void hal_fill_mansion_steps_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV12MansionSteps;
    ov63_fill_shared(vt);
    vt[0]  = (void *)ms_init;
    vt[3]  = (void *)ms_clean;
    vt[6]  = (void *)ms_behavior;
    vt[9]  = (void *)ms_render;
    vt[12] = (void *)ms_pdes;
    vt[16] = (void *)ms_d1;
    vt[17] = (void *)ms_d0;
}

// ============================================================================
// FALL_BLOCK_BBH (66) -- _ZTV12FallBlockBbh, 32 slots
// ============================================================================
//
// NINE of them on level 12; 844-byte Platform. Slots 6/9/27/31 are the SAME
// four ov098 bodies FALL_BLOCK_WF and FALL_BLOCK_BFS seat (all four already
// linked; slot 31 is the poof-dust death slot 27 arms, not Platform::Kill).
// Its factory ends `p[0] = _ZTV20daObjTh_Fall_Block_c; p[0] = VT1;` -- the
// base-then-placeholder shape -- so the registry gets a wrapper that reseats
// p[0] onto the host table, the port_factory_fall_block_bfs recipe. The
// RTTI spelling is aliased onto the host array because the D1/D0 restore
// the table by that name (their intermediate VT2 = _ZTV10dBgActor_c store
// is a dying-object write over already-hosted storage).
extern "C" {
int func_ov098_0213a36c(char *self);            /* slot 6  Behavior */
int func_ov098_0213a314(char *self);            /* slot 9  Render */
int *_ZN12FallBlockBbhD1Ev(void *t);            /* slot 16, matched .c */
int *_ZN12FallBlockBbhD0Ev(void *t);            /* slot 17, matched .c */
void func_ov098_0213a284(char *self);           /* slot 27 */
void func_ov098_0213a17c(char *self);           /* slot 31 */
void *FallBlockBbh_Spawn(void);
DSSTATE_BEGIN
void *_ZTV12FallBlockBbh[32];
DSSTATE_END
}
#pragma comment(linker, "/alternatename:__ZTV20daObjTh_Fall_Block_c=__ZTV12FallBlockBbh")
extern "C" void *port_factory_fall_block_bbh(void)
{
    void *p = FallBlockBbh_Spawn();
    if (p)
        *(void **)p = (void *)_ZTV12FallBlockBbh;
    return p;
}
struct FallBlockBbh { int InitResources(); int CleanupResources(); };
static int __fastcall fbb_init(void *s, void *)
{ return ((FallBlockBbh *)s)->FallBlockBbh::InitResources(); }
static int __fastcall fbb_clean(void *s, void *)
{ return ((FallBlockBbh *)s)->FallBlockBbh::CleanupResources(); }
static int __fastcall fbb_behavior(void *s, void *)
{ return func_ov098_0213a36c((char *)s); }
static int __fastcall fbb_render(void *s, void *)
{ port_actor_render_probe("FALL_BLOCK_BBH", (char *)s + 0xd4);
  return func_ov098_0213a314((char *)s); }
static int __fastcall fbb_d1(void *s, void *)
{ return (int)(size_t)_ZN12FallBlockBbhD1Ev(s); }
static int __fastcall fbb_d0(void *s, void *)
{ return (int)(size_t)_ZN12FallBlockBbhD0Ev(s); }
/* Slot 27 is OnHitByMegaChar(Player &): the caller pushes the player, so the
   veneer takes the third parameter to pop it even though the ov098 body reads
   only the receiver -- the fb_slot27 contract in hal/actor_classes_wf.cpp. */
static int __fastcall fbb_slot27(void *s, void *, void *)
{ func_ov098_0213a284((char *)s); return 0; }
static int __fastcall fbb_slot31(void *s, void *)
{ func_ov098_0213a17c((char *)s); return 0; }
extern "C" void hal_fill_fall_block_bbh_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV12FallBlockBbh;
    ov63_fill_shared(vt);
    vt[0]  = (void *)fbb_init;
    vt[3]  = (void *)fbb_clean;
    vt[6]  = (void *)fbb_behavior;
    vt[9]  = (void *)fbb_render;
    vt[16] = (void *)fbb_d1;
    vt[17] = (void *)fbb_d0;
    vt[27] = (void *)fbb_slot27;
    vt[31] = (void *)fbb_slot31;
}

// ============================================================================
// MAD_PIANO (249) -- _ZTV8MadPiano, 32 slots
// ============================================================================
//
// 0x6e4-byte Platform: ModelAnim +0x320, ShadowModel x3 (+0x384/+0x3ac/
// +0x3d4), an array of TWO MovingCylinderClsnWithPos at +0x48c built through
// func_020733a8, WithMeshClsn +0x50c. Spawn stores _ZTV8MadPiano directly.
// Slot 31 is Platform::Kill unchanged. Its D1 (matched .c) restores
// _ZTV8Platform mid-chain -- already-hosted storage, a dying-object write.
// Its two PMF state dispatchers are the host copies in Bbh_PmfDispatch.c and
// its state table is seated in ov63_bringup above.
extern "C" {
int *_ZN8MadPianoD1Ev(void *self);              /* slot 16, matched .c */
/* what mp_clean spells out by hand (see its banner) */
int _ZN16MeshColliderBase9IsEnabledEv(void *self);
void _ZN16MeshColliderBase7DisableEv(void *self);
void _ZN13SharedFilePtr7ReleaseEv(void *sfp);
extern unsigned char data_ov063_0211ef80[], data_ov063_0211ef88[],
                     data_ov063_0211ef90[];
void *_ZN8MadPianoD0Ev(void *thiz);             /* slot 17, matched extern-C */
void *MadPiano_Spawn(void);
DSSTATE_BEGIN
void *_ZTV8MadPiano[32];
DSSTATE_END
}
struct MadPiano { int InitResources(); int Behavior(); int Render(); };
static int __fastcall mp_init(void *s, void *)
{ return ((MadPiano *)s)->MadPiano::InitResources(); }
/* slot 3, HOST THUNK, not the matched TU: src/actors/MadPiano/
   _ZN8MadPiano16CleanupResourcesEv.cpp spells its three SharedFilePtrs
   G0/G1/G2, and hal/cxx_aliases.cpp has already bound G0/G1 to SignPost's
   ov002 pointers -- linking it would Release SignPost's LIVE files on every
   level-12 teardown. The ov045 PoleLift ep_clean ruling, one global deeper.
   The ROM body (0x0211de3c, disassembled + relocs): IsEnabled on the
   collider at +0x124, Disable if so, then Release on 0x0211ef80 (model),
   0x0211ef90 (attack anim), 0x0211ef88 (collision), in that pool order. */
static int __fastcall mp_clean(void *s, void *)
{
    char *t = (char *)s;
    if (_ZN16MeshColliderBase9IsEnabledEv(t + 0x124))
        _ZN16MeshColliderBase7DisableEv(t + 0x124);
    _ZN13SharedFilePtr7ReleaseEv(data_ov063_0211ef80);
    _ZN13SharedFilePtr7ReleaseEv(data_ov063_0211ef90);
    _ZN13SharedFilePtr7ReleaseEv(data_ov063_0211ef88);
    return 1;
}
static int __fastcall mp_behavior(void *s, void *)
{ return ((MadPiano *)s)->MadPiano::Behavior(); }
static int __fastcall mp_render(void *s, void *)
{ port_actor_render_probe("MAD_PIANO", (char *)s + 0x320);
  return ((MadPiano *)s)->MadPiano::Render(); }
static int __fastcall mp_d1(void *s, void *)
{ return (int)(size_t)_ZN8MadPianoD1Ev(s); }
static int __fastcall mp_d0(void *s, void *)
{ return (int)(size_t)_ZN8MadPianoD0Ev(s); }
extern "C" void hal_fill_mad_piano_vtable(void)
{
    ov63_bringup();
    void *volatile *vt = (void *volatile *)_ZTV8MadPiano;
    ov63_fill_shared(vt);
    vt[0]  = (void *)mp_init;
    vt[3]  = (void *)mp_clean;
    vt[6]  = (void *)mp_behavior;
    vt[9]  = (void *)mp_render;
    vt[16] = (void *)mp_d1;
    vt[17] = (void *)mp_d0;
    vt[31] = (void *)ov63_kill;
}

// ---- faces -----------------------------------------------------------------
// Two C-name bridges the probe measured (census/evidence/p7_ov063_classified):
//
// 1. _ZN5Model12SetPolygonIDEi: func_ov063_02119074.c and
//    func_ov063_0211aa34.c call it by C name; the body is the matched TU
//    compiled as the real MSVC method (?SetPolygonID@Model@@QAEXH@Z, in the
//    map). The face bridges cdecl into __thiscall -- an /alternatename can
//    never do that (the method_faces.cpp law).
extern "C" void _ZN5Model12SetPolygonIDEi(void *self, int id)
{ ((Model *)self)->Model::SetPolygonID(id); }
//
// 2. ?ReleaseCap@CapEnemy@@QAEXABUVector3@@@Z: func_ov063_021162c8.cpp
//    declares CapEnemy locally with a VOID-returning ReleaseCap and calls it
//    as a method, so MSVC wants the void spelling -- include/CapEnemy.h's
//    Actor*-returning declaration mangles differently and cannot serve. The
//    method defined here IS the face: it forwards into the matched C-linkage
//    arm9 body (0x020061b0, linked). The dropped return matches the caller,
//    which ignores it.
extern "C" void *_ZN8CapEnemy10ReleaseCapERK7Vector3(void *self, const Vector3 *v);
struct CapEnemy {
    void ReleaseCap(const Vector3 &v);
    void UpdateCapPos(const Vector3 &p, const Vector3_16 &a);
};
void CapEnemy::ReleaseCap(const Vector3 &v)
{ _ZN8CapEnemy10ReleaseCapERK7Vector3(this, &v); }
//
// 3. _ZN8CapEnemy12UpdateCapPosERK7Vector3RK10Vector3_16: the Boo render
//    host copy calls it by C name; the matched TU compiles as the real MSVC
//    method. Same bridge, opposite direction from 2.
extern "C" void _ZN8CapEnemy12UpdateCapPosERK7Vector3RK10Vector3_16(
    void *self, const Vector3 *p, const Vector3_16 *a)
{ ((CapEnemy *)self)->CapEnemy::UpdateCapPos(*p, *a); }

// ---- MSVC-typed spellings of mounted C storage -----------------------------
// Matched .cpp TUs declare these outside extern "C", so MSVC mangles the
// reference while the mount defines the one real C symbol -- the
// data_02082128 M48 case (hal/actor_classes_bbh.cpp), eleven ov063 words
// deep. Data only; a data alias has no this-register contract to break.
#pragma comment(linker, "/alternatename:?data_ov063_0211edc4@@3USharedFilePtr@@A=_data_ov063_0211edc4")
#pragma comment(linker, "/alternatename:?data_ov063_0211edcc@@3USharedFilePtr@@A=_data_ov063_0211edcc")
#pragma comment(linker, "/alternatename:?data_ov063_0211edd4@@3USharedFilePtr@@A=_data_ov063_0211edd4")
#pragma comment(linker, "/alternatename:?data_ov063_0211eddc@@3USharedFilePtr@@A=_data_ov063_0211eddc")
#pragma comment(linker, "/alternatename:?data_ov063_0211ede4@@3USharedFilePtr@@A=_data_ov063_0211ede4")
#pragma comment(linker, "/alternatename:?data_ov063_0211edec@@3USharedFilePtr@@A=_data_ov063_0211edec")
#pragma comment(linker, "/alternatename:?data_ov063_0211edf4@@3USharedFilePtr@@A=_data_ov063_0211edf4")
#pragma comment(linker, "/alternatename:?data_ov063_0211edec@@3PAXA=_data_ov063_0211edec")
/* MadPianoD0's vtable externs sit outside its extern-C block, same rule;
   both names already have host storage (this file / hal/actor_vtables.cpp) */
#pragma comment(linker, "/alternatename:?_ZTV8MadPiano@@3PAPAXA=__ZTV8MadPiano")
#pragma comment(linker, "/alternatename:?_ZTV8Platform@@3PAPAXA=__ZTV8Platform")
