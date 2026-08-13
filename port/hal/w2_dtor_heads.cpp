// LANE w2-b SEAT (run linkw, wave 2) -- destructor heads on the ROM vtable
// storage the port hosts but never fills.
//
// This is wave 1's meshcollider_dtor_seat mechanism applied to what a full
// sweep of the port's hosted vtable storage turned up. The sweep read every
// two-word head in config/arm9 and every overlay config against the port's
// host definitions, and asked one question per table: do the ROM's own
// relocations at base+0 and base+4 name a matched TU that the link does not
// carry? Thirty tables answered yes. Twenty-nine of them are blocked for
// reasons recorded at the bottom of this file. What is left is seated here.
//
// WHY A SEAT AND NOT JUST A SLICE LINE, one more time. Release links with
// /OPT:REF, so a matched TU added to a slice with nothing referencing it is
// dropped before the map is written and the headline does not move. The
// reference edge IS the work; a ROM vtable slot the port never filled is the
// honest place to put one, because the ROM says what belongs there.
//
// ===========================================================================
// SEATED: _ZTV12CylinderClsn, arm9 0x0208e6ec, slots 0 and 1.
//
// The ROM's own two words, config/arm9/relocs.txt:
//
//     from:0x0208e6ec kind:load to:0x020150a8 module:main   D1 (complete)
//     from:0x0208e6f0 kind:load to:0x0201507c module:main   D0 (deleting)
//
// and config/arm9/symbols.txt puts a class destructor on both addresses:
//
//     _ZN12CylinderClsnD1Ev kind:func addr:0x020150a8
//     _ZN12CylinderClsnD0Ev kind:func addr:0x0201507c
//
// THE INDEX CONVENTION IS THE CLASS'S OWN, not an inference. src/
// _ZN12CylinderClsnD1Ev.c -- byte-matched, and the body this seat names at
// index 0 -- opens by writing the table base as the object's vptr:
//
//     *(int*)self = (int)data_0208e6ec; // set vptr
//
// so 0x0208e6ec is the vptr address, index 0 is the first virtual slot, and
// the Itanium pair sits at 0 and 1. src/_ZN12CylinderClsnD0Ev.c writes the
// same base and adds the Memory::operator_delete2 call, which is the D1/D0
// split every class in this band shows.
//
// TWO WORDS, NO MORE. dsd's next data symbol after 0x0208e6ec is 0x0208e704,
// so this head has room for six words, but there is relocation evidence for
// exactly two and this file does not own the storage. Index 2 and up stay
// zero.
//
// NOTHING FILLS IT AND NOTHING DISPATCHES IT. The storage is
// `int data_0208e6ec[4]` in hal/cxx_aliases.cpp, in the BSS ring the gate-10
// aliases land on; the only other mention of the name in the whole port is
// the /alternatename pragma four lines above it. No fill in the tree ever
// wrote a slot and no call site reads one, so this changes no behaviour: it
// makes the port's copy of the table carry the two words the ROM puts there,
// and that is the reference edge that pulls the two matched TUs into the link.
// The callee closure was already linked before this seat (func_02014fa4 and
// Memory::operator_delete2 are both in the map).
//
// ABI. The flat-C bodies are `void *f(void *this)` with `this` on the stack.
// A ROM-shadow dispatch through an MSVC vtable would arrive __thiscall with
// `this` in ecx, so each slot takes the ecx->arg adapter rather than the body
// directly -- the same convention hal/meshcollider_dtor_seat.cpp and
// hal/model_dtor_seat.cpp use for every slot they seat.
// ===========================================================================
//
// ===========================================================================
// SEATED (wave 3, lane w3-b): _ZTV5Scene, arm9 0x02092680, slot 1.
//
// This is the head group 4 at the bottom of this file found and could not
// take. Group 4's blocker was storage, not shape: the body's callee chain
// reaches Scene::ResetFadersAndSound, which writes data_0209f1e4, and the
// port hosted that DS BSS symbol nowhere. It is hosted now, in
// hal/auto_bss.cpp with its sizing evidence, so the head can be seated.
//
// THE ROM's OWN WORD, config/arm9/relocs.txt:
//
//     from:0x02092684 kind:load to:0x0202e638 module:main
//
// and config/arm9/symbols.txt puts a Scene virtual on both ends:
//
//     _ZTV5Scene                       kind:data(any)          addr:0x02092680
//     _ZN5Scene19BeforeInitResourcesEv kind:function(arm,...)  addr:0x0202e638
//
// THE INDEX CONVENTION IS THE TABLE'S OWN. 0x02092684 is base+4, so this is
// slot 1, and slot 1 of an ActorBase-derived table is BeforeInitResources by
// the slot order hal/stage_bridges.cpp reads out of the sibling table at
// 0x020921c0. The two tables agree face for face: 0x02092680's slot 0 is
// ActorBase::InitResources and 0x0209268c's is ActorBase::CleanupResources,
// which are slots 0 and 3 of that order.
//
// THE HEAD IS EIGHTEEN WORDS, and every one of them is a cited relocation
// (from:0x02092680 through from:0x020926c4; dsd's next data symbol is
// 0x020926c8, so 0x48/4 = 18 -- the same eighteen-entry shape the Stage's
// table has). Only slot 1 is seated in this batch. Slots 0, 3, 6, 9, 12, 13,
// 14 and 15 name ActorBase faces that are ALREADY in the map, so a seat there
// buys no edge and would only invent an adapter for a body whose real face is
// a __thiscall C++ method (CleanupResources) or a no-argument C body
// (InitResources). The other nine slots are unlinked matched TUs and are
// listed in group 4 below with what each still needs.
//
// RE-PROVING THE THREE THINGS THE MECHANISM DEPENDS ON.
//
// (a) REAL HOSTED STORAGE. `void *_ZTV5Scene[20]` in hal/stage_bridges.cpp,
//     line 83, in that file's extern "C" block. It is the port's own array,
//     not an alias of another table and not romdata.
//
// (b) NOTHING ELSE WRITES THE TABLE. The mechanism below is a pre-main
//     static initialiser, so it is only sound if no runtime fill writes the
//     same storage -- a fill would simply overwrite the seat and the port's
//     table would still not carry the ROM's word. Swept, and this is the
//     evidence, not a recollection:
//
//       - grep for `_ZTV5Scene` and `data_02092680` across the whole tree
//         (port/, src/, unmatched/, include/, config/) returns: the
//         definition in hal/stage_bridges.cpp:83, prose in that file's
//         comment and in this one, the config row, the decl_common.h
//         declaration, and eleven src/ TUs.
//       - hal/stage_bridges.cpp's own fill, hal_fill_stage_vtable, writes
//         _ZTV5Stage[0..19]. It does NOT touch _ZTV5Scene; that array is
//         labelled "transient ctor install, storage only" on its own line.
//       - ALL ELEVEN src/ mentions write the table's ADDRESS into an object's
//         vptr. Not one of them stores into _ZTV5Scene[i]. Read out:
//           _ZN5StageC3Ev.c      *(int*)p = (int)_ZTV5Scene;
//           _ZN5StageD0Ev.c      thiz->vtable = (void **)_ZTV5Scene;
//           _ZN5StageD2Ev.c      thiz->vtable = (void **)_ZTV5Scene;
//           _ZN5SceneD0Ev.c      self->vtable = (void **)_ZTV5Scene;
//           _ZN5SceneD1Ev.c      self->vtable = &_ZTV5Scene;
//           _ZN5SceneD2Ev.c      self->vtable = &_ZTV5Scene;
//           _ZN9BootSceneD0Ev.c  self->vtable = (void **)_ZTV5Scene;
//           StarSelect_Spawn.cpp *(void***)p = (void**)_ZTV5Scene;
//         (plus three that only name it in a comment). So the storage has
//         exactly one writer, and after this change that writer is this file.
//
// (c) NOTHING DISPATCHES IT, and the reason is structural rather than lucky.
//     _ZTV5Scene is an INTERMEDIATE-BASE table in the Itanium ctor/dtor
//     chain, and every one of the eight writers above overwrites the vptr
//     with another table before the object is used -- there is no call
//     between the two stores in any of them:
//
//       _ZN5StageC3Ev   data_0208e4b8, then _ZTV5Scene, then _ZTV5Stage (wins)
//       _ZN5SceneD0Ev   _ZTV5Scene, then data_0208e4b8, then ~ActorBase
//       _ZN5SceneD1/D2  same pair
//       _ZN5StageD0/D2  _ZTV5Stage, three sub-object dtors, then _ZTV5Scene,
//                       then data_0208e4b8 -- the Scene store is the second
//                       to last statement and nothing reads it
//       StarSelect_Spawn data_0208e4b8, then _ZTV5Scene, then
//                       data_ov003_020b1704 (wins)
//       _ZN9BootSceneD0Ev same shape as the Scene dtors
//
//     And only ONE of those eight is in the link today: _ZN5StageC3Ev, the
//     three-store constructor whose last store wins. The other seven are not
//     in walk_window.map at all. So no live object has _ZTV5Scene as its
//     vptr at any point a virtual could be called, which is the same
//     condition the CylinderClsn seat above rests on.
//
// WHAT THE BODY DOES IF IT EVER IS DISPATCHED. Scene::BeforeInitResources
// calls Scene::ResetFadersAndSound and then Scene::Initialise3dGraphics, and
// the second half of that chain (Initialise3dGraphics, func_0205583c,
// func_020554bc, func_020556d0) is nothing but stores to DS I/O registers at
// 0x0400xxxx. Those addresses ARE mapped in the host -- port/ntr/ppu.cpp puts
// engine A's register file at 0x04000000 and hal/message_pump.cpp already
// writes *(vu32 *)0x4000000 directly -- so this is a poke of the modelled
// register file, not a wild store. It still never runs today, per (c).
//
// ABI. Slot 1's matched body is flat C, `Bool f(struct Scene *self)` with
// `self` on the stack; an MSVC vtable dispatch arrives __thiscall with `this`
// in ecx, so the slot takes the same ecx->arg adapter every other seat in
// this family uses, and this one forwards the return value because the face
// returns one (0 aborts the level init).
// ===========================================================================

