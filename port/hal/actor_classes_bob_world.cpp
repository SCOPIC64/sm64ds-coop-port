// Gate 33: the vtables of Bob-omb Battlefield's mechanisms, terrain objects
// and pickups. Everything in that level that is not a living creature.
//
// Same law as hal/actor_classes.cpp, which carries the long version: MSVC
// slot order, __fastcall thunks, every call qualified, and a named trap for
// anything the port cannot service. The ten shared slots are filled by the
// local copy of ac_fill_shared below rather than by reaching into the other
// file, so this one merges with the enemy stream without either touching the
// other's statics.
//
// ---- how the roster was established ---------------------------------------
//
// Bob-omb Battlefield is level 6 and ov014; see the header of
// slice_gate33.txt for the three reads that settle it. Every id below came
// out of ov014's own object tables, was resolved through the EU
// ACTOR_SPAWN_TABLE at arm9 0x02090864, and its vtable was checked against
// the ROM's Itanium RTTI -- the typeinfo pointer four bytes before the table.
// That last step matters because the config's class names are shifted by one
// through a run of ov002 (the gate-20 finding), and the RTTI is the only
// thing that says which way.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"

extern "C" {
int _ZN5Actor19BeforeInitResourcesEv(void *self);              /* slot 1  */
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a);  /* slot 2  */
int _ZN5Actor14BeforeBehaviorEv(void *self);                   /* slot 7  */
int _ZN5Actor12BeforeRenderEv(void *self);                     /* slot 10 */
int _ZN5Actor13OnYoshiTryEatEv(void *self);                    /* slot 18 */
extern int data_02099f24[];               /* the frame phase */
extern unsigned char data_020a4b4c;       /* the spawn spine's own step */
const char *port_actor_class_name(unsigned id);
void port_actor_render_probe(const char *cls, void *model);
}

static int __fastcall bw_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall bw_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall bw_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
/* Slots 5, 8 and 11 are ARM tail-call veneers on the ROM, so the thunk calls
   the target directly rather than forwarding through the veneer's own face
   and dropping the argument riding in r1. */
static void __fastcall bw_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
static int __fastcall bw_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall bw_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
static int __fastcall bw_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
static void __fastcall bw_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
/* SLOTS 13 AND 14 KEEP THE GATE-16 TRAP, and the reason was measured rather
   than assumed this time. The vtable law wants both wired to ActorBase's own
   Virtual34/Virtual38 -- a heap-pressure path calls Virtual34 on every actor
   on the behaviour list, so a class that traps there dies on a frame that has
   nothing to do with it. Both are matched src, and so is everything under
   them, but everything under them is the SOLID HEAP ALLOCATOR: 17 SolidHeap
   files, 12 SolidHeapAllocator files and a synthetic sixteen-slot
   _ZTV9SolidHeap. That is a subsystem next to gate 3a's ExpandingHeap, not
   two slots on this gate's classes. The full chain is written out in the head
   of slice_gate33.txt so the next attempt starts with the bill in hand.
   Until then the trap names the slot, the actor and the phase, which is the
   loud version of "no level has reached this yet". */
static int __fastcall bw_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
static int __fastcall bw_yoshi(void *s, void *)
{ return _ZN5Actor13OnYoshiTryEatEv(s); }
static void __fastcall bw_pdes_base(void *s, void *)
{ ((ActorBase *)s)->ActorBase::OnPendingDestroy(); }

static void bw_trap_report(void *self, int slot)
{
    unsigned id = self ? *(unsigned short *)((char *)self + 0xc) : 0u;
    std::fprintf(stderr,
                 "FATAL: vtable slot %d is not hosted (actor id %u %s, "
                 "phase %d, spawn step %d)\n", slot, id,
                 port_actor_class_name(id), data_02099f24[0],
                 (int)data_020a4b4c);
    std::abort();
}
static int __fastcall bw_trap13(void *s, void *) { bw_trap_report(s, 13); return 0; }
static int __fastcall bw_trap14(void *s, void *) { bw_trap_report(s, 14); return 0; }
static int __fastcall bw_trap16(void *s, void *) { bw_trap_report(s, 16); return 0; }
static int __fastcall bw_trap17(void *s, void *) { bw_trap_report(s, 17); return 0; }
static int __fastcall bw_trap19(void *s, void *) { bw_trap_report(s, 19); return 0; }

