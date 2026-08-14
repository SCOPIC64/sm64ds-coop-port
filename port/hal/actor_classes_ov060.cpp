// RUN LINKW WAVE 5 (lane w5-e): THE BOWSER FIGHT ARENA'S FLOOR -- ov060,
// level 38 (koopa2_boss), one class hosted, six measured and banked.
//
// ov060 is the pack all three koopaN_boss arenas load (LoadOrUnloadObject-
// Overlays short-circuits idx 0x24/0x26/0x28 to ov060 alone, an arm9 read).
// This lane mounts it per symbol (port/ov060_syms.txt, where the full cast
// map, the one dsd label shift and every width pin are written down), wires
// its 106 func_ov060_* TUs and six sinits (port/slice_w5e.txt), and hosts
// BOWSER_FIRE_SEA_ARENA (166, daKpa2Bg_c) -- the tilting platform that IS
// the arena's walkable floor. With it skipped, the level-38 census boots
// honest but Mario has nothing to stand on except the lava.
//
// WHY ONLY ONE CLASS THIS WAVE, measured not guessed:
//   - Bowser (279) dispatches two 8-byte state-pair tables (0x0211a4e0..,
//     0x0211a734..) whose words are ov060 CODE addresses the mount keeps raw;
//     hosting him is the MrBlizzard/Crate StateDispatch shape, and his init
//     spawns BowserTail (278) -- registering one without the other risks the
//     ov091 null-child fault. BowserFire (280) and Shockwaves (281) are his
//     fight-time spawns; SpikeBomb (284) is placed x8 but its Behavior is
//     part of the same fight choreography. BowserSkyPlatform (167,
//     daKpa3Bg_c) dispatches the bss Entry triple __sinit_ov060_0211a000
//     copies from the raw code-pair words at 0x0211a930/38/40 (reloc reader
//     0x021182ac, measured). All six need seats this lane does not build;
//     their cast map, widths and factory shapes are banked in
//     port/ov060_syms.txt for wave 6.
//   - The arena class's closure was reloc-walked body by body: two
//     func_ov060 helpers (02117a64/02117ae0, in the slice), the already-
//     seated BeforeClsn installer func_020393d4 (the c9a1731da family), its
//     two SharedFilePtrs (0211affc model / 0211aff4 clsn, built by
//     __sinit_ov060_02119f94), and ONE cross-window CLPS read -- nothing
//     touches the state tables. It is hostable without a single new seam.
//
// ---- THE CLEANUP THUNK (the one body held out of the slice) ---------------
// src/_ZN18BowserFireSeaArena16CleanupResourcesEv.cpp spells its two
// SharedFilePtrs G0/G1, and hal/cxx_aliases.cpp binds those single global
// names to SignPost's LIVE ov002 pointers -- the identical PoleLift trap
// hal/actor_classes_ov045.cpp documents. Linking it would Release SignPost's
// files on every arena teardown. relocs.txt (0x02117b94/98) says the ROM
// body releases 0x0211affc then 0x0211aff4 behind an IsEnabled-guarded
// Disable of the collider at +0x374 (calls 0x020393dc IsEnabled then
// 0x02039140 Disable); arena_clean below is that transcription, the ep_clean
// recipe verbatim.
//
// ---- THE CLPS ALIAS -------------------------------------------------------
// InitResources passes the collider CLPS as `&func_021115bc` -- dsd's
// code-flavored name for 0x021115bc in the twenty-way-shared level-overlay
// window. For this class's only level the owner is ov046 (level 38's own
// image), so the name is aliased onto the one-symbol per-symbol ov046 mount
// (port/ov046_syms.txt). CLPS records carry no pointers; the copy cannot
// diverge from the whole-image mount the loaders walk.
#include <cstdio>
#include "dsstate_seg.h"
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
/* the arm9 shared half -- the same family every hosted 31/32-slot table
   carries; the slot addresses were read off _ZTV18BowserFireSeaArena's own
   reloc run and match the ov045/wf tables slot for slot */
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
void _ZN5Actor24OnHitByCannonBlastedCharERS_(void *self, void *o); /* slot 26 */
void _ZN5Actor15OnHitByMegaCharER6Player(void *self, void *p);     /* slot 27 */
void _ZN5Actor19OnHitFromUnderneathERS_(void *self, void *o);      /* slot 28 */
int _ZN5Actor16OnAimedAtWithEggEv(void *self);                     /* slot 29 */
void _ZN8Platform4KillEv(void *self);                              /* slot 31 */