// ---- one linkage alias the chain needs -------------------------------------
//
// The C-LINKAGE FLIP, the same one hal/cxx_aliases.cpp exists for, one class
// further down this chain. src/Initialise3dGraphics.cpp calls the clear-colour
// register write as `extern "C" void _ZN3G3X13SetClearColorEtiiib(...)`, but
// the matched definition, src/_ZN3G3X13SetClearColorEtiiib.cpp, writes it as a
// real C++ static member:
//
//     struct G3X { static void SetClearColor(unsigned short, int, int, int, bool); };
//     void G3X::SetClearColor(...) { ... }
//
// so MSVC emits it decorated and the Itanium name is undefined. The decorated
// name is lifted verbatim from the object rather than written out by hand:
//
//     dumpbin /symbols _ZN3G3X13SetClearColorEtiiib.cpp.obj
//     009 00000000 SECT3 notype () External | ?SetClearColor@G3X@@SAXGHHH_N@Z
//         (public: static void __cdecl G3X::SetClearColor(unsigned short,int,int,int,bool))
//
// This is the exact shape hal/cxx_aliases.cpp already closes for IRQ::Disable
// and IRQ::Restore, and it runs in the same direction those two do -- the
// alias names an undefined symbol and points it at a defined one, nothing is
// redefined. It lives here rather than in cxx_aliases.cpp only because this
// lane owns this file and not that one; it belongs in cxx_aliases.cpp with the
// rest of the family whenever a change that owns that file passes through.
#pragma comment(linker, \
    "/alternatename:__ZN3G3X13SetClearColorEtiiib=?SetClearColor@G3X@@SAXGHHH_N@Z")

