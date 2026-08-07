// GATE 89: the POWER_STAR class (178, ov002), and GATE 90: STAR_CAMERA (177,
// ov002).
//
// Same law as hal/actor_classes_wf_enemy.cpp -- ROM slot order, __fastcall
// thunks that call QUALIFIED or forward to C-named free functions, unhosted
// slots trap by name -- and the THIRTY-ONE slot shape, because PowerStar is an
// Enemy subclass (through Actor), not the twenty-slot ActorBase shape.
//
// ---- POWER_STAR (178) -----------------------------------------------------
//
// _ZTV9PowerStar @ ov002 0x0210ab3c, 31 slots, RTTI daStar_c (aliased
// _ZTV8daStar_c at the same address). PowerStar_Spawn installs it directly
// (p[0] = (int)_ZTV9PowerStar) after _ZN5EnemyC2Ev, so the vtable is a HOST
// array this file declares and the registry fills -- the ov079 rule, a mounted
// vtable would hand the factory DS code addresses.
//
// The class overrides, read out of the ROM vtable (relocs.txt):
//   slot 0  InitResources  020eb63c  _ZN9PowerStar13InitResourcesEv (matched)
//   slot 3  CleanupResources 020eac18 _ZN9PowerStar16CleanupResourcesEv (matched)
//   slot 6  Behavior       020eb05c  _ZN9PowerStar8BehaviorEv -- HOST COPY
//                          (the PMF state dispatch, port/unmatched/PowerStar_States)
//   slot 9  Render         020eacf4  _ZN9PowerStar6RenderEv (matched)
//   slot 16 D1             020e6c40  _ZN9PowerStarD1Ev (matched, C-named .c)
//   slot 17 D0             020e6c90  _ZN9PowerStarD0Ev (matched, C-named .c)
//   slot 18 OnYoshiTryEat  020e8ee8  func_ov002_020e8ee8 (matched)
//   slot 19 (egg)          020e8edc  func_ov002_020e8edc (matched)
// every other slot is an Actor/ActorBase base method, the ac31/we31 shared set.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
int _ZN5Actor19BeforeInitResourcesEv(void *self);          /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a); /* slot 2 */
int _ZN5Actor14BeforeBehaviorEv(void *self);               /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);                 /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                /* slot 18 default */
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

/* the Enemy tier's own eight-entry death table, seated the first time an
   Enemy-family class registers (idempotent) */
void port_enemy_death_states_seat(void);
}

// ---- the trap --------------------------------------------------------------
static void star_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "FATAL: vtable slot %d is not hosted (actor id %u %s, "
                 "phase %d, spawn step %d)\n",
                 slot, id, port_actor_class_name(id), data_02099f24[0],
                 (int)data_020a4b4c);
    std::abort();
}
#define STAR_TRAP(n) \
    static int __fastcall star_trap##n(void *s, void *) \
    { star_trap_report(s, n); return 0; }
STAR_TRAP(13) STAR_TRAP(14) STAR_TRAP(30)
#undef STAR_TRAP

