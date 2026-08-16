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
// repointed at the host factory, seated in data_020a4bb8 at the class's id,
// plus a vtable fill. Nothing in this file is a new mechanism. What is new is only
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
// 3. THE OAM SPRITE TEMPLATES, SPELLED AS FUNCTIONS. Four TUs of the star
//    select's closure name SEVENTEEN distinct ov001 sprite-template tables as
//    `extern void *func_020abXXXX[]` -- thirteen of them in
//    dScStarSel_c::Render (src/func_ov003_020ae6f4.cpp) alone. No func_020ab*
//    symbol exists in any config: the addresses are ov001 DATA, and the
//    "func_" prefix is a decomp-side guess at what lives there. Each alias
//    below binds the guess to the address's real name, read out of
//    config/arm9/overlays/ov001/symbols.txt.
//    FIVE OF THE SEVENTEEN WERE ALREADY ALIASED, in hal/sub_actors.cpp's own
//    ov001 block, because the HUD's render TUs spell them the same way -- the
//    same defect in a different overlay's source (020ab948, 020ab9c8,
//    020aba70, 020abad0, 020abad8; that block's sixth entry, 020abd88, this
//    closure does not use). Those five are not repeated here, so the twelve
//    below are exactly the spellings this lane is the first caller of. Ten of
//    the twelve addresses were not in the port's ov001 mount either and joined
//    it this lane (port/ov001_syms.txt, which also records why the address is
//    ov001's and not ov000's).
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

// ---- run link60 lane L2: the two dScDSMT_c PLACEHOLDER SPELLINGS -----------
//
// The ov007 dtor pair spells its vptr restores with the per-TU placeholder
// names the ov003 write-up named as blocker 2 for dScTitle_c. Both are
// resolved here to the address the ROM actually stores, read out of the
// literal pool of each body in extracted/overlays/overlay_0007.bin:
//
//   func_ov007_020cc028 (D2)   ldr r1,=0x021032e8 ; str -> _ZTV9dScDSMT_c
//                              ldr r0,=0x02092680 ; str -> _ZTV8dScene_c
//                              ldr r1,=0x0208e4b8 ; str -> _ZTV7dBase_c
//   func_ov007_020cc070 (D0)   the same three, spelled VT0 / VT1 / VT2, then
//                              Memory::Deallocate(this, *0x020a0eac) spelled G0
//
// 0x02092680 is _ZTV5Scene and 0x0208e4b8 is _ZTV12ActorDerived, both matched
// arm9 data symbols already in the build, and 0x021032e8 is the class's own
// table inside the ov007 mount. TWO of the six names need a face:
//
//   _ZTV9dScDSMT_c   UNIQUE to src/func_ov007_020cc028.c -- no other TU in the
//                    tree spells it -- so aliasing it to the ROM's own address
//                    is exact and cannot collide.
//   _ZTV8dScene_c    spelled by four TUs (ov003 x2, ov005, ov007), all of them
//                    Scene subclasses restoring the SAME base table, and none
//                    of the other three is in any slice today. _ZTV5Scene is
//                    the right answer for all four.
//
// The other four resolve to storage that already exists and this file adds
// nothing for them: _ZTV7dBase_c is hal/sub_actors.cpp's TRAP-FILLED 18-slot
// array, VT0/VT2 are hal/actor_vtables.cpp's shared placeholders, VT1 is
// hal/auto_bss.cpp's and G0 is hal/cxxname_bridge.cpp's. That is a KNOWN
// DIVERGENCE and not a fix: on the ROM D0's three stores put back
// 0x021032e8 / _ZTV5Scene / _ZTV12ActorDerived, and on the host they put back
// three unrelated arrays. It is the doctrine hal/actor_vtables.cpp already
// states -- "installed transiently during teardown and never dispatched" --
// and a dispatch through _ZTV7dBase_c traps rather than going quiet. Neither
// dtor is entered in any run this lane made; see port/ov007_seat.txt.
#pragma comment(linker, "/alternatename:__ZTV9dScDSMT_c=_data_ov007_021032e8")
#pragma comment(linker, "/alternatename:__ZTV8dScene_c=__ZTV5Scene")

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

/* ---- run link60 lane L2: dScDSMT_c, the TITLE SCREEN, scene id 1 ----------
   The ov007 mount (port/ov007_syms.txt) hosts all three records this needs, so
   unlike the ov003 seat NOTHING here is a fresh host array: the SpawnInfo, the
   class vtable and the graphCallback_c sub-object vtable are the mount's own
   storage at the ROM's names and the ROM's spacing, and the fill writes host
   thunks INTO them. That is the right treatment when the table is inside a
   mounted data span -- defining a second array of the same name would be a
   duplicate symbol, and leaving the mounted one alone would leave eighteen raw
   DS code addresses live in a table the factory installs. */
extern unsigned char data_ov007_02103264[];  /* SpawnInfo, 8 bytes, +4 reads 1 */
extern unsigned char data_ov007_021032e8[];  /* _ZTV9dScDSMT_c, 88 bytes/22 words */
extern unsigned char data_ov007_021032b0[];  /* graphCallback_c, 16 bytes/4 words */
void port_ov007_pack_check(void);            /* generated by tools/ovdata.py */
void port_ov007_syms_patch(void);

int *func_ov007_020ccad0(void);              /* the factory */
int  func_ov007_020cc4c0(char *self);        /* slot 0  InitResources */
int  func_ov007_020cc45c(void);              /* slot 3  CleanupResources */
int  func_ov007_020cc2cc(char *self);        /* slot 6  Behavior */
int  func_ov007_020cc2b0(void *self);        /* slot 9  Render */
void func_ov007_020cc2ac(void);              /* slot 12 OnPendingDestroy */
int *func_ov007_020cc028(int *self);         /* slot 16 D2 */
int *func_ov007_020cc070(int *self);         /* slot 17 D0 */
/* THE MOUNT'S OTHER EIGHT RAW CODE WORDS. port/ov007_binding_diff.txt section
   3 counts eighteen pointer words the mount leaves holding DS addresses
   because ov007's .text is not mounted, and asks the wiring lane the right
   question about each: "is it dereferenced". The answer for every one of the
   eighteen is now YES, and here is the whole set with where each is handled:
     7   the class vtable's own overridden slots      scene_fill_title
     2   the graphCallback_c table's two ov007 slots  scene_fill_title
     1   the SpawnInfo's +0 factory word              port_scene_registry_install
     6   data_ov007_02103290, the graph-callback      HERE
         table InitResources hands to
         func_ov007_020b7138 as an argument
     2   data_ov007_02103254 / _02103258              HERE
   The six were found the hard way: with the vtables filled and the table left
   raw, the boot faulted at eip 0x01cccab4 accessing 0x020ccab4, which is
   func_ov007_020ccab4's DS address dispatched as a host one, three frames
   under InitResources (func_ov007_020b7138 -> func_ov007_020b68e8 ->
   func_ov007_020c3df4). All eight bodies are matched and in the slice. */
