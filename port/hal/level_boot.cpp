// The real level boot, host side.
//
// Everything the game needs to load the castle grounds for real -- the level
// overlay, the collision file, the object tables -- instead of the harness's
// hand-staged KCL and invented spawn point. Nothing here is behaviour:
// Stage::LoadClsnAndObjects and its fifteen sub-loaders are the matched src
// files (slice_gate14.txt), and this is the seam they need.
//
// ---- the overlay mount ----------------------------------------------------
//
// Castle grounds is level 1, whose LVL_Overlay lives in ov009 at 0x02112bdc.
// A DS overlay is linked at a fixed base and loaded there unrelocated, so the
// ROM image already carries absolute pointers -- the object tables, the CLPS
// block, the path nodes. Mounting it on the host is therefore two steps:
// copy the whole image into one host array (ovdata.py --whole; per-symbol
// arrays break every walk that steps past the symbol dsd happened to name),
// then rewrite the words the delink table says point back inside it.
//
// port_ov009_at() turns a DS address into the host address of the same byte,
// which is how every constant below is spelled.
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "MeshCollider.h"

extern "C" {
void port_ov009_patch(void);
void *port_ov009_at(unsigned ds);
extern unsigned char port_ov009_image[];
extern const unsigned port_ov009_ds_base, port_ov009_ds_end;
}

/* Castle grounds, level 1. */
enum {
    OV009_LVL_OVERLAY = 0x02112bdc,
};

/* LVL_Overlay, the fields the boot uses. */
struct PortLvlOverlay {
    unsigned char *clps;         /* 0x00 */
    unsigned char *objTable;     /* 0x04 */
    unsigned short bmdFileId;    /* 0x08 */
    unsigned short kclFileId;    /* 0x0a */
    unsigned short icgFileId;    /* 0x0c */
    unsigned short iclFileId;    /* 0x0e */
    unsigned char *subTables;    /* 0x10, stride 0xc */
    unsigned char subCount;      /* 0x14 */
    unsigned char flags;         /* 0x15 */
    unsigned char pad16[2];
    unsigned int unk18;          /* 0x18 */
};

extern "C" void *port_ov009_mount(void)
{
    static void *lvl;
    if (lvl)
        return lvl;
    port_ov009_patch();
    lvl = port_ov009_at(OV009_LVL_OVERLAY);
    if (!lvl) {
        std::fprintf(stderr, "FATAL: ov009 mount: 0x%08x outside the overlay "
                     "[0x%08x, 0x%08x)\n", (unsigned)OV009_LVL_OVERLAY,
                     port_ov009_ds_base, port_ov009_ds_end);
        std::abort();
    }
    return lvl;
}

// ---- the loader dispatch table ---------------------------------------------
//
// LoadObjects indexes data_ov002_0210cbb8 with `kind & 0x1f` and the ROM's
// table is FIFTEEN entries long: indices 15..31 read whatever data follows,
// which in ov002 is the actor-id table at 0x0210cbf4. Real level data never
// carries an index past 14, so the overrun is unreachable rather than a bug,
// but the host copy closes it anyway -- the tail is null and LoadObjects
// already skips null entries.
//
// The table is hand-built rather than ovdata-mounted for the obvious reason:
// its fifteen words are ov002 CODE addresses, meaningless on the host.
/* Called on every sub-loader dispatch, which is the first point INSIDE
   Stage::LoadClsnAndObjects at which host code runs: SetFile has just
   configured the level collider and nothing has raycast against it yet.
   That matters because of RISK 1 (see port_stage_a_boot). The entrance
   loader spawns the Player, and St_LevelEnter_Init drops a ground ray to
   find the floor under the gate -- with the collider still carrying
   SetFile's 1.0 vectors the ray misses and he starts the level at
   0x80000000.

   SM64DS_TRACE_LOADERS=1 also names each loader as it runs, which is the
   only window into the boot: everything inside it is matched src. */
extern "C" void port_clsn_pair_apply(void);

extern "C" void port_loader_enter(int idx, const void *tbl)
{
    port_clsn_pair_apply();
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_TRACE_LOADERS") != 0;
    if (on)
        std::printf("  [load] %2d count %u entries %p\n", idx,
                    ((const unsigned char *)tbl)[1],
                    *(const void *const *)((const char *)tbl + 4));
}

extern "C" {
void _Z19LoadStandardObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z19LoadEntranceObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z19LoadPathNodeObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z15LoadPathObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z15LoadViewObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z17LoadSimpleObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z25LoadTeleportSourceObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z23LoadTeleportDestObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z14LoadFogObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z15LoadDoorObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z15LoadExitObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z22LoadMinimapTileObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z23LoadMinimapScaleObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z23LoadUnusedType13ObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);
void _Z21LoadStarCameraObjectsRN11LVL_Overlay11ObjSubTableEij(void *, int, unsigned);

}  /* extern "C" */

