// ===========================================================================
// RUN LINK60, LANE A1: ov085's LAST TWO SCREENED CLASSES.
//
//   183  WALL_SIGN  13daObjKanban_c  x9   L2 x6, L4 x1, L5 x1, L50 x1
//   185  TOAD       11daKinopio_c    x10  L2 x4, L4 x2, L5 x3, L50 x1
//
// Nineteen placed instances across four levels that are already mounted and
// already boot. Both counts are the port's own runtime census on this lane's
// baseline, not a spawn-table read: with neither class registered the
// pre-spawn gate declined exactly `- 183 x6 / - 185 x4` on level 2 and the
// three smaller sets elsewhere.
//
// ov085 was brought up at gate 18 for the RABBIT (187), the RABBIT_KEY (229)
// and the LAKITU_BRO (235); the mount (port/ov085_syms.txt, per symbol) and
// the six sinits are already there. What this file adds is two more of the
// same overlay's six factories, so there is no bring-up here at all -- the
// SharedFilePtrs both classes load through are built by
// __sinit_ov085_0212f2a8 and __sinit_ov085_0212fdb0, both of which
// hal/actor_overlays.cpp has been calling since gate 18.
//
// ---- THE CONFIG'S NAMES ARE NOT SHIFTED, and gate 18 checked that ---------
//
// hal/actor_classes.cpp's gate-18 header carries the check for all six of
// ov085's factories, this pair included: every one installs a table whose
// slot 0 is its OWN InitResources and whose slot 16 is its OWN D1, and the
// ROM's RTTI agrees from the other side (daObjKanban_c is a signboard,
// daKinopio_c is Toad). Re-verified here with port/tools/vtspan.py, which
// prints the two tables slot by slot.
//
// ---- THE WIDTHS: 32 AND 31, AND THE SECOND ONE IS THE INTERESTING ONE -----
//
// WALL_SIGN's _ZTV13daObjKanban_c at 0x02130400 is 32 slots and EVERY route
// agrees: semantic tail, terminator, raw reloc run and next dsd symbol. Slot
// 31 is Platform::Kill (ov002 0x020ee55c) inherited, which is the Platform
// signature, and slot 32 is not a code pointer.
//
// TOAD's table at 0x0212feb8 is 31, and the routes DISAGREE: the typeinfo
// route reads 54. It is wrong, and it is wrong in the documented way -- the
// FOURTH width-trap shape lane w20 found on ov026, a class's pointer-to-member
// SOURCE TABLE packed immediately behind its vtable. Slot 31 at 0x0212ff34 is
// a real relocated code pointer (func_ov085_0212a0b8), slot 32 is a literal
// zero, and dsd names one symbol per pair at 8-byte stride from there --
// data_ov085_0212ff34, _0212ff3c, _0212ff44, _0212ff4c and on, which
// port/ov085_syms.txt has carried as 0x8-sized rows since gate 18. Those are
// {function, 0} pairs, not more vtable. The semantic tail says the virtual
// list ended at slot 30 (Actor::OnAimedAtWithEggReturnVec) and the terminator
// and next-dsd-symbol routes agree at 31. THIRTY-ONE, and the slot-31
// "override" the typeinfo route reports is the first pair's function word.
//
// A [54] array would not be wrong in the wild-call direction, but it would
// make the port claim eleven virtuals a plain Actor does not have and would
// bury a real override if one ever turned up; a [31] array is the ROM's
// answer.
//
// ---- SLOT 6 IS NOT DECOMPILED ON EITHER CLASS -----------------------------
//
// Neither _ZN8WallSign8BehaviorEv (0x0212eea4, 0x30c) nor
// _ZN4Toad8BehaviorEv (0x02129878, 0x204) exists in src/ in any form. Both are
// HOST REIMPLEMENTATIONS written from the ROM, in
// port/unmatched/Ov085_Behaviors.cpp, and both are queued as decomp crack
// targets -- when the matched bodies land, the two functions come out of that
// file and the slice takes the src TUs instead, with nothing here changing.
// That file's header carries the derivation and the two convention hazards.
//
// ---- SLOT 16 ON TOAD IS SPELLED HERE, THE SlideDecorationSilverStar CASE --
//
// src/_ZN4ToadD1Ev.cpp is the shadow-class MSVC destructor shape: the TU
// declares its own `struct Actor` / `struct MovingCylinderClsn` /
// `struct ModelAnim` / `struct ShadowModel` / `struct Toad : Actor` and
// defines the destructor, so MSVC emits ??1Toad@@UAE@XZ whose body calls
// ??1ShadowModel@@QAE@XZ, ??1ModelAnim@@QAE@XZ,
// ??1MovingCylinderClsn@@QAE@XZ and ??1Actor@@UAE@XZ -- four decorated names
// that exist nowhere in this link, because the port carries those bodies under
// the ROM's C names. Linking that TU is four unresolved externals and no
// _ZN4ToadD1Ev at all, so slot 16 is spelled inline below from the ROM D1's
// own five relocs (0x02129020, 0x40 bytes):
//     0x0212905c  load -> 0x0212feb8              the table
//     0x02129034  call -> arm9 0x02015ff8         ShadowModel::~ShadowModel
//     0x0212903c  call -> arm9 0x0201691c         ModelAnim::~ModelAnim
//     0x02129044  call -> arm9 0x020149a4         MovingCylinderClsn::~...
//     0x0212904c  call -> arm9 0x020112c8         Actor::~Actor (D2)
// Toad's D0 is NOT hosted: it is a flat .c TU carrying the ROM's Itanium name
// in extern "C", it is in the slice, and it already does that chain plus
// Memory::Deallocate. WALL_SIGN needs neither -- both of ITS destructors are
// plain .c with C names and both are in the slice.
//
// ---- WALL_SIGN'S RENDER IS LINKED, NOT HOSTED, AND THAT IS ADJUDICATED ----
//
// src/_ZN8WallSign6RenderEv.cpp is the local-six-virtual-shadow shape over the
// member at +0xd4 that lane w19 swept, and the screening note flagged it as
// needing the ModelAnim-vs-Model adjudication before seating. It is the MODEL
// side of that split, so it links:
//
//   * the member at +0xd4 is a plain Model. Platform::Platform constructs it,
//     both destructors run _ZN5ModelD1Ev on +0xd4, and InitResources feeds it
//     Model::LoadFile + ModelBase::SetFile. Not a ModelAnim.
//   * hal/cxxname_bridge.cpp DUAL-FILLS _ZTV5Model[4] AND [5] with mv_render
//     for exactly this numbering skew, so the shadow's ROM slot 5 lands on
//     Model::Render either way.
//   * the ROM confirms the dispatch is what the shadow says: 0x0212ee7c is
//     `ldr r2, [r0, #0xd4]! / ldr r2, [r2, #0x14] / blx r2`, slot 5 with one
//     null argument.
//
// _ZTV9ModelAnim cannot be dual-filled -- Virtual18 really is at its slot 5 --
// which is the whole of the w19 bug and is why the RABBIT's and the
// LAKITU_BRO's renders are host copies in port/unmatched/Ov085_Renders.cpp and
// this one is not. Same reading as SlideDecorationSilverStar::Render, which
// has been linked since wave 10. TOAD's Render is easier still: the ROM calls
// _ZN5Model6RenderEPK7Vector3 NON-virtually (a plain bl at 0x02129864), and
// the matched TU spells exactly that.
//
// ---- WHAT TOAD'S InitResources DOES AT RUNTIME, and the 269 question ------
//
// Toad::InitResources spawns actor 0x10d = 269 when its +0x20b variant byte is
// 1 and func_02013a44 (the save's can-have-cap test) passes -- the cap a Toad
// hands back. 269 IS REGISTERED: CAP, hal/actor_classes.inc gate 51, with
// Cap_SpawnInfo at ov002 0x021095cc. ov002 is the always-resident engine
// overlay, so the class is available on every level and this is not the
// MontyMoleRock shape (an unregistered id declined by the pre-spawn gate, then
// a ROM-faithful store through the NULL it returns).
// ===========================================================================
#include <cstdio>
#include "dsstate_seg.h"
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"
/* Four of this pair's bodies are REAL C++ METHODS in src/ and are called as
   methods against the generated headers; the rest are extern "C" in src/ and
   are declared as C names below. Which is which was read off the link, not
   assumed:
     183  InitResources / Render                 -> methods
     183  CleanupResources / D1 / D0             -> C names
     185  CleanupResources                       -> method
     185  InitResources / Render / D0            -> C names   */