// ---- the ten shared lifecycle halves plus Actor's tail ---------------------
static int __fastcall star_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall star_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall star_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
static void __fastcall star_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall star_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall star_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall star_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall star_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall star_pdes(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall star_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall star_v50(void *s, void *)
{ return _ZN5Actor9Virtual50Ev(s); }
static int __fastcall star_pounded(void *s, void *, void *o)
{ _ZN5Actor15OnGroundPoundedERS_(s, o); return 0; }
static int __fastcall star_atk1(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked1ERS_(s, o); return 0; }
static int __fastcall star_atk2(void *s, void *, void *o)
{ _ZN5Actor11OnAttacked2ERS_(s, o); return 0; }
static int __fastcall star_kicked(void *s, void *, void *o)
{ _ZN5Actor8OnKickedERS_(s, o); return 0; }
static int __fastcall star_pushed(void *s, void *, void *o)
{ _ZN5Actor8OnPushedERS_(s, o); return 0; }
static int __fastcall star_cannon(void *s, void *, void *o)
{ _ZN5Actor24OnHitByCannonBlastedCharERS_(s, o); return 0; }
static int __fastcall star_mega(void *s, void *, void *p)
{ _ZN5Actor15OnHitByMegaCharER6Player(s, p); return 0; }
static int __fastcall star_under(void *s, void *, void *o)
{ _ZN5Actor19OnHitFromUnderneathERS_(s, o); return 0; }

/* The shared half of a 31-slot Actor table: Actor's four Before/After pairs,
   ActorBase's OnHeapCreated/OnPendingDestroy, Virtual50 and the eight combat
   hooks, plus the traps at 13/14/30. Slot 12 is ActorBase::OnPendingDestroy;
   slots 18/19 are class overrides the caller writes. */
static void star31_fill_shared(void **vt)
{
    vt[1] = (void *)star_binit;
    vt[2] = (void *)star_ainit;
    vt[4] = (void *)star_bclean;
    vt[5] = (void *)star_aclean;
    vt[7] = (void *)star_bbeh;
    vt[8] = (void *)star_abeh;
    vt[10] = (void *)star_bren;
    vt[11] = (void *)star_aren;
    vt[12] = (void *)star_pdes;
    vt[13] = (void *)star_trap13;
    vt[14] = (void *)star_trap14;
    vt[15] = (void *)star_heap;
    vt[20] = (void *)star_v50;
    vt[21] = (void *)star_pounded;
    vt[22] = (void *)star_atk1;
    vt[23] = (void *)star_atk2;
    vt[24] = (void *)star_kicked;
    vt[25] = (void *)star_pushed;
    vt[26] = (void *)star_cannon;
    vt[27] = (void *)star_mega;
    vt[28] = (void *)star_under;
    vt[29] = (void *)star_trap30;   /* OnAimedAtWithEgg default -- overwritten below */
    vt[30] = (void *)star_trap30;
}

// ============================================================================
// POWER_STAR (actor 178), _ZTV9PowerStar @ ov002 0x0210ab3c
// ============================================================================
extern "C" {
int _ZN9PowerStar13InitResourcesEv(void *self);   /* face: below */
int _ZN9PowerStar16CleanupResourcesEv(void *self); /* face: below */
int _ZN9PowerStar6RenderEv(void *self);            /* face: below */
int _ZN9PowerStar8BehaviorEv(void *self);          /* host copy */
int *_ZN9PowerStarD1Ev(int *self);
int *_ZN9PowerStarD0Ev(int *self);
int func_ov002_020e8ee8(void *self);               /* slot 18 override */
int func_ov002_020e8edc(void *self);               /* slot 19 override */
void *_ZTV9PowerStar[31];
void port_power_star_states_seat(void);   /* port/unmatched/PowerStar_States */

/* the two Actor combat-tail slots PowerStar leaves as Actor's own bodies but
   the ROM table names explicitly at 29/30 (OnAimedAtWithEgg /
   OnAimedAtWithEggReturnVec). 29 forwards to Actor's; 30 returns a Vector3 by
   value, an ABI a thunk cannot bridge, so it TRAPS -- nothing aims a Yoshi egg
   at the star as Mario, same as every Enemy class in the port. */
int _ZN5Actor16OnAimedAtWithEggEv(void *self);     /* slot 29 */
}
#pragma comment(linker, "/alternatename:__ZTV8daStar_c=__ZTV9PowerStar")

/* PowerStar's Cleanup/InitResources and the state helper func_ov002_020e8ef0
   declare a handful of data symbols as bare typed globals OUTSIDE extern "C", so
   MSVC C++-mangles the references while the ov002 mount / arm9 romdata / a hal
   file define them with C linkage. Alias each mangled reference onto the C
   symbol -- the same seam actor_faces_bob.cpp closes for data_ov002_0211092c.
   Not a src edit. The three 0x8016/0x801a/0x801b SharedFilePtrs and the 0x8018
   one PowerStar::CleanupResources releases: */
#pragma comment(linker, "/alternatename:?data_ov002_02110934@@3DA=_data_ov002_02110934")
#pragma comment(linker, "/alternatename:?data_ov002_02110944@@3DA=_data_ov002_02110944")
#pragma comment(linker, "/alternatename:?data_ov002_02110924@@3DA=_data_ov002_02110924")
#pragma comment(linker, "/alternatename:?data_ov002_02110964@@3DA=_data_ov002_02110964")
/* the four arm9/hal globals func_ov002_020e8ef0 (the coin-record state) reads: */
#pragma comment(linker, "/alternatename:?data_0209b454@@3IA=_data_0209b454")
#pragma comment(linker, "/alternatename:?data_0209f228@@3EA=_data_0209f228")
#pragma comment(linker, "/alternatename:?data_0209f2ac@@3EA=_data_0209f2ac")
#pragma comment(linker, "/alternatename:?data_0209f358@@3PAFA=_data_0209f358")

/* The VS-star obtained array. GiveVsStars writes data_0209f310[idx] as a
   signed-char array; NumVsStarsObtained reads data_0209f310 then walks the run
   from &data_0209f311, the ROM's own next byte (f310 is 1 byte, f311 the next
   3, then f314 begins a DIFFERENT symbol -- the level area table, hosted in
   camera_bridges.cpp). f310 and f311 must therefore be the first two bytes of
   ONE contiguous block. MSVC grouped sections place them adjacently: two
   $NNNN contributions to one section, laid out in address order, the same
   idiom romdata.py's CONTIG runs use. A generous 8-byte head keeps room for
   the 4-byte ROM run (f310..f313) plus slack. */
#pragma section(".hvsstar$0000", read, write)
#pragma section(".hvsstar$0001", read, write)
extern "C" {
__declspec(allocate(".hvsstar$0000")) signed char data_0209f310[1];
__declspec(allocate(".hvsstar$0001")) signed char data_0209f311[31];
}
/* func_ov002_020d94cc declares data_0209f310 as a bare `signed char` outside
   extern "C" (a second, C++-mangled spelling); alias it onto the C symbol. */
#pragma comment(linker, "/alternatename:?data_0209f310@@3CA=_data_0209f310")

/* func_ov002_020e6df8 (the VS-star spawn helper) declares the same two mount
   symbols with their real struct types (Anim2 / int*) OUTSIDE extern "C", a
   second MSVC mangling of names the ov002 mount defines with C linkage. Alias
   these too. data_ov002_02110944 is one of the SharedFilePtr/Anim2 pointers
   the sinit builds; data_ov002_0210aa0c is the Vector3 collision offset now in
   the ov002 mount. */
#pragma comment(linker, "/alternatename:?data_ov002_02110944@@3UAnim2@@A=_data_ov002_02110944")
#pragma comment(linker, "/alternatename:?data_ov002_0210aa0c@@3PAHA=_data_ov002_0210aa0c")

/* func_ov002_020e81e0 calls Particle::System::New with a (System*)-returning,
   Vector3-taking signature MSVC mangles differently from the two aliases
   cxx_aliases.cpp already carries. Point this third spelling at the same
   matched Itanium body. */
#pragma comment(linker, "/alternatename:?New@System@Particle@@SAPAU12@IIHHHPBUVector3@@PAUCallback@2@@Z=__ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE")

static int __fastcall ps_init(void *s, void *)
{ return _ZN9PowerStar13InitResourcesEv(s); }
static int __fastcall ps_clean(void *s, void *)
{ return _ZN9PowerStar16CleanupResourcesEv(s); }
static int __fastcall ps_behavior(void *s, void *)
{ return _ZN9PowerStar8BehaviorEv(s); }
static int __fastcall ps_render(void *s, void *)
{ port_actor_render_probe("POWER_STAR", (char *)s + 0x30c);
  return _ZN9PowerStar6RenderEv(s); }
static int __fastcall ps_d1(void *s, void *)
{ return (int)(size_t)_ZN9PowerStarD1Ev((int *)s); }
static int __fastcall ps_d0(void *s, void *)
{ return (int)(size_t)_ZN9PowerStarD0Ev((int *)s); }
static int __fastcall ps_yoshi(void *s, void *)
{ return func_ov002_020e8ee8(s); }
static int __fastcall ps_s19(void *s, void *)
{ return func_ov002_020e8edc(s); }
static int __fastcall ps_aimed(void *s, void *)
{ return _ZN5Actor16OnAimedAtWithEggEv(s); }

extern "C" void hal_fill_power_star_vtable(void)
{
    void **vt = _ZTV9PowerStar;
    port_enemy_death_states_seat();
    port_power_star_states_seat();
    star31_fill_shared(vt);
    vt[0] = (void *)ps_init;
    vt[3] = (void *)ps_clean;
    vt[6] = (void *)ps_behavior;
    vt[9] = (void *)ps_render;
    vt[16] = (void *)ps_d1;
    vt[17] = (void *)ps_d0;
    vt[18] = (void *)ps_yoshi;
    vt[19] = (void *)ps_s19;
    vt[29] = (void *)ps_aimed;
}

// ---- method faces ----------------------------------------------------------
// The vtable's C-named references onto the matched lifecycle TUs. PowerStar's
// InitResources/CleanupResources/Render are real MSVC methods against
// include/PowerStar.h; D1/D0 and the two func_ov002_* overrides are already
// C-named free functions in their own TUs.
#include "PowerStar.h"
extern "C" {
int _ZN9PowerStar13InitResourcesEv(void *self)
{ return ((PowerStar *)self)->PowerStar::InitResources(); }
int _ZN9PowerStar16CleanupResourcesEv(void *self)
{ return ((PowerStar *)self)->PowerStar::CleanupResources(); }
int _ZN9PowerStar6RenderEv(void *self)
{ return ((PowerStar *)self)->PowerStar::Render(); }
}
