// THE MINIGAME SCENE: ov004's dScMgBase_c framework, ov006's classes, and the
// two things a minigame needs that no scene before it did -- a THIRTY-SIX slot
// vtable and a set of OVERLAY CONSTRUCTORS that have to run before the scene
// spawns. Run link60, lane MG1 (the pathfinder).
//
// Read port/mg_fanout_costs.txt for what this cost and what the next thirty
// cost after it. Read port/scene_boot_map.txt for the spine this rides, which
// is unchanged: the port still adds nothing to the ROM's own
// Scene::SetSceneToSpawn -> Scene::SpawnIfNecessary -> func_02043098 chain.
//
// ---- 1. WHY THIS IS NOT scene_boot.cpp's SHAPE ----------------------------
//
// The star select and the title screen are eighteen-slot _ZTV5Scene tables
// with seven slots overridden. A minigame is not. dScMgBase_c derives from
// Scene and ADDS EIGHTEEN VIRTUALS OF ITS OWN, so its table
// (data_ov004_020bc0c0) is thirty-six slots, and every ov006 minigame class
// inherits that width. vtspan's three routes agree on 36 for the base and for
// dScMgCurling_c's data_ov006_0213c304 alike.
//
// scene_fill_shared() cannot be pointed at one of these, and the reason is
// worth having in front of whoever seats the second minigame: it is not the
// width, it is the INDEX LIST. dScMgBase_c overrides five of the eleven slots
// that function writes (1, 2, 5, 7 and 10), so calling it on a minigame table
// would quietly replace five framework overrides with the base bodies they
// exist to displace, and the tail would be a separate, milder bug. The generic
// answer is port_scene_fill_rom() in hal/scene_boot.cpp, which keys on the ROM
// WORD the slot holds rather than on the slot number and therefore cannot
// write a slot the ROM did not park a shared body in. It works at any width.
//
// ---- 2. THE OVERLAY CONSTRUCTORS, AND WHERE THEY RUN ----------------------
//
// This is the part no lane had done. ov006 has thirty-one .init constructors
// and ov004 four, inventoried by the mount lane (port/ov006_syms.txt,
// port/ov004_syms.txt) and explicitly NOT run: "identified but not wired to
// run". They are wired here.
//
// THE ROM RUNS THEM AT OVERLAY LOAD. func_0201a694, the spawn spine's
// pre-spawn hook, calls GetSceneOverlayID(id); for an id IsMinigameActorID
// accepts (0x169..0x186) that answers ov006, and func_0201a798 then loads
// ov004 FIRST and ov006 second. The DS's LoadOverlay walks each overlay's
// static-init range -- ov006's is 0x0213356c..0x021335ec, thirty-two words,
// thirty-one function pointers and a NULL terminator -- and calls every entry.
// So on the real machine all thirty-five have run before the factory is
// reached, and NONE of them has run during a level, because ov002 and ov006
// are never co-resident.
//
// THE PORT'S PRECEDENT IS PER-MODE, WHICH IS THE SAME STATEMENT. A level run
// calls port_actor_overlays_sinits() out of hal/level_boot.cpp and runs the
// level overlays' constructors there; nothing runs them on any other path. So
// the minigame set runs from port_scene_mg_overlay_load() below, called out of
// port_scene_run BEFORE the spawn and ONLY when the requested id is one
// IsMinigameActorID accepts -- the ROM's own predicate, linked, not a range
// this file re-spells.
//
// IT DELIBERATELY DOES NOT RUN FROM port_scene_registry_install(). That
// function runs on EVERY boot, level runs included, because the ROM's own
// spawn-table edge has to be real for /OPT:REF on every target. Running
// ov006's constructors there would be a divergence with teeth rather than a
// harmless early call: __sinit_ov004_020b948c calls func_020731dc, which
// THREADS A NODE ONTO AN ARM9-GLOBAL DESTRUCTOR LIST, and that list is walked
// on the level path. (The same sinit calls func_020733a8, which an earlier
// version of this block named as a second threader. It is not one -- it is an
// MSL array-construction primitive -- and the argument stands on
// func_020731dc alone.) The fill is safe to run on every boot because
// it only writes ov004/ov006 mount storage nothing else reads; the
// constructors are not, and the split is drawn there for that reason.
//
// ---- 3. WHAT DOES NOT RUN, AND WHY, ALL OF IT -----------------------------
//
// TWO OF THE THIRTY-FIVE HAVE NO MATCHED SOURCE AND NO DELINK BLOCK. This is
// a hole in the decomp, not a choice made here:
//
//   __sinit_ov004_020b955c   0x574 bytes.  config/arm9/overlays/ov004/
//     symbols.txt names it; config/arm9/overlays/ov004/delinks.txt's .init run
//     goes `start:0x020b948c end:0x020b955c` and then stops, so no block
//     covers the address. No src file mentions it.
//   __sinit_ov006_0213014c   0x284 bytes.  Same shape: ov006's .init blocks go
//     `start:0x021300b0 end:0x0213014c` and the next block starts at
//     0x021303d0, so 0x0213014c is uncovered.
//
// So the runnable sets are THREE of ov004's four and THIRTY of ov006's
// thirty-one. The two are named rather than stood in for, because a plausible
// constructor is exactly the guess port/tools/inferred_stub_guard exists to
// refuse, and a constructor's whole job is to leave state behind.
//
// WHAT THE MISSING TWO WOULD HAVE BUILT IS NOT KNOWN TO THIS LANE. Neither is
// reachable from dScMgCurling_c's own closure, so the pathfinder does not need
// them; whether some other minigame does is a question the fan-out answers per
// class, and port/mg_fanout_costs.txt carries it as an open column rather than
// as a zero.
//
// ---- 4. WHAT THE CONSTRUCTORS ACTUALLY DO ---------------------------------
//
// Two shapes, and the split is clean. Read out of all thirty-three sources.
//
//   SHARED-FILE-POINTER BUILDERS (10 of the 33). They call
//   SharedFilePtr::Construct / func_02017a24 / func_02017acc and register a
//   destructor with func_020731dc, which is the ov085 / ov100 / ov015 shape
//   the level path already runs a dozen of. Nothing about them is new.
//
//   mwcc POINTER-TO-MEMBER PAIR TABLES (the rest). No calls at all: each is a
//   run of struct assignments copying eight-byte {code, adj} records out of
//   .data statics into .bss dispatch tables. dScMgCurling_c's own is
//   __sinit_ov006_021304ac, which copies TWENTY-FIVE pairs from
//   0x0213c1e4..0x0213c2bc into seven tables at 0x021418b0..0x02141950. All
//   twenty-five read {code, 0} in the ROM image, verified word by word.
//
// THE PAIR TABLES ARE THE REASON THE SINITS MATTER AND ALSO THE REASON THIS
// LANE STOPS WHERE IT DOES. The words they copy are DS CODE ADDRESSES, and
// func_ov006_020e3528 (slot 6, the Behavior) dispatches them as
// `(c->*data_ov006_02141950[j].pmf[0])()`. That is an mwcc pointer-to-member
// call, and the port cannot make one: MSVC's member-pointer representation is
// not mwcc's, sizeof differs so the table stride is wrong, and the word is a
// DS address either way. It is the same wall port/unmatched/
// Player_ChangeState.cpp was written for. See section 6.
//
// ---- 5. THE VTABLE, DERIVED --------------------------------------------
//
// THE CLASS IS dScMgCurling_c AND "MgShuffleShell" IS THE LOCALISED NAME OF
// THE SAME MINIGAME. The peer screening picked actor id 0x176 by the symbol
// MgShuffleShell_Spawn; the ROM's own RTTI string at 0x0213c2d0 reads
// "14dScMgCurling_c", and InitResources loads /MG/d_2d_mg_bg_curling1_ncg.bin
// and .../curling2_ncg.bin by name. So src/MgShuffleShell_Spawn.c's
// `_ZTV14dScMgCurling_c` is NOT a per-TU placeholder guess of the ov007 VT0
// kind -- it is the right class name that happens not to be a config symbol
// name. The address it means is settled by the ROM anyway:
//     config/arm9/overlays/ov006/relocs.txt
//     from:0x020e3850 kind:load to:0x0213c304 module:overlay(6)
// and 0x020e3850 is inside MgShuffleShell_Spawn (0x020e3820, 0x34 bytes).
//
// THE SPAWN RECORD, read at (addr - 0x020bfec0) out of
// extracted/overlays/overlay_0006.bin, is EIGHT BYTES and not the ov007 shape:
//     0x0213c214  20 38 0e 02   0x020e3820  the factory
//                 76 01 76 01   374, 374    the id, twice
// The next dsd symbol is at 0x0213c21c, so there is no inline type name after
// it the way dScDSMT_c's record has one. The registry's +4 cross-check reads
// back 374 and passes.
//
// ALL THIRTY-SIX SLOTS, and the fill below writes every one of them. "ruled"
// means the body was disassembled out of the ROM image and compared
// instruction for instruction with src/ before it was seated; the evidence per
// body is in port/tools/inferred_stub_adjudicated.txt.
//
//   slot  ROM word    module  body
//    0   020e3578    ov006   InitResources            ruled REAL_DECOMP
//    1   020b0930    ov004   BeforeInitResources      ruled REAL_DECOMP
//    2   020b08f0    ov004   AfterInitResources       ruled REAL_DECOMP
//    3   02043bf0    arm9    ActorBase::CleanupResources
//    4   0202e5f0    arm9    Scene::BeforeCleanupResources
//    5   020b0840    ov004   AfterCleanupResources    ruled REAL_DECOMP
//    6   020e3528    ov006   Behavior                 ruled REAL_DECOMP
//    7   020b0620    ov004   BeforeBehavior           ruled REAL_DECOMP
//    8   0202e3c8    arm9    Scene::AfterBehavior            VENEER
//    9   020e34ec    ov006   Render                   ruled REAL_DECOMP
//   10   020b04f4    ov004   BeforeRender             ruled REAL_DECOMP
//   11   0202e398    arm9    Scene::AfterRender              VENEER
//   12   020b04e8    ov004   OnPendingDestroy         ruled REAL_DECOMP
//   13   0204357c    arm9    ActorBase::Virtual34
//   14   0204349c    arm9    ActorBase::Virtual38
//   15   02043494    arm9    ActorBase::OnHeapCreated
//   16   020e0638    ov006   D2                       (no marker)
//   17   020e065c    ov006   D0                       ruled REAL_DECOMP
//   18   020e3470    ov006   state reset              ruled REAL_DECOMP
//   19   020b2994    ov004                            ruled REAL_DECOMP
//   20   020b2990    ov004                            ruled REAL_DECOMP
//   21   020b298c    ov004                            ruled REAL_DECOMP
//   22   020ae198    ov004                            ruled REAL_DECOMP
//   23   020ae1a0    ov004                            ruled REAL_DECOMP
//   24   020ae140    ov004                            ruled REAL_DECOMP
//   25   020ae128    ov004                            ruled REAL_DECOMP
//   26   020b04e0    ov004                            ruled REAL_DECOMP
//   27   020af27c    ov004                            ruled REAL_DECOMP
//   28   020af04c    ov004                            ruled REAL_DECOMP
//   29   020af094    ov004                            ruled REAL_DECOMP
//   30   020aeed8    ov004                            ruled REAL_DECOMP
//   31   020b2880    ov004                            ruled REAL_DECOMP
//   32   020b27f4    ov004                            ruled REAL_DECOMP
//   33   020b265c    ov004                            (no marker)
//   34   020ae3b4    ov004                            (no marker)
//   35   020ad660    ov004                            (no marker)
//   --   word 36 reads 2f474d2f, the ASCII "/MG/" of the next symbol's path
//        string, which is what closes the table.
//
// TWENTY-FIVE OF THE THIRTY-SIX BODIES CARRIED THE "recovered from vtable slot
// identity" MARKER and all twenty-five were ruled REAL_DECOMP against the ROM
// before being seated -- twenty ov004, five ov006. TWENTY OF THE TWENTY-FIVE
// ARE dScMgBase_c's OWN and are therefore paid ONCE for all thirty minigames,
// which is the single biggest fact in the cost model.
//
// THE TABLE IS INSIDE THE ov006 MOUNT, so the fill writes host thunks into the
// mount's own storage rather than into a fresh host array -- the ov007
// treatment, for the ov007 reason: a second host array of the same name would
// be a duplicate symbol, and leaving the mounted one alone would leave live
// wild DS pointers in a table the factory installs.
//
// ---- 6. WHERE THIS STOPS ---------------------------------------------------
//
// See port/mg_fanout_costs.txt. The blocker is the mwcc pointer-to-member
// dispatch in section 4 and it is an ABI wall, not a missing body: every one
// of the twenty-five state functions has a matched src TU except
// func_ov006_020e1854, and the dispatch that would reach them cannot be
// compiled by MSVC from the ROM's own source shape.
//
// TWO HOST-ABI DEFECTS IN THE FRAMEWORK'S OWN SOURCE were found by the
// adjudication and are recorded here because they bite the moment slots 5 and
// 7 are dispatched, and neither is visible to the byte gate:
//
//   include/decl_Scene.h declares `extern int _ZN5Scene14BeforeBehaviorEv();`
//     with EMPTY PARENS inside extern "C", so src/func_ov004_020b0620.cpp's
//     call at line 52 passes no `this`. The real definition
//     (src/_ZN5Scene14BeforeBehaviorEv.cpp:47) takes `char* self` and
//     dereferences it immediately. On ARM this is a ride-through and correct
//     -- r0 already holds self -- and on the host the callee reads the stack.
//     Two lines BELOW it, the same header spells the sibling correctly,
//     `_ZN5Scene19BeforeInitResourcesEv(void*)` at line 25, which is what
//     makes this a defect rather than a convention.
//   src/func_ov004_020b0840.c declares `extern void func_0203cbc0(void);` and
//     calls it with no argument at line 28, while the ROM has the pointer
//     being deleted live in r0. The port's own host body is
//     `func_0203cbc0(void *p) { _ZdlPv(p); }` in
//     port/unmatched/func_02073244_hostcopy.c, so the call hands operator
//     delete whatever is on the stack. Seven other src TUs declare the same
//     function WITH a pointer parameter.
//
// Both are the argument-ride-through class hal/scene_actor_faces.cpp's header
// derives, in the framework rather than in the veneers, and both want the same
// remedy: a PORT_HOST_ABI host copy in port/unmatched/ that places the
// argument, leaving src/ and the byte gate alone. NOT TAKEN HERE, because
// neither can be exercised until the dispatch in section 4 exists, and a host
// copy nobody can call is a count rather than a port.