extern unsigned char data_ov007_02103290[];  /* 6 callbacks + the vtable head */
extern unsigned char data_ov007_02103254[];
extern unsigned char data_ov007_02103258[];
int  func_ov007_020ccab4(int a);
int  func_ov007_020cca98(int a);
void func_ov007_020cca80(void);
int  func_ov007_020cca74(void);
int  func_ov007_020cca68(void);
unsigned char func_ov007_020cc600(int arg);
void func_ov007_020c3e4c(void *arg);
void func_ov007_020c3e64(void *arg);
/* the graphCallback_c sub-object's own four, at +0x50 */
int  func_ov007_020cc110(void);              /* gc slot 0 */
int  func_ov007_020cc0f4(void *self);        /* gc slot 2 */
int  _ZN5Scene14GraphCallback1Ev(void *self);/* gc slot 1, matched arm9 */
int  _ZN5Scene14GraphCallback3Ev(void *self);/* gc slot 3, matched arm9 */

/* the seat, minus the Stage (hal/level_boot.cpp) */
void _ZN5Scene9SetFadersEP15FaderBrightness(void *thiz);

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
extern void *data_0209f5bc;          /* the installed fader; hal/fader_wipes.cpp */

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

// ============================================================================
// run link60 lane L2: everything the ov007 slice needed that was not a TU.
// ============================================================================
//
// ---- 1. THE TITLE SCREEN'S OWN arm9 STORAGE -------------------------------
//
// Eight arm9 globals no level path has ever reached. Five of them are ONE
// LOGICAL OBJECT that dsd split into five symbols, and getting that wrong is
// the undersized-global trap in its sharpest form: dScDSMT_c::InitResources
// writes data_0209b340[0x27], which is byte +0x9c of a symbol whose delta to
// the next dsd name is ELEVEN. Five separate host globals would put that store
// 145 bytes past the end of its array.
//
// So the run 0x0209b33c .. 0x0209b3ec is hosted CONTIGUOUSLY, at the ROM's own
// spacing, using the ordered-section idiom tools/ovdata.py already uses for a
// packed mount, and port_l2_pack_check() proves the layout at run time rather
// than trusting the linker to have honoured it. Sizes are ROM deltas from
// config/arm9/symbols.txt: 4, 11, 3, 138, 20.
//
// All five are BSS, so zeroed host storage reads exactly what the DS's own
// cleared BSS reads. They sit inside .dsstate so a save state captures them.
#pragma section(".dsstate$l2_00", read, write)
#pragma section(".dsstate$l2_01", read, write)
#pragma section(".dsstate$l2_02", read, write)
#pragma section(".dsstate$l2_03", read, write)
#pragma section(".dsstate$l2_04", read, write)
extern "C" {
/* the file-select record pointer */
__declspec(allocate(".dsstate$l2_00")) __declspec(align(1))
unsigned char data_0209b33c[4];
/* the title state block. ELEVEN bytes by dsd delta, reached at +0x9c */
__declspec(allocate(".dsstate$l2_01")) __declspec(align(1))
unsigned char data_0209b340[11];
__declspec(allocate(".dsstate$l2_02")) __declspec(align(1))
unsigned char data_0209b34b[3];
__declspec(allocate(".dsstate$l2_03")) __declspec(align(1))
unsigned char data_0209b34e[138];
__declspec(allocate(".dsstate$l2_04")) __declspec(align(1))
unsigned char data_0209b3d8[20];
}

DSSTATE_BEGIN
extern "C" {
/* The three that stand alone. data_0209d4a8 is the Scene fader pointer
   InitResources parks and CleanupResources clears; data_0209d524 is the
   "object overlays are loaded" flag both InitResources and Behavior branch on;
   data_0208ee3c is arm9 .DATA, not bss, so it carries the ROM's own value --
   read at extracted/arm9_dec.bin + (addr - 0x02004000), the base
   port/tools/romdata.py documents, which is 01 00 00 00. Hosting it zeroed
   would have been a silent one-bit lie. */
/* data_0209d4a8 is NOT defined here: hal/w8a_stage_storage.cpp already hosts
   it. Only the mangled spelling is added, below. */
int data_0209d524;
unsigned char data_0208ee3c[4] = { 1, 0, 0, 0 };
}
DSSTATE_END

static void port_l2_pack_check(void)
{
    struct { const unsigned char *p; int want; const char *n; } k[] = {
        { data_0209b33c,   0, "data_0209b33c" },
        { data_0209b340,   4, "data_0209b340" },
        { data_0209b34b,  15, "data_0209b34b" },
        { data_0209b34e,  18, "data_0209b34e" },
        { data_0209b3d8, 156, "data_0209b3d8" },
    };
    for (int i = 0; i < 5; ++i)
        if (k[i].p - data_0209b33c != k[i].want)
            std::fprintf(stderr, "  [scene] L2 PACK BROKEN: %s at +%d, ROM "
                         "says +%d\n", k[i].n,
                         (int)(k[i].p - data_0209b33c), k[i].want);
}

// ---- 1b. THREE HAND-ASM PRIMITIVES, HOSTED ---------------------------------
//
// PORT_HOST_ABI: all three TUs are `asm void` blocks, which MSVC cannot parse.
// src/Matrix3x3_LoadIdentity.c, src/MultiStore32Bytes.c and src/func_02052ec8.c
// carry the tree's HAND-ASM PRIMITIVE banner -- they were assembly in the
// original and there is no C to decompile them to -- so they are transcribed
// here instead of being compiled, they are NOT in the slice, and none of the
// three counts as linked. The asm is short enough to state and to check
// against, and the whole slice was swept for the pattern so these are all of
// them rather than the ones that happened to surface.
//
//   func_02052ec8(m)   the 4x4 sibling of the one below: seven stmia bursts
//     ({r2,r3} {r1,r3} {r1,r2,r3} {r1,r3} {r1,r2,r3} {r1,r3} {r1,r2}) with
//     r2 = 0x1000 and r1 = r3 = 0, which lands 0x1000 at words 0, 5, 10 and 15
//     and zero everywhere else -- sixteen words, a 4x4 identity. The register
//     list order is what fixes the pattern: stmia stores lowest register
//     first, so {r1,r2,r3} is 0, 0x1000, 0 and not 0x1000, 0, 0.
//
//   Matrix3x3_LoadIdentity(m)   mov r2,#0x1000 / str r2,[r0,#0x20] / mov r3,#0
//                               / stmia r0!,{r2,r3} / mov r1,#0
//                               / stmia r0!,{r1,r3} / stmia r0!,{r2,r3}
//                               / stmia r0!,{r1,r3}
//     which lays down 0x1000,0,0, 0,0x1000,0, 0,0,0x1000 -- a 3x3 identity in
//     the game's 1.12 fixed point, with [8] written FIRST and the other eight
//     in four pairs. Its one caller, src/func_ov007_020c42f8.c, declares it
//     `void Matrix3x3_LoadIdentity(Mat3 *m)`, which is the shape used here.
//
//   MultiStore32Bytes(val,dst,len)  fills len BYTES from dst with the 4-byte
//     val: eight words at a time up to (len >> 5) << 5, then one word at a
//     time up to dst + len. All five call sites declare the same three
//     parameters, so there is no argument-count question here.
extern "C" void func_02052ec8(int *m)
{
    for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 0x1000;
}
extern "C" void Matrix3x3_LoadIdentity(int *m)
{
    m[8] = 0x1000;
    m[0] = 0x1000; m[1] = 0;
    m[2] = 0;      m[3] = 0;
    m[4] = 0x1000; m[5] = 0;
    m[6] = 0;      m[7] = 0;
}
extern "C" void MultiStore32Bytes(unsigned val, int *dst, int len)
{
    for (int i = 0; i + 4 <= len; i += 4)
        *dst++ = (int)val;
}