static void bw_fill_shared(void **vt)
{
    vt[1] = (void *)bw_binit;
    vt[2] = (void *)bw_ainit;
    vt[4] = (void *)bw_bclean;
    vt[5] = (void *)bw_aclean;
    vt[7] = (void *)bw_bbeh;
    vt[8] = (void *)bw_abeh;
    vt[10] = (void *)bw_bren;
    vt[11] = (void *)bw_aren;
    vt[13] = (void *)bw_trap13;
    vt[14] = (void *)bw_trap14;
    vt[15] = (void *)bw_heap;
    vt[18] = (void *)bw_yoshi;
    vt[19] = (void *)bw_trap19;
}

// ---- CommonModel, the first time it is hosted ------------------------------
//
// Not an actor. The coin carries TWO of them (+0xd8 and +0x114) and every
// hosted class before this gate used Model or ModelAnim instead, so its
// vtable and its one cross-face have to land with the coin.
//
// _ZTV11CommonModel is arm9 0x0208e8a4 and it is THREE SLOTS: D1, D0, and
// DoSetFile. The class's Render is non-virtual, which is the whole difference
// from Model. Under MSVC the two destructor slots fold into one, so DoSetFile
// is slot 1 there and slot 2 in ROM/Itanium numbering -- and both get filled,
// the same dual-fill _ZTV5Model already carries in hal/cxxname_bridge.cpp,
// because a TU dispatching through a LOCAL shadow class counts the ROM's way.
//
// The live dispatcher is ModelBase::SetFile, which Coin::InitResources calls
// four times by its Itanium name.
#include "CommonModel.h"
extern "C" {
void *_ZTV11CommonModel[8];
}

static int __fastcall cm_dosetfile(void *self, void *, char *file, int a, int b)
{ return ((CommonModel *)self)->CommonModel::DoSetFile(file, a, b); }
static int __fastcall cm_dtor_trap(void *, void *)
{
    std::fprintf(stderr, "FATAL: CommonModel vtable slot 0 dispatched -- the "
                 "port destroys a CommonModel by calling its D1 directly\n");
    std::abort();
    return 0;
}

static void hal_fill_common_model_vtable(void)
{
    _ZTV11CommonModel[0] = (void *)cm_dtor_trap;
    _ZTV11CommonModel[1] = (void *)cm_dosetfile;   /* MSVC numbering */
    _ZTV11CommonModel[2] = (void *)cm_dosetfile;   /* ROM numbering */
}

/* Coin::Render calls CommonModel::Render by its Itanium name from a TU that
   declared it extern "C", so the reference is cdecl and the definition is a
   real MSVC __thiscall method. A forwarding face, never an alias. The ROM's
   Render returns void and every caller ignores the result. */
extern "C" int _ZN11CommonModel6RenderEPK7Vector3(void *self, const void *scale)
{
    ((CommonModel *)self)->CommonModel::Render((const Vector3 *)scale);
    return 1;
}

/* The coin's collect path reaches the red-coin star by C name from
   func_ov002_020b10a0, and the definition is a real MSVC method. The shadow
   declaration is the CALLER'S: a decorated name is its own name, its class's
   name, its calling convention and its parameter types, nothing about
   layout, so one method is the whole declaration that is needed. */
struct StarMarkerFace { void SpawnRedCoinStarIfNecessary(); };
#pragma comment(linker, "/alternatename:?SpawnRedCoinStarIfNecessary@StarMarkerFace@@QAEXXZ=?SpawnRedCoinStarIfNecessary@StarMarker@@QAEXXZ")
extern "C" void _ZN10StarMarker27SpawnRedCoinStarIfNecessaryEv(void *self)
{ ((StarMarkerFace *)self)->SpawnRedCoinStarIfNecessary(); }