#include <cstdio>
#include <cstdlib>

extern "C" {

/* generated by tools/ovdata.py into build/port/host-src/. This seat is the
   FIRST caller either mount has ever had: port/ov004_ov006_binding_diff.txt
   section 4 records that /OPT:REF strips both pack checks because "a mount
   with no registry rows has no caller". */
void port_ov004_pack_check(void);
void port_ov004_syms_patch(void);
void port_ov006_pack_check(void);
void port_ov006_syms_patch(void);

/* the ROM's own predicate, matched, linked from the slice. Not re-spelled. */
int IsMinigameActorID(unsigned int id);

/* ov004's four .init constructors, less the one with no source. */
void __sinit_ov004_020b948c(void);
void __sinit_ov004_020b9ad0(void);
void __sinit_ov004_020b9b24(void);

/* ov006's thirty-one, less __sinit_ov006_0213014c, which has none. In the
   ROM's own .ctor order, which is address order. */
void __sinit_ov006_0212f4c4(void);
void __sinit_ov006_0212f52c(void);
void __sinit_ov006_0212f660(void);
void __sinit_ov006_0212f6b4(void);
void __sinit_ov006_0212fc7c(void);
void __sinit_ov006_0212fd48(void);
void __sinit_ov006_021300b0(void);
void __sinit_ov006_021303d0(void);
void __sinit_ov006_021304ac(void);
void __sinit_ov006_02130758(void);
void __sinit_ov006_02130a04(void);
void __sinit_ov006_02130a08(void);
void __sinit_ov006_02130df8(void);
void __sinit_ov006_02130e9c(void);
void __sinit_ov006_02130f00(void);
void __sinit_ov006_02130f64(void);
void __sinit_ov006_021311c8(void);
void __sinit_ov006_021314e4(void);
void __sinit_ov006_021318a0(void);
void __sinit_ov006_0213195c(void);
void __sinit_ov006_02131a38(void);
void __sinit_ov006_02131cd0(void);
void __sinit_ov006_02131fa4(void);
void __sinit_ov006_021322bc(void);
void __sinit_ov006_02132894(void);
void __sinit_ov006_02132970(void);
void __sinit_ov006_02132f68(void);
void __sinit_ov006_0213322c(void);
void __sinit_ov006_0213326c(void);
void __sinit_ov006_021333e0(void);

/* the mount storage the fill writes into */
extern unsigned char data_ov006_0213c304[];   /* dScMgCurling_c, 36 slots */
extern unsigned char data_ov004_020bc0c0[];   /* dScMgBase_c,    36 slots */
extern unsigned char MgShuffleShell_SpawnInfo[];

/* the class's own six overrides */
int   func_ov006_020e3578(void *self);        /* slot  0 InitResources */
int   func_ov006_020e3528(void *self);        /* slot  6 Behavior      */
int   func_ov006_020e34ec(void *self);        /* slot  9 Render        */
int   func_ov006_020e0638(void *self);        /* slot 16 D2            */
int   func_ov006_020e065c(void *self);        /* slot 17 D0            */
void  func_ov006_020e3470(void *self);        /* slot 18 state reset   */

/* dScMgBase_c's own twenty-three, in slot order */
int   func_ov004_020b0930(char *c);
void  func_ov004_020b08f0(void *c, unsigned f);
void  func_ov004_020b0840(void *c, unsigned f);
int   func_ov004_020b0620(void *c);
int   func_ov004_020b04f4(void *c);
void  func_ov004_020b04e8(void);
int   func_ov004_020b2994(void);
void  func_ov004_020b2990(void);
void  func_ov004_020b298c(void);
int   func_ov004_020ae198(void);
int   func_ov004_020ae1a0(void);
int   func_ov004_020ae140(void *c);
int   func_ov004_020ae128(void *c);
int   func_ov004_020b04e0(void);
void  func_ov004_020af27c(void *c);
void  func_ov004_020af04c(void *c);
void  func_ov004_020af094(void *c);
void  func_ov004_020aeed8(void *c);
void  func_ov004_020b2880(void);
void  func_ov004_020b27f4(void);
void  func_ov004_020b265c(void *c);
void  func_ov004_020ae3b4(void *c);
void  func_ov004_020ad660(void);

/* the base ctor the factory calls, and the factory itself */
void *func_ov004_020b2adc(char *self);
int  *MgShuffleShell_Spawn(void);

/* hal/scene_boot.cpp */
unsigned port_scene_fill_rom(void **vt, unsigned n);

}  /* extern "C" */

