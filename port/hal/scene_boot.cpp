// THE SCENE BOOT: how the port enters a scene that is not a level.
//
// ---- what a scene is -------------------------------------------------------
//
// A scene is an ACTOR. There is no separate scene machinery in this game: the
// title screen, the star select, the game-over screen, every minigame and the
// level itself are all actors spawned through the same spine, and the thing
// that makes one of them a "scene" is that its class derives from Scene rather
// than from Actor, and that its actor id is one the arm9 knows to load an
// overlay for. That is the whole difference.
//
// So hosting a scene is the ORDINARY REGISTRY SHAPE this port already uses for
// a hundred actor classes: the ROM's own SpawnInfo record with its factory word
// repointed at the host factory, seated in data_020a4bb8 at the class's id, plus
// a vtable fill. Nothing in this file is a new mechanism. What is new is only
// that the ids are scene ids and that something drives the ROM's own chain.
//
// ---- the chain, end to end, every step a matched arm9 TU -------------------
//
//   Scene::SetSceneToSpawn(id, param)     0x0202e36c
//       data_02092664 = id, data_0209f5b8 = param
//   Scene::SpawnIfNecessary()             0x0202e26c
//       if data_02092660 == 0 and data_02092664 != 0x187:
//           func_02013edc(id, param, 1)   0x02013edc, a tail-call veneer
//         -> func_02042fe4(id, param, 1)  0x02042fe4
//         -> func_02043098(id, 0, param, 1)         THE SPAWN SPINE
//              data_020a4b50 = id; func_02043060(id) the pre-spawn hook;
//              func_02043180(id, 0, param, 1) the spawn context;
//              (*data_020a4bb8[id])()      the factory;
//              func_020433b8(obj)          thread it onto the init list
//       then data_02092664 = 0x187 and data_02092660 = 1, so it spawns once.
//
// Every one of those is a matched TU and all but Scene::SpawnIfNecessary were
// already in the walk_window link set before this lane; that one arrives with
// port/slice_scene1.txt. THE PORT ADDS NOTHING TO THE CHAIN. port_scene_boot
// below calls the first two functions and that is all it does.
//
// ---- the overlay load, and why the host does not run it --------------------
//
// On the DS the spine's PRE-SPAWN HOOK is what mounts the scene's overlay.
// func_0201a5cc seats data_020a4b58 = func_0201a694 and data_020a4b5c =
// func_0201a614, and func_02043060 calls the first with the pending actor id
// before the factory runs:
//
//   func_0201a694(id) -> GetSceneOverlayID(id)   ids 2/4/8 -> ov003,
//                                                1 -> ov007, 5 -> ov005,
//                                                3/6/7 -> ov002,
//                                                0x169..0x186 -> ov006
//                     -> func_0201a754(old) then func_0201a798(new)
//                        the unload-before-load discipline, and the one place
//                        ov004 is ever loaded (alongside ov006).
//
// The port's occupant of that hook slot is port_prespawn_hook (the registry
// gate, hal/actor_registry.cpp), which returns the same 2/3 the ROM's hook
// returns. That substitution is not new here and it is not a stand-in for the
// overlay load: the port has no overlay loader because every overlay it hosts
// is a static host array mounted at build time (port/tools/ovdata.py). ov003 is
// mounted exactly the way ov009 or ov016 is. There is no LoadOverlay to run and
// nothing for it to do.
//
// ---- the three classes, and the one this file seats ------------------------
//
// ov003 carries three, and the arm9 ACTOR_SPAWN_TABLE (0x02090864) names all
// three by relocation -- these are reads out of config/arm9/relocs.txt, not
// inferences:
//
//   id 2  0x0209086c -> 0x020b1380   dScTitle_c     the debug level select
//   id 4  0x02090874 -> 0x020b16b4   dScStarSel_c   the star select   <- seated
//   id 8  0x02090884 -> 0x020b1750   dScGameOver_c  the game-over screen
//
// and each record's +4 halfword carries its own id back (2, 4, 8), which is the
// registry's own cross-check and is asserted below. The record is mounted bytes
// (port/ov003_syms.txt); only the +0 factory word is repointed.
//
// ONLY THE STAR SELECT IS SEATED HERE, and what holds the other two is neither
// the mount nor the chain: eleven of their fourteen slot bodies carry the
// "recovered from vtable slot identity" marker, and three of their dtor TUs
// spell per-TU placeholders that exist in no config. Both blockers are written
// out in full at the bottom of port/slice_scene1.txt.
//
// ---- the vtable ------------------------------------------------------------
//
// All three tables are the 18-slot Scene shape and agree with _ZTV5Scene (arm9
// 0x02092680) slot for slot everywhere they do not override. Slots 1/2/4/5/
// 7/8/10/11 are Scene's own halves, 13/14/15 ActorBase's; 0/3/6/9/12/16/17 are
// the class's own. The tables are left OUT of port/ov003_syms.txt (the
// ov079/ov080/ov081 convention), because the mounted bytes would be DS code
// addresses and nothing rebases a code word; the one the port dispatches
// through is the host array below.
//
// THE SLOT CONTENTS ARE THE ROM'S. Each was read out of
// extracted/overlays/overlay_0003.bin at addr - 0x020ad660 (that image's length
// is 0x4240, which equals the OVT's ram_size for ov003, so the read is exact --
// the dsd export is a different file and is not the RAM image), cross-checked
// against config/arm9/overlays/ov003/relocs.txt. No slot here is invented and
// none is a trap: every one of the eighteen words has a body.
//
//   0  InitResources           func_ov003_020af8a0    ov003
//   1  BeforeInitResources     Scene::                0x0202e638
//   2  AfterInitResources      Scene::                0x0202e62c
//   3  CleanupResources        func_ov003_020af86c    ov003
//   4  BeforeCleanupResources  Scene::                0x0202e5f0
//   5  AfterCleanupResources   Scene::                0x0202e5d0
//   6  Behavior                func_ov003_020af038    ov003
//   7  BeforeBehavior          Scene::                0x0202e3d4
//   8  AfterBehavior           Scene::                0x0202e3c8
//   9  Render                  func_ov003_020ae6f4    ov003
//  10  BeforeRender            Scene::                0x0202e3a4
//  11  AfterRender             Scene::                0x0202e398
//  12  OnPendingDestroy        func_ov003_020ae6f0    ov003
//  13  Virtual34               ActorBase::            0x0204357c
//  14  Virtual38               ActorBase::            0x0204349c
//  15  OnHeapCreated           ActorBase::            0x02043494
//  16  D2                      func_ov003_020addfc    ov003
//  17  D0                      func_ov003_020ade54    ov003
//
// ---- and how a run gets here -----------------------------------------------
//
// SM64DS_SCENE=<id> in walk_window, mirroring SM64DS_LEVEL. The level harness
// hands the whole run over at the top of main -- after the host bring-up (the
// fixed-address reservation, the root heap, the ov002 mount's pointer pass and
// its static initialisers, the model vtable fills) and before anything level-
// shaped happens -- and port_scene_run below owns the rest of the process.
//
// IT IS A HANDOVER RATHER THAN A MODE INSIDE THE LEVEL LOOP, and the reason is
// structural rather than stylistic. walk_window's frame is written around the
// Player the entrance spawned: it reads the player pointer unguarded from the
// input block to the camera rig to the HUD. A scene has no Player, no Stage, no
// level overlay and no entrance, so there is nothing for those lines to read.
// The frame here is the same five-phase split walk_window makes and the same
// one func_02044120 makes on the DS -- phases 4/2/3 the tick, phase 5 the
// render bucket inside the host's render frame, phase 1 the scene-tree pass
// that closes it -- with the level's own passes simply absent because there is
// no level.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ntr/gx.h"
#include "ntr/ppu.h"

