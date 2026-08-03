// The vtables of the actor classes the registry carries, filled at runtime.
//
// THE VTABLE LAW, as gate 9 wrote it (hal/actor_vtables.cpp) and gate 13
// repeated for the Camera (hal/camera_bridges.cpp):
//
//   * MSVC slot order, which for these classes is the ROM's -- include/
//     ActorBase.h declares the destructor LAST on purpose, so slots 0..15
//     agree and only the tail can diverge. Every slot below was read out of
//     the class's own _ZTV in the overlay image with its relocations applied,
//     not inferred from the header.
//   * Every entry is a __fastcall thunk: ecx carries `this` exactly as
//     __thiscall does and the dummy edx absorbs fastcall's second register.
//     That is the convention the four Process wrappers in port/unmatched/
//     call through.
//   * Every thunk calls QUALIFIED, never virtual.
//   * Slots the port cannot service TRAP BY NAME rather than being left null
//     or pointing into the overlay image.
//
// Filled at runtime rather than written as a static array because the ROM
// factories install the vtable themselves (`p[0] = (int)_ZTV4Tree`), so the
// symbol has to be an array of the right size in the first place -- and
// because InitResources can dispatch through the vptr the constructor just
// installed, which means the table must be up before the first spawn.
//
// ---- the shared half -------------------------------------------------------
//
// Ten of the twenty slots are the same functions in every one of these
// classes: Actor's four Before/After pairs, ActorBase::OnHeapCreated, and
// Actor::OnYoshiTryEat. They are written once here.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
int _ZN5Actor19BeforeInitResourcesEv(void *self);      /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a); /* slot 2 */
int _ZN5Actor14BeforeBehaviorEv(void *self);           /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);             /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);            /* slot 18 */
}

static int __fastcall ac_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ac_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ac_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
/* Slots 5, 8 and 11 are ARM tail-call veneers on the ROM -- two instructions
   that drop into ActorBase's implementation with the argument still riding in
   r1. A host forward through the veneer's own C face would lose it, so the
   thunk calls the target directly, the same reading the Player's AfterBehavior
   slot already takes. */
static void __fastcall ac_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall ac_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ac_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
/* SM64DS_ACTOR_PROBE=2: why an actor did or did not reach its own Render.
   Actor::BeforeRender is the gate -- it refuses on WRONG_AREA (0x20) and on
   OFF_SCREEN (0x8) when the actor asked to be culled off screen (0x2) -- and
   both bits are set by Actor::BeforeBehavior out of the SpawnInfo's own
   radius and cull distance. Printed once per actor id. */
static int __fastcall ac_bren(void *s, void *)
{
    int r = _ZN5Actor12BeforeRenderEv(s);
    static int on = -1;
    if (on < 0) {
        const char *e = std::getenv("SM64DS_ACTOR_PROBE");
        on = e && e[0] == '2';
    }
    if (on) {
        static unsigned said[64];
        unsigned id = *(unsigned short *)((char *)s + 0xc);
        int seen = 0;
        for (int i = 0; i < 64 && said[i]; ++i)
            if (said[i] == id) seen = 1;
        if (!seen) {
            for (int i = 0; i < 64; ++i)
                if (!said[i]) { said[i] = id; break; }
            const char *c = (const char *)s;
            std::printf("[bren] id %3u -> %d  flags %08x area %d radius %d "
                        "cull %d view (%d,%d,%d)\n", id, r,
                        *(const unsigned *)(c + 0xb0),
                        *(const signed char *)(c + 0xcc),
                        *(const int *)(c + 0xb8) >> 12,
                        *(const int *)(c + 0xbc) >> 12,
                        *(const int *)(c + 0x74) >> 12,
                        *(const int *)(c + 0x78) >> 12,
                        *(const int *)(c + 0x7c) >> 12);
        }
    }
    return r;
}
static void __fastcall ac_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
static int __fastcall ac_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall ac_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static void __fastcall ac_pdes_base(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); }