// ---- the tick witness ------------------------------------------------------
//
// One counter per dispatched slot, the same instrument the ov003 and ov007
// seats carry and for the same reason: an object that exists and an object
// that RUNS look identical from outside, and only the counters tell them
// apart. Thirty-six wide, because the table is.
static unsigned g_mg_hits[36];

#define MG_SLOT(n) (++g_mg_hits[(n)])

static int  __fastcall mg_init(void *s, void *)
{ MG_SLOT(0);  return func_ov006_020e3578(s); }
static int  __fastcall mg_beh(void *s, void *)
{ MG_SLOT(6);  return func_ov006_020e3528(s); }
static int  __fastcall mg_render(void *s, void *)
{ MG_SLOT(9);  return func_ov006_020e34ec(s); }
static void *__fastcall mg_d2(void *s, void *)
{ MG_SLOT(16); return (void *)(size_t)func_ov006_020e0638(s); }
static void *__fastcall mg_d0(void *s, void *)
{ MG_SLOT(17); return (void *)(size_t)func_ov006_020e065c(s); }
static int  __fastcall mg_reset(void *s, void *)
{ MG_SLOT(18); func_ov006_020e3470(s); return 1; }

/* SM64DS_SCENE_SLOT9=0 and SM64DS_SCENE_SLOT0=0, the two diagnostics the ov003
   and ov007 seats already carry, counted separately so a run can never read a
   no-op as the real body having run. */