#include "WallSign.h"
#include "Toad.h"

extern "C" {
/* ---- the arm9 shared half, read off both tables' own reloc runs ---------- */
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
void port_actor_slot_decline(const char *what);   /* func_02043fdc_hostcopy */
void port_actor_render_probe(const char *cls, void *model); /* actor_classes */

/* ---- WALL_SIGN (183), _ZTV8WallSign / _ZTV13daObjKanban_c 0x02130400 ----- */
int _ZN8WallSign16CleanupResourcesEv(void);        /* slot 3, takes nothing */
int _ZN8WallSign8BehaviorEv(void *self);           /* slot 6, HOST REIMPL   */
int *_ZN8WallSignD1Ev(int *self);                  /* slot 16 */
int *_ZN8WallSignD0Ev(int *self);                  /* slot 17 */
void *WallSign_Spawn(void);                        /* the factory */

/* ---- TOAD (185), _ZTV4Toad / _ZTV11daKinopio_c 0x0212feb8 ---------------- */
int _ZN4Toad13InitResourcesEv(char *self);         /* slot 0  */
int _ZN4Toad8BehaviorEv(void *self);               /* slot 6, HOST REIMPL   */
int _ZN4Toad6RenderEv(char *self);                 /* slot 9  */
int *_ZN4ToadD0Ev(int *self);                      /* slot 17 */
void *Toad_Spawn(void);                            /* the factory */

/* what Toad's hosted slot 16 has to spell out by hand */
void _ZN11ShadowModelD1Ev(void *self);
void _ZN9ModelAnimD1Ev(void *self);
void *_ZN18MovingCylinderClsnD1Ev(void *self);
void *_ZN5ActorD2Ev(void *self);

/* ---- the two host vtable arrays ------------------------------------------
   Both tables answer to TWO names at ONE address, the gate-18 shape: each
   factory stores the recovered class name and each destructor restores the
   ROM's RTTI name. config/arm9/overlays/ov085/symbols.txt carries both labels
   on each address, and port/ov085_syms.txt leaves every _ZTV* out of the mount
   on purpose, so nothing else defines either. The arrays are under the
   factories' names and the RTTI names are /alternatename onto them -- the
   Rabbit/RabbitKey/LakituBro recipe three sections up in
   hal/actor_classes.cpp, unchanged. */
DSSTATE_BEGIN
void *_ZTV8WallSign[32];
void *_ZTV4Toad[31];
DSSTATE_END
}
/* Both LHS names are declared extern by the destructor TUs (include/
   decl_common.h lines 474 and 536) and DEFINED nowhere in this link, which is
   what port/tools/alternatename_guard.py requires. */