#include "dsstate_seg.h"

// ---- THE LINKAGE FACES ov003's OWN SOURCES NEED -----------------------------
//
// Three shapes, all of them decomp-side spellings rather than port decisions,
// and all three closed BY ADDRESS rather than by name.
//
// 1. C-NAMED SYMBOLS DECLARED AT C++ LINKAGE. src/func_ov003_020af038.cpp (the
//    Behavior) declares nine of its globals outside an extern "C" block, so
//    MSVC mangles the references. The definitions are the port's ordinary
//    C-named hosted globals; these aliases bind the mangled spelling to them.
//    Same mechanism, same reason, as hal/cxx_aliases.cpp.
#pragma comment(linker, "/alternatename:?data_0209f5bc@@3PAUObj@@A=_data_0209f5bc")
#pragma comment(linker, "/alternatename:?data_0209f5e8@@3FA=_data_0209f5e8")
#pragma comment(linker, "/alternatename:?data_02092128@@3EA=_data_02092128")
#pragma comment(linker, "/alternatename:?data_02092114@@3EA=_data_02092114")
#pragma comment(linker, "/alternatename:?data_0209f1f0@@3EA=_data_0209f1f0")
#pragma comment(linker, "/alternatename:?data_020a0de8@@3PAURow020a0de8@@A=_data_020a0de8")
#pragma comment(linker, "/alternatename:?data_020a0de9@@3PAY03EA=_data_020a0de9")
#pragma comment(linker, "/alternatename:?data_020a0e58@@3PAGA=_data_020a0e58")
#pragma comment(linker, "/alternatename:?data_020a0e5a@@3PAGA=_data_020a0e5a")
//
// 2. NAMESPACE-QUALIFIED FUNCTIONS. Two ov003 TUs reach arm9 functions through
//    a locally-declared namespace (`namespace G2 { short *GetBG0ScrPtr(); }`,
//    `class Sound { static void UnsetPlayerVoiceGroup(); }`) rather than
//    through the mangled C name the matched TU defines.
#pragma comment(linker, "/alternatename:?GetBG0ScrPtr@G2@@YAPAFXZ=__ZN2G212GetBG0ScrPtrEv")
#pragma comment(linker, "/alternatename:?UnsetPlayerVoiceGroup@Sound@@SAXXZ=__ZN5Sound21UnsetPlayerVoiceGroupEv")
//
// 3. THE OAM SPRITE TEMPLATES, SPELLED AS FUNCTIONS. dScStarSel_c::Render
//    (src/func_ov003_020ae6f4.cpp) declares thirteen ov001 sprite-template
//    tables as `extern void *func_020abXXXX[]`. No func_020ab* symbol exists
//    in any config: the addresses are ov001 DATA, and the "func_" prefix is a
//    decomp-side guess at what lives there. Each alias below binds the guess
//    to the address's real name, read out of
//    config/arm9/overlays/ov001/symbols.txt. The ten that were not already in
//    the port's ov001 mount joined it this lane (port/ov001_syms.txt, which
//    also records why the address is ov001's and not ov000's).
//    SIX OF THE THIRTEEN WERE ALREADY ALIASED, in hal/sub_actors.cpp's own
//    ov001 block, because the HUD's render TUs spell them the same way -- the
//    same defect in a different overlay's source. Those six (020ab948,
//    020ab9c8, 020aba70, 020abad0, 020abad8, 020abd88) are NOT repeated here;
//    only the seven this lane is the first caller of are.
#pragma comment(linker, "/alternatename:_func_020ab938=_data_ov001_020ab938")
#pragma comment(linker, "/alternatename:_func_020ab940=_data_ov001_020ab940")
#pragma comment(linker, "/alternatename:_func_020abb18=_data_ov001_020abb18")
#pragma comment(linker, "/alternatename:_func_020abb34=_data_ov001_020abb34")
#pragma comment(linker, "/alternatename:_func_020abb54=_data_ov001_020abb54")
#pragma comment(linker, "/alternatename:_func_020abb74=_data_ov001_020abb74")
#pragma comment(linker, "/alternatename:_func_020abb94=_data_ov001_020abb94")
#pragma comment(linker, "/alternatename:_func_020abbb4=_data_ov001_020abbb4")
#pragma comment(linker, "/alternatename:_func_020abbf4=_data_ov001_020abbf4")
#pragma comment(linker, "/alternatename:_func_020abcb4=_data_ov001_020abcb4")
#pragma comment(linker, "/alternatename:_func_020abd78=_data_ov001_020abd78")
#pragma comment(linker, "/alternatename:_func_020abd80=_data_ov001_020abd80")