static unsigned g_mg_render_skipped, g_mg_init_skipped;
static int __fastcall mg_render_noop(void *, void *)
{ ++g_mg_render_skipped; return 1; }
static int __fastcall mg_init_noop(void *, void *)
{ ++g_mg_init_skipped; return 1; }

// ---- dScMgBase_c's own twenty-three ---------------------------------------
static int  __fastcall mb_binit(void *s, void *)   { MG_SLOT(1);  return func_ov004_020b0930((char *)s); }
static void __fastcall mb_ainit(void *s, void *, unsigned f) { MG_SLOT(2);  func_ov004_020b08f0(s, f); }
static void __fastcall mb_aclean(void *s, void *, unsigned f){ MG_SLOT(5);  func_ov004_020b0840(s, f); }
static int  __fastcall mb_bbeh(void *s, void *)    { MG_SLOT(7);  return func_ov004_020b0620(s); }
static int  __fastcall mb_bren(void *s, void *)    { MG_SLOT(10); return func_ov004_020b04f4(s); }
static int  __fastcall mb_pdes(void *, void *)     { MG_SLOT(12); func_ov004_020b04e8(); return 0; }
static int  __fastcall mb_v19(void *, void *)      { MG_SLOT(19); return func_ov004_020b2994(); }
static int  __fastcall mb_v20(void *, void *)      { MG_SLOT(20); func_ov004_020b2990(); return 0; }
static int  __fastcall mb_v21(void *, void *)      { MG_SLOT(21); func_ov004_020b298c(); return 0; }
static int  __fastcall mb_v22(void *, void *)      { MG_SLOT(22); return func_ov004_020ae198(); }
static int  __fastcall mb_v23(void *, void *)      { MG_SLOT(23); return func_ov004_020ae1a0(); }
static int  __fastcall mb_v24(void *s, void *)     { MG_SLOT(24); return func_ov004_020ae140(s); }
static int  __fastcall mb_v25(void *s, void *)     { MG_SLOT(25); return func_ov004_020ae128(s); }
static int  __fastcall mb_v26(void *, void *)      { MG_SLOT(26); return func_ov004_020b04e0(); }
static int  __fastcall mb_v27(void *s, void *)     { MG_SLOT(27); func_ov004_020af27c(s); return 0; }
static int  __fastcall mb_v28(void *s, void *)     { MG_SLOT(28); func_ov004_020af04c(s); return 0; }
static int  __fastcall mb_v29(void *s, void *)     { MG_SLOT(29); func_ov004_020af094(s); return 0; }
static int  __fastcall mb_v30(void *s, void *)     { MG_SLOT(30); func_ov004_020aeed8(s); return 0; }
static int  __fastcall mb_v31(void *, void *)      { MG_SLOT(31); func_ov004_020b2880(); return 0; }
static int  __fastcall mb_v32(void *, void *)      { MG_SLOT(32); func_ov004_020b27f4(); return 0; }
static int  __fastcall mb_v33(void *s, void *)     { MG_SLOT(33); func_ov004_020b265c(s); return 0; }
static int  __fastcall mb_v34(void *s, void *)     { MG_SLOT(34); func_ov004_020ae3b4(s); return 0; }
static int  __fastcall mb_v35(void *, void *)      { MG_SLOT(35); func_ov004_020ad660(); return 0; }