#pragma comment(linker, "/alternatename:__ZTV13daObjKanban_c=__ZTV8WallSign")
#pragma comment(linker, "/alternatename:__ZTV11daKinopio_c=__ZTV4Toad")

/* ---- five C++-mangled DATA references onto their C-named definitions ------
   The hal/cxx_aliases.cpp case, kept in this lane's own file so the shared one
   is not touched. Two matched TUs in this slice declare mount symbols at
   NAMESPACE SCOPE in a .cpp without extern "C", so MSVC decorates what is a
   C name everywhere else, and the link says so by name:
     _ZN4Toad16CleanupResourcesEv.cpp  four SharedFilePtrs it releases
     func_ov085_0212943c.cpp           the TALK animation's file pointer
   Decorated names lifted verbatim from the link log. Every LHS is REFERENCED
   only and defined nowhere, which is what port/tools/alternatename_guard.py
   requires; every RHS is defined by the generated ov002/ov085 mounts. */
#pragma comment(linker, "/alternatename:?data_ov002_0210d9a0@@3PAHA=_data_ov002_0210d9a0")
#pragma comment(linker, "/alternatename:?data_ov002_0210d9c0@@3PAHA=_data_ov002_0210d9c0")
#pragma comment(linker, "/alternatename:?data_ov002_0210da40@@3PAHA=_data_ov002_0210da40")
#pragma comment(linker, "/alternatename:?data_ov085_02130480@@3PAHA=_data_ov085_02130480")
#pragma comment(linker, "/alternatename:?data_ov085_02130490@@3HA=_data_ov085_02130490")