extern "C" {

/* the spawn table and the two Scene entry points (matched arm9) */
extern void **data_020a4bb8;                     /* hal/actor_vtables.cpp */
int _ZN5Scene15SetSceneToSpawnEjj(unsigned id, unsigned param);
int _ZN5Scene16SpawnIfNecessaryEv(void);
extern unsigned short data_02092664;             /* the pending scene id */
extern signed char data_02092110;                /* the CURRENT SUBLEVEL */
extern signed char SUBLEVEL_LEVEL_TABLE[];       /* arm9 0x02075298 */
extern unsigned char data_02092660;              /* "a scene has spawned";
                                                    defined below, in .dsstate */

/* the mounted SpawnInfo record (port/ov003_syms.txt) */
extern unsigned char StarSelect_SpawnInfo[];     /* dScStarSel_c  id 4 */

/* the factory (matched src, port/slice_scene1.txt) */
void *StarSelect_Spawn(void);                    /* dScStarSel_c */

/* Scene's own lifecycle halves, slots 1/2/4/5/7/8/10/11.
   Flat C names: every one of these TUs defines the mangled name at C linkage
   the way the rest of the port spells arm9 methods. */
int  _ZN5Scene19BeforeInitResourcesEv(void *self);      /* slot 1  */
int  _ZN5Scene22BeforeCleanupResourcesEv(void *self);   /* slot 4  */
void _ZN5Scene21AfterCleanupResourcesEj(void *self, unsigned a); /* slot 5 */
int  _ZN5Scene14BeforeBehaviorEv(void *self);           /* slot 7  */
int  _ZN5Scene12BeforeRenderEv(void *self);             /* slot 10 */

/* slots 2, 8, 11 and 15, through hal/scene_actor_faces.cpp. Three of the four
   are the ROM's TAIL-CALL VENEERS taken straight to their target with both
   arguments, because a (void)->(void) transcription of a veneer loses them on
   the host; that file has the derivation. */
void port_scene_after_init(void *self, unsigned a);
void port_scene_after_behavior(void *self, unsigned a);
void port_scene_after_render(void *self, unsigned a);
int  port_scene_on_heap_created(void *self);

/* dScStarSel_c's own seven */
void func_ov003_020af8a0(void *self);            /* slot 0  InitResources */
int  func_ov003_020af86c(void);                  /* slot 3  CleanupResources */
int  func_ov003_020af038(void *self);            /* slot 6  Behavior */
int  func_ov003_020ae6f4(void *self);            /* slot 9  Render */
void func_ov003_020ae6f0(void);                  /* slot 12 OnPendingDestroy */
int  func_ov003_020addfc(void *self);            /* slot 16 D2 */
int *func_ov003_020ade54(void *self);            /* slot 17 D0 */

/* THE VTABLE, a host array. The name is the ROM's own data symbol, which is
   what StarSelect_Spawn writes into the object's +0 word and what the D2 and
   D0 bodies write back on the way down, so the spelling has to be exactly this
   and port/ov003_syms.txt has to leave it out of the mount. 18 words.
   The other two classes' tables (data_ov003_020b1650, data_ov003_020b179c) are
   left out of the mount as well -- they are code-pointer tables and nothing
   rebases a code word -- and are not defined here either, because nothing in
   the link set names them. */
DSSTATE_BEGIN
void *data_ov003_020b1704[18];                   /* dScStarSel_c */
DSSTATE_END

/* the seat, minus the Stage (hal/level_boot.cpp) */
void port_scene_a2_seat(void);

/* the frame: the same calls, in the same order, that walk_window's own loop
   makes (hal/actor_registry.cpp, hal/fader_wipes.cpp, hal/sub_screen.cpp,
   hal/message_compositor.cpp) */
void port_actor_tick(void);          /* phases 4/2/3 */
void port_actor_render(void);        /* phase 5 */
void port_actor_scene_pass(void);    /* phase 1 */
void port_fader_advance(void);
void hal_sub_screen_frame_begin(void);
void hal_sub_screen_present(unsigned int *dst, int w, int h);
void port_message_composite_engine_a(void *fb);
void sdat_host_tick(void);           /* hal/sdat/consumer.cpp */

extern int data_020a4b6c[8];         /* the scene tree: head, callback, 0 */

}  /* extern "C" */