extern "C" {

/* _ZTV12CylinderClsn -- storage `int data_0208e6ec[4]` in hal/cxx_aliases.cpp */
extern int data_0208e6ec[];

void *_ZN12CylinderClsnD1Ev(void *self);   /* arm9 0x020150a8 */
void *_ZN12CylinderClsnD0Ev(void *self);   /* arm9 0x0201507c */

/* _ZTV5Scene -- storage `void *_ZTV5Scene[20]` in hal/stage_bridges.cpp */
extern void *_ZTV5Scene[];

int _ZN5Scene19BeforeInitResourcesEv(void *self);   /* arm9 0x0202e638 */

}

static void __fastcall cyl_d1(void *self, void *) { _ZN12CylinderClsnD1Ev(self); }
static void __fastcall cyl_d0(void *self, void *) { _ZN12CylinderClsnD0Ev(self); }

static int __fastcall scene_before_init(void *self, void *)
{
    return _ZN5Scene19BeforeInitResourcesEv(self);
}

extern "C" void hal_seat_w2_dtor_heads(void)
{
    data_0208e6ec[0] = (int)(size_t)cyl_d1;
    data_0208e6ec[1] = (int)(size_t)cyl_d0;
    _ZTV5Scene[1] = (void *)scene_before_init;
}