static void port_load0(void *t, int a, unsigned b)
{ port_loader_enter(0, t); _Z19LoadStandardObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load1(void *t, int a, unsigned b)
{ port_loader_enter(1, t); _Z19LoadEntranceObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load2(void *t, int a, unsigned b)
{ port_loader_enter(2, t); _Z19LoadPathNodeObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load3(void *t, int a, unsigned b)
{ port_loader_enter(3, t); _Z15LoadPathObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load4(void *t, int a, unsigned b)
{ port_loader_enter(4, t); _Z15LoadViewObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load5(void *t, int a, unsigned b)
{ port_loader_enter(5, t); _Z17LoadSimpleObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load6(void *t, int a, unsigned b)
{ port_loader_enter(6, t); _Z25LoadTeleportSourceObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load7(void *t, int a, unsigned b)
{ port_loader_enter(7, t); _Z23LoadTeleportDestObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load8(void *t, int a, unsigned b)
{ port_loader_enter(8, t); _Z14LoadFogObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load9(void *t, int a, unsigned b)
{ port_loader_enter(9, t); _Z15LoadDoorObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load10(void *t, int a, unsigned b)
{ port_loader_enter(10, t); _Z15LoadExitObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load11(void *t, int a, unsigned b)
{ port_loader_enter(11, t); _Z22LoadMinimapTileObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load12(void *t, int a, unsigned b)
{ port_loader_enter(12, t); _Z23LoadMinimapScaleObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load13(void *t, int a, unsigned b)
{ port_loader_enter(13, t); _Z23LoadUnusedType13ObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }
static void port_load14(void *t, int a, unsigned b)
{ port_loader_enter(14, t); _Z21LoadStarCameraObjectsRN11LVL_Overlay11ObjSubTableEij(t, a, b); }

extern "C" {
typedef void (*PortObjLoader)(void *, int, unsigned);
PortObjLoader data_ov002_0210cbb8[32] = {
    port_load0,        /*  0 */
    port_load1,        /*  1 */
    port_load2,        /*  2 */
    port_load3,            /*  3 */
    port_load4,            /*  4 */
    port_load5,          /*  5 */
    port_load6,  /*  6 */
    port_load7,    /*  7 */
    port_load8,             /*  8 */
    port_load9,            /*  9 */
    port_load10,            /* 10 */
    port_load11,     /* 11 */
    port_load12,    /* 12 */
    port_load13,    /* 13 */
    port_load14,      /* 14 */
    /* 15..31: the ROM's overrun, made explicit */
};
}  /* extern "C" */

/* Two loaders define plain C++ names (their TUs never wrapped the definition
   in extern "C"); the table above wants the Itanium name every other caller
   uses. */
#pragma comment(linker, "/alternatename:__Z15LoadDoorObjectsRN11LVL_Overlay11ObjSubTableEij=?LoadDoorObjects@@YAXAAUObjSubTable@LVL_Overlay@@HI@Z")
#pragma comment(linker, "/alternatename:__Z21LoadStarCameraObjectsRN11LVL_Overlay11ObjSubTableEij=?LoadStarCameraObjects@@YAXAAUObjSubTable@LVL_Overlay@@HI@Z")

// ---- LoadFile(handle) ------------------------------------------------------
//
// The ROM's LoadFile is func_0201818c(handle, 1), the archive loader's
// refcounted entry point. The port's file seam is one level up, at
// SharedFilePtr (hal/fs.cpp), so this is the same contract expressed there:
// one persistent SharedFilePtr per handle, loaded once, pointer returned.
//
// It deliberately does NOT run MeshCollider::UpdateFileOffsets, which is what
// makes it different from MeshCollider::LoadFile. The caller here is
// Stage::LoadClsnAndObjects, and its very next line is the fixup. Doing it in
// both places rebases the four header words twice, and the fixup is
// `ptr = &file + (int)ptr` -- not idempotent, so the second pass sends the
// positions array off into whatever follows the file.
extern "C" {
struct PortSharedFilePtr {
    unsigned short fileID;
    unsigned char numRefs;
    unsigned char pad;
    char *filePtr;
};
struct PortSharedFilePtr *_ZN13SharedFilePtr9ConstructEj(struct PortSharedFilePtr *,
                                                         unsigned);
void _ZN13SharedFilePtr8LoadFileEv(struct PortSharedFilePtr *);

void *LoadFile(int handle)
{
    enum { SLOTS = 16 };
    static PortSharedFilePtr slot[SLOTS];
    static int used;
    for (int i = 0; i < used; ++i)
        if (slot[i].fileID && slot[i].filePtr &&
            (int)slot[i].fileID == handle)
            return slot[i].filePtr;
    if (used >= SLOTS) {
        std::fprintf(stderr, "FATAL: LoadFile: out of host file slots\n");
        std::abort();
    }
    PortSharedFilePtr *s = &slot[used];
    _ZN13SharedFilePtr9ConstructEj(s, (unsigned)handle);
    _ZN13SharedFilePtr8LoadFileEv(s);
    if (!s->filePtr) {
        std::fprintf(stderr, "FATAL: LoadFile(%d): no bytes\n", handle);
        std::abort();
    }
    /* Construct rewrites fileID from the ov0 handle to the FAT file id, so
       the cache key above matches only when both agree; keep the handle. */
    ++used;
    s->fileID = (unsigned short)handle;
    return s->filePtr;
}

/* Method faces: the three MeshCollider helpers the boot calls by their
   Itanium names while their definitions are real MSVC members. */
void _ZN12MeshCollider17UpdateFileOffsetsER8KCL_File(void *file)
{ MeshCollider::UpdateFileOffsets(*(KCL_File *)file); }
int _ZNK12MeshCollider16GetOctreeOriginYEv(const void *self)
{ return ((const MeshCollider *)self)->MeshCollider::GetOctreeOriginY(); }
int _ZNK12MeshCollider13GetUnkOctreeYEv(const void *self)
{ return ((const MeshCollider *)self)->MeshCollider::GetUnkOctreeY(); }

// ---- the globals the sub-loaders store through -----------------------------
//
// Every "Load<Kind>Objects" that is not a spawner is a two-word veneer:
// store the table pointer in one global, the count in another. Storage only;
// the consumers (minimap, fog, teleport) are Stage B and C.
short data_ov002_0211118c;   /* the per-level spawn counter, ov002 bss */
int data_02092138;           /* world Y min (func_0202a850) */
int data_020a0d8c[4];        /* path count */
int data_0209f31c[4];        unsigned char data_0209f258[4];   /* fog */
int data_0209f328[4];        unsigned char data_0209f214[4];   /* entrances */
int data_0209f334[4];        unsigned char data_0209f2e8[4];   /* minimap tiles */
int data_0209f348[4];        unsigned char data_0209f25c[4];   /* minimap scale */
unsigned char data_0209f2d0[4];                                /* teleport dest
                                                                  count; the
                                                                  pointer
                                                                  data_0209f330
                                                                  is auto_bss */
int data_0209f338[4];        /* the unused type-13 word */
/* the CURRENT LVL_Overlay: storage is hal/actor_vtables.cpp, parked on a
   zeroed block for the no-level case; the boot points it at the real one */
extern unsigned char *data_0209f340;
}  /* extern "C" */

// ---- the save block, contiguous --------------------------------------------
//
// LoadEntranceObjects reads data_0209caa0[0x41]. The symbol dsd named
// data_0209caa0 is 0x14 bytes; byte 0x41 lands inside data_0209cad2, two
// symbols further on. That is the ordinary decomp shape -- one save-file
// struct the delink split five ways at the boundaries code happened to
// reference -- and separate host arrays make the read land on whatever the
// linker put next.
//
// Grouped sections put them back in ROM order, the mechanism romdata.py uses
// for the camera-mode table. Every delta here equals the symbol's own size
// and all four are even, so align(2) packs with no interior padding.
#define SAVEBLK(sec, name, size) \
    __pragma(section(sec, read, write))                          \
    extern "C" __declspec(allocate(sec)) __declspec(align(2))    \
    unsigned char name[size] = {0}

SAVEBLK(".savblk$0000", data_0209caa0, 0x14);
SAVEBLK(".savblk$0001", data_0209cab4, 0x1e);
SAVEBLK(".savblk$0002", data_0209cad2, 0x12);
SAVEBLK(".savblk$0003", data_0209cae4, 0x10);

#undef SAVEBLK

// ---- Stage A ---------------------------------------------------------------
//
// A1 runs the real boot with every spawner switched off, so what it proves is
// exactly the geometry: the KCL comes from the level's own kclFileId, the
// CLPS block is the level's own (not a zeroed stand-in), the path table and
// its 220 nodes are seated, and the world-Y bounds come out of the octree.
//
// Suppression is a write into the HOST copy of the overlay -- counts set to
// zero -- rather than a branch in the loader, because the loader is matched
// src and stays untouched. subCount = 0 removes the whole sub-table, which is
// where the 89 Standard/Simple objects live.
static void port_stage_suppress(PortLvlOverlay *o, unsigned kind_mask,
                                int drop_subtables)
{
    if (drop_subtables)
        o->subCount = 0;
    unsigned n = *(unsigned short *)o->objTable;
    unsigned char *e = *(unsigned char **)(o->objTable + 4);
    for (unsigned i = 0; i < n; ++i, e += 8)
        if (kind_mask & (1u << (e[0] & 0x1f)))
            e[1] = 0;
}

extern "C" {
void _ZN5Stage18LoadClsnAndObjectsER11LVL_OverlayjR12MeshCollider(void *ovl,
                                                                  unsigned p,
                                                                  void *mc);
extern signed char data_0209f2f8;    /* current level */
extern int data_0209f264[];          /* current entrance */
extern int data_0209f220[];          /* current star filter */
extern int data_0209212c;            /* world Y max */
extern int data_020a0d84[];          /* path table base (auto_bss) */
extern int data_020a0d88[];          /* path node base (auto_bss) */

/* Loader indices, for the suppression masks. */
enum {
    LOADER_ENTRANCE = 1,
    LOADER_DOOR = 9,
    LOADER_EXIT = 10,
};

static void *g_stage_mc;

/* The two-line convention documented below, applied wherever the boot first
   hands control back to the host. Idempotent by construction. */
void port_clsn_pair_apply(void)
{
    if (!g_stage_mc)
        return;
    *(int *)((char *)g_stage_mc + 0x2c) = 0x40000;   /* file -> world, 64.0 */
    *(int *)((char *)g_stage_mc + 0x38) = 0x40;      /* world -> file, 1/64 */
}

/* SM64DS_REAL_BOOT stage selector: 1 = A1 (geometry only). */
void *port_stage_a_boot(void *mc, int spawn_entrances)
{
    g_stage_mc = mc;
    PortLvlOverlay *o = (PortLvlOverlay *)port_ov009_mount();

    unsigned mask = (1u << LOADER_DOOR) | (1u << LOADER_EXIT);
    if (!spawn_entrances)
        mask |= 1u << LOADER_ENTRANCE;
    port_stage_suppress(o, mask, 1);

    data_0209f2f8 = 1;          /* castle grounds */
    data_0209f264[0] = 0;       /* entrance 0, the castle gate */
    data_0209f220[0] = 1;       /* star filter: ADVENTURE */
    data_0209f340 = (unsigned char *)o;

    /* ONE BIT, TWO JOBS, and they pull opposite ways on a port with no
       sound engine.
       LoadClsnAndObjects' last decision is the intro cutscene: mode 0 plus
       bit 7 of data_0209caa0[2] clear (= the intro has not played) runs
       StartIntroCutscene, which loads a sound group and, three calls down,
       reads the DS console-type word at 0x027ffc40. So the bit has to be
       SET across the boot.
       The same bit is the one Player::InitResources tests to decide whether
       to load the character's voice bank -- the identical unhosted sound
       path. So it has to be CLEAR when the Player initialises.
       Scoping it to the boot satisfies both while the Player is still the
       harness's (Stage A1). Stage C, which brings the sound seam, is where
       the bit gets to mean what it means. */
    unsigned char intro_seen = (unsigned char)(data_0209caa0[8] & 0x80);
    data_0209caa0[8] |= 0x80;   /* word 2 bit 7: the intro has played */
    _ZN5Stage18LoadClsnAndObjectsER11LVL_OverlayjR12MeshCollider(o, 0, mc);
    if (!intro_seen)
        data_0209caa0[8] &= ~0x80;

    /* RISK 1 RESOLVED (2026-08-03 pair A/B). The real SetFile leaves the
       collider's file<->world vectors at 1.0, and on the ROM that is right:
       its ITCM octree walk bakes the <<6 into its own vertex and origin
       loads. The port's transcription of that walk
       (port/unmatched/MeshCollider_DetectClsn_RaycastLine.cpp) instead
       consumes the pair as the conversion at its boundary, so with SetFile's
       stock 1.0 vectors every ground ray misses. Two lines, and they are the
       documented calling convention of our walk, not a fudge factor.
       Retires when the transcription grows the ROM's shifted loads. */
    port_clsn_pair_apply();
    return o;
}

// ---- Stage A2: the actor registry -------------------------------------------
//
// func_02043098 is the spawn spine, and it reads two things the host has to
// provide. data_020a4bb8[id] is the SpawnInfo: a factory function pointer at
// +0 that it CALLS, and behaviour/render priorities at +4/+6 that the
// ActorBase constructor reads back out of the same record. A null slot is a
// call through zero, so the table cannot simply be left empty.
//
// The gate is the pre-spawn hook at data_020a4b58. func_02043060 calls it
// with the actor id BEFORE the table is touched, and 3 means "abort, cleanly"
// -- an exercised ROM path, since LoadEntranceObjects already stores a
// possibly-null result. So the registry carries exactly the two classes this
// stage hosts and the hook turns everything else away.
//
// The SpawnInfo records are the ROM's own (Player_SpawnInfo from ov002,
// Camera_SpawnInfo from arm9), mounted for their priority bytes, with only
// the factory word repointed at the host.
extern "C" {
extern void **data_020a4bb8;          /* storage: hal/actor_vtables.cpp */
extern int data_020a4b58[4];          /* the pre-spawn hook slot
                                         (storage: hal/player_bridges.cpp) */
extern unsigned char Player_SpawnInfo[];      /* ov002 0x0210a704 */
extern unsigned char Camera_SpawnInfo[];      /* arm9  0x02086d78 */
void *_ZN6PlayerC3Ev(void);                   /* ov002 0x020e6c0c */
void *_ZN6CameraC1Ev(void *);                 /* arm9  0x0200e444 */
int hal_camera_check_layout(void);
void hal_fill_camera_vtable(void);
extern void *data_0209f318;
extern int data_0209f5c0[];
}

enum { ACTOR_PLAYER = 0xbf, ACTOR_CAMERA = 0x14c };

/* The two factories. Both ROM factories allocate their own object and ignore
   the argument register they were entered with, which is why they can be
   called through a no-argument pointer at all. */
static void *port_factory_player(void) { return _ZN6PlayerC3Ev(); }
static void *port_factory_camera(void) { return _ZN6CameraC1Ev(0); }

/* The pre-spawn hook. Everything the registry does not carry gets a clean
   abort and one line of output, so a level's unhosted classes are a list
   rather than a crash. */
extern "C" int port_prespawn_hook(void *idv)
{
    unsigned id = (unsigned)(size_t)idv;
    if (id == ACTOR_PLAYER || id == ACTOR_CAMERA)
        return 2;                     /* what a null hook returns: proceed */
    {
        static unsigned char said[512];
        if (id < sizeof said && !said[id]) {
            said[id] = 1;
            std::printf("  [spawn] actor 0x%x not registered, skipped\n", id);
        }
    }
    return 3;
}

/* ---- the scene root -------------------------------------------------------
   func_02042ffc refuses to spawn anything under a null parent, and the
   ActorBase constructor links the new actor's SceneNode (+0x14) under the
   parent's. data_0209f5c0 is that parent -- the Stage actor on the ROM, which
   the port does not build yet. What the spawn spine actually needs of it is a
   SceneNode, so the host root is an ActorBase-shaped block with a zeroed node
   whose actor back-pointer is itself, exactly what
   ActorBase::SceneNode::Reset + the ctor's `+0x24 = this` produce. */
static unsigned char hal_scene_root[0x40];

/* ---- the Player vtable ----------------------------------------------------
   Spawning through func_02043098 ends in func_020433b8 -> the init Process,
   which dispatches BeforeInitResources / InitResources / AfterInitResources
   through the object's vptr. The Player's vptr is data_ov002_0210a83c, real
   ov002 data carrying ov002 CODE addresses -- fine to mount, impossible to
   call. The slots the port can service are overwritten with host thunks in
   place (the ovdata contract: callers patch code pointers at runtime); the
   rest trap by name rather than jumping into the overlay image. */
extern "C" {
unsigned char data_ov002_0210a83c[];
int _ZN6Player13InitResourcesEv(void *self);
int _ZN5Actor19BeforeInitResourcesEv(void *self);
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned r);
int _ZN5Actor14BeforeBehaviorEv(void *self);
int hal_player_behavior(void *self);
int func_02043288(void *self);         /* port/unmatched: the behaviour Process */
}

/* Method faces for the init chain. Everything the spawn spine touches is
   reached by its Itanium name from a .c TU, i.e. cdecl, while these three
   definitions are real MSVC __thiscall methods -- a linker alias would hand
   the body an ecx that never held `this`. */
#include "ActorBase.h"
extern "C" int _ZN9ActorBase19BeforeInitResourcesEv(void *self)
{ return ((ActorBase *)self)->ActorBase::BeforeInitResources() ? 1 : 0; }


static int __fastcall ps_init(void *s, void *)
{
    /* Bit 7 of the save block's word 2 says the intro has played, and
       Player::InitResources reads it to decide whether to load the
       character's voice bank -- unhosted sound, the same engine the intro
       cutscene reaches. The boot needs the bit set (see port_stage_a_boot);
       the Player needs it clear. Scoped to the one call that cares. */
    unsigned char saved = data_0209caa0[8];
    data_0209caa0[8] &= ~0x80;
    int r = _ZN6Player13InitResourcesEv(s);
    data_0209caa0[8] = saved;
    return r;
}
static int __fastcall ps_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
static void __fastcall ps_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
static int __fastcall ps_behavior(void *s, void *)
{ return hal_player_behavior(s); }
/* Slots 7 and 8, read out of ov002's own _ZTV6Player at 0x0210a83c with its
   relocation table applied: 0x02010fd4 = Actor::BeforeBehavior and 0x02010fc8
   = Actor::AfterBehavior. The second is a `ldr ip,[pc]; bx ip' veneer onto
   ActorBase::AfterBehavior (0x02043af8), so the thunk calls the target
   directly -- a host forward through the veneer's own C face would drop the
   argument the ARM tail call rides through in r0/r1. */
static int __fastcall ps_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
static void __fastcall ps_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }

static const char *const hal_player_slot_name[20] = {
    "InitResources", "BeforeInitResources", "AfterInitResources",
    "CleanupResources", "BeforeCleanupResources", "AfterCleanupResources",
    "Behavior", "BeforeBehavior", "AfterBehavior",
    "Render", "BeforeRender", "AfterRender",
    "OnPendingDestroy", "Virtual34", "Virtual38", "OnHeapCreated",
    "~Player (D1)", "~Player (D0)", "OnYoshiTryEat", "OnTurnIntoEgg"};
static int hal_player_trap_slot;
static int __fastcall ps_trap(void *, void *)
{
    std::fprintf(stderr, "FATAL: Player vtable slot %d (%s) is not hosted\n",
                 hal_player_trap_slot,
                 hal_player_slot_name[hal_player_trap_slot & 19]);
    std::abort();
    return 0;
}

extern "C" void hal_fill_player_vtable(void)
{
    void **vt = (void **)data_ov002_0210a83c;
    for (int i = 0; i < 20; ++i)
        vt[i] = (void *)ps_trap;
    vt[0] = (void *)ps_init;
    vt[1] = (void *)ps_binit;
    vt[2] = (void *)ps_ainit;
    vt[6] = (void *)ps_behavior;
    vt[7] = (void *)ps_bbeh;
    vt[8] = (void *)ps_abeh;
    /* Slot 9 (Render) stays trapped: the harness renders the Player itself
       (hal_render_player_world), because Player::Render is the ROM's whole
       model/shadow/particle chain and only its body walk is hosted. */
}

/* The per-frame tick the ROM's processing list runs on every actor:
   func_02043288 = ActorBase::Process over slots 7/6/8. Actor::BeforeBehavior
   is the half the harness never had -- it is what copies pos into PREV POS,
   and prev pos is the start of every line WithMeshClsn's continuous update
   casts. Driving Behavior bare left prev at the constructor's zero, so the
   first frame at the gate swept a segment from the world origin. */
extern "C" int hal_player_process(void *self)
{ return func_02043288(self); }

/* ---- the entrance-driven boot ---------------------------------------------
   Seats everything LoadEntranceObjects reads, then runs the same boot with
   the Entrance table left switched on. The Player and the Camera come out of
   the entrance record: position, rotation, area, entrance id and entrance
   type, all of it the level's own. */
extern "C" {
extern unsigned char data_0209f21c;    /* controller count */
extern unsigned char data_0209f250;    /* local player index */
extern int data_0209fc5c[];            /* per-player "this slot is live" */
extern unsigned char data_02092128[];  /* per-player character */
extern signed char data_02092120;      /* currently shown area, -1 = none */
extern int data_0209f32c[];            /* water level */
extern int data_0209fc48;              /* the running cutscene, 0 = none */
extern int data_0209f20c[], data_0209f294[], data_0209f2c4[], data_0209b454[];
extern int data_0209ee90[];            /* +0x44 is the projection's W scale */
}

extern "C" void port_stage_a2_seat(void)
{
    /* the scene tree root the spawn spine links under */
    std::memset(hal_scene_root, 0, sizeof hal_scene_root);
    *(void **)(hal_scene_root + 0x24) = hal_scene_root;   /* node.actor */
    data_0209f5c0[0] = (int)(size_t)hal_scene_root;

    /* one local player, index 0, playing Mario, with the slot marked live so
       LoadEntranceObjects keeps the pointer it spawns */
    data_0209f21c = 1;
    data_0209f250 = 0;
    data_0209fc5c[0] = 1;
    data_02092128[0] = 0;
    /* data_0209caa0[0x41], which is byte 0xf of data_0209cad2 -- the third
       symbol of the run. Spelled at its owner rather than as an index past
       the first symbol's declared 0x14 bytes, which MSVC turns into a
       compile-time range check and a fast-fail. */
    if (data_0209cab4 - data_0209caa0 != 0x14 ||
        data_0209cad2 - data_0209caa0 != 0x32 ||
        data_0209cae4 - data_0209caa0 != 0x44)
        std::fprintf(stderr, "  [a2] SAVE BLOCK NOT CONTIGUOUS: +%d +%d +%d\n",
                     (int)(data_0209cab4 - data_0209caa0),
                     (int)(data_0209cad2 - data_0209caa0),
                     (int)(data_0209cae4 - data_0209caa0));
    data_0209cad2[0x41 - 0x32] = 0;

    /* Engine state the CAMERA's own boot reads, which under the entrance
       path runs inside LoadClsnAndObjects rather than after it. The harness
       used to stage this next to its hand-built camera; the same values, one
       step earlier. data_0209ee90[0x44/4] is the one that shows: it is the W
       scale Camera::Render hands PerspectiveW_, and at 0 the projection
       collapses and the frame comes out empty. */
    data_02092120 = -1;                 /* no area shown -> ChangeArea skips */
    data_0209f32c[0] = 0;               /* water level */
    data_0209fc48 = 0;                  /* not in a cutscene */
    data_0209f20c[0] = data_0209f294[0] = data_0209f2c4[0] = 0;
    data_0209b454[0] = 0;
    data_0209ee90[0x44 / 4] = 0x1000;

    /* the registry: two ids, ROM priorities, host factories */
    *(void **)(Player_SpawnInfo + 0) = (void *)port_factory_player;
    *(void **)(Camera_SpawnInfo + 0) = (void *)port_factory_camera;
    data_020a4bb8[ACTOR_PLAYER] = Player_SpawnInfo;
    data_020a4bb8[ACTOR_CAMERA] = Camera_SpawnInfo;
    data_020a4b58[0] = (int)(size_t)port_prespawn_hook;

    hal_fill_player_vtable();
    std::printf("[a2] registry: root %p, PLAYER %p, CAMERA %p\n",
                (void *)hal_scene_root, (void *)Player_SpawnInfo,
                (void *)Camera_SpawnInfo);
    if (!hal_camera_check_layout())
        std::fprintf(stderr, "  [cam] LAYOUT CHECK FAILED\n");
    hal_fill_camera_vtable();
}

/* ---- the path-binding guard ---------------------------------------------
   The player's path binding (+0x670) is the path id of the CLPS entry under
   his feet, and func_ov002_020c0108 reads the bound path's nodes into a
   THREE-element stack array. On the ROM that is safe by construction: of
   castle grounds' 22 CLPS entries only two name a path at all (5 and 3), and
   both of those paths have exactly two nodes. Every longer path in the level
   is for actors, which read them through PathPtr with their own storage.
   The port can reach the unsafe case, and not through the level data.
   WithMeshClsn's floor ClsnResult is only partly hosted, so when the ground
   tracking copies a record no walk filled, the path id reads 0 -- and path 0
   has seven nodes, which is 84 bytes into a 36-byte frame.
   Until the floor result is real, reject a binding the level cannot produce,
   say so once per id, and leave the genuine ones alone. */
extern "C" int port_stage_path_guard(void *player)
{
    char *c = (char *)player;
    unsigned id = *(unsigned *)(c + 0x670);
    if (id == 0xff)
        return 0;
    const unsigned char *tbl = (const unsigned char *)(size_t)data_020a0d84[0];
    int count = data_020a0d8c[0];
    if (tbl && (int)id < count && tbl[id * 6 + 2] <= 3)
        return 0;
    {
        static unsigned said;
        if (id < 32 && !(said & (1u << id))) {
            said |= 1u << id;
            std::fprintf(stderr, "  [path] binding %u rejected (%d nodes, the "
                         "tracking's floor record is not hosted yet)\n", id,
                         tbl && (int)id < count ? tbl[id * 6 + 2] : -1);
        }
    }
    *(unsigned *)(c + 0x670) = 0xff;
    return 1;
}

/* ---- probes --------------------------------------------------------------
   The boot is a pointer rewrite over Nintendo bytes followed by matched code
   walking it, so what matters is what the game ends up reading. */
void port_ov009_probe(void)
{
    const PortLvlOverlay *o = (const PortLvlOverlay *)port_ov009_mount();
    std::printf("[ov009] image %p .. %p (DS 0x%08x .. 0x%08x)\n",
                (void *)port_ov009_image,
                (void *)(port_ov009_image +
                         (port_ov009_ds_end - port_ov009_ds_base)),
                port_ov009_ds_base, port_ov009_ds_end);
    std::printf("[ov009] LVL_Overlay: clps %p objTable %p bmd %u kcl %u "
                "subTables %p subCount %u flags %02x\n",
                (void *)o->clps, (void *)o->objTable, o->bmdFileId,
                o->kclFileId, (void *)o->subTables, o->subCount, o->flags);
    unsigned n = *(const unsigned short *)o->objTable;
    const unsigned char *e = *(const unsigned char *const *)(o->objTable + 4);
    std::printf("[ov009] objTable: %u kinds at %p\n", n, (const void *)e);
    for (unsigned i = 0; i < n; ++i, e += 8)
        std::printf("        kind 0x%02x (grp %d idx %2d) count %3u entries %p\n",
                    e[0], (e[0] >> 5) & 7, e[0] & 0x1f, e[1],
                    *(const void *const *)(e + 4));
    for (unsigned s = 0; s < o->subCount; ++s) {
        const unsigned char *t =
            *(const unsigned char *const *)(o->subTables + s * 0xc);
        if (!t)
            continue;
        unsigned m = *(const unsigned short *)t;
        const unsigned char *se = *(const unsigned char *const *)(t + 4);
        std::printf("[ov009] sub[%u] table %p: %u kinds\n", s,
                    (const void *)t, m);
        for (unsigned i = 0; i < m; ++i, se += 8)
            std::printf("        kind 0x%02x (grp %d idx %2d) count %3u "
                        "entries %p\n", se[0], (se[0] >> 5) & 7, se[0] & 0x1f,
                        se[1], *(const void *const *)(se + 4));
    }
}

void _ZN7PathPtr6FromIDEj(void *self, unsigned id);
unsigned _ZNK7PathPtr8NumNodesEv(const void *self);
void _ZNK7PathPtr7GetNodeER7Vector3j(const void *self, int *out, unsigned idx);

void port_stage_a_probe(void *mc_)
{
    MeshCollider *mc = (MeshCollider *)mc_;
    const PortLvlOverlay *o = (const PortLvlOverlay *)port_ov009_mount();

    /* CLPS: "CLPS" magic, u16 entry size, u16 count, then the records --
       byte 0 the surface type, byte 4 the path id (0xff = none). */
    const unsigned char *clps = o->clps;
    unsigned esize = *(const unsigned short *)(clps + 4);
    unsigned ecount = *(const unsigned short *)(clps + 6);
    std::printf("[clsn] clps %p magic %.4s entrySize %u count %u\n",
                (const void *)clps, (const char *)clps, esize, ecount);
    for (unsigned i = 0; i < ecount; ++i) {
        const unsigned char *r = clps + 8 + i * esize;
        std::printf("        [%2u] type %02x path %02x  %02x %02x %02x %02x "
                    "%02x %02x\n", i, r[0], r[4], r[1], r[2], r[3], r[5],
                    r[6], r[7]);
    }

    /* The KCL the boot loaded, and the surface types the walk will resolve
       through the block above. The triangle array runs from tris up to the
       octree the header's fourth word points at. */
    {
        const KCL_File *f = mc->kclFile;
        long tricount = ((const char *)f->unk_0c - (const char *)f->tris) / 16;
        std::printf("[clsn] kclFile %p positions %p normals %p tris %p "
                    "octree %p (%ld triangles)\n",
                    (const void *)f, (const void *)f->positions,
                    (const void *)f->normals, (const void *)f->tris,
                    (const void *)f->unk_0c, tricount);
        int seen[256];
        std::memset(seen, 0, sizeof seen);
        int distinct = 0;
        if (tricount < 0 || tricount > 65536)
            tricount = 256;
        for (long t = 1; t <= tricount; ++t) {
            unsigned a = f->tris[t].attribute & 0xff;
            if (a < 256 && !seen[a]) { seen[a] = 1; ++distinct; }
        }
        std::printf("[clsn] %ld triangles: %d distinct surface types (",
                    tricount, distinct);
        for (int a = 0; a < 256; ++a)
            if (seen[a]) std::printf(" %d", a);
        std::printf(" )\n");
        std::printf("[clsn] world Y bounds: min %d (%.1f) max %d (%.1f)\n",
                    data_02092138, data_02092138 / 4096.0f,
                    data_0209212c, data_0209212c / 4096.0f);
        std::printf("[clsn] collider pair: 0x%x / 0x%x\n",
                    *(int *)((char *)mc_ + 0x2c), *(int *)((char *)mc_ + 0x38));
    }

    /* Paths: the table the two CLPS entries 16/17 bind to. */
    std::printf("[path] table %p count %d nodes %p\n",
                (void *)(size_t)data_020a0d84[0], data_020a0d8c[0],
                (void *)(size_t)data_020a0d88[0]);
    for (unsigned id = 0; id < 2; ++id) {
        static const unsigned probe_ids[2] = {5, 3};
        int path[2] = {0, 0};
        _ZN7PathPtr6FromIDEj(path, probe_ids[id]);
        unsigned nodes = _ZNK7PathPtr8NumNodesEv(path);
        std::printf("[path] FromID(%u) -> rec %p firstNode %u count %u\n",
                    probe_ids[id], (void *)(size_t)path[0],
                    *(unsigned short *)(size_t)path[0], nodes);
        for (unsigned k = 0; k < nodes && k < 4; ++k) {
            int v[3];
            _ZNK7PathPtr7GetNodeER7Vector3j(path, v, k);
            std::printf("        node %u = (%.0f, %.0f, %.0f)\n", k,
                        v[0] / 4096.0f, v[1] / 4096.0f, v[2] / 4096.0f);
        }
    }
}
}  /* extern "C" */