// ---- the traps -------------------------------------------------------------
// 13/14 are ActorBase::Virtual34/Virtual38, two u32 arguments each, bodies not
// linked into the port at all; 30 is Actor::OnAimedAtWithEggReturnVec, an SRET
// method no __fastcall thunk shape here models. Trap by name rather than seat
// a body with the wrong contract.
static void ov85_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "UNHOSTED: ov085 vtable slot %d is not hosted (actor id %u "
                 "%s)\n", slot, id, port_actor_class_name(id));
    { static char _m[128];
      std::snprintf(_m, sizeof _m, "unhosted ov085 vtable slot %d on id %u %s",
                    slot, id, port_actor_class_name(id));
      port_actor_slot_decline(_m); }
}
static int __fastcall ov85_trap13(void *s, void *) { ov85_trap_report(s, 13); return 0; }
static int __fastcall ov85_trap14(void *s, void *) { ov85_trap_report(s, 14); return 0; }
static int __fastcall ov85_trap30(void *s, void *) { ov85_trap_report(s, 30); return 0; }

// ---- the shared 1..30 half -------------------------------------------------
static int __fastcall ov85_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ov85_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ov85_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall ov85_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall ov85_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ov85_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall ov85_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall ov85_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall ov85_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall ov85_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall ov85_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static int __fastcall ov85_egg(void *s, void *, void *p)
{ _ZN5Actor13OnTurnIntoEggER6Player(s, p); return 0; }
static int __fastcall ov85_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall ov85_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall ov85_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall ov85_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall ov85_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall ov85_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall ov85_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall ov85_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall ov85_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }
static int __fastcall ov85_aimed(void *s, void *)
{ return _ZN5Actor16OnAimedAtWithEggEv(s); }
static int __fastcall ov85_kill(void *s, void *)
{ _ZN8Platform4KillEv(s); return 0; }

static void ov85_fill_shared(void **vt)
{
    vt[1]  = (void *)ov85_binit;
    vt[2]  = (void *)ov85_ainit;
    vt[4]  = (void *)ov85_bclean;
    vt[5]  = (void *)ov85_aclean;
    vt[7]  = (void *)ov85_bbeh;
    vt[8]  = (void *)ov85_abeh;
    vt[10] = (void *)ov85_bren;
    vt[11] = (void *)ov85_aren;
    vt[12] = (void *)ov85_pdes;
    vt[13] = (void *)ov85_trap13;
    vt[14] = (void *)ov85_trap14;
    vt[15] = (void *)ov85_heap;
    vt[18] = (void *)ov85_yoshi;
    vt[19] = (void *)ov85_egg;
    vt[20] = (void *)ov85_v50;
    vt[21] = (void *)ov85_pounded;
    vt[22] = (void *)ov85_atk1;
    vt[23] = (void *)ov85_atk2;
    vt[24] = (void *)ov85_kicked;
    vt[25] = (void *)ov85_pushed;
    vt[26] = (void *)ov85_cannon;
    vt[27] = (void *)ov85_mega;
    vt[28] = (void *)ov85_under;
    vt[29] = (void *)ov85_aimed;
    vt[30] = (void *)ov85_trap30;
}