/* The framework's own twenty-three, keyed on the ROM word each slot holds, so
   the same list serves EVERY minigame class: a derived class that overrides
   one of them simply does not hold that word. This is what makes the fan-out
   cheap, and it is the whole reason the fill is address-keyed. */
struct MgFace { unsigned ds; void *host; };
static const MgFace kMgBaseFaces[] = {
    {0x020b0930u, (void *)mb_binit},  {0x020b08f0u, (void *)mb_ainit},
    {0x020b0840u, (void *)mb_aclean}, {0x020b0620u, (void *)mb_bbeh},
    {0x020b04f4u, (void *)mb_bren},   {0x020b04e8u, (void *)mb_pdes},
    {0x020b2994u, (void *)mb_v19},    {0x020b2990u, (void *)mb_v20},
    {0x020b298cu, (void *)mb_v21},    {0x020ae198u, (void *)mb_v22},
    {0x020ae1a0u, (void *)mb_v23},    {0x020ae140u, (void *)mb_v24},
    {0x020ae128u, (void *)mb_v25},    {0x020b04e0u, (void *)mb_v26},
    {0x020af27cu, (void *)mb_v27},    {0x020af04cu, (void *)mb_v28},
    {0x020af094u, (void *)mb_v29},    {0x020aeed8u, (void *)mb_v30},
    {0x020b2880u, (void *)mb_v31},    {0x020b27f4u, (void *)mb_v32},
    {0x020b265cu, (void *)mb_v33},    {0x020ae3b4u, (void *)mb_v34},
    {0x020ad660u, (void *)mb_v35},
};