const char *port_actor_class_name(unsigned id);   /* hal/actor_registry */
void port_actor_slot_decline(const char *what);   /* func_02043fdc.cpp */
void port_actor_render_probe(const char *cls, void *model); /* hal/actor_classes */

/* the generated per-symbol mounts (build/host-src/ov060_syms.c, ov046_syms.c) */
void port_ov060_pack_check(void);
void port_ov060_syms_patch(void);
void port_ov046_pack_check(void);
void port_ov046_syms_patch(void);
/* ov060's six sinits, all linkable (no ov045-style shared-window sinit) */
void __sinit_ov060_021195dc(void);
void __sinit_ov060_02119df0(void);
void __sinit_ov060_02119f94(void);
void __sinit_ov060_0211a000(void);
void __sinit_ov060_0211a388(void);
void __sinit_ov060_0211a428(void);

/* the arena's own bodies (the .cpp methods are faced at file bottom) */
int _ZN18BowserFireSeaArena13InitResourcesEv(void *self);     /* slot 0  */
int _ZN18BowserFireSeaArena8BehaviorEv(void *self);           /* slot 6  */
int _ZN18BowserFireSeaArena6RenderEv(void *self);             /* slot 9  */
int *_ZN18BowserFireSeaArenaD1Ev(int *self);                  /* slot 16 */
int *_ZN18BowserFireSeaArenaD0Ev(int *self);                  /* slot 17 */
void *BowserFireSeaArena_Spawn(void);
/* what arena_clean spells by hand (the ep_clean recipe) */
int _ZN16MeshColliderBase9IsEnabledEv(void *self);
void _ZN16MeshColliderBase7DisableEv(void *self);
void _ZN13SharedFilePtr7ReleaseEv(void *sfp);
extern int data_ov060_0211affc[], data_ov060_0211aff4[];
/* the Enable face's C-linkage dispatcher (hal/cxxname_bridge.cpp routing) */
int _ZN16MeshColliderBase6EnableEP5Actor(void *self, void *actor);

DSSTATE_BEGIN
void *_ZTV18BowserFireSeaArena[32];
DSSTATE_END
}

/* 0x0211a8b0 answers to both names; the D1/D0 (.c, real extern spellings, not
   VT placeholders) restore it by the RTTI one. daKpa2Bg = the koopa2 arena
   background. The base-table store in the same bodies is _ZTV10dBgActor_c,
   already defined and host-filled by hal/actor_classes.cpp. */
#pragma comment(linker, "/alternatename:__ZTV10daKpa2Bg_c=__ZTV18BowserFireSeaArena")

/* The arena CLPS spelling onto the one-symbol ov046 mount (header note). */
#pragma comment(linker, "/alternatename:_func_021115bc=_data_ov046_021115bc")

/* The ov066-window spellings of ov060's own bss cells (ov060_syms.txt's
   window note): same bytes, one storage, sibling names aliased on. */
#pragma comment(linker, "/alternatename:_data_ov066_0211ac20=_data_ov060_0211ac20")
#pragma comment(linker, "/alternatename:_data_ov066_0211ac68=_data_ov060_0211ac68")
#pragma comment(linker, "/alternatename:_data_ov066_0211acd0=_data_ov060_0211acd0")

/* The MSVC-decorated spellings C++ TUs in the pack use for the same mounted
   storage (the actor_faces_bob / bowserpuzzle @@3PA precedent; data aliases
   are exact, there is no `this` to lose). The P8C one is a pointer-to-member
   array of a TU-local class -- the cells are BUILT and DISPATCHED by host
   code on both sides, so the stride is self-consistent (the Painting trap
   needs ROM-record reads, which this is not). */
