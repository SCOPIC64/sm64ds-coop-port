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

typedef void (*PortObjLoader)(void *, int, unsigned);
PortObjLoader data_ov002_0210cbb8[32] = {
    _Z19LoadStandardObjectsRN11LVL_Overlay11ObjSubTableEij,        /*  0 */
    _Z19LoadEntranceObjectsRN11LVL_Overlay11ObjSubTableEij,        /*  1 */
    _Z19LoadPathNodeObjectsRN11LVL_Overlay11ObjSubTableEij,        /*  2 */
    _Z15LoadPathObjectsRN11LVL_Overlay11ObjSubTableEij,            /*  3 */
    _Z15LoadViewObjectsRN11LVL_Overlay11ObjSubTableEij,            /*  4 */
    _Z17LoadSimpleObjectsRN11LVL_Overlay11ObjSubTableEij,          /*  5 */
    _Z25LoadTeleportSourceObjectsRN11LVL_Overlay11ObjSubTableEij,  /*  6 */
    _Z23LoadTeleportDestObjectsRN11LVL_Overlay11ObjSubTableEij,    /*  7 */
    _Z14LoadFogObjectsRN11LVL_Overlay11ObjSubTableEij,             /*  8 */
    _Z15LoadDoorObjectsRN11LVL_Overlay11ObjSubTableEij,            /*  9 */
    _Z15LoadExitObjectsRN11LVL_Overlay11ObjSubTableEij,            /* 10 */
    _Z22LoadMinimapTileObjectsRN11LVL_Overlay11ObjSubTableEij,     /* 11 */
    _Z23LoadMinimapScaleObjectsRN11LVL_Overlay11ObjSubTableEij,    /* 12 */
    _Z23LoadUnusedType13ObjectsRN11LVL_Overlay11ObjSubTableEij,    /* 13 */
    _Z21LoadStarCameraObjectsRN11LVL_Overlay11ObjSubTableEij,      /* 14 */
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
extern int data_0209caa0[];          /* the save block; [2] bit 7 = intro seen */

/* Loader indices, for the suppression masks. */
enum {
    LOADER_ENTRANCE = 1,
    LOADER_DOOR = 9,
    LOADER_EXIT = 10,
};

/* SM64DS_REAL_BOOT stage selector: 1 = A1 (geometry only). */
void *port_stage_a_boot(void *mc, int spawn_entrances)
{
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
    unsigned char intro_seen = (unsigned char)(data_0209caa0[2] >> 7) & 1;
    data_0209caa0[2] |= 0x80;
    _ZN5Stage18LoadClsnAndObjectsER11LVL_OverlayjR12MeshCollider(o, 0, mc);
    if (!intro_seen)
        data_0209caa0[2] &= ~0x80;

    /* RISK 1 RESOLVED (2026-08-03 pair A/B). The real SetFile leaves the
       collider's file<->world vectors at 1.0, and on the ROM that is right:
       its ITCM octree walk bakes the <<6 into its own vertex and origin
       loads. The port's transcription of that walk
       (port/unmatched/MeshCollider_DetectClsn_RaycastLine.cpp) instead
       consumes the pair as the conversion at its boundary, so with SetFile's
       stock 1.0 vectors every ground ray misses. Two lines, and they are the
       documented calling convention of our walk, not a fudge factor.
       Retires when the transcription grows the ROM's shifted loads. */
    *(int *)((char *)mc + 0x2c) = 0x40000;   /* file -> world, 64.0 */
    *(int *)((char *)mc + 0x38) = 0x40;      /* world -> file, 1/64 */
    return o;
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