// ---- 2. THE OVERLAY LOADER, WHICH THE HOST DOES NOT HAVE -------------------
//
// The four overlay-id symbols dScDSMT_c names. On the DS these are LINKER
// symbols whose ADDRESS is the id -- src spells the argument `(int)&overlay_100`
// -- and MSVC has no way to give a C++ global an absolute address, so on the
// host `&overlay_100` is an ordinary host address and NOT 100. Stated rather
// than papered over.
//
// It costs nothing measurable and the reason is worth writing down. The two
// consumers are both linked matched TUs and both are keyed on the id:
//   func_02017e94(id)  unload. It scans data_0209d3c4[12] for an entry whose
//                      first word equals the key and returns early when there
//                      is none. InitResources calls it twice, UNCONDITIONALLY,
//                      on the first frame -- so this one really does run, and
//                      with a host-address key it finds nothing and returns,
//                      which is also what the ROM does when the overlay is not
//                      resident.
//   LoadOverlay(id)    load, reached only from Behavior's result == 6 branch,
//                      which is a menu confirm. No idle run reaches it.
// Neither could do the real thing anyway: the port has no overlay loader,
// because every overlay it hosts is a static host array mounted at build time
// (this file's own section 2). The ids are here so the symbols resolve, and
// the day an overlay loader exists they become its first customer.
extern "C" { int overlay_64, overlay_66, overlay_100, overlay_102; }

/* THE FOUR ENTRY POINTS THEMSELVES, faced rather than linked, and this is the
   one place this lane traded linkage for honesty on purpose.
   All four have matched src TUs and all four were IN the slice for one build.
   Taking them meant taking the DS card overlay/archive loader under them --
   FS_LoadOverlay, func_02018c00, func_0203d7b8, func_0205e088, func_02017fd0,
   func_02018908, func_0205dc0c -- plus hosting data_0208ecf4, the 13-entry
   archive-mount table whose entries are DS STRING POINTERS, and data_02075998
   / data_02075804, the object-overlay id tables. Eleven TUs and three
   pointer-bearing arm9 tables, to drive a load that resolves to nothing:
   the host has no overlay loader at all, because every overlay it hosts is a
   static host array mounted at build time (section 2 of this file), and
   hal/fs.cpp resolves archive-interior file ids lazily so an archive is never
   mounted and never not mounted. That is the same trade the LoadArchive face
   above already makes, in the same direction, and it is recorded as a cost:
   ELEVEN MATCHED TUs THIS SLICE COULD HAVE COUNTED AND DID NOT.
   The observable each face has to reproduce is nothing: three return void and
   the fourth is void, none has an out-parameter, and the ROM's own answer when
   the overlay is not resident is to do nothing. */
extern "C" void LoadOverlay(int)                       {}
extern "C" void func_02017e94(int)                     {}
extern "C" void UnloadArchives(void)                   {}
extern "C" void LoadOrUnloadObjectOverlays(void (*)(int), int) {}

/* PORT_HOST_ABI: CP15::EnableDTCM sets the ARM946 control register's DTCM
   enable bit, and the host has no CP15. Its TU is a fourth `asm` block MSVC
   cannot parse (`asm { mrc p15,0,v,c1,c0,0 }` / or 0x10000 / `mcr` back), so
   it is not in the slice and does not count as linked. The ROM's observable is
   the control-register value it returns, and the one thing any caller does
   with that is hand it back to CP15::DisableDTCM to restore. Returning the bit
   set is the answer that round-trips; there is no host state to change. */
extern "C" unsigned _ZN4CP1510EnableDTCMEv(void) { return 0x10000u; }

/* THE DS BACKUP-MEDIA DRIVER, the second family this lane refused, and the
   second time for the same reason. SaveData::ReadDataFromCart and
   SaveData::SaveDataToCart are matched TUs and were both in the slice for one
   build; under them is the card/backup driver (func_0203da3c, func_0206045c,
   func_02057020, func_0205ff80, func_0205ff70, func_02057078 and the rest of
   the CARD_ chain), which the host does not have and which no amount of
   hosted arm9 data substitutes for. Their four callers -- ReadFileData,
   ReadMinigameData, EraseSaveFile and SaveFile -- ARE in the slice and do
   link, so what is faced is the two leaves and not the save logic above them.
   THE OBSERVABLE. Both return an int the caller reads as "did the transfer
   happen". Returning 0 is the ROM's own answer on a failed read, and the four
   callers' failure path is SetDefaultValues, which is exactly right for a
   host with no cart: the file select comes up on default save data. Returning
   1 would claim bytes were moved that were not. THIS IS WHY THE FILE SELECT
   SHOWS EMPTY FILES rather than whatever the last session had, and that is a
   real behaviour gap, not a cosmetic one -- it is named in
   port/ov007_seat.txt section 5 rather than left for someone to find. */
extern "C" int _ZN8SaveData16ReadDataFromCartEPcjj(char *, unsigned, unsigned)
{ return 0; }
extern "C" int _ZN8SaveData14SaveDataToCartEPcjj(char *, unsigned, unsigned)
{ return 0; }