// ============================================================================
// WALL_SIGN (actor 183, ov085) x9
// ============================================================================
static int __fastcall ws_init(void *s, void *)
{ return ((WallSign *)s)->WallSign::InitResources(); }
static int __fastcall ws_clean(void *, void *)
{ return _ZN8WallSign16CleanupResourcesEv(); }
static int __fastcall ws_behavior(void *s, void *)
{ return _ZN8WallSign8BehaviorEv(s); }
/* the matched TU, dispatching ROM slot 5 over the plain Model at +0xd4; see
   this file's header for why that is the linkable half of the w19 split */
static int __fastcall ws_render(void *s, void *)
{ port_actor_render_probe("WALL_SIGN", (char *)s + 0xd4);
  return ((WallSign *)s)->WallSign::Render(); }
static int __fastcall ws_d1(void *s, void *)
{ return (int)(size_t)_ZN8WallSignD1Ev((int *)s); }
static int __fastcall ws_d0(void *s, void *)
{ return (int)(size_t)_ZN8WallSignD0Ev((int *)s); }

extern "C" void hal_fill_wall_sign_vtable(void)
{
    void **vt = _ZTV8WallSign;
    ov85_fill_shared(vt);
    vt[0]  = (void *)ws_init;
    vt[3]  = (void *)ws_clean;
    vt[6]  = (void *)ws_behavior;
    vt[9]  = (void *)ws_render;
    vt[16] = (void *)ws_d1;
    vt[17] = (void *)ws_d0;
    vt[31] = (void *)ov85_kill;   /* Platform::Kill, inherited */
}

// ============================================================================
// TOAD (actor 185, ov085) x10
// ============================================================================
static int __fastcall td_init(void *s, void *)
{ return _ZN4Toad13InitResourcesEv((char *)s); }
static int __fastcall td_clean(void *s, void *)
{ return ((Toad *)s)->Toad::CleanupResources(); }
static int __fastcall td_behavior(void *s, void *)
{ return _ZN4Toad8BehaviorEv(s); }
static int __fastcall td_render(void *s, void *)
{ port_actor_render_probe("TOAD", (char *)s + 0x108);
  return _ZN4Toad6RenderEv((char *)s); }
/* Slot 16, the ROM D1's own five relocs; see this file's header. Member
   destruction in REVERSE declaration order, which is the order the ROM runs
   them in: ShadowModel 0x16c, ModelAnim 0x108, MovingCylinderClsn 0xd4, then
   Actor's D2. The vtable store the ROM opens with IS kept here, unlike the
   nine gate-31 hosted destructors that leave it out: this class HAS a host
   array under that name, so the store has somewhere real to point and costs
   one instruction. */
static int __fastcall td_d1(void *s, void *)
{
    ((void **)s)[0] = (void *)_ZTV4Toad;
    _ZN11ShadowModelD1Ev((char *)s + 0x16c);
    _ZN9ModelAnimD1Ev((char *)s + 0x108);
    _ZN18MovingCylinderClsnD1Ev((char *)s + 0xd4);
    _ZN5ActorD2Ev(s);
    return (int)(size_t)s;
}
static int __fastcall td_d0(void *s, void *)
{ return (int)(size_t)_ZN4ToadD0Ev((int *)s); }

extern "C" void hal_fill_toad_vtable(void)
{
    void **vt = _ZTV4Toad;
    ov85_fill_shared(vt);
    vt[0]  = (void *)td_init;
    vt[3]  = (void *)td_clean;
    vt[6]  = (void *)td_behavior;
    vt[9]  = (void *)td_render;
    vt[16] = (void *)td_d1;
    vt[17] = (void *)td_d0;
    /* no slot 31: a plain Actor, 31 slots total, ends at 30 */
}