DSSTATE_BEGIN
extern "C" {
/* Scene::SpawnIfNecessary's "a scene has already spawned" latch. The matched
   src/_ZN5Scene21AfterCleanupResourcesEj.cpp DEFINES this byte (a namespace-
   scope `unsigned char data_02092660;` inside extern "C", which in C++ is a
   definition, not a tentative one), and a definition inside src/ cannot be
   bracketed into .dsstate without editing the byte-verified tree. So that TU
   is a host copy for this lane -- port/unmatched/Scene_AfterCleanupResources
   .cpp, same body, the global declared extern -- and the byte lives here,
   inside the bracket, where a save state captures it.
   The arm9 BSS words the star select's own path reaches and no level path
   does. Each is sized by ROM SPAN (the next symbol's address), not by the
   width of the field the one caller happens to touch -- the undersized-global
   trap. All five are bss, so zeroed host storage reads exactly what the DS's
   own cleared BSS reads:
     data_0209d3c0  4   func_02019018's 3D-engine-enabled flag
     data_0209d464  4   func_020190b8's "3D geometry engine armed" latch
     data_0209d478  4   SetBg2Offset's Y
     data_0209d49c  4   SetBg2Offset's X
     data_020a60a8  4   GXS::BeginLoadOBJExtPltt's saved bank bits
     data_020a8048  4   the Vram__Map family's LCDC cursor
     data_020a804c 12   and its three-word bank record                       */
int data_0209d3c0;
int data_0209d464;
int data_0209d478;
int data_0209d49c;
int data_020a60a8;
int data_020a8048;
int data_020a804c[3];
unsigned char data_02092660;
}
DSSTATE_END

/* ---- the one PORT_HOST_ABI face this lane needs ---------------------------
   PORT_HOST_ABI: LoadArchive mounts a NARC through the DS card loader, and the
   host has no mount step to make.
   src/LoadArchive.c walks data_0208ecf4, the ROM's 13-entry archive-mount
   table of {ptr, heap, idBase, idEnd, shortName, narcPath}, and calls
   func_02018934 to pull the whole NARC into the game heap. The port resolves
   archive-interior file IDs (>= 0x8000) LAZILY instead: hal/fs.cpp's
   port_fs_archive_fill walks port_archive_map -- the host-shaped copy
   port/tools/romdata.py generates from that same ROM table -- finds the
   archive whose id range covers the request, loads its image off disk on first
   use and decodes the member. So on the host an archive is never "mounted" and
   never "not mounted"; the id resolves either way.
   That makes the ROM's mount call a no-op whose only observable is its return
   value, which is "is archive N available". On the host every archive in the
   table is available, so the answer is 1. Returning it is what lets
   LoadTextNarcs (matched, in the slice) run its real language switch and what
   lets dScStarSel_c::InitResources proceed to the LoadFile calls the fs seam
   really does serve.
   The alternative -- linking the matched TU -- would need data_0208ecf4 hosted
   as raw ROM bytes with its DS string pointers, plus func_02018934 and the
   four-TU card-loader chain under it, to produce a mount the fs seam then
   ignores. That is a fake, not a fix, so the seam is here in the open. */
extern "C" int LoadArchive(int idx)
{
    return (unsigned)idx < 13u;
}

/* ---- C-name faces for three namespaced arm9 functions ---------------------
   The cxxname_bridge pattern in reverse: these three matched TUs define their
   function INSIDE a C++ namespace (`namespace GX { void SetBankForTex(u16) }`,
   `namespace G3X { void SetFog(bool,int,int,int) }`), so they export a mangled
   name, while src/func_ov003_020af8a0.c calls them by the C name every other
   arm9 spelling in the port uses. One forwarding definition each. Their
   siblings (SetBankForTexPltt, SetBankForSubBG, SetBankForSubOBJ,
   SetBankForSubOBJExtPltt) are .c TUs and already export the C name, which is
   why only these three are here. */