/* dScMgCurling_c's own six, the per-class half. The fan-out writes one of
   these arrays per minigame and reuses everything above it. */
static const MgFace kCurlingFaces[] = {
    {0x020e3578u, (void *)mg_init},   {0x020e3528u, (void *)mg_beh},
    {0x020e34ecu, (void *)mg_render}, {0x020e0638u, (void *)mg_d2},
    {0x020e065cu, (void *)mg_d0},     {0x020e3470u, (void *)mg_reset},
};

static unsigned mg_apply(void **vt, unsigned n, const MgFace *f, unsigned nf)
{
    unsigned hit = 0;
    for (unsigned i = 0; i < n; ++i) {
        const unsigned ds = (unsigned)(size_t)vt[i];
        for (unsigned k = 0; k < nf; ++k)
            if (f[k].ds == ds) { vt[i] = f[k].host; ++hit; break; }
    }
    return hit;
}

/* Count the words still holding a DS address, which is the only honest check
   that the fill is complete. A minigame table is 36 slots and every one of
   them is dispatched by something, so a nonzero answer here is a wild call
   waiting to happen and the seat says so out loud rather than booting. */
static unsigned mg_raw_left(void **vt, unsigned n)
{
    unsigned left = 0;
    for (unsigned i = 0; i < n; ++i) {
        const unsigned w = (unsigned)(size_t)vt[i];
        if (w >= 0x02000000u && w < 0x02400000u)
            ++left;
    }
    return left;
}