#pragma comment(linker, "/alternatename:?data_ov060_0211ac20@@3PAHA=_data_ov060_0211ac20")
#pragma comment(linker, "/alternatename:?data_ov060_0211ac68@@3PAHA=_data_ov060_0211ac68")
#pragma comment(linker, "/alternatename:?data_ov060_0211ac70@@3PAHA=_data_ov060_0211ac70")
#pragma comment(linker, "/alternatename:?data_ov060_0211ae9c@@3PAP8C@@AEXXZA=_data_ov060_0211ae9c")
#pragma comment(linker, "/alternatename:?data_ov060_0211aed4@@3PAUTabEnt@@A=_data_ov060_0211aed4")
#pragma comment(linker, "/alternatename:?data_ov060_0211b1ac@@3PAUEntry@@A=_data_ov060_0211b1ac")

/* Two statics wanted by pack TUs under their STRUCT-tagged MSVC decorations
   (the class-tagged PAV variants already exist in hal/actor_faces_bob.cpp;
   statics are cdecl on both sides, so the aliases are exact), and the
   mangled-in-mangled free-function spelling of UpdatePosWithTransform (@@YA,
   the one alias-eligible decoration class), aliased onto the same defined
   MSVC static the @@3PAHA data spelling in hal/actor_classes_bob_world.cpp
   already targets. */
#pragma comment(linker, "/alternatename:?FindWithActorID@Actor@@SAPAU1@IPAU1@@Z=__ZN5Actor15FindWithActorIDEjPS_")
#pragma comment(linker, "/alternatename:?LoadFile@MeshCollider@@SAPAUKCL_File@@AAUSharedFilePtr@@@Z=__ZN12MeshCollider8LoadFileER13SharedFilePtr")
#pragma comment(linker, "/alternatename:?_ZN16MeshColliderBase22UpdatePosWithTransformERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_@@YAXXZ=?UpdatePosWithTransform@MeshColliderBase@@SAXAAU1@PAUActor@@AAUClsnResult@@AAUVector3@@PAUVector3_16@@4@Z")

/* Link residue of the wired pack, each with its named precedent:
   - the arena Init TU spells three C-linkage bodies at C++ linkage (no
     extern "C" on its decls); all three decorations are @@YA cdecl free
     functions, the one alias-eligible class, onto their defined C names --
     the wf_enemy_bridges UpdatePosAndAngs recipe (its @@YAXXZ sibling is
     already there; the arena's decls return int, hence @@YAH here);
   - Bowser_IsAnimAtLastFrame declares Animation::GetFrameCount returning
     int; the matched method (slice_gate21) emits the u32 form. Identical
     receiver, identical args, return in eax either way -- the
     StarMarkerFace member-onto-member alias precedent;
   - func_ov060_02112ee0 wants Sound::StopLoadedMusic_Layer1 by its GNU
     spelling; the matched TU (slice_gate10) defines the MSVC STATIC --
     no `this`, cdecl both sides, exact. */
#pragma comment(linker, "/alternatename:?_ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block@@YAHPAX00HF0@Z=__ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block")
#pragma comment(linker, "/alternatename:?func_020393d4@@YAHPAX0@Z=_func_020393d4")
#pragma comment(linker, "/alternatename:?_ZN16MeshColliderBase16UpdatePosAndAngsERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_@@YAHXZ=__ZN16MeshColliderBase16UpdatePosAndAngsERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_")
#pragma comment(linker, "/alternatename:?GetFrameCount@Animation@@QBEHXZ=?GetFrameCount@Animation@@QBEIXZ")
#pragma comment(linker, "/alternatename:__ZN5Sound22StopLoadedMusic_Layer1Ej=?StopLoadedMusic_Layer1@Sound@@SAXI@Z")

/* ?Enable@MeshColliderBase@@QAEXPAUActor@@@Z -- the STRUCT-Actor variant of
   the face hal/shutter_bob_face.cpp defines for `class Actor` spellers. A
   thiscall member can never be an /alternatename; this TU includes Actor.h,
   so Actor is a struct here and the symbol this definition emits is
   byte-for-byte the one the pack TUs reference. Forwards to the C-linkage
   dispatcher like every other Enable. */
struct MeshColliderBase {
    void Enable(Actor *a);
};
void MeshColliderBase::Enable(Actor *a)
{ _ZN16MeshColliderBase6EnableEP5Actor(this, a); }