// ---- 2b. FIVE MORE arm9 GLOBALS, AND ONE THAT IS A VTABLE ------------------
//
// data_0208ea6c IS NOT DATA, it is a twelve-slot vtable in arm9 .data, and it
// is on the critical path: func_02017278 (the factory's last call, which
// constructs the object's member at +0x54) writes four vptrs in construction
// order and this is the last of them, and func_02017254 (which BOTH dtors
// call) writes it back and then runs Color::D1. Its twelve words are all
// relocated code addresses -- 0x02017254, 0x02017228, 0x0201721c, 0x020171f0,
// 0x020171c8, 0x02017684, 0x02017670, 0x02017628, 0x0201761c, 0x02017610, a
// non-code word at +0x2c, 0x02017450.
//
// IT IS TRAP-FILLED, NOT ROM-FILLED, and that is a deliberate difference from
// its three siblings. data_0208eafc, data_0208eacc and data_0208eb2c -- the
// other three vptrs func_02017278 writes -- are already in the build carrying
// their raw ROM bytes, so a dispatch through any of them today jumps to a DS
// address. This one goes the other way, on hal/sub_actors.cpp's precedent: a
// dispatch lands on a named trap instead of on 0x02017254 interpreted as a
// host address. NOTHING DISPATCHED THROUGH IT IN ANY RUN THIS LANE MADE.
// Filling the twelve for real is twelve more bodies and is not this lane's.
static void l2_trap(const char *name);
static void l2_vt_trap(void) { l2_trap("data_0208ea6c vtable slot"); }
DSSTATE_BEGIN
extern "C" {
void *data_0208ea6c[12];
/* data_0208eb2c is the SAME SHAPE and the same critical path: func_02017278
   writes it as the third of the member's four vptrs, and its ten words are
   relocated code addresses too (0x020175c4, 0x02017598, 0x020174e0,
   0x020176d8, 0x02017698, ...). Trap-filled for the same reason. The other
   two the ctor writes, data_0208eafc and data_0208eacc, are already in the
   build carrying raw ROM bytes and this lane did not touch them. */
void *data_0208eb2c[10];
/* data_0208eacc, the SECOND of the member's four vptrs, joins them: 48 bytes,
   twelve words, relocated code addresses. Three of the four are now filled
   here and only data_0208eafc is still shipping raw ROM bytes, because
   nothing this lane compiled asked for it. */
void *data_0208eacc[12];
/* AND THE FOURTH, data_0208eafc, which closes the set func_02017278 writes.
   This one is NOT uniformly pointers: only three of its twelve words are
   relocated (+0 -> 0x0201786c, +4 -> 0x02017848, +44 -> 0x0208ea00) and the
   nine in between are literal zero in the ROM. So it carries its ROM bytes
   and only the three relocated words are trap-filled at seat time, which is
   the difference between hosting a vtable and hosting a table that happens to
   start with two function pointers. The +44 word points at another arm9 data
   symbol rather than at code and is left as the ROM has it. */
unsigned char data_0208eafc[48] = {
    108,120,1,2, 72,120,1,2, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
    0,0,0,0, 0,0,0,0, 0,0,0,0, 0,234,8,2
};
int data_020a80e4[8];                              /* bss, 32 by ROM span */
/* a 256-byte pure-data table func_020538b8 indexes. NO relocation anywhere in
   config/arm9/relocs.txt lands inside 0x02086214 .. 0x02086314, so the ROM
   bytes are the whole truth and they are carried verbatim. */
unsigned char data_02086214[256] = {
    0,0,81,0,163,0,244,0,70,1,151,1,233,1,58,2,
    139,2,220,2,45,3,126,3,207,3,32,4,112,4,193,4,
    17,5,97,5,177,5,1,6,81,6,160,6,239,6,62,7,
    141,7,220,7,42,8,120,8,198,8,20,9,97,9,174,9,
    251,9,72,10,148,10,224,10,44,11,119,11,194,11,13,12,
    87,12,161,12,235,12,52,13,125,13,198,13,15,14,86,14,
    158,14,229,14,44,15,115,15,185,15,255,15,68,16,137,16,
    206,16,18,17,86,17,153,17,220,17,31,18,97,18,163,18,
    228,18,37,19,102,19,166,19,230,19,37,20,100,20,162,20,
    224,20,30,21,91,21,152,21,213,21,17,22,76,22,136,22,
    194,22,253,22,55,23,112,23,170,23,226,23,27,24,83,24,
    138,24,193,24,248,24,46,25,100,25,154,25,207,25,4,26,
    56,26,108,26,159,26,211,26,5,27,56,27,106,27,156,27,
    205,27,254,27,46,28,94,28,142,28,190,28,237,28,27,29,
    74,29,120,29,165,29,211,29,255,29,44,30,88,30,132,30,
    176,30,219,30,6,31,48,31,90,31,132,31,174,31,215,31
};
unsigned char data_0209a624[4] = { 1, 0, 0, 0 };   /* arm9 .data, ROM value */
int data_020a637c[9];                              /* bss, 36 by ROM span */
int data_020a80cc[6];                              /* bss, 24 by ROM span */
/* data_0209caa0 is NOT defined here: hal/level_boot.cpp already hosts it as
   a 0x14 save block, and its own header records that the ROM object is wider
   than the dsd symbol. Only the mangled spelling below is added. */
}
DSSTATE_END
static void l2_fill_0208ea6c(void)
{
    for (int i = 0; i < 12; ++i) data_0208ea6c[i] = (void *)l2_vt_trap;
    for (int i = 0; i < 10; ++i) data_0208eb2c[i] = (void *)l2_vt_trap;
    for (int i = 0; i < 12; ++i) data_0208eacc[i] = (void *)l2_vt_trap;
    ((void **)data_0208eafc)[0] = (void *)l2_vt_trap;
    ((void **)data_0208eafc)[1] = (void *)l2_vt_trap;
}