// ---- the overlay load ------------------------------------------------------
//
// The port's stand-in for what func_0201a798 does on the DS: bring ov004 and
// then ov006 up, in that order, which is the ROM's order and not a preference.
/* THE MOUNTS, brought up on EVERY boot, which is the ov007 seat's shape and is
   correct for the same reason: a mount patch only rebases the mount's own
   in-span DATA pointers, it writes nothing a level can observe, and the fill
   below has to run on every boot anyway so that /OPT:REF sees the ROM's own
   spawn-table edge on every target. This is also the FIRST CALLER either mount
   has ever had -- port/ov004_ov006_binding_diff.txt section 4 records that
   /OPT:REF strips both generated pack checks because "a mount with no registry
   rows has no caller", which is why neither appears in the baseline map.
   SPLIT FROM THE CONSTRUCTORS DELIBERATELY. The constructors are NOT safe on
   every boot and the header block says why; keeping them in a separate
   function is what makes the difference visible rather than a comment. */
static void port_scene_mg_mounts(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    port_ov004_pack_check();
    port_ov004_syms_patch();
    port_ov006_pack_check();
    port_ov006_syms_patch();
}

extern "C" void port_scene_mg_overlay_load(void)
{
    static int done;
    if (done)
        return;
    done = 1;

    /* idempotent, and already run by the fill on this boot. Called again so
       this function is correct read on its own. */
    port_scene_mg_mounts();

    /* ov004 FIRST, then ov006, because that is the order func_0201a798 loads
       them in and constructors are order-sensitive by definition. */
    __sinit_ov004_020b948c();
    __sinit_ov004_020b9ad0();
    __sinit_ov004_020b9b24();

    __sinit_ov006_0212f4c4(); __sinit_ov006_0212f52c();
    __sinit_ov006_0212f660(); __sinit_ov006_0212f6b4();
    __sinit_ov006_0212fc7c(); __sinit_ov006_0212fd48();
    __sinit_ov006_021300b0(); __sinit_ov006_021303d0();
    __sinit_ov006_021304ac(); __sinit_ov006_02130758();
    __sinit_ov006_02130a04(); __sinit_ov006_02130a08();
    __sinit_ov006_02130df8(); __sinit_ov006_02130e9c();
    __sinit_ov006_02130f00(); __sinit_ov006_02130f64();
    __sinit_ov006_021311c8(); __sinit_ov006_021314e4();
    __sinit_ov006_021318a0(); __sinit_ov006_0213195c();
    __sinit_ov006_02131a38(); __sinit_ov006_02131cd0();
    __sinit_ov006_02131fa4(); __sinit_ov006_021322bc();
    __sinit_ov006_02132894(); __sinit_ov006_02132970();
    __sinit_ov006_02132f68(); __sinit_ov006_0213322c();
    __sinit_ov006_0213326c(); __sinit_ov006_021333e0();

    std::printf("[scene] ov004+ov006 mounted and 33 of 35 overlay "
                "constructors run (ov004 3/4, ov006 30/31; "
                "__sinit_ov004_020b955c and __sinit_ov006_0213014c have no "
                "matched source and no delink block)\n");
    std::fflush(stdout);
}

/* Called from port_scene_run for a minigame id, ahead of the spawn. Split from
   the fill so a reader can see that the CONSTRUCTORS are gated on the id and
   the FILL is not. */
extern "C" void port_scene_mg_prepare(int id)
{
    if (!IsMinigameActorID((unsigned)id))
        return;
    port_scene_mg_overlay_load();
}