namespace GX { void SetBankForTex(unsigned short); void SetBankForOBJ(unsigned short); }
namespace G3X { void SetFog(bool enable, int a, int b, int c); }
extern "C" void _ZN2GX13SetBankForTexEt(unsigned short v) { GX::SetBankForTex(v); }
extern "C" void _ZN2GX13SetBankForOBJEt(unsigned short v) { GX::SetBankForOBJ(v); }
extern "C" void _ZN3G3X6SetFogEbiii(int e, int a, int b, int c)
{ G3X::SetFog(e != 0, a, b, c); }

/* The matched ActorBase methods for slots 13 and 14, and the reason this is a
   LOCAL declaration rather than include/ActorBase.h.
   MSVC encodes virtualness in the mangled name, and the TUs do not agree on
   it: src/_ZN9ActorBase9Virtual34Ejj.cpp and its Virtual38 sibling declare
   their method NON-virtual in a local struct (so the definitions are
   ?Virtual34@ActorBase@@QAEHII@Z, which is what hal/lk4_solidheap_seat.cpp
   already links against), while src/_ZN9ActorBase13OnHeapCreatedEv.cpp
   includes ActorBase.h and defines the VIRTUAL ?OnHeapCreated@ActorBase@@UAE_NXZ
   that hal/actor_vtables.cpp already links against. Only
   the two Virtual3x are the ones this file needs, so this file declares them
   non-virtual and hal/scene_actor_faces.cpp -- which includes the real header
   -- carries everything that has to be a virtual method. */
struct ActorBase {
    int Virtual34(unsigned a, unsigned b);           /* slot 13 body */
    int Virtual38(unsigned a, unsigned b);           /* slot 14 body */
};

// ---- the shared eleven -----------------------------------------------------
static int  __fastcall sc_binit(void *s, void *)
{ return _ZN5Scene19BeforeInitResourcesEv(s); }
static void __fastcall sc_ainit(void *s, void *, unsigned a)
{ port_scene_after_init(s, a); }
static int  __fastcall sc_bclean(void *s, void *)
{ return _ZN5Scene22BeforeCleanupResourcesEv(s); }
static void __fastcall sc_aclean(void *s, void *, unsigned a)
{ _ZN5Scene21AfterCleanupResourcesEj(s, a); }
static int  __fastcall sc_bbeh(void *s, void *)
{ return _ZN5Scene14BeforeBehaviorEv(s); }
static void __fastcall sc_abeh(void *s, void *, unsigned a)
{ port_scene_after_behavior(s, a); }
static int  __fastcall sc_bren(void *s, void *)
{ return _ZN5Scene12BeforeRenderEv(s); }
static void __fastcall sc_aren(void *s, void *, unsigned a)
{ port_scene_after_render(s, a); }
static int  __fastcall sc_v34(void *s, void *, unsigned a, unsigned b)
{ return ((ActorBase *)s)->ActorBase::Virtual34(a, b); }
static int  __fastcall sc_v38(void *s, void *, unsigned a, unsigned b)
{ return ((ActorBase *)s)->ActorBase::Virtual38(a, b); }
static int  __fastcall sc_heap(void *s, void *)
{ return port_scene_on_heap_created(s); }

static void scene_fill_shared(void **vt)
{
    vt[1]  = (void *)sc_binit;
    vt[2]  = (void *)sc_ainit;
    vt[4]  = (void *)sc_bclean;
    vt[5]  = (void *)sc_aclean;
    vt[7]  = (void *)sc_bbeh;
    vt[8]  = (void *)sc_abeh;
    vt[10] = (void *)sc_bren;
    vt[11] = (void *)sc_aren;
    vt[13] = (void *)sc_v34;
    vt[14] = (void *)sc_v38;
    vt[15] = (void *)sc_heap;
}

// ---- dScStarSel_c, id 4 ------------------------------------------------
//
// THE TICK WITNESS. One counter per dispatched slot, printed at the end of the
// run. A scene that "boots" is not a scene that RUNS, and the difference is
// invisible from the outside: the object exists either way. These say how many
// times the ROM's own processing lists actually entered ov003 code, which is
// the only evidence that the chain is live rather than merely linked.
static unsigned g_ss_hits[18];
static int  __fastcall ss_init(void *s, void *)
{ ++g_ss_hits[0];  func_ov003_020af8a0(s); return 1; }
static int  __fastcall ss_clean(void *, void *)
{ ++g_ss_hits[3];  return func_ov003_020af86c(); }
static int  __fastcall ss_beh(void *s, void *)
{ ++g_ss_hits[6];  return func_ov003_020af038(s); }
static int  __fastcall ss_render(void *s, void *)
{ ++g_ss_hits[9];  return func_ov003_020ae6f4(s); }
static int  __fastcall ss_pdes(void *, void *)
{ ++g_ss_hits[12]; func_ov003_020ae6f0(); return 0; }
/* the SM64DS_SCENE_SLOT9=0 stand-in. Counted separately so a run can never
   read its no-op as the real Render having run. */