// ---- the trap --------------------------------------------------------------
static void ov60_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "UNHOSTED: ov060 vtable slot %d is not hosted (actor id %u "
                 "%s)\n", slot, id, port_actor_class_name(id));
    { static char _m[128];
      std::snprintf(_m, sizeof _m, "unhosted ov060 vtable slot %d on id %u %s",
                    slot, id, port_actor_class_name(id));
      port_actor_slot_decline(_m); }
}
#define OV60_TRAP(n) \
    static int __fastcall ov60_trap##n(void *s, void *) \
    { ov60_trap_report(s, n); return 0; }
OV60_TRAP(13) OV60_TRAP(14) OV60_TRAP(17) OV60_TRAP(30)
#undef OV60_TRAP

static int __fastcall ov60_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ov60_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ov60_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall ov60_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall ov60_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ov60_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall ov60_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall ov60_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall ov60_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall ov60_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall ov60_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
/* slot 19 takes the three-parameter shape so it emits `ret 4`: the dispatch
   site pushes the Player the callee pops -- the wf_turn_egg contract. */
static int __fastcall ov60_turn_egg(void *s, void *, void *p)
{ _ZN5Actor13OnTurnIntoEggER6Player(s, p); return 0; }
static int __fastcall ov60_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall ov60_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall ov60_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall ov60_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall ov60_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall ov60_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall ov60_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall ov60_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall ov60_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }
static int __fastcall ov60_egg(void *s, void *)
{ return _ZN5Actor16OnAimedAtWithEggEv(s); }
/* slot 31, the Platform tail; the arena's table carries it (id 166 is a
   PlatformC2 build and its reloc run has 0x020ee55c at slot 31). */
static int __fastcall ov60_kill(void *s, void *)
{ _ZN8Platform4KillEv(s); return 0; }

/* The shared half, slots 1..30, read off _ZTV18BowserFireSeaArena's own
   reloc run (slot 12 is ActorBase::OnPendingDestroy 0x02043ac0 there, so it
   is written here; a wave-6 class that overrides 12 -- Bowser and
   ExtendingPlatform-shaped ids -- writes its own after this returns).
   Slots 13/14 are the ActorBase Virtual34/38 traps and 30 declines, the
   wf/ov45 reading.

   THE POINTER IS VOLATILE ON PURPOSE -- the gate-200 elided-stores bug
   (hal/actor_classes_ov002g200.cpp): MSVC 19.44 x86 /O2 can delete a static
   filler's stores when it is called with several distinct extern-array
   arguments. One caller today, seven tomorrow. */
static void ov60_fill_shared(void *volatile *vt)
{
    vt[1]  = (void *)ov60_binit;
    vt[2]  = (void *)ov60_ainit;
    vt[4]  = (void *)ov60_bclean;
    vt[5]  = (void *)ov60_aclean;
    vt[7]  = (void *)ov60_bbeh;
    vt[8]  = (void *)ov60_abeh;
    vt[10] = (void *)ov60_bren;
    vt[11] = (void *)ov60_aren;
    vt[12] = (void *)ov60_pdes;
    vt[13] = (void *)ov60_trap13;
    vt[14] = (void *)ov60_trap14;
    vt[15] = (void *)ov60_heap;
    vt[17] = (void *)ov60_trap17;
    vt[18] = (void *)ov60_yoshi;
    vt[19] = (void *)ov60_turn_egg;
    vt[20] = (void *)ov60_v50;
    vt[21] = (void *)ov60_pounded;
    vt[22] = (void *)ov60_atk1;
    vt[23] = (void *)ov60_atk2;
    vt[24] = (void *)ov60_kicked;
    vt[25] = (void *)ov60_pushed;
    vt[26] = (void *)ov60_cannon;
    vt[27] = (void *)ov60_mega;
    vt[28] = (void *)ov60_under;
    vt[29] = (void *)ov60_egg;
    vt[30] = (void *)ov60_trap30;
}