/* The trap. Slot 13/14 are the actor's own solid-heap creation and 19 is
   OnTurnIntoEgg; nothing on the castle grounds reaches any of them, and a
   named abort is the honest placeholder for a slot the port has not proved. */
static const char *g_trap_class = "?";
static int g_trap_slot;
#define ACTRAP(cls, n)                                                       \
    static int __fastcall cls##_trap##n(void *, void *)                      \
    {                                                                        \
        std::fprintf(stderr, "FATAL: %s vtable slot %d is not hosted\n",     \
                     #cls, n);                                               \
        std::abort();                                                        \
        return 0;                                                            \
    }

/* Fill the ten shared slots plus the three traps; the caller writes its own
   six. Keeps each class's fill down to the lines that are actually its own. */
static void ac_fill_shared(void **vt, int (__fastcall *trap)(void *, void *))
{
    vt[1] = (void *)ac_binit;
    vt[2] = (void *)ac_ainit;
    vt[4] = (void *)ac_bclean;
    vt[5] = (void *)ac_aclean;
    vt[7] = (void *)ac_bbeh;
    vt[8] = (void *)ac_abeh;
    vt[10] = (void *)ac_bren;
    vt[11] = (void *)ac_aren;
    vt[13] = (void *)trap;
    vt[14] = (void *)trap;
    vt[15] = (void *)ac_heap;
    vt[18] = (void *)ac_yoshi;
    vt[19] = (void *)trap;
}

// ---- TREE (actor 286, ov002) -----------------------------------------------
//
// _ZTV4Tree, ov002 0x0210ac00. Five of the six own slots are plain C-named
// free functions in their TUs (the mwcc form) and two are real MSVC methods
// against include/Tree.h; the thunks bridge both faces.
//
// Behavior takes no argument at all. That is the ROM's shape, not a
// transcription slip: Tree::Behavior ticks the five GLOBAL CylinderClsn lists
// at data_ov002_02110a48 rather than anything on `this`, so mwcc's r0 went
// unread. Twenty-one Tree actors therefore all run the same body and the
// cylinders are ticked twenty-one times over -- also the ROM's behaviour.
#include "Tree.h"
extern "C" {
int _ZN4Tree13InitResourcesEv(char *self);
int _ZN4Tree8BehaviorEv(void);
void _ZN4Tree16OnPendingDestroyEv(void);
int _ZN4TreeD1Ev(char *self);
void *_ZN4TreeD0Ev(char *self);
void *_ZTV4Tree[20];
extern int data_ov002_02110a48[5];   /* the five variant cylinder lists */
}

ACTRAP(Tree, 13)
static int __fastcall tree_init(void *s, void *)
{ return _ZN4Tree13InitResourcesEv((char *)s); }
static int __fastcall tree_clean(void *s, void *)
{ return ((Tree *)s)->Tree::CleanupResources(); }
static int __fastcall tree_behavior(void *, void *)
{ return _ZN4Tree8BehaviorEv(); }
/* SM64DS_TREE_PROBE=1: the five variant lists as InitResources built them --
   one CylinderClsnWithPos per Tree actor, its position stored in SCENE units
   (Vec3_AsrInPlace by 3) with the trunk's 30-unit lift already added, which is
   what Render clips and draws at. Printed back in world units so it can be
   read against the level's own object table. */
static void tree_probe(void)
{
    static int done;
    if (done || !std::getenv("SM64DS_TREE_PROBE"))
        return;
    done = 1;
    for (int i = 0; i < 5; ++i) {
        int n = 0;
        std::printf("[tree] variant %d:", i);
        for (int *p = (int *)(size_t)data_ov002_02110a48[i]; p && n < 64;
             p = (int *)(size_t)p[0x12], ++n)
            std::printf(" (%d,%d,%d)", p[0] * 8 / 4096,
                        (p[1] - 0x1e000) * 8 / 4096, p[2] * 8 / 4096);
        std::printf("  [%d]\n", n);
    }
}

/* SM64DS_TREE_PROBE=2 additionally draws variant 4's Model through the
   HARNESS path (hal_render_model, the one the castle uses) at the origin, so
   the collapsed-geometry question is answered on one BMD by two renderers
   rather than by argument. */
extern "C" void hal_render_model(void *model, int scaleShift);
static void tree_probe_harness(void *self)
{
    const char *e = std::getenv("SM64DS_TREE_PROBE");
    if (!e || e[0] != '2')
        return;
    char *mdl = (char *)self + 0xd4 + 4 * 0x50;
    const unsigned char *bf = *(const unsigned char *const *)(mdl + 0x0c);
    hal_render_model(mdl, bf ? (int)bf[0] : 0);
}

static int __fastcall tree_render(void *s, void *)
{ tree_probe(); tree_probe_harness(s); return ((Tree *)s)->Tree::Render(); }
static int __fastcall tree_pdes(void *, void *)
{ _ZN4Tree16OnPendingDestroyEv(); return 0; }
static int __fastcall tree_d1(void *s, void *)
{ return _ZN4TreeD1Ev((char *)s); }
static int __fastcall tree_d0(void *s, void *)
{ return (int)(size_t)_ZN4TreeD0Ev((char *)s); }

extern "C" void hal_fill_cylinder_withpos_vtable(void);

extern "C" void hal_fill_tree_vtable(void)
{
    void **vt = _ZTV4Tree;
    hal_fill_cylinder_withpos_vtable();
    ac_fill_shared(vt, Tree_trap13);
    vt[0] = (void *)tree_init;
    vt[3] = (void *)tree_clean;
    vt[6] = (void *)tree_behavior;
    vt[9] = (void *)tree_render;
    vt[12] = (void *)tree_pdes;
    vt[16] = (void *)tree_d1;
    vt[17] = (void *)tree_d0;
}

// ---- AMBIENT_SOUND_EFFECTS (actor 350, ov002) x5 ---------------------------
//
// THE CONFIG'S NAMES ARE SHIFTED BY ONE CLASS HERE, and the ROM settles it.
// The spawn table's entry for 350 is AmbientSoundEffects_SpawnInfo, whose
// factory word is 0x020f1b94 -- the function config/arm9/overlays/ov002 calls
// AmbientSoundEffects_Spawn -- and that function's literal pool installs the
// vtable at 0x0210b4c8, which the same config calls _ZTV14EnemySwitchTag.
// Every method in that table (0x020f198c..0x020f1ac4) is likewise named
// _ZN14EnemySwitchTag*. mwcc emits a class as [methods..., Spawn], so the
// block BEFORE a factory belongs to it; the names were attached one block
// late from the factory onward.
//
// The reading is confirmed from the other side: the Behavior in that block
// calls Sound::PlayLong with data_ov002_0210b498[param] -- an ambient loop
// table -- and skips it in the three underwater camera modes. That is
// AMBIENT SOUND EFFECTS, whatever the symbol says.
//
// So the port takes the ROM's word and uses the _ZN14EnemySwitchTag* files,
// which is also what makes it link: AmbientSoundEffects_Spawn.c already
// references _ZTV14EnemySwitchTag by that name. Nothing is renamed.
//
// It renders nothing (Render is `return 1`) and its five instances are silent
// while the Sound:: layer is stubbed, so what this row proves is the registry
// generalising to a second class: five actors spawn, initialise, land on both
// processing lists and tick every frame.
#include "EnemySwitchTag.h"
extern "C" {
int _ZN14EnemySwitchTag6RenderEv(void);
int _ZN14EnemySwitchTag16CleanupResourcesEv(void);
void _ZN14EnemySwitchTag16OnPendingDestroyEv(void);
void *_ZTV14EnemySwitchTag[20];
}

ACTRAP(AmbientSound, 13)
static int __fastcall amb_init(void *s, void *)
{ return ((EnemySwitchTag *)s)->EnemySwitchTag::InitResources(); }
static int __fastcall amb_clean(void *, void *)
{ return _ZN14EnemySwitchTag16CleanupResourcesEv(); }
static int __fastcall amb_behavior(void *s, void *)
{ return ((EnemySwitchTag *)s)->EnemySwitchTag::Behavior(); }
static int __fastcall amb_render(void *, void *)
{ return _ZN14EnemySwitchTag6RenderEv(); }
static int __fastcall amb_pdes(void *, void *)
{ _ZN14EnemySwitchTag16OnPendingDestroyEv(); return 0; }

extern "C" void hal_fill_ambient_sound_vtable(void)
{
    void **vt = _ZTV14EnemySwitchTag;
    ac_fill_shared(vt, AmbientSound_trap13);
    vt[0] = (void *)amb_init;
    vt[3] = (void *)amb_clean;
    vt[6] = (void *)amb_behavior;
    vt[9] = (void *)amb_render;
    vt[12] = (void *)amb_pdes;
    /* 16/17 keep the trap: nothing on the castle grounds destroys one of
       these, and their two TUs need a shape the port has no reason to build
       (a real ~Actor and the VT/HEAP placeholders). */
    vt[16] = (void *)AmbientSound_trap13;
    vt[17] = (void *)AmbientSound_trap13;
}

/* Silence the unused-static warning for the two shared helpers a class with
   its own OnPendingDestroy does not take. */
extern "C" void *hal_actor_shared_pdes(void) { return (void *)ac_pdes_base; }

// ---- Platform, the base three of these classes destruct through ------------
//
// Every Platform-derived destructor is the mwcc two-step: install the class's
// own vtable, run the members that belong to the derived part, install the
// BASE vtable, run the rest. The base one is ov002 0x0210ae38, which the
// destructors spell _ZTV10dBgActor_c (Platform is dBgActor_c in the ROM's own
// RTTI) and which config/arm9/overlays/ov002/symbols.txt calls
// _ZTV17ExclamationSwitch -- a name attached one class late, the same skew
// AMBIENT_SOUND_EFFECTS ran into. The ROM settles it: slots 16/17 of that
// table are _ZN8PlatformD2Ev and _ZN8PlatformD0Ev, and slots 0/3/6/9 are
// ActorBase's own do-nothing bodies, which is exactly a base-class table.
//
// It is only ever installed BETWEEN two member teardowns and nothing dispatches
// through it while it is there, so the port gives it the shared half and traps
// the rest: if a future class does dispatch, it says which slot.
extern "C" { void *_ZTV10dBgActor_c[20]; }
ACTRAP(Platform, 0)
extern "C" void hal_fill_platform_vtable(void)
{
    static int done;
    if (done) return;
    done = 1;
    for (int i = 0; i < 20; ++i)
        _ZTV10dBgActor_c[i] = (void *)Platform_trap0;
    ac_fill_shared(_ZTV10dBgActor_c, Platform_trap0);
    _ZTV10dBgActor_c[12] = (void *)ac_pdes_base;
}

// ---- BLACK_BRICK_BLOCK (actor 17, ov002) x1 --------------------------------
//
// _ZTV13BigBrickBlock, ov002 0x02108adc. One class body serves six actor ids
// and switches on its own; 17 is 0x11, the variant that tracks a star and
// carries the 1.0-scaled KCL. The castle grounds' single instance is at
// (-3300, -650, 20), under the bridge.
//
// Four of the six own slots are real MSVC methods against include/
// BigBrickBlock.h; InitResources and the two destructors are C-form TUs.
#include "BigBrickBlock.h"
extern "C" {
int _ZN13BigBrickBlock13InitResourcesEv(void *self);
int *_ZN13BigBrickBlockD1Ev(int *self);
int *_ZN13BigBrickBlockD0Ev(int *self);
void *_ZTV13BigBrickBlock[20];
}
/* The destructors spell the class's own table by its RTTI name. */
#pragma comment(linker, "/alternatename:__ZTV13daObjBlockL_c=__ZTV13BigBrickBlock")

ACTRAP(BigBrickBlock, 13)
static int __fastcall bbb_init(void *s, void *)
{ return _ZN13BigBrickBlock13InitResourcesEv(s); }
static int __fastcall bbb_clean(void *s, void *)
{ return ((BigBrickBlock *)s)->BigBrickBlock::CleanupResources(); }
static int __fastcall bbb_behavior(void *s, void *)
{ return ((BigBrickBlock *)s)->BigBrickBlock::Behavior(); }
extern "C" void port_actor_render_probe(const char *cls, void *model);
static int __fastcall bbb_render(void *s, void *)
{
    port_actor_render_probe("BLACK_BRICK_BLOCK", (char *)s + 0xd4);
    return ((BigBrickBlock *)s)->BigBrickBlock::Render();
}
static int __fastcall bbb_d1(void *s, void *)
{ return (int)(size_t)_ZN13BigBrickBlockD1Ev((int *)s); }
static int __fastcall bbb_d0(void *s, void *)
{ return (int)(size_t)_ZN13BigBrickBlockD0Ev((int *)s); }

extern "C" void hal_fill_black_brick_block_vtable(void)
{
    void **vt = _ZTV13BigBrickBlock;
    hal_fill_platform_vtable();
    ac_fill_shared(vt, BigBrickBlock_trap13);
    vt[0] = (void *)bbb_init;
    vt[3] = (void *)bbb_clean;
    vt[6] = (void *)bbb_behavior;
    vt[9] = (void *)bbb_render;
    vt[12] = (void *)ac_pdes_base;
    vt[16] = (void *)bbb_d1;
    vt[17] = (void *)bbb_d0;
}

// ---- SIGN_POST (actor 184, ov002) x5 ---------------------------------------
//
// _ZTV8SignPost, ov002 0x02109af8 (RTTI: daObjTatefuda_c -- tatefuda is a
// signboard). Five of them around the grounds. This is the first class whose
// InitResources RAYCASTS THE LEVEL: it drops a ground ray from 100 units above
// the object record's Y and plants the post on whatever it finds, so the five
// signs read back the same floor the Player stands on.
//
// It also carries three collision objects at once -- a MovingMeshCollider for
// the solid post, a MovingCylinderClsn for the read trigger, and a WithMeshClsn
// of its own -- which is the shape every Platform actor after it has.
//
// THE TALK PATH IS GUARDED, and this is the one place a slot is not the ROM's.
// Behavior's read branch runs when the Player is in range with its 0xc8 word
// set, and func_ov002_020bb060 ends in the Message system (Message::Show and
// the dialogue box's OAM/font machinery), none of which the port hosts. The
// guard is in hal/actor_vtables.cpp at that function, not here: the sign
// still turns to face the player and still blocks him, it just does not open
// a box that has nothing to draw with.
#include "SignPost.h"
extern "C" {
int _ZN8SignPost8BehaviorEv(char *self);
int *_ZN8SignPostD1Ev(int *self);
int *_ZN8SignPostD0Ev(int *self);
void *_ZTV8SignPost[20];
}
#pragma comment(linker, "/alternatename:__ZTV15daObjTatefuda_c=__ZTV8SignPost")

ACTRAP(SignPost, 13)
static int __fastcall sp_init(void *s, void *)
{ return ((SignPost *)s)->SignPost::InitResources(); }
static int __fastcall sp_clean(void *s, void *)
{ return ((SignPost *)s)->SignPost::CleanupResources(); }
static int __fastcall sp_behavior(void *s, void *)
{ return _ZN8SignPost8BehaviorEv((char *)s); }
/* SM64DS_ACTOR_PROBE=1: one line per class the first time its Render runs,
   with what the Model has to draw with -- the BMD the file pointer resolved
   to and the model matrix's own translation row, which Platform writes in
   SCENE units. A class that never prints is being culled before Render;
   one that prints with a null file has a load problem instead. */
extern "C" void port_actor_render_probe(const char *cls, void *model)
{
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_ACTOR_PROBE") != 0;
    if (!on) return;
    static const char *said[8];
    static int n;
    for (int i = 0; i < n; ++i)
        if (said[i] == cls) return;
    if (n < 8) said[n++] = cls;
    const int *m = (const int *)((const char *)model + 0x1c);
    std::printf("[actor] %-17s model %p file %p mat.t (%d,%d,%d) scene\n",
                cls, model, *(void *const *)((const char *)model + 0x0c),
                m[9] >> 12, m[10] >> 12, m[11] >> 12);
}

static int __fastcall sp_render(void *s, void *)
{
    port_actor_render_probe("SIGN_POST", (char *)s + 0xd4);
    return ((SignPost *)s)->SignPost::Render();
}
static int __fastcall sp_d1(void *s, void *)
{ return (int)(size_t)_ZN8SignPostD1Ev((int *)s); }
static int __fastcall sp_d0(void *s, void *)
{ return (int)(size_t)_ZN8SignPostD0Ev((int *)s); }

extern "C" void port_sign_post_states_seat(void);   /* port/unmatched */

extern "C" void hal_fill_sign_post_vtable(void)
{
    void **vt = _ZTV8SignPost;
    /* the sinit copied the ROM's own five {Init, Main} pairs into
       data_ov002_0210e084, which means DS code addresses; seat the host
       bodies over them (the ovdata contract, see the file header there) */
    port_sign_post_states_seat();
    hal_fill_platform_vtable();
    ac_fill_shared(vt, SignPost_trap13);
    vt[0] = (void *)sp_init;
    vt[3] = (void *)sp_clean;
    vt[6] = (void *)sp_behavior;
    vt[9] = (void *)sp_render;
    vt[12] = (void *)ac_pdes_base;
    vt[16] = (void *)sp_d1;
    vt[17] = (void *)sp_d0;
}

// ---- ONE_UP_MUSHROOM (actor 276, ov002) x3 ---------------------------------
//
// _ZTV13OneUpMushroom, ov002 0x021083c8 (RTTI: da1up_c). Three of them on the
// castle roof at param 0x00f3, which is mushroom type 3.
//
// Two slots are this class's own rather than Actor's, and they are the reason
// the class exists: slot 18 (OnYoshiTryEat) returns 4 -- "swallow me" -- and
// slot 19 (OnTurnIntoEgg) is the collect path, GiveLives plus the 1UP logo
// actor. Both are hosted; nothing on this level reaches them while the
// character is Mario.
//
// Behavior is the HOST COPY in port/unmatched/OneUpMushroom_Behavior.cpp: the
// ROM's dispatches the mushroom type through an mwcc pointer-to-member table
// and MSVC has no representation for one. See that file's header.
#include "OneUpMushroom.h"
extern "C" {
int _ZN13OneUpMushroom8BehaviorEv(void *self);       /* port/unmatched */
void _ZN13OneUpMushroom16OnPendingDestroyEv(void);
int *_ZN13OneUpMushroomD1Ev(int *self);
int *_ZN13OneUpMushroomD0Ev(int *self);
int func_ov002_020af3a0(void);                       /* OnYoshiTryEat */
void func_ov002_020af2b0(char *self, int arg);       /* OnTurnIntoEgg */
void *_ZTV13OneUpMushroom[20];
}
#pragma comment(linker, "/alternatename:__ZTV7da1up_c=__ZTV13OneUpMushroom")

ACTRAP(OneUpMushroom, 13)
static int __fastcall oum_init(void *s, void *)
{ return ((OneUpMushroom *)s)->OneUpMushroom::InitResources(); }
static int __fastcall oum_clean(void *s, void *)
{ return ((OneUpMushroom *)s)->OneUpMushroom::CleanupResources(); }
static int __fastcall oum_behavior(void *s, void *)
{ return _ZN13OneUpMushroom8BehaviorEv(s); }
static int __fastcall oum_render(void *s, void *)
{
    port_actor_render_probe("ONE_UP_MUSHROOM", (char *)s + 0x300);
    return (int)((OneUpMushroom *)s)->OneUpMushroom::Render();
}
static int __fastcall oum_pdes(void *, void *)
{ _ZN13OneUpMushroom16OnPendingDestroyEv(); return 0; }
static int __fastcall oum_d1(void *s, void *)
{ return (int)(size_t)_ZN13OneUpMushroomD1Ev((int *)s); }
static int __fastcall oum_d0(void *s, void *)
{ return (int)(size_t)_ZN13OneUpMushroomD0Ev((int *)s); }
static int __fastcall oum_yoshi(void *, void *)
{ return func_ov002_020af3a0(); }
static int __fastcall oum_egg(void *s, void *, int a)
{ func_ov002_020af2b0((char *)s, a); return 0; }

extern "C" void port_one_up_mushroom_types_seat(void);   /* port/unmatched */

extern "C" void hal_fill_one_up_mushroom_vtable(void)
{
    void **vt = _ZTV13OneUpMushroom;
    /* same as the sign: __sinit_ov002_02100adc left fourteen DS code
       addresses in data_ov002_0210dc00 */
    port_one_up_mushroom_types_seat();
    ac_fill_shared(vt, OneUpMushroom_trap13);
    vt[0] = (void *)oum_init;
    vt[3] = (void *)oum_clean;
    vt[6] = (void *)oum_behavior;
    vt[9] = (void *)oum_render;
    vt[12] = (void *)oum_pdes;
    vt[16] = (void *)oum_d1;
    vt[17] = (void *)oum_d0;
    vt[18] = (void *)oum_yoshi;
    vt[19] = (void *)oum_egg;
}

// ---- CylinderClsnWithPos ---------------------------------------------------
//
// Not an actor, but the Tree's InitResources constructs one per instance and
// the constructor installs _ZTV19CylinderClsnWithPos by name. Four slots
// (arm9 0x0208e6bc: D1, D0, GetPos, GetOwnerID) and the last two really do
// dispatch -- every cylinder the Tree threads onto data_0209cee8 shares that
// list with the Player's own, and the cylinder pass asks each node where it
// is and who owns it. A zeroed table would be a call through null the first
// time Mario walked near a trunk.
extern "C" {
void *_ZTV19CylinderClsnWithPos[4];
void *_ZN19CylinderClsnWithPosD1Ev(void *self);
void *_ZN19CylinderClsnWithPosD0Ev(void *self);
void *_ZN19CylinderClsnWithPos6GetPosEv(void *self);
}
#include "CylinderClsnWithPos.h"

static void *__fastcall ccp_d1(void *s, void *)
{ return _ZN19CylinderClsnWithPosD1Ev(s); }
static void *__fastcall ccp_d0(void *s, void *)
{ return _ZN19CylinderClsnWithPosD0Ev(s); }
static void *__fastcall ccp_getpos(void *s, void *)
{ return _ZN19CylinderClsnWithPos6GetPosEv(s); }
static unsigned __fastcall ccp_ownerid(void *s, void *)
{ return ((CylinderClsnWithPos *)s)->CylinderClsnWithPos::GetOwnerID(); }

extern "C" void hal_fill_cylinder_withpos_vtable(void)
{
    _ZTV19CylinderClsnWithPos[0] = (void *)ccp_d1;
    _ZTV19CylinderClsnWithPos[1] = (void *)ccp_d0;
    _ZTV19CylinderClsnWithPos[2] = (void *)ccp_getpos;
    _ZTV19CylinderClsnWithPos[3] = (void *)ccp_ownerid;
}