static unsigned g_ss_render_skipped;
static int  __fastcall ss_render_noop(void *, void *)
{ ++g_ss_render_skipped; return 1; }
static void *__fastcall ss_d2(void *s, void *)
{ return (void *)(size_t)func_ov003_020addfc(s); }
static void *__fastcall ss_d0(void *s, void *)   { return func_ov003_020ade54(s); }

static void scene_fill_starsel(void)
{
    void **vt = data_ov003_020b1704;
    scene_fill_shared(vt);
    vt[0]  = (void *)ss_init;
    vt[3]  = (void *)ss_clean;
    vt[6]  = (void *)ss_beh;
    /* SM64DS_SCENE_SLOT9=0 leaves slot 9 on a no-op. The A/B that separates
       "the scene's own Render hangs or faults" from "the port's render bucket
       walks its list wrong", which nothing else in the port can tell apart:
       both show up as the frame never finishing. The default is the real body;
       this is a diagnostic, not a fallback, and the battery never sets it. */
    {
        const char *s9 = std::getenv("SM64DS_SCENE_SLOT9");
        vt[9] = (s9 && s9[0] == '0') ? (void *)ss_render_noop
                                     : (void *)ss_render;
    }
    vt[12] = (void *)ss_pdes;
    vt[16] = (void *)ss_d2;
    vt[17] = (void *)ss_d0;
}

// ---- the registry seat -----------------------------------------------------
//
// Deliberately NOT a row in hal/actor_classes.inc. That table is the LEVEL
// cast, walked by the census and by the SM64DS_SKIP_CLASS knob, and a scene is
// not part of any level's cast. The seat is the same three statements
// port_actor_registry_install makes per row -- repoint the factory word, park
// the record in data_020a4bb8, run the fill -- and it makes exactly the same
// cross-check.
struct PortSceneClass {
    unsigned short id;
    const char *name;
    unsigned char *info;
    void *(*factory)(void);
    void (*fill)(void);
};

/* ONE ROW. dScTitle_c (id 2) and dScGameOver_c (id 8) are derived to the same
   depth -- SpawnInfo, factory and all eighteen vtable slots are recorded in
   port/ov003_syms.txt -- and are NOT seated, for two reasons that are named in
   full at the bottom of port/slice_scene1.txt: eleven of their fourteen slot
   bodies carry the "recovered from vtable slot identity" marker that
   port/tools/inferred_stub_guard refuses until each is ruled against the ROM,
   and three of their dtor TUs spell their vptr writes as per-TU placeholders
   (VT0/VT1/VT2, _ZTV10dScTitle_c, _ZTV8dScene_c, _ZTV7dBase_c, G0) that exist
   in no config. Adding them is a row here plus a block in the slice. */
static const PortSceneClass port_scene_classes[] = {
    {4, "SCENE_STAR_SELECT", StarSelect_SpawnInfo, StarSelect_Spawn,
     scene_fill_starsel},
    {0, 0, 0, 0, 0},
};

extern "C" const char *port_scene_class_name(unsigned id)
{
    for (const PortSceneClass *k = port_scene_classes; k->name; ++k)
        if (k->id == id)
            return k->name;
    return "?";
}

extern "C" void port_scene_registry_install(void)
{
    int n = 0;
    for (const PortSceneClass *k = port_scene_classes; k->name; ++k) {
        unsigned rec = *(unsigned short *)(k->info + 4);
        if (rec != k->id) {
            std::fprintf(stderr, "  [scene] %s: SpawnInfo at %p says id %u, "
                         "the spawn table says %u -- WRONG RECORD\n", k->name,
                         (void *)k->info, rec, k->id);
            continue;
        }
        *(void **)(k->info + 0) = (void *)k->factory;
        data_020a4bb8[k->id] = k->info;
        k->fill();
        ++n;
    }
    std::printf("[scene] %d scene classes registered (ov003)\n", n);
}

// ---- the boot ---------------------------------------------------------------
//
// SM64DS_SCENE=<id> mirrors SM64DS_LEVEL: it names what to boot and nothing
// else. port_scene_env_want returns -1 when it is unset, which is the level
// harness's "not a scene run".
extern "C" int port_scene_env_want(void)
{
    static int want = -2;
    if (want != -2)
        return want;
    const char *e = std::getenv("SM64DS_SCENE");
    want = e ? std::atoi(e) : -1;
    return want;
}

/* Boot the scene. Two calls into matched arm9 and a report; the port does not
   spawn anything itself and does not touch data_02092664 by hand. Returns the
   scene object the ROM's spine made, or null. */