// ---- COIN (288), RED_COIN (289), BLUE_COIN (290) -- ov002 ------------------
//
// _ZTV4Coin, ov002 0x021087ec, RTTI 8daCoin_c. ONE CLASS, THREE IDS: the
// three factories are identical (948 bytes, Actor base, two CommonModels, a
// ShadowModel, a MovingCylinderClsn and a WithMeshClsn) and all three write
// this table, so one fill serves all three registry rows. The class reads
// mActorID back to tell which it is -- CleanupResources releases the red
// coin's own SharedFilePtr on 0x121, Behavior retires a blue coin's area on
// 0x122.
//
// Bob-omb Battlefield names 60 coins and 8 red coins in group 0 alone, which
// is the eight-red-coin star. Blue coins come from a BlueCoinSwitch, so the
// level's tables never name 290; it is registered anyway because it is the
// same table and the same factory.
//
// Behavior is the HOST COPY in port/unmatched/Coin_Behavior.cpp: the ROM's
// dispatches the behaviour state through an mwcc pointer-to-member table.
//
// SLOT 16 IS THE ROM'S D0 BODY MINUS ITS FINAL Deallocate, written out here
// rather than compiled from src/_ZN4CoinD1Ev.cpp, which is a real C++
// destructor over its own shadow hierarchy. This slot is LIVE and has to be:
// AfterCleanupResources dispatches it and then deallocates, and a coin is
// destroyed every single time one is collected. Member order is the reverse
// of the factory's construction order, which is what the ROM's own D0 does.
extern "C" {
int _ZN4Coin13InitResourcesEv(char *self);
int _ZN4Coin16CleanupResourcesEv(char *self);
int _ZN4Coin8BehaviorEv(void *self);            /* port/unmatched */
int func_ov002_020b2a90(void);                  /* OnYoshiTryEat */
void func_ov002_020b2a34(char *self, int arg);  /* OnTurnIntoEgg */
void *_ZTV4Coin[20];
void port_coin_states_seat(void);               /* port/unmatched */
/* the five member destructors the D1 body runs, in reverse order */
void _ZN12WithMeshClsnD1Ev(void *);
void _ZN18MovingCylinderClsnD1Ev(void *);
void _ZN11ShadowModelD1Ev(void *);
void _ZN11CommonModelD1Ev(void *);
void *_ZN5ActorD2Ev(void *);
}
/* Coin's own D0 spells its table by the RTTI name. */
#pragma comment(linker, "/alternatename:__ZTV8daCoin_c=__ZTV4Coin")

#include "Coin.h"

static int __fastcall coin_init(void *s, void *)
{ return _ZN4Coin13InitResourcesEv((char *)s); }
static int __fastcall coin_clean(void *s, void *)
{ return _ZN4Coin16CleanupResourcesEv((char *)s); }
static int __fastcall coin_behavior(void *s, void *)
{ return _ZN4Coin8BehaviorEv(s); }
/* SM64DS_ACTOR_PROBE=1 for a CommonModel. The shared probe in
   hal/actor_classes.cpp reads a MODEL's layout (file at +0x0c, transforms at
   +0x14, matrix at +0x1c) and a CommonModel is a different, shorter object --
   vptr, modelFile at +0x04, the pooled components at +0x08, its 0x30-byte
   matrix at +0x0c and nothing after. Handing the shared probe one prints four
   numbers read past the end of it. */
static void coin_model_probe(const char *what, const char *m)
{
    static int on = -1;
    static int said;
    if (on < 0) on = std::getenv("SM64DS_ACTOR_PROBE") != 0;
    if (!on || said > 1) return;
    ++said;
    const int *t = (const int *)(m + 0x0c + 0x24);   /* mat4x3 translation */
    std::printf("[actor] %-17s model %p file %p pool %p mat.t (%d,%d,%d) "
                "scene\n", what, (const void *)m,
                *(void *const *)(m + 0x04), *(void *const *)(m + 0x08),
                t[0] >> 12, t[1] >> 12, t[2] >> 12);
}

static int __fastcall coin_render(void *s, void *)
{
    /* Two CommonModels: 0xd8 is the one drawn with flag 0x10 clear and 0x114
       the one drawn with it set. */
    coin_model_probe("COIN", (const char *)s + 0xd8);
    return ((Coin *)s)->Coin::Render();
}
static int __fastcall coin_d1(void *s, void *)
{
    *(void **)s = (void *)_ZTV4Coin;
    _ZN12WithMeshClsnD1Ev((char *)s + 0x1ac);
    _ZN18MovingCylinderClsnD1Ev((char *)s + 0x178);
    _ZN11ShadowModelD1Ev((char *)s + 0x150);
    _ZN11CommonModelD1Ev((char *)s + 0x114);
    _ZN11CommonModelD1Ev((char *)s + 0xd8);
    _ZN5ActorD2Ev(s);
    return (int)(size_t)s;
}
static int __fastcall coin_yoshi(void *, void *)
{ return func_ov002_020b2a90(); }
static int __fastcall coin_egg(void *s, void *, int a)
{ func_ov002_020b2a34((char *)s, a); return 0; }