// ---- how this fill gets called ---------------------------------------------
//
// Every seat wave 1 landed is called by name from hal/level_boot.cpp, at the
// tail of the boot that installs the collision and model tables. This lane
// does not own that file, and the campaign's rule is that a lane never edits
// another lane's file, so the call edge is a namespace-scope object whose
// constructor runs the fill -- ordinary C++ static initialisation, entirely
// inside the one file this lane owns.
//
// THAT IS ONLY SAFE BECAUSE OF WHAT IS SEATED. A pre-main write would be
// wrong for any table a runtime fill also writes (hal/stage_bridges.cpp's
// trap fill would simply overwrite it, and the seat would silently do
// nothing). The table above is written by nothing else in the port, so
// "before main" and "at level boot" put the same two words in the same two
// slots, and the storage is zero-initialised before any dynamic initialiser
// runs, so there is no ordering question with its own definition either.
//
// The fill keeps external C linkage so a later change that does own
// level_boot.cpp can call it explicitly and drop the object below.
namespace {
struct W2SeatDtorHeads {
    W2SeatDtorHeads() { hal_seat_w2_dtor_heads(); }
};
W2SeatDtorHeads g_w2_seat_dtor_heads;
}

// ===========================================================================
// THE REST OF THE SWEEP, and why nothing else is seated here.
//
// The sweep read config/arm9 plus all 103 overlay configs and asked, for every
// vtable-shaped symbol the port names anywhere under hal/, unmatched/ and
// tests/: do the ROM's relocations at base+0 or base+4 name a matched TU the
// link does not carry? Thirty tables answered yes. One is seated above. The
// other twenty-nine fall into five groups.
//
// 1. THE mwcc THUNK ARTEFACTS -- four tables, eight bodies. NOT LINKABLE.
//
//      data_02099274[0,1]  func_020375c0 / func_020375b0
//      data_020992b4[0,1]  func_0203781c / func_0203780c
//      data_02099348[0,1]  func_02037d94 / func_02037d84
//      data_02099358[0,1]  func_02037db4 / func_02037da4
//
//    These four are the SECONDARY-BASE sub-vtables of the collision classes
//    wave 1 seated: 0x02099274 is 0x10 past RaycastGround's head, 0x020992b4
//    0x10 past RaycastLine's, and src/func_02037c40.c -- SphereClsn's matched
//    deleting destructor, already linked -- names all three of the third
//    class's vptrs in its own comment: [this+0] = 0x02099338, [this+0x10] =
//    0x02099348, [this+0x38] = 0x02099358. So the addresses are right and the
//    index convention is right.
//
//    The BODIES are the problem. Every one of the eight is a compiler-emitted
//    this-adjusting destructor thunk, and their matched sources are mwcc
//    ARTEFACT reconstructions: a synthetic `struct Derived : Base1, Base2`
//    whose only job is to make the compiler emit the thunk. Under MSVC none
//    of them defines a symbol called func_0203xxxx at all. Measured, not
//    assumed:
//      - src/func_020375b0.cpp, src/func_0203780c.cpp and src/func_02037d84.cpp
//        are BYTE-IDENTICAL to each other (md5 cf4b564a...). Each compiles to
//        ??1Derived@@UAE@XZ, so any two of them together are LNK2005
//        ("already defined in func_020375b0.obj"), and each leaves
//        ??1Base1@@UAE@XZ / ??1Base2@@UAE@XZ permanently undefined.
//      - the four .c halves are C++ inside a .c (`virtual` in a struct) and do
//        not compile as C: func_020375c0.c gives C2061 on line 6.
//    This is the same class wave-1 lane l3 hit on the six _ZThn80_ Animation
//    thunks, one family over, and it ends the same way. The four tables keep
//    their zeros.
//
// 2. MSVC DESTRUCTOR FOLDING -- three slots, already ruled by lane l3 and
//    re-confirmed here against the live fills.
//      _ZTV11ShadowModel[1] <- _ZN11ShadowModelD0Ev: the folded slot 0 holds
//        the D1 thunk (hal/cxxname_bridge.cpp:387) that PowerStar's embedded
//        shadow is destroyed through, and the ROM's second dtor word has no
//        MSVC index to live at.
//      _ZTV11CommonModel[1] <- _ZN11CommonModelD0Ev: same shape --
//        hal/actor_classes_bob_world.cpp writes [0] = cm_d1 and [1] =
//        cm_dosetfile, so the deleting body has nowhere to go.
//      data_0208e87c[0] <- _ZN9ModelBaseD1Ev: one slot up, and l3 already put
//        the deleting body (the useful half) in the single folded slot.
//
// 3. TABLES A RUNTIME FILL OWNS. _ZTV5Stage's head is Stage::InitResources and
//    Stage::BeforeInitResources (0x020921c0 -> 0x0202cc0c, +4 -> 0x0202ddc8),
//    and nothing dispatches the Stage today -- but hal_fill_stage_vtable
//    trap-fills all twenty slots at boot, so a seat from this file would be
//    overwritten and the port's table would still not carry the ROM's words.
//    Stage::InitResources is the level-load spine besides (its closure names
//    Sound, VRAM banks, skybox, fog, the 2D graphics load and the whole
//    LVL_Overlay path), which is a lane of its own, not a head seat.
//
// 4. A HEAD WHOSE BODY NEEDS STORAGE THE PORT DOES NOT HOST. _ZTV5Scene
//    (0x02092680) is genuine storage in hal/stage_bridges.cpp that no fill
//    writes and nothing dispatches -- the right shape -- and its two words are
//    from:0x02092680 -> 0x02043c80 _ZN9ActorBase13InitResourcesEv (already
//    linked) and from:0x02092684 -> 0x0202e638 _ZN5Scene19BeforeInitResourcesEv
//    (matched, unlinked). Seating index 1 would be +1, but its callee chain
//    reaches _ZN5Scene19ResetFadersAndSoundEv, which writes data_0209f1e4 --
//    a dsd BSS symbol at 0x0209f1e4 that the port hosts NOWHERE, in hal/ or in
//    the generated romdata. Linking the body means first deciding where that
//    global lives and how wide it is, which is a hosting decision for the file
//    that owns that BSS ring, not something a head seat should invent. Left
//    for a lane that owns it; the chain behind it (func_0205583c,
//    Initialise3dGraphics, func_020554bc, func_020556d0, G3X::SetClearColor)
//    is all matched and all unlinked, so it is worth more than one TU.
//
// 5. LIVE ACTOR AND ENGINE TABLES. The remaining eighteen are overlay actor
//    vtables (OneUpLogo, SkiLift, BabyPenguin, MrBlizzard, LakituBro,
//    UnchainedChomp, MetalNet, HUD, InvisiblePole, InvisibleSecret, the
//    Rickshaw/Cloud/painting tables), the Player vtable at
//    data_ov002_0210a83c, the object-loader table data_ov002_0210cbb8, and the
//    four particle callback tables. Every one of them is filled by the port
//    today and DISPATCHED, and in each case the unlinked matched body is an
//    InitResources, a loader or a SpawnParticles that the port currently
//    services with a host stand-in. Swapping one in is a behaviour change in
//    live code and belongs to the lane that owns that actor or subsystem, not
//    to a destructor-head seat. Two of them are worth flagging anyway:
//    data_ov002_0210cbb8[1] is LoadEntranceObjects, served by
//    unmatched/LoadEntranceObjects.cpp, and the four particle tables put
//    Particle::SimpleCallback::SpawnParticles at index 0 in the ROM where
//    hal/particle_vtable.cpp seats the BASE Particle::Callback::SpawnParticles.
//
// ONE CAVEAT ON THE OVERLAY HALF OF THE SWEEP. Overlays share address space,
// so an address resolved across all 101 configs can come back with a class
// name from the wrong overlay (0x02112408 answers both _ZTV12SwitchPillar in
// ov047 and _ZTV14daObjC0Water_c in ov012). Nothing in group 5 was taken, so
// no seat rests on that, but a future sweep should resolve overlay addresses
// inside one module rather than globally.
// ===========================================================================