extern "C" void *port_scene_boot(int id)
{
    if (id < 0 || id >= 512 || !data_020a4bb8[id]) {
        std::fprintf(stderr, "FATAL: scene %d is not a hosted scene. Hosted:",
                     id);
        for (const PortSceneClass *k = port_scene_classes; k->name; ++k)
            std::fprintf(stderr, " %u (%s)", k->id, k->name);
        std::fprintf(stderr, "\n");
        return 0;
    }
    std::printf("[scene] %d = %s\n", id, port_scene_class_name((unsigned)id));

    /* THE ROM'S OWN TWO CALLS. SetSceneToSpawn parks the id; SpawnIfNecessary
       runs the spine. data_02092660 is the "already spawned" latch and starts
       zeroed, so the second call takes its spawning branch. */
    _ZN5Scene15SetSceneToSpawnEjj((unsigned)id, 0);
    const int r = _ZN5Scene16SpawnIfNecessaryEv();
    if (!r) {
        std::fprintf(stderr, "  [scene] Scene::SpawnIfNecessary declined "
                     "(pending id %u, latch %u)\n",
                     (unsigned)data_02092664, (unsigned)data_02092660);
        return 0;
    }
    std::printf("[scene] spawned %p, vptr %p\n", (void *)(size_t)r,
                *(void **)(size_t)r);
    return (void *)(size_t)r;
}