// ---- the coin's models are preloaded by Stage::InitResources ---------------
//
// Coin::InitResources does NOT load its own file for coin types 0 and 1. It
// reads data_ov002_020ff06c[type]->filePtr and hands it straight to
// ModelBase::SetFile, on the assumption that somebody already loaded it. That
// somebody is Stage::InitResources (arm9 0x0202cc0c), which walks a four-entry
// table of SharedFilePtr* at arm9 0x020756f0 and loads each one. The four
// entries -- and config/arm9/relocs.txt names all four -- are exactly the
// coin's two pairs:
//
//     0x020756f0 -> ov002 0x0210da48   file 0x8006   coin type 0, model B
//     0x020756f4 -> ov002 0x0210d9b8   file 0x8005   coin type 0, model A
//     0x020756f8 -> ov002 0x0210da50   file 0x8008   coin type 1, model B
//     0x020756fc -> ov002 0x0210d9f8   file 0x8007   coin type 1, model A
//
// Stage::InitResources DOES NOT RUN IN THE PORT: hal/stage_bridges.cpp fills
// every one of the Stage's twenty slots with a trap, and gate 26 landed the
// Stage as a scene root rather than as a running actor. Without the preload a
// spawned coin walks straight into Model::LoadTexAndPal with a null BMD_File
// and faults at +0xc -- which is exactly what the first run did, and the
// backtrace named every frame of it.
//
// So the load happens here, out of the same four SharedFilePtrs by name, with
// the same Model::LoadFile the ROM calls. Nothing is invented: the fileIDs are
// asserted against the ones __sinit_ov002_02100560 constructed them with, so a
// mount that has drifted says so instead of loading a stranger's archive.
// WHEN Stage::InitResources IS HOSTED THIS COMES OUT WHOLE -- it is one call
// and one table, in the wrong place only because its owner is not running.
extern "C" {
void *_ZN5Model8LoadFileER13SharedFilePtr(void *ptr);
extern unsigned char data_ov002_0210da48[], data_ov002_0210d9b8[],
    data_ov002_0210da50[], data_ov002_0210d9f8[];
}

static void port_coin_models_preload(void)
{
    static const struct { unsigned char *ptr; unsigned short file;
                          const char *what; }
    k[] = {
        {data_ov002_0210da48, 0x8006, "coin type 0 model B"},
        {data_ov002_0210d9b8, 0x8005, "coin type 0 model A"},
        {data_ov002_0210da50, 0x8008, "coin type 1 model B"},
        {data_ov002_0210d9f8, 0x8007, "coin type 1 model A"},
    };
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof k / sizeof k[0]; ++i) {
        unsigned short id = *(unsigned short *)k[i].ptr;
        if (id != k[i].file) {
            std::fprintf(stderr, "FATAL: %s: the sinit constructed file %04x, "
                         "the ROM's own table says %04x -- WRONG BYTES\n",
                         k[i].what, id, k[i].file);
            std::abort();
        }
        _ZN5Model8LoadFileER13SharedFilePtr(k[i].ptr);
    }
}

extern "C" void hal_fill_coin_vtable(void)
{
    void **vt = _ZTV4Coin;
    hal_fill_common_model_vtable();
    port_coin_models_preload();
    port_coin_states_seat();
    bw_fill_shared(vt);
    vt[0] = (void *)coin_init;
    vt[3] = (void *)coin_clean;
    vt[6] = (void *)coin_behavior;
    vt[9] = (void *)coin_render;
    vt[12] = (void *)bw_pdes_base;
    vt[16] = (void *)coin_d1;
    /* 17 keeps the trap: the ROM's destroy path is the D1 dispatch plus an
       explicit Deallocate by AfterCleanupResources, and nothing calls the
       deleting form. */
    vt[17] = (void *)bw_trap17;
    vt[18] = (void *)coin_yoshi;
    vt[19] = (void *)coin_egg;
}