// ---- the mount bring-up ----------------------------------------------------
// The ov45_bringup shape: no lane owns hal/actor_overlays.cpp this wave
// either, so the pack bring-up rides the first registry fill. Whoever next
// owns actor_overlays.cpp should move this body beside the ov013/ov045
// blocks and cut the guard to a call. The ov046 one-symbol mount rides the
// same guard: its only consumer is this cast.
extern "C" void port_ov60_bringup(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    port_ov060_pack_check();
    port_ov060_syms_patch();
    port_ov046_pack_check();
    port_ov046_syms_patch();
    __sinit_ov060_021195dc();
    __sinit_ov060_02119df0();
    __sinit_ov060_02119f94();
    __sinit_ov060_0211a000();
    __sinit_ov060_0211a388();
    __sinit_ov060_0211a428();
}

// ============================================================================
// BOWSER_FIRE_SEA_ARENA (id 166) -- table 0x0211a8b0, 32 slots
// ============================================================================
//
// PlatformC2 build; Model at +0x324, MovingMeshCollider at +0x374. The
// arena's tilting disc over the fire sea: its InitResources loads its model
// and collision through the two __sinit_ov060_02119f94 SharedFilePtrs,
// installs MeshColliderBase::UpdatePosWithTransform as the BeforeClsn
// callback through func_020393d4 (the contested-slot family c9a1731da seats
// -- the third mover proof after ov015's bridge and JRB's SHIP_UP), and its
// Behavior tilts the disc under the player through the two func_ov060
// helpers. Own slots 0/3/6/9/16/17 + Platform::Kill at 31.
static int __fastcall arena_init(void *s, void *)
{ return _ZN18BowserFireSeaArena13InitResourcesEv(s); }
/* slot 3, HOST THUNK, not the matched TU -- the G0/G1 trap (file header). */
static int __fastcall arena_clean(void *s, void *)
{
    char *t = (char *)s;
    if (_ZN16MeshColliderBase9IsEnabledEv(t + 0x374))
        _ZN16MeshColliderBase7DisableEv(t + 0x374);
    _ZN13SharedFilePtr7ReleaseEv(data_ov060_0211affc);
    _ZN13SharedFilePtr7ReleaseEv(data_ov060_0211aff4);
    return 1;
}
static int __fastcall arena_behavior(void *s, void *)
{ return _ZN18BowserFireSeaArena8BehaviorEv(s); }
static int __fastcall arena_render(void *s, void *)
{ port_actor_render_probe("BOWSER_FIRE_SEA_ARENA", (char *)s + 0x324);
  return _ZN18BowserFireSeaArena6RenderEv(s); }
static int __fastcall arena_d1(void *s, void *)
{ return (int)(size_t)_ZN18BowserFireSeaArenaD1Ev((int *)s); }
static int __fastcall arena_d0(void *s, void *)
{ return (int)(size_t)_ZN18BowserFireSeaArenaD0Ev((int *)s); }
extern "C" void hal_fill_bowser_fire_sea_arena_vtable(void)
{
    port_ov60_bringup();
    void *volatile *vt = (void *volatile *)_ZTV18BowserFireSeaArena;
    ov60_fill_shared(vt);
    vt[0]  = (void *)arena_init;
    vt[3]  = (void *)arena_clean;
    vt[6]  = (void *)arena_behavior;
    vt[9]  = (void *)arena_render;
    vt[16] = (void *)arena_d1;
    vt[17] = (void *)arena_d0;
    vt[31] = (void *)ov60_kill;
}

// ---- method faces ----------------------------------------------------------
// The three bodies src defines as real C++ methods against
// BowserFireSeaArena.h (Init/Behavior/Render; the D1/D0 are .c and callable
// directly). The IceSheet/ov045 recipe: the face is the C-name bridge INTO
// the matched method, not a host copy of it.
#include "BowserFireSeaArena.h"
extern "C" {
int _ZN18BowserFireSeaArena13InitResourcesEv(void *self)
{ return ((BowserFireSeaArena *)self)->BowserFireSeaArena::InitResources(); }
int _ZN18BowserFireSeaArena8BehaviorEv(void *self)
{ return ((BowserFireSeaArena *)self)->BowserFireSeaArena::Behavior(); }
int _ZN18BowserFireSeaArena6RenderEv(void *self)
{ return ((BowserFireSeaArena *)self)->BowserFireSeaArena::Render(); }
}