// ---- the run ---------------------------------------------------------------
//
//   SM64DS_SCENE=<id>            which scene (2, 4 or 8). Nothing else here
//                                runs unless this is set.
//   SM64DS_SCENE_FRAMES=<n>      how many frames (default 300, the battery's).
//   SM64DS_SCENE_BMP=<path>      write the last frame.
//   SM64DS_SCENE_NO_RENDER=1     tick only, no render bucket. The A/B that
//                                separates a Behavior fault from a Render one.
//   SM64DS_SCENE_TRACE=1         name each render sub-step on stderr before
//                                it runs, so a HANG can be attributed. A
//                                fault already names itself through the fault
//                                probe; a hang leaves nothing behind without
//                                this.
//   SM64DS_SCENE_SUBLEVEL=<n>    which sublevel the scene is ABOUT. Default 6.
//   SM64DS_SCENE_SLOT9=0         leave the scene's Render slot on a no-op.
//
// ---- SM64DS_SCENE_SUBLEVEL, and why a default of 0 is not an option --------
//
// dScStarSel_c is a scene ABOUT a course: every branch in its InitResources,
// its Behavior and its Render is gated on SublevelToLevel(data_02092110), the
// arm9 SUBLEVEL_LEVEL_TABLE (0x02075298) lookup that turns the current
// sublevel into a course number. The ROM never reaches this scene without one
// -- you pick a course, the game latches the sublevel, and the star select
// comes up for it -- so the sublevel is an INPUT to the scene the way
// SM64DS_LEVEL is an input to a level boot, and the harness has to supply it.
//
// A run that does not is not neutral, it is invalid, and this cost the lane a
// hang before the reason was found. data_02092110 starts at 0, and
// SUBLEVEL_LEVEL_TABLE[0] is 0xff -- read back through its own declaration
// (include/decl_common.h: `extern signed char SUBLEVEL_LEVEL_TABLE[]`) that is
// -1, the "not a level" sentinel. Every `< 0xF` and `<= 0xE` gate in the scene
// then passes, so InitResources' star block (guarded by an UNSIGNED
// `(u32)level <= 0xE`, which 0xff fails) never runs and never writes the star
// count at +0x114, while Render's loops (guarded by the SIGNED compare, which
// -1 passes) walk that uninitialised count. Measured: the run hangs inside
// dScStarSel_c::Render on frame 0, with slot 9 no-op'd it completes, and the
// count at +0x114 is whatever the heap left there.
//
// The default is 6, the first main course (SUBLEVEL_LEVEL_TABLE[6] = 0,
// Bob-omb Battlefield), because it is the smallest real answer: course 0 with
// a zeroed save gives a one-entry star grid.
//
// Returns the process exit code. Called from walk_window's main once the host
// bring-up is done, and it does not return to the level path.
extern "C" int port_scene_run(void)
{
    const int scene = port_scene_env_want();
    const int frames = std::getenv("SM64DS_SCENE_FRAMES")
                           ? std::atoi(std::getenv("SM64DS_SCENE_FRAMES")) : 300;
    const int no_render = std::getenv("SM64DS_SCENE_NO_RENDER") != 0;
    const char *bmp = std::getenv("SM64DS_SCENE_BMP");
    const int trace = std::getenv("SM64DS_SCENE_TRACE") != 0;
    const char *sub = std::getenv("SM64DS_SCENE_SUBLEVEL");
    const int sublevel = sub ? std::atoi(sub) : 6;

    /* The seat, minus the Stage. Everything in it -- the message archive, the
       registry and its gate, the five processing-list callbacks, the model and
       heap vtable seats -- is bring-up a scene needs exactly as much as a level
       does. The one thing a scene must not get is a Stage actor: on the DS the
       Stage IS the level scene (ACTOR_SPAWN_TABLE[3], ids 3/6/7 -> ov002), and
       two scene roots is not a state the game can be in. */
    port_scene_a2_seat();

    /* THE SDAT ROOT, before anything can ask the sequencer a question.
       dScStarSel_c::InitResources opens with Sound::LoadInitialGroup(3) and
       Sound::LoadAndSetMusic_Layer1(0x16 or 0x24) -- the scene picks its own
       music the way a level picks its course theme -- and
       LoadAndSetMusic_Layer1 walks data_020a5bb8 + 0x84 UNCONDITIONALLY, with
       no self-initialising branch the way Sound::Play has. With the root
       unseated that is a read of 0x00000084 off a null base, which is exactly
       what the first scene boot faulted on (eax=0, accessing 00000084, inside
       Sound::InfoSequenceEntry::GetWithID +0x8).
       One tick with nothing queued seats it. This is the same call and the
       same reason hal/star_flow.cpp's seat_course_sound makes it before the
       level's own LoadGroupAndSetBank, moved to where a scene run needs it:
       on the DS the SDAT root is up long before any scene spawns, because the
       boot spine (func_0201a054 -> func_02053a8c / func_02053c40) brings the
       sound system up before Scene::PrepareToSpawnBoot ever runs. The level
       harness gets it inside port_stage_a_boot, which a scene run does not
       call, so it belongs here. */
    sdat_host_tick();

    /* THE SUBLEVEL THE SCENE IS ABOUT, before it spawns. See the header block:
       this is an input, not a default, and 0 is not a legal value for it. */
    data_02092110 = (signed char)sublevel;
    std::printf("[scene] sublevel %d -> course %d\n", sublevel,
                (int)SUBLEVEL_LEVEL_TABLE[sublevel & 0x3f]);

    void *obj = port_scene_boot(scene);
    if (!obj) {
        std::fprintf(stderr, "scene %d did not spawn\n", scene);
        std::fflush(stdout);
        return 3;
    }
    /* DID THE SCENE BECOME THE TREE ROOT? This is the ROM's own answer to "is
       this really the scene", not the port's. func_0203b438's no-parent branch
       writes the first SceneNode it is handed into data_020a4b6c[0], and on a
       scene run the scene actor is the first thing spawned, so the tree head
       must come back as the scene's own node -- ActorBase::sceneNode, pinned
       at +0x14 by the ROM's own `str r4, [r5, #0x10]` owner back-pointer (see
       include/ActorBase.h). A level run prints the same line with the Stage in
       that slot, which is the point: on the DS the Stage IS scene 3, and here
       something else is. */
    {
        void *node = (char *)obj + 0x14;
        void *head = (void *)(size_t)data_020a4b6c[0];
        std::printf("[scene] tree root %p, scene node %p (%s), pending id "
                    "0x%x, latch %u\n", head, node,
                    head == node ? "the scene IS the root"
                                 : "NOT the root, something else is",
                    (unsigned)data_02092664, (unsigned)data_02092660);
    }
    std::fflush(stdout);

    static ntr::Framebuffer fb;
    for (int frame = 0; frame < frames; ++frame) {
        hal_sub_screen_frame_begin();
        port_actor_tick();
        port_fader_advance();

        if (!no_render) {
            /* SM64DS_SCENE_TRACE=1 names the render sub-step a run is inside,
               unbuffered, so a HANG (not a fault, which the probe already
               catches) can be attributed without a debugger. Each line goes
               out before the step it names. */
            if (trace) std::fprintf(stderr, "[scene-trace] f%d gx_reset\n", frame);
            ntr::gx_reset();
            if (trace) std::fprintf(stderr, "[scene-trace] f%d actor_render\n", frame);
            port_actor_render();
            if (trace) std::fprintf(stderr, "[scene-trace] f%d clear\n", frame);
            for (int x = 0; x < ntr::SCREEN_W; ++x) fb.px[0][x] = 0xFF101820u;
            for (int y = 1; y < ntr::SCREEN_H; ++y)
                std::memcpy(fb.px[y], fb.px[0],
                            ntr::SCREEN_W * sizeof(fb.px[0][0]));
            if (trace) std::fprintf(stderr, "[scene-trace] f%d gx_render\n", frame);
            ntr::gx_render(fb);
            if (trace) std::fprintf(stderr, "[scene-trace] f%d composite\n", frame);
            port_message_composite_engine_a(&fb);
            if (trace) std::fprintf(stderr, "[scene-trace] f%d sub_present\n", frame);
            hal_sub_screen_present(&fb.px[0][0], ntr::SCREEN_W, ntr::SCREEN_H);
            if (trace) std::fprintf(stderr, "[scene-trace] f%d render done\n", frame);
        }
        port_actor_scene_pass();

        if (frame == 0)
            std::printf("[scene] f0 ticked\n");
        std::fflush(stdout);
    }

    if (bmp && !no_render)
        ntr::ppu_write_bmp(bmp, fb);
    /* THE WITNESS. Not "the scene booted" -- how many times each of the class's
       own slots was entered by the ROM's own processing lists. A scene that
       spawns and a scene that RUNS look identical from outside: the object
       exists either way, and the [scene] lines above would print the same for
       an object nothing ever ticked. */
    std::printf("[scene] ov003 slot hits: init %u, behavior %u, render %u, "
                "cleanup %u, pending-destroy %u%s\n",
                g_ss_hits[0], g_ss_hits[6], g_ss_hits[9], g_ss_hits[3],
                g_ss_hits[12],
                g_ss_render_skipped
                    ? "  [RENDER SLOT NO-OP'd: SM64DS_SCENE_SLOT9=0]" : "");
    std::printf("[scene] %d frames of scene %d (%s), clean\n", frames, scene,
                port_scene_class_name((unsigned)scene));
    std::fflush(stdout);
    return 0;
}