// ---- THE DEBUG SPAWN HOOK (port mod) ---------------------------------------
//
// NEEDS RECONCILING AT MERGE. port-beta-lvl is adding a general actor-spawn
// debug hook and this is the minimal stand-in for it, scoped to this file so
// that taking it out later is one deletion. It exists because the classes in
// this gate belong to a level the port cannot boot yet: the boot in
// hal/level_boot.cpp is wired to ov009 by name, and generalising it is
// port-beta-lvl's work, not this gate's.
//
//     SM64DS_SPAWN_ACTOR=id[:param][,id[:param]...]
//
// Spawns each id once, at the local player's own position and area, on the
// first tick after the level has finished booting. Ids are decimal or 0x
// hex; the optional param is the level-table `param` word the actor reads out
// of +8, which for most of these classes chooses a variant.
//
// It runs on the frame AFTER the boot rather than inside it so that the
// spawn spine sees exactly the state a level-table spawn sees: the scene root
// seated, the class table installed, the player alive and positioned.
extern "C" {
void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(unsigned actorID,
                                                   unsigned param1,
                                                   const int *pos,
                                                   const short *rot,
                                                   int areaID,
                                                   int deathTableID);
extern int data_0209f394[];       /* the local players; [0] is the object */
extern short data_ov002_0211118c; /* the per-level spawn counter */
extern int data_020a4b78[];       /* the behaviour processing list */
extern short data_0209f358[];     /* the coin counter GiveCoins increments */
}

static unsigned short g_watch[16];
static int g_watch_n;

extern "C" void port_bob_debug_spawn(void)
{
    static int done;
    const char *spec;
    const char *p;
    const char *player;
    int pos[3];
    signed char area;

    if (done)
        return;
    spec = std::getenv("SM64DS_SPAWN_ACTOR");
    if (!spec) { done = 1; return; }
    player = (const char *)(size_t)data_0209f394[0];
    if (!player)
        return;                  /* the boot has not produced him yet */
    done = 1;

    pos[0] = *(const int *)(player + 0x5c);
    pos[1] = *(const int *)(player + 0x60);
    pos[2] = *(const int *)(player + 0x64);
    area = *(const signed char *)(player + 0xcc);

    for (p = spec; *p; ) {
        char *end;
        long id = std::strtol(p, &end, 0);
        long param = 0;
        void *a;
        if (end == p) break;
        p = end;
        if (*p == ':') { param = std::strtol(p + 1, &end, 0); p = end; }
        while (*p == ',' || *p == ' ') ++p;
        /* Two units above the player's feet, so a class that drops to the
           ground has something to drop through. */
        {
            int at[3] = {pos[0], pos[1] + 0x20000, pos[2]};
            short rot[3] = {0, 0, 0};
            short uniq = data_ov002_0211118c++;
            a = _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
                    (unsigned)id, (unsigned)param, at, rot, area, uniq);
        }
        std::printf("[spawnhook] actor %ld param 0x%lx at (%d,%d,%d) area %d "
                    "-> %p\n", id, param, pos[0] >> 12, pos[1] >> 12,
                    pos[2] >> 12, (int)area, a);
        if (g_watch_n < (int)(sizeof g_watch / sizeof g_watch[0]))
            g_watch[g_watch_n++] = (unsigned short)id;
    }
}

/* The read-back, and it is what proves BEHAVIOUR rather than survival: every
   sixty frames, how many of each watched id are still on the behaviour list,
   where the first of them is, and what the coin counter says. A pickup shows
   up here as the count going down and data_0209f358 going up in the same
   report -- which is the whole of a coin working, and it also means the D1 in
   slot 16 ran and AfterCleanupResources deallocated behind it. */
extern "C" void port_bob_debug_watch(void)
{
    static int frame;
    int i;
    if (!g_watch_n || (frame++ % 60))
        return;
    std::printf("[watch] frame %d coins %d:", frame - 1, data_0209f358[0]);
    for (i = 0; i < g_watch_n; ++i) {
        int n = 0;
        const int *first = 0;
        for (int *node = (int *)(size_t)data_020a4b78[0]; node && n < 4096;
             node = (int *)(size_t)node[1]) {
            char *o = (char *)(size_t)node[2];
            if (!o || *(unsigned short *)(o + 0xc) != g_watch[i])
                continue;
            if (!n++) first = (const int *)(o + 0x5c);
        }
        std::printf("  %u x%d", g_watch[i], n);
        if (first)
            std::printf(" @(%d,%d,%d)", first[0] >> 12, first[1] >> 12,
                        first[2] >> 12);
    }
    std::printf("\n");
}