// ---- the fill --------------------------------------------------------------
extern "C" void port_scene_fill_curling(void)
{
    /* THE MOUNTS BEFORE THE FILL, and the order is real rather than
       defensive: port_scene_registry_install calls this fill at the tail of
       port_stage_a2_seat, which is BEFORE port_scene_run reaches
       port_scene_mg_prepare, so if the mounts were only brought up there the
       fill would run first every time. The two do not in fact collide -- the
       patch rebases DATA pointers and every word this fill writes is a CODE
       slot, which the binding diff measured as "left raw, target is ov006's
       own .text" -- but a fill that depends on that not colliding is a fill
       that breaks the day a mount grows. */
    port_scene_mg_mounts();

    void **base = (void **)data_ov004_020bc0c0;
    void **vt   = (void **)data_ov006_0213c304;

    /* THE BASE TABLE IS FILLED TOO, and it is not ceremony. The factory calls
       func_ov004_020b2adc, which writes data_0208e4b8, then _ZTV5Scene, then
       data_ov004_020bc0c0 into self[0] before the factory's own write of the
       derived table lands. Nothing dispatches in that window today -- every
       call the base ctor makes is direct -- but the base table is live
       storage inside a mounted span either way, and leaving thirty-six raw DS
       words in a table the ROM installs is the thing that produced the ov007
       lane's "eip 0x01cccab4 accessing 0x020ccab4" fault. */
    port_scene_fill_rom(base, 36);
    mg_apply(base, 36, kMgBaseFaces,
             sizeof kMgBaseFaces / sizeof kMgBaseFaces[0]);

    /* the derived table: shared arm9 words, then the framework's, then the
       class's own six. Order does not matter -- the three key sets are
       disjoint by construction, since a word is one address -- but it reads
       in inheritance order. */
    port_scene_fill_rom(vt, 36);
    mg_apply(vt, 36, kMgBaseFaces,
             sizeof kMgBaseFaces / sizeof kMgBaseFaces[0]);
    mg_apply(vt, 36, kCurlingFaces,
             sizeof kCurlingFaces / sizeof kCurlingFaces[0]);

    /* the two diagnostics, applied after the fill so they override it */
    {
        const char *s0 = std::getenv("SM64DS_SCENE_SLOT0");
        const char *s9 = std::getenv("SM64DS_SCENE_SLOT9");
        if (s0 && s0[0] == '0') vt[0] = (void *)mg_init_noop;
        if (s9 && s9[0] == '0') vt[9] = (void *)mg_render_noop;
    }

    {
        const unsigned lb = mg_raw_left(base, 36);
        const unsigned lv = mg_raw_left(vt, 36);
        if (lb || lv) {
            std::fprintf(stderr, "  [scene] MINIGAME FILL INCOMPLETE: "
                         "dScMgBase_c leaves %u of 36 raw DS words, "
                         "dScMgCurling_c leaves %u. A dispatch of any of them "
                         "jumps to a DS address as a host one.\n", lb, lv);
            std::fflush(stderr);
        }
    }
}

/* The registry's factory column is void *(*)(void) and the matched factory
   returns int *. One typed forwarder rather than a cast through an
   incompatible function pointer, the same shape title_spawn has. */
extern "C" void *port_mg_curling_spawn(void)
{
    return (void *)MgShuffleShell_Spawn();
}

extern "C" void port_scene_mg_hits(void)
{
    unsigned total = 0;
    for (int i = 0; i < 36; ++i) total += g_mg_hits[i];
    std::printf("[scene] ov006 slot hits: init %u, behavior %u, render %u, "
                "cleanup %u, pending-destroy %u%s\n",
                g_mg_hits[0], g_mg_hits[6], g_mg_hits[9], g_mg_hits[3],
                g_mg_hits[12],
                g_mg_render_skipped
                    ? "  [RENDER SLOT NO-OP'd: SM64DS_SCENE_SLOT9=0]" : "");
    if (g_mg_init_skipped)
        std::printf("[scene] INIT SLOT NO-OP'd: SM64DS_SCENE_SLOT0=0, %u "
                    "time(s)\n", g_mg_init_skipped);
    /* the framework half, which is the number the fan-out cares about: it says
       how much of dScMgBase_c a minigame boot actually exercises. */
    std::printf("[scene] 36-slot table, %u total slot entries across all "
                "slots; framework slots entered:", total);
    for (int i = 0; i < 36; ++i)
        if (g_mg_hits[i]) std::printf(" %d(x%u)", i, g_mg_hits[i]);
    std::printf("\n");
    std::fflush(stdout);
}