// ---- 3. NINETEEN UNMATCHED ov007 BODIES, AS LOUD TRAPS ---------------------
//
// ov007 is 548 functions and 528 have a matched src TU. The nineteen that do
// not are inside delink blocks marked incomplete, they are called from bodies
// that ARE matched, and there is no C for them anywhere in the tree. A
// plausible hand-written body would be exactly the guess the inferred-stub
// guard exists to refuse, so each is a TRAP that names itself once and returns
// zero. A run that enters one says so on stderr and keeps going, which is what
// makes "none of them was entered" a measurement instead of an assumption.
// NONE OF THE NINETEEN FIRED IN ANY RUN THIS LANE MADE; port/ov007_seat.txt
// carries the counter readback.
static unsigned g_l2_trap_hits;
static void l2_trap(const char *name)
{
    ++g_l2_trap_hits;
    /* BOTH STREAMS, AND FLUSHED, and that is not belt-and-braces. stderr goes
       to the flight recorder's playlog FILE -- walk_window redirects it -- so a
       parent capturing the child's pipes never sees a word of it, which is
       exactly how port/tools/battery.py's SCENE_BLOCKED marker check reads the
       run. stdout is the stream the battery can see, and the flush is what
       survives the fault that usually follows a trap. */
    std::fprintf(stderr, "  [scene] UNMATCHED ov007 body entered: %s "
                 "(returns 0; port/ov007_seat.txt section 5)\n", name);
    std::fflush(stderr);
    std::printf("  [scene] UNMATCHED ov007 body entered: %s "
                "(returns 0; port/ov007_seat.txt section 5)\n", name);
    std::fflush(stdout);
}
extern "C" unsigned port_l2_trap_hits(void) { return g_l2_trap_hits; }
#define L2_UNMATCHED(sym)                                                      \
    extern "C" int sym(void);                                                  \
    extern "C" int sym(void) { l2_trap(#sym); return 0; }
L2_UNMATCHED(func_ov007_020ae834)
L2_UNMATCHED(func_ov007_020b1718)
L2_UNMATCHED(func_ov007_020b2998)
L2_UNMATCHED(func_ov007_020b46b0)
L2_UNMATCHED(func_ov007_020b8188)
L2_UNMATCHED(func_ov007_020ba05c)
L2_UNMATCHED(func_ov007_020beeb0)
L2_UNMATCHED(func_ov007_020c19cc)
L2_UNMATCHED(func_ov007_020c20b8)
L2_UNMATCHED(func_ov007_020c368c)
L2_UNMATCHED(func_ov007_020c4684)
L2_UNMATCHED(func_ov007_020c6e68)
L2_UNMATCHED(func_ov007_020c7d60)
L2_UNMATCHED(func_ov007_020c9688)
L2_UNMATCHED(func_ov007_020caeac)
L2_UNMATCHED(func_ov007_020cb4b0)
L2_UNMATCHED(func_ov007_020cb7c0)
L2_UNMATCHED(func_ov007_020cbbb0)
/* 020b8fd4 surfaced only in the SECOND link, because its one caller spells it
   untagged as func_020b8fd4 (face (a) below) and the untagged name resolved
   before the tagged one was ever asked for. Nineteen, not eighteen. */
L2_UNMATCHED(func_ov007_020b8fd4)
#undef L2_UNMATCHED

/* FOUR MORE UNMATCHED BODIES, NOT ov007's, that the arm9 closure pulled in.
   Same treatment and the same counter, listed apart because they are a
   different debt: three are arm9/itcm functions with no C anywhere in the
   tree, and two are cross-overlay calls out of src/func_0201a458.c into
   overlays this build does not host. Naming them here is what makes them
   visible; guessing bodies for them would not be.
     func_01ffaa34  ITCM, config/arm9/itcm/symbols.txt names it and no src
                    defines it. Reached from func_ov007_020c5c14.
     func_02054c80  reached from func_02054430; no arm9 symbol at that address
                    at all, so dsd's name is a guess about an address, not a
                    function the config knows.
     func_0211d9c0  reached from func_0201a458 (the heap-for-the-next-scene
     func_02140d80  helper). Both are addresses in overlays this build does
                    not host, and ov007 is not co-resident with either. */
#define L2_UNMATCHED(sym)                                                          extern "C" int sym(void);                                                      extern "C" int sym(void) { l2_trap(#sym); return 0; }
L2_UNMATCHED(func_01ffaa34)
L2_UNMATCHED(func_02054c80)
L2_UNMATCHED(func_0211d9c0)
L2_UNMATCHED(func_02140d80)
#undef L2_UNMATCHED

// ---- 4. NAME-SPELLING FACES, AND NOT ONE OF THEM IS A BODY -----------------
//
// (a) TWENTY-ONE ov007 FUNCTIONS THE SOURCE SPELLS WITHOUT ITS OVERLAY TAG.
//     `func_020b2160` and its twenty siblings are addresses inside ov007's own
//     .text (0x020ad660 .. 0x020ccb54) that some ov007 TUs name with the bare
//     `func_` prefix while the config -- and the TU that defines each body --
//     names them func_ov007_*. Checked one at a time: every one of the
//     twenty-one has an ov007 function symbol at that exact address, and
//     twenty of the twenty-one have NO symbol at that address in ov002 or
//     ov006 either, so there is no ambiguity to resolve. The exception is
//     0x020c897c, which ov002 also names; ov002 and ov007 are mutually
//     exclusive occupants of one slot and the caller here is ov007's, so
//     ov007's body is the right destination in this direction.
#pragma comment(linker, "/alternatename:_func_020b2160=_func_ov007_020b2160")
#pragma comment(linker, "/alternatename:_func_020b2370=_func_ov007_020b2370")
#pragma comment(linker, "/alternatename:_func_020b2728=_func_ov007_020b2728")
#pragma comment(linker, "/alternatename:_func_020b2cf0=_func_ov007_020b2cf0")
#pragma comment(linker, "/alternatename:_func_020b413c=_func_ov007_020b413c")
#pragma comment(linker, "/alternatename:_func_020b4464=_func_ov007_020b4464")
#pragma comment(linker, "/alternatename:_func_020b7658=_func_ov007_020b7658")
#pragma comment(linker, "/alternatename:_func_020b7a00=_func_ov007_020b7a00")
#pragma comment(linker, "/alternatename:_func_020b7a34=_func_ov007_020b7a34")
#pragma comment(linker, "/alternatename:_func_020b8fd4=_func_ov007_020b8fd4")
#pragma comment(linker, "/alternatename:_func_020b91b4=_func_ov007_020b91b4")
#pragma comment(linker, "/alternatename:_func_020bee14=_func_ov007_020bee14")
#pragma comment(linker, "/alternatename:_func_020bfaf0=_func_ov007_020bfaf0")
#pragma comment(linker, "/alternatename:_func_020c232c=_func_ov007_020c232c")
#pragma comment(linker, "/alternatename:_func_020c2390=_func_ov007_020c2390")
#pragma comment(linker, "/alternatename:_func_020c3df4=_func_ov007_020c3df4")
#pragma comment(linker, "/alternatename:_func_020c78b0=_func_ov007_020c78b0")
#pragma comment(linker, "/alternatename:_func_020c80a4=_func_ov007_020c80a4")
#pragma comment(linker, "/alternatename:_func_020c844c=_func_ov007_020c844c")
#pragma comment(linker, "/alternatename:_func_020c897c=_func_ov007_020c897c")
#pragma comment(linker, "/alternatename:_func_020c93b4=_func_ov007_020c93b4")
//
// (b) FIVE arm9 FUNCTIONS THE SOURCE NAMES BY ADDRESS. Each resolves to a real
//     arm9 symbol at OFFSET ZERO -- not an interior address, which is the
//     phantom-seat shape -- so each is a spelling and nothing more:
//       0x02018144 Deallocate            0x0201816c LoadFile
//       0x0203c280 Heap::_Deallocate     0x0203c28c Heap::Allocate
//       0x02055574 G3X::SetClearColor
#pragma comment(linker, "/alternatename:_func_02018144=_Deallocate")
#pragma comment(linker, "/alternatename:_func_0201816c=_LoadFile")
#pragma comment(linker, "/alternatename:_func_0203c280=__ZN4Heap11_DeallocateEPv")
#pragma comment(linker, "/alternatename:_func_0203c28c=__ZN4Heap8AllocateEj")
#pragma comment(linker, "/alternatename:_func_02055574=__ZN3G3X13SetClearColorEtiiib")
//
// (c) THE _ZN6Player17St_EndingFly_MainEv NAMING TRAP, which the 2d map warns
//     about in its section 1 and which this slice is the first build to hit.
//     0x020c3d1c is an ov007 function and the community label on it is a
//     Player state name; ov007 is the title scene and has no Player. The
//     matched TU is src/_ZN6Player17St_EndingFly_MainEv.cpp, it is in this
//     slice, and it defines the flat Itanium name. Its callers spell the same
//     body FOUR ways between them -- once by address and three times as a
//     C++ method, with three different MSVC manglings because the three
//     declaring TUs disagree on the return type and on staticness -- so all
//     four are pointed at the one definition.
#pragma comment(linker, "/alternatename:_func_020c3d1c=__ZN6Player17St_EndingFly_MainEv")
#pragma comment(linker, "/alternatename:?St_EndingFly_Main@Player@@QAEHXZ=__ZN6Player17St_EndingFly_MainEv")
#pragma comment(linker, "/alternatename:?St_EndingFly_Main@Player@@QAEXXZ=__ZN6Player17St_EndingFly_MainEv")
#pragma comment(linker, "/alternatename:?St_EndingFly_Main@Player@@YAXXZ=__ZN6Player17St_EndingFly_MainEv")
//
// (d) SIX C++-DECLARED CALLS ONTO FLAT DEFINITIONS. The cxxname_bridge defect
//     in its usual direction: an ov007 TU declares the callee inside a struct
//     or namespace, so MSVC mangles the reference, while the matched TU
//     defines the flat Itanium name.
#pragma comment(linker, "/alternatename:?DispOn@GX@@SAXXZ=__ZN2GX6DispOnEv")
#pragma comment(linker, "/alternatename:?DisableAllBanks@GX@@SAXXZ=__ZN2GX15DisableAllBanksEv")
#pragma comment(linker, "/alternatename:?LoadOBJ@GX@@SAXPBXII@Z=__ZN2GX7LoadOBJEPKvjj")
#pragma comment(linker, "/alternatename:?SetBankForTexPltt@GX@@YAXG@Z=__ZN2GX17SetBankForTexPlttEt")
#pragma comment(linker, "/alternatename:?div@cstd@@YAHHH@Z=__ZN4cstd3divEii")
//     Scene::SetFaders IS NOT IN THAT LIST AND MUST NOT BE, and the reason is
//     worth the paragraph because it cost this lane a fault that read like a
//     vtable bug. `?SetFaders@Scene@@QAEXPAUFaderBrightness@@@Z` is __THISCALL
//     -- `this` in ECX, the FaderBrightness* on the stack -- and the matched
//     TU defines a __CDECL function taking ONE stack argument. An
//     /alternatename between the two LINKS, and then the callee reads the
//     stack argument as `this`: dScDSMT_c::InitResources calls it with
//     this = self+0x54 and fb = self+0x50, so the body ran against self+0x50,
//     whose vptr is the FOUR-slot graphCallback_c table, and dispatched slot 9
//     of it -- eight words past the end, into the RTTI name string that starts
//     at 0x021032c0. The fault was eip 0x61226c6c reading 0x61626c6c, which is
//     the bytes "llba" out of "graphCallback_c". This is
//     hal/scene_actor_faces.cpp's veneer trap in a new dress: an alias cannot
//     change a calling convention, so where the conventions differ the answer
//     is a FACE that re-lands the arguments.
struct FaderBrightness;
struct Scene { void SetFaders(FaderBrightness *fb); };
void Scene::SetFaders(FaderBrightness *)
{
    /* the ROM body ignores its second argument; only `this` is used */
    _ZN5Scene9SetFadersEP15FaderBrightness(this);
}
//     ...and FOUR IN THE OPPOSITE DIRECTION. Here the ov007 caller spells the
//     FLAT Itanium name and the matched TU defines a real C++ static member,
//     so the alias runs flat -> mangled. The decorations are read out of the
//     compiled .obj with dumpbin rather than hand-derived, because a wrong
//     decoration here is a silent no-op alias. ModelComponents::Render is a
//     fifth of the same shape but its definition is
//     port/unmatched/ModelComponents_Render.cpp, already in the build, so its
//     matched TU came OUT of the slice rather than becoming a duplicate.
#pragma comment(linker, "/alternatename:__ZN9Animation17UpdateFileOffsetsER8BCA_File=?UpdateFileOffsets@Animation@@SAXAAUBCA_File@@@Z")
#pragma comment(linker, "/alternatename:__ZN15TextureSequence17UpdateFileOffsetsER8BTP_File=?UpdateFileOffsets@TextureSequence@@SAXAAUBTP_File@@@Z")
#pragma comment(linker, "/alternatename:__ZN5Model13LoadTexAndPalER8BMD_File=?LoadTexAndPal@Model@@SAXAAUBMD_File@@@Z")
#pragma comment(linker, "/alternatename:__ZN15ModelComponents6RenderEP9Matrix4x3P7Vector3=?Render@ModelComponents@@QAEXPAUMatrix4x3@@PAUVector3@@@Z")
//     ...and three more of the same shape in the SaveData family, whose TUs
//     are in the slice and define real C++ members while their ov007 and arm9
//     callers spell the flat name. Decorations from dumpbin, again.
#pragma comment(linker, "/alternatename:__ZN8SaveData8SaveFileEjP12FileSaveData=?SaveFile@SaveData@@SAHIPAUFileSaveData@@@Z")
#pragma comment(linker, "/alternatename:__ZN8SaveData16SetDefaultValuesEP12FileSaveData=?SetDefaultValues@SaveData@@QAEXPAUFileSaveData@@@Z")
#pragma comment(linker, "/alternatename:__ZN8SaveData18SetDefaultValuesMgEP16MinigameSaveData=?SetDefaultValuesMg@SaveData@@QAEXPAUMinigameSaveData@@@Z")
//
// (e) ELEVEN MANGLED DATA SPELLINGS onto storage that already exists. Three of
//     them are the SAME symbol, data_ov007_0210342c, mangled three ways
//     because three TUs declare it with three different types. None of the
//     eleven is a stand-in: every right-hand side is either the ov007 mount's
//     own byte array or an arm9 global the build already defines.
#pragma comment(linker, "/alternatename:?data_02082214@@3QBFB=_data_02082214")
#pragma comment(linker, "/alternatename:?data_0209d4a8@@3HA=_data_0209d4a8")
#pragma comment(linker, "/alternatename:?data_0209f1e0@@3EA=_data_0209f1e0")
#pragma comment(linker, "/alternatename:?data_ov007_02102f28@@3PADA=_data_ov007_02102f28")
#pragma comment(linker, "/alternatename:?data_ov007_0210342c@@3PADA=_data_ov007_0210342c")
#pragma comment(linker, "/alternatename:?data_ov007_0210342c@@3PAHA=_data_ov007_0210342c")
#pragma comment(linker, "/alternatename:?data_ov007_0210342c@@3PAUS@@A=_data_ov007_0210342c")
#pragma comment(linker, "/alternatename:?data_ov007_02104b9c@@3PAHA=_data_ov007_02104b9c")
#pragma comment(linker, "/alternatename:?data_ov007_02104b9c@@3PAUB9C@@A=_data_ov007_02104b9c")
#pragma comment(linker, "/alternatename:?data_ov007_02104ba0@@3PAHA=_data_ov007_02104ba0")
#pragma comment(linker, "/alternatename:?data_ov007_02104ba0@@3PAUBA0@@A=_data_ov007_02104ba0")
#pragma comment(linker, "/alternatename:?data_ov007_02104bc0@@3PAHA=_data_ov007_02104bc0")
#pragma comment(linker, "/alternatename:?data_ov007_02104bd8@@3PAXA=_data_ov007_02104bd8")
#pragma comment(linker, "/alternatename:?data_0209caa0@@3HA=_data_0209caa0")

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

/* THE 18-SLOT SHAPE IS HARDCODED HERE, AND IT IS AN ov003 FINDING RATHER THAN
   A LAW ABOUT SCENES. All three ov003 classes were read out of the overlay
   image and all three are exactly _ZTV5Scene's eighteen slots with seven
   overridden, so this writes eleven fixed indices. Nothing has checked that
   ov006's minigame classes are the same shape: they derive from dScMgBase_c,
   which derives from Scene, and a class that adds virtuals of its own has a
   LONGER table whose tail this function would leave unwritten while its
   indices 1..15 still land correctly. Whoever seats the first ov006 scene
   reads that class's table out of the image first and does not assume this. */
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

// ---- dScDSMT_c, id 1, the TITLE SCREEN and FILE SELECT (ov007) -------------
//
// THE WIDTH IS 18 AND THE ROM SAYS SO THREE WAYS, so scene_fill_shared's
// hardcoded shape above is left alone rather than parameterised: a width
// parameter with one caller and one value would be ceremony, and the next
// scene to disagree is the one that should add it.
//
//   1. the reloc run. config/arm9/overlays/ov007/relocs.txt has eighteen
//      CONSECUTIVE relocated words at 0x021032e8 .. 0x0210332c and then stops.
//      0x02103330, 0x02103334, 0x02103338 and 0x0210333c carry no relocation.
//   2. the ROM bytes. The mount emits data_ov007_021032e8 as 88 bytes -- the
//      delta to the next dsd symbol, which is the .bss base 0x02103340 -- and
//      words 18..21 of those 22 are 0x00000000 in the image. Trailing zeros,
//      not slots.
//   3. the shape. Every one of the eleven non-overridden slots holds the SAME
//      arm9 address ov003's three classes hold, byte for byte: 0x0202e638,
//      0x0202e62c, 0x0202e5f0, 0x0202e5d0, 0x0202e3d4, 0x0202e3c8, 0x0202e3a4,
//      0x0202e398, 0x0204357c, 0x0204349c, 0x02043494. dScDSMT_c is _ZTV5Scene
//      with seven slots overridden and adds no virtual of its own.
//
// THE FOUR TRAILING WORDS ARE LEFT AS THE ROM HAS THEM. The fill writes 0..17
// and does not touch 18..21, so a dispatch past the end reads the ROM's zeros
// and faults at 0 rather than running whatever the linker put next -- the
// failure vtspan.py exists to prevent, in its loud direction.
//
// THE SECOND TABLE. The factory writes a sub-object vptr at +0x50 as well
// (`p[0x50/4] = data_0208ee14; p[0x50/4] = data_ov007_021032b0`), and
// data_ov007_021032b0 is dScDSMT_c::graphCallback_c's own four-slot table --
// the RTTI name N9dScDSMT_c15graphCallback_cE at 0x021032c0, reached through
// the typeinfo record at 0x02103284 that sits one word below the table. It is
// in the mount too and it gets the same treatment. Slots 1 and 3 are matched
// arm9 Scene methods; the eleven-slot shared fill does not apply to it.
static unsigned g_ti_hits[18];
static int  __fastcall ti_init(void *s, void *)
{ ++g_ti_hits[0];  return func_ov007_020cc4c0((char *)s); }
static int  __fastcall ti_clean(void *, void *)
{ ++g_ti_hits[3];  return func_ov007_020cc45c(); }
static int  __fastcall ti_beh(void *s, void *)
{ ++g_ti_hits[6];  return func_ov007_020cc2cc((char *)s); }
static int  __fastcall ti_render(void *s, void *)
{ ++g_ti_hits[9];  return func_ov007_020cc2b0(s); }
static int  __fastcall ti_pdes(void *, void *)
{ ++g_ti_hits[12]; func_ov007_020cc2ac(); return 0; }
/* the SM64DS_SCENE_SLOT9=0 stand-in, counted separately for the same reason
   ss_render_noop is: a no-op must never read as the real body having run. */
static unsigned g_ti_render_skipped;
static int  __fastcall ti_render_noop(void *, void *)
{ ++g_ti_render_skipped; return 1; }
/* SM64DS_SCENE_SLOT0=0, the same diagnostic one slot up, and it exists for a
   blocker that is not the port's: dScDSMT_c::InitResources dies inside an
   UNMATCHED ROM function. Counted separately for the same reason -- a run can
   never read the no-op as InitResources having run -- and the battery's
   retire probe re-runs the bare scene every pass, so the moment
   func_ov007_020c9688 gets a body this stops being needed and says so. */
static unsigned g_ti_init_skipped;
static int  __fastcall ti_init_noop(void *, void *)
{ ++g_ti_init_skipped; return 1; }
static void *__fastcall ti_d2(void *s, void *) { return func_ov007_020cc028((int *)s); }
static void *__fastcall ti_d0(void *s, void *) { return func_ov007_020cc070((int *)s); }
/* graphCallback_c */
static int __fastcall ti_gc0(void *, void *)  { return func_ov007_020cc110(); }
static int __fastcall ti_gc1(void *s, void *) { return _ZN5Scene14GraphCallback1Ev(s); }
static int __fastcall ti_gc2(void *s, void *) { return func_ov007_020cc0f4(s); }
static int __fastcall ti_gc3(void *s, void *) { return _ZN5Scene14GraphCallback3Ev(s); }

static void scene_fill_title(void)
{
    /* THE MOUNT COMES UP FIRST, and the order matters in one direction only.
       port_ov007_syms_patch() rebases the mount's own in-span pointer words;
       every word this fill writes is a CODE address, which is outside the
       mount's coverage, so the patch cannot undo the fill -- but the fill
       would be undone if it ran first and the patch happened to cover a slot,
       so the safe order is the one the ov043/ov045 mounts already use. */
    static int done;
    if (!done) {
        done = 1;
        port_l2_pack_check();
        l2_fill_0208ea6c();
        port_ov007_pack_check();
        port_ov007_syms_patch();
    }

    void **vt = (void **)data_ov007_021032e8;
    scene_fill_shared(vt);
    {
        const char *s0 = std::getenv("SM64DS_SCENE_SLOT0");
        vt[0] = (s0 && s0[0] == '0') ? (void *)ti_init_noop
                                     : (void *)ti_init;
    }
    vt[3]  = (void *)ti_clean;
    vt[6]  = (void *)ti_beh;
    {
        const char *s9 = std::getenv("SM64DS_SCENE_SLOT9");
        vt[9] = (s9 && s9[0] == '0') ? (void *)ti_render_noop
                                     : (void *)ti_render;
    }
    vt[12] = (void *)ti_pdes;
    vt[16] = (void *)ti_d2;
    vt[17] = (void *)ti_d0;

    void **gc = (void **)data_ov007_021032b0;
    gc[0] = (void *)ti_gc0;
    gc[1] = (void *)ti_gc1;
    gc[2] = (void *)ti_gc2;
    gc[3] = (void *)ti_gc3;

    /* The graph-callback table, words 0..5. Words 6 and 7 are NOT touched:
       word 7 is the typeinfo pointer at 0x021032ac, which points at
       0x02103284 and is INSIDE the mount, so the cross pass already rebased
       it, and word 6 carries no relocation at all. Writing either would undo
       correct work. */
    void **cb = (void **)data_ov007_02103290;
    cb[0] = (void *)func_ov007_020ccab4;
    cb[1] = (void *)func_ov007_020cca98;
    cb[2] = (void *)func_ov007_020cca80;
    cb[3] = (void *)func_ov007_020cca74;
    cb[4] = (void *)func_ov007_020cca68;
    cb[5] = (void *)func_ov007_020cc600;

    *(void **)data_ov007_02103254 = (void *)func_ov007_020c3e4c;
    *(void **)data_ov007_02103258 = (void *)func_ov007_020c3e64;
}

/* The registry's factory column is void *(*)(void) and the matched factory
   returns int *. One typed forwarder rather than a cast through an
   incompatible function pointer; /OPT:REF follows it to the real body. */
static void *title_spawn(void) { return (void *)func_ov007_020ccad0(); }

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
    /* DOES THIS SCENE READ data_02092110? See the audit in port_scene_run: the
       harness used to write the sublevel on every run, and for a scene that
       never reads it that write is not inert, it is a state the ROM would
       never have been in at that moment. Measured per class from the ROM's own
       relocations, not asserted. */
    unsigned char reads_sublevel;
};

/* TWO ROWS. dScTitle_c (id 2) and dScGameOver_c (id 8) are derived to the same
   depth -- SpawnInfo, factory and all eighteen vtable slots are recorded in
   port/ov003_syms.txt -- and are NOT seated, for two reasons that are named in
   full at the bottom of port/slice_scene1.txt: eleven of their fourteen slot
   bodies carry the "recovered from vtable slot identity" marker that
   port/tools/inferred_stub_guard refuses until each is ruled against the ROM,
   and three of their dtor TUs spell their vptr writes as per-TU placeholders
   (VT0/VT1/VT2, _ZTV10dScTitle_c, _ZTV8dScene_c, _ZTV7dBase_c, G0) that exist
   in no config. Adding them is a row here plus a block in the slice.
   ov007's dScDSMT_c hit BOTH of those blockers and cleared them rather than
   being excused from them: its six marker-carrying bodies are ruled against
   the ROM in port/tools/inferred_stub_adjudicated.txt, and its two placeholder
   spellings are resolved by the two faces at the top of this file.

   NO ROW GOES IN hal/actor_classes.inc FOR EITHER OF THEM, and the ov004/ov007
   map's step 1 ("the row lives in hal/actor_classes.inc") is superseded by the
   spine this file established rather than followed. That table is the LEVEL
   cast: hal/actor_registry.cpp walks it to build the census and it is what the
   SM64DS_SKIP_CLASS knob indexes. A scene is not part of any level's cast, a
   second seat of id 1 would have two writers for data_020a4bb8[1], and the
   three statements the row would make are exactly the three
   port_scene_registry_install makes below. */
static const PortSceneClass port_scene_classes[] = {
    {4, "SCENE_STAR_SELECT", StarSelect_SpawnInfo, StarSelect_Spawn,
     scene_fill_starsel, 1},
    {1, "SCENE_TITLE", data_ov007_02103264, title_spawn,
     scene_fill_title, 0},
    {0, 0, 0, 0, 0, 0},
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
    std::printf("[scene] %d scene classes registered (ov003, ov007)\n", n);
}

/* Does the scene the run is booting read data_02092110? Unknown ids answer 0,
   which is the conservative direction: an id with no row does not spawn. */
static int scene_reads_sublevel(int id)
{
    for (const PortSceneClass *k = port_scene_classes; k->name; ++k)
        if (k->id == (unsigned)id)
            return k->reads_sublevel != 0;
    return 0;
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
//   SM64DS_SCENE=<id>            which scene. 4 (the star select) is the only
//                                one seated; ov003's other two ids, 2 and 8,
//                                are refused by name. Nothing else here runs
//                                unless this is set.
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

    /* THE SUBLEVEL THE SCENE IS ABOUT, before it spawns -- FOR THE SCENES THAT
       READ IT, and only those. See the header block: for dScStarSel_c this is
       an input, not a default, and 0 is not a legal value for it.
       ---- THE AUDIT lane L2 owed this line ----------------------------------
       The paragraph that used to stand here ended "whoever seats ov007 or an
       ov006 scene audits this line rather than inheriting it". That audit is
       done and the answer is that dScDSMT_c DOES NOT READ IT. The evidence is
       the ROM's own relocations, counted both ways so the negative is a
       measurement and not a failure to find something:
           relocations into 0x02092110 from anywhere in ov007   0
           relocations into 0x02092110 from anywhere in ov003   6
             (0x020ae0a0, 0x020ae194, 0x020ae344 in dScStarSel_c's Render
              closure, 0x020aefcc in its Behavior, 0x020af824 in its
              InitResources, 0x020b041c in dScGameOver_c's)
       and no ov007 source TU names data_02092110, SublevelToLevel or
       SUBLEVEL_LEVEL_TABLE anywhere in the 527.
       SO THE WRITE IS GATED, because leaving it would let the harness fabricate
       an input. It is worse than inert for id 1: hal/level_change.cpp reads the
       SAME BYTE as "the next level, -1 = nothing pending" and
       port_level_change_pending() is exactly `data_02092110 >= 0`. The ROM
       ships it as -1 (romdata.py: data_02092110[4] = {255,0,0,0}), the title
       screen is what comes up at boot before any level is pending, and writing
       6 there parks a level change the title screen would never have parked.
       Nothing in a scene run consumes it today -- port_scene_run does not
       return to walk_window's level loop, which is the only caller of
       port_level_change_pending -- so this is a latent cross-talk closed
       before it fires, not a bug that was firing.
       A CORRECTION THAT COMES WITH IT: the header block above says
       "data_02092110 starts at 0". It does not; it starts at -1, and
       build/port/host-src/romdata.c is where that is visible. The conclusion
       the star-select lane drew from it is untouched -- an unsupplied sublevel
       is still invalid and still walks an uninitialised star count -- but the
       sentinel it lands on is the table's index -1, not its index 0. */
    if (scene_reads_sublevel(scene)) {
        data_02092110 = (signed char)sublevel;
        std::printf("[scene] sublevel %d -> course %d\n", sublevel,
                    (int)SUBLEVEL_LEVEL_TABLE[sublevel & 0x3f]);
    } else {
        std::printf("[scene] sublevel NOT written (%s does not read "
                    "data_02092110; 0 relocations from ov007). It holds %d.\n",
                    port_scene_class_name((unsigned)scene),
                    (int)data_02092110);
    }

    /* ---- THE INSTALLED-FADER SLOT, RESET TO THE ROM'S BOOT VALUE ----------
       dScDSMT_c::InitResources calls Scene::SetFaders, and dScStarSel_c never
       does, so this lane is the first caller in the port's history. The
       matched TU opens:

           if (data_0209f5bc) { if (data_0209f5bc->vt->f14(...)) ... }

       -- a dispatch through slot 5 of whatever fader is currently installed.
       hal/fader_wipes.cpp PRE-SEATS that word (`void *data_0209f5bc =
       &hal_wipes[0];`) because two level-path functions deref it with no null
       check, and hal_wipes[0] is a host C++ HalFaderWipe whose MSVC vtable is
       not the ROM's FaderBrightness table. Slot 5 of it is not a method; the
       first boot of this scene faulted at eip 0x61226c6c reading 0x61626c6c,
       which is the bytes "llba" out of a string literal sitting behind the
       host vtable in .rdata.

       THE ROM'S OWN VALUE HERE IS ZERO, and that is why this is a reset and
       not a workaround. config/arm9/symbols.txt has data_0209f5bc as
       kind:bss, so the DS clears it at boot, and the title screen is what
       comes up before any fader has ever been installed -- the `if` is FALSE
       on the real machine at this exact moment. The port's pre-seat is a
       LEVEL-PATH invention, correct for the path it was written for and wrong
       for a scene that runs the ROM's own guard. Restoring the zero makes the
       guard take the branch the DS takes, and Scene::SetFaders' last two
       statements put a real object back in the word before anything else can
       read it.

       Applied to EVERY scene run, not just id 1, because it is the ROM's boot
       state and not an ov007 special case; the 46-level battery is untouched
       (port_scene_run is not on the level path) and scene 4 is re-measured
       green with it. THE REAL FIX IS NOT HERE: hal/fader_wipes.cpp should give
       its wipe objects ROM-SHAPED vtables so a ROM dispatch through slot 5
       lands on a method, and then this reset can go. ROUTED. */
    data_0209f5bc = 0;
    std::fprintf(stderr, "  [scene] data_0209f5bc reset to 0 (arm9 bss; the "
                 "ROM's own value before any scene installs a fader), was "
                 "%p\n", data_0209f5bc);
    std::fflush(stderr);

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
    {
        const int t = (scene == 1);
        const unsigned *h = t ? g_ti_hits : g_ss_hits;
        const unsigned sk = t ? g_ti_render_skipped : g_ss_render_skipped;
        std::printf("[scene] %s slot hits: init %u, behavior %u, render %u, "
                    "cleanup %u, pending-destroy %u%s\n",
                    t ? "ov007" : "ov003",
                    h[0], h[6], h[9], h[3], h[12],
                    sk ? "  [RENDER SLOT NO-OP'd: SM64DS_SCENE_SLOT9=0]" : "");
        if (g_ti_init_skipped)
            std::printf("[scene] INIT SLOT NO-OP'd: SM64DS_SCENE_SLOT0=0, %u "
                        "time(s)\n", g_ti_init_skipped);
        if (port_l2_trap_hits())
            std::printf("[scene] unmatched-body traps entered: %u\n",
                        port_l2_trap_hits());
        else
            std::printf("[scene] unmatched-body traps entered: 0 (none of the "
                        "23 named bodies was reached)\n");
    }
    std::printf("[scene] %d frames of scene %d (%s), clean\n", frames, scene,
                port_scene_class_name((unsigned)scene));
    std::fflush(stdout);
    return 0;
}
