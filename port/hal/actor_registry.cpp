// The actor registry: which classes the port can build, and the gate that
// turns the rest away.
//
// ---- what the ROM reads ----------------------------------------------------
//
// func_02043098 is the spawn spine and it consults exactly two globals.
// data_020a4bb8[actorID] is a SpawnInfo*: a factory function pointer at +0
// that the spine CALLS, and two halfwords at +4/+6 that the ActorBase
// constructor reads back out of the same record and drops into the two
// processing-list nodes it just built. A null slot is a call through zero, so
// the table cannot be left empty for an id the level names.
//
// data_020a4b58 is the pre-spawn hook. func_02043060 calls it with the actor
// id BEFORE the table is touched and 3 means "abort, cleanly" -- an exercised
// ROM path, since LoadEntranceObjects already stores a possibly-null result.
// So the gate is: registered ids proceed, everything else is named once and
// skipped. A level's unhosted classes come out as a list rather than a crash.
//
// ---- the SpawnInfo records are Nintendo's ----------------------------------
//
// Every record here is the ROM's own, mounted through port/tools/ovdata.py,
// with ONLY the factory word repointed at the host. The two priority
// halfwords are what the ActorBase constructor re-reads, and inventing them
// would put the actor in the wrong place in both processing lists. The ROM
// also happens to store the actor's own id in the +4 halfword, which makes a
// free cross-check: if the record's id and the table's id disagree, the
// registry is pointing at the wrong bytes and says so.
//
// ---- the per-frame processing lists ---------------------------------------
//
// func_02044120 is the ROM's whole actor frame and it is five list walks:
//
//     phase 4  data_020a4ba8 -> func_020432e4   cleanup Process   (slots 3/4/5)
//     phase 2  data_020a4b88 -> func_0204335c   init Process      (slots 0/1/2)
//     phase 3  data_020a4b78 -> func_02043288   behaviour Process (slots 6/7/8)
//     phase 5  data_020a4b98 -> func_0204322c   render Process    (slots 9/10/11)
//     phase 1  data_020a4b6c -> func_02043880   scene-tree housekeeping
//
// read straight out of the five PMF pairs __sinit_02075154 copies into the
// list heads (arm9 0x02099f48/50/60/68/70; each is {function, 0}, a plain
// nonvirtual pointer-to-member). The port seats the same five functions in
// the same slots and walks the lists with host copies of func_02043fdc /
// func_020441cc, because MSVC has no representation for an mwcc PMF -- the
// same treatment func_0204335c and func_02043288 already have.
//
// The frame is SPLIT here rather than driven through one func_02044120 call
// for one reason: the render pass has to run inside the host's render frame,
// after gx_reset and the camera push. Phases 4/2/3 are the tick, phase 5 is
// the render bucket, phase 1 closes the frame. Same functions, same order.
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

/* storage: hal/actor_vtables.cpp (actorID -> SpawnInfo*) */
extern void **data_020a4bb8;
/* storage: hal/player_bridges.cpp (the pre-spawn hook slot) */
extern int data_020a4b58[4];

/* the five processing lists. data_020a4b6c is the scene tree (three words:
   head, callback pair); the other four are {head, tail, callback pair}. */
extern int data_020a4b6c[8];
extern int data_020a4b78[8];
extern int data_020a4b88[8];
extern int data_020a4b98[8];
extern int data_020a4ba8[8];
extern int data_02099f24[4];       /* the current phase */

/* the four Process wrappers (host copies in port/unmatched/) and the
   scene-tree housekeeping pass (matched src) */
int func_0204335c(void *self);
int func_02043288(void *self);
int func_0204322c(void *self);
int func_020432e4(void *self);
int func_02043880(void *self);
void *func_02043fdc(void *list);
void *func_020441cc(void *list);

}  /* extern "C" */

// ---- the class table -------------------------------------------------------
//
// One row per class the port can build. `info` is the mounted ROM SpawnInfo,
// `factory` the matched <Class>_Spawn compiled for the host, and `fill` the
// runtime vtable fill (the vtable law: MSVC slot order, host __fastcall
// thunks, everything else trapped by name).
struct PortActorClass {
    unsigned short id;
    const char *name;
    unsigned char *info;
    void *(*factory)(void);
    void (*fill)(void);
};

extern "C" {
extern unsigned char Player_SpawnInfo[];      /* ov002 0x0210a704 */
extern unsigned char Camera_SpawnInfo[];      /* arm9  0x02086d78 */
void *_ZN6PlayerC3Ev(void);                   /* ov002 0x020e6c0c */
void *_ZN6CameraC1Ev(void *);                 /* arm9  0x0200e444 */
void hal_fill_player_vtable(void);
void hal_fill_camera_vtable(void);
}

/* Both ROM factories allocate their own object and ignore the argument
   register they were entered with, which is why they can be called through a
   no-argument pointer at all. */
static void *port_factory_player(void) { return _ZN6PlayerC3Ev(); }
static void *port_factory_camera(void) { return _ZN6CameraC1Ev(0); }
/* The bottom screen's classes put the CONSTRUCTOR in the SpawnInfo's +0 word
   rather than a separate Spawn veneer -- it allocates and returns the object
   itself, which is all the spine asks of a factory. */
extern "C" int *_ZN3HUDC1Ev(void);
static void *port_factory_hud(void) { return _ZN3HUDC1Ev(); }
extern "C" int *_ZN7MinimapC1Ev(void);
static void *port_factory_minimap(void) { return _ZN7MinimapC1Ev(); }

#include "actor_classes.inc"

static const PortActorClass port_actor_classes[] = {
    {0x0bf, "PLAYER", Player_SpawnInfo, port_factory_player,
     hal_fill_player_vtable},
    /* The camera's vtable fill is the gate-13 seam and runs from the same
       place the layout check does; the registry only needs its factory. */
    {0x14c, "CAMERA", Camera_SpawnInfo, port_factory_camera, 0},
    PORT_ACTOR_CLASS_ROWS
    {0, 0, 0, 0, 0},
};

// ---- the census ------------------------------------------------------------
//
// The boot is one pass over the level's own object tables, so what it spawned
// is a fact worth reading back rather than inferring. Two counters per id,
// filled by the gate itself.
enum { PORT_ACTOR_IDS = 512 };
static unsigned short g_spawned[PORT_ACTOR_IDS];
static unsigned short g_skipped[PORT_ACTOR_IDS];

extern "C" int port_prespawn_hook(void *idv)
{
    unsigned id = (unsigned)(size_t)idv;
    if (id < PORT_ACTOR_IDS && data_020a4bb8[id]) {
        ++g_spawned[id];
        return 2;                     /* what a null hook returns: proceed */
    }
    if (id < PORT_ACTOR_IDS) {
        if (!g_skipped[id])
            std::printf("  [spawn] actor 0x%x not registered, skipped\n", id);
        ++g_skipped[id];
    }
    return 3;
}

/* Name an actor id for a diagnostic; the trap in actor_classes.cpp is the
   customer. Never returns null so it can sit inside a printf. */
extern "C" const char *port_actor_class_name(unsigned id)
{
    for (const PortActorClass *k = port_actor_classes; k->name; ++k)
        if (k->id == id)
            return k->name;
    return "?";
}

extern "C" void port_actor_census(void)
{
    int spawned = 0, skipped = 0, kinds = 0, skipkinds = 0;
    for (int i = 0; i < PORT_ACTOR_IDS; ++i) {
        spawned += g_spawned[i];
        skipped += g_skipped[i];
        kinds += g_spawned[i] != 0;
        skipkinds += g_skipped[i] != 0;
    }
    std::printf("[census] %d spawned (%d classes), %d skipped (%d classes)\n",
                spawned, kinds, skipped, skipkinds);
    for (int i = 0; i < PORT_ACTOR_IDS; ++i)
        if (g_spawned[i]) {
            const char *nm = "?";
            for (const PortActorClass *k = port_actor_classes; k->name; ++k)
                if (k->id == i) nm = k->name;
            std::printf("         + %3d x%-3u %s\n", i, g_spawned[i], nm);
        }
    for (int i = 0; i < PORT_ACTOR_IDS; ++i)
        if (g_skipped[i])
            std::printf("         - %3d x%-3u\n", i, g_skipped[i]);
}

// ---- installation ----------------------------------------------------------
extern "C" void port_actor_registry_install(void)
{
    int n = 0;
    /* SM64DS_SKIP_CLASS=NAME[,NAME...] leaves a class unregistered, so the
       spine's own gate turns it away and the level boots without it. The
       cheapest way to ask "is this class the one breaking the run" -- it is
       how the HUD was pinned as the owner of a fault three phases downstream
       of it. Substring match, so SM64DS_SKIP_CLASS=HUD,BIRD works. */
    const char *skip = std::getenv("SM64DS_SKIP_CLASS");
    for (const PortActorClass *k = port_actor_classes; k->name; ++k) {
        if (skip && std::strstr(skip, k->name)) {
            std::printf("  [reg] %s skipped by SM64DS_SKIP_CLASS\n", k->name);
            continue;
        }
        if (!k->info || !k->factory) {
            std::fprintf(stderr, "  [reg] %s has no SpawnInfo or factory\n",
                         k->name);
            continue;
        }
        /* The ROM stores the actor's own id in the record's +4 halfword. A
           mismatch means the mount is pointing at the wrong bytes, which
           would otherwise show up as two silently wrong list priorities. */
        {
            unsigned rec = *(unsigned short *)(k->info + 4);
            if (rec != k->id)
                std::fprintf(stderr, "  [reg] %s: SpawnInfo at %p says id %u, "
                             "registry says %u -- WRONG RECORD\n", k->name,
                             (void *)k->info, rec, k->id);
        }
        *(void **)(k->info + 0) = (void *)k->factory;
        data_020a4bb8[k->id] = k->info;
        if (k->fill) k->fill();
        ++n;
    }
    data_020a4b58[0] = (int)(size_t)port_prespawn_hook;
    std::printf("[reg] %d actor classes registered, the gate is armed\n", n);
}

// ---- the frame -------------------------------------------------------------
//
// Seat the five list callbacks the way __sinit_02075154 does, with the host
// copies of the Process wrappers in place of the ROM's PMFs. The list heads
// are the port's own zeroed storage, so the two words the ROM's sinit copies
// out of data_02099f48..70 are written directly here.
typedef int (*PortListFn)(void *);

extern "C" void port_actor_lists_seat(void)
{
    /* {head, callback, 0} -- the scene tree, walked by func_020441cc */
    data_020a4b6c[1] = (int)(size_t)(PortListFn)func_02043880;
    data_020a4b6c[2] = 0;
    /* {head, tail, callback, 0} -- the four walked by func_02043fdc */
    data_020a4b88[2] = (int)(size_t)(PortListFn)func_0204335c;
    data_020a4b88[3] = 0;
    data_020a4b78[2] = (int)(size_t)(PortListFn)func_02043288;
    data_020a4b78[3] = 0;
    data_020a4b98[2] = (int)(size_t)(PortListFn)func_0204322c;
    data_020a4b98[3] = 0;
    data_020a4ba8[2] = (int)(size_t)(PortListFn)func_020432e4;
    data_020a4ba8[3] = 0;
}

/* Phases 4, 2 and 3 of func_02044120: cleanup, the init pass for anything
   spawned since the last frame, then behaviour. */
/* SM64DS_TRACE_LISTS=1: name every node on a list before it is walked --
   actor id, alive state, kill flag and vtable. The only window into a frame
   that is otherwise entirely matched code walking Nintendo's own structures. */
static void port_list_trace(const char *name, int *list)
{
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_TRACE_LISTS") != 0;
    if (!on)
        return;
    std::printf("  [list] %s head %08x:", name, list[0]);
    for (int *n = (int *)(size_t)list[0]; n; n = (int *)(size_t)n[1]) {
        char *o = (char *)(size_t)n[2];
        std::printf(" {node %p actor %p id %u alive %u kill %u vt %p}", (void *)n,
                    (void *)o, o ? *(unsigned short *)(o + 0xc) : 0u,
                    o ? *(unsigned char *)(o + 0xe) : 0u,
                    o ? *(unsigned char *)(o + 0xf) : 0u,
                    o ? *(void **)o : (void *)0);
    }
    std::printf("\n");
}

/* GATE 33, AND IT NEEDS RECONCILING AT MERGE. The one call site of the debug
   spawn hook in hal/actor_classes_bob_world.cpp: SM64DS_SPAWN_ACTOR spawns a
   named actor id at the player's position on the first tick after the boot,
   which is how the classes of a level the port cannot boot yet get exercised.
   A no-op with the variable unset, and port-beta-lvl's general hook replaces
   it whole. */
extern "C" void port_bob_debug_spawn(void);
extern "C" void port_bob_debug_watch(void);

extern "C" void port_actor_tick(void)
{
    port_bob_debug_spawn();
    port_bob_debug_watch();
    data_02099f24[0] = 4;
    port_list_trace("cleanup", data_020a4ba8);
    func_02043fdc(data_020a4ba8);
    data_02099f24[0] = 2;
    port_list_trace("pending", data_020a4b88);
    func_02043fdc(data_020a4b88);
    data_02099f24[0] = 3;
    port_list_trace("behaviour", data_020a4b78);
    func_02043fdc(data_020a4b78);
    data_02099f24[0] = 0;
}

/* Phase 5: the render bucket, in render-priority order. Runs inside the
   host's render frame rather than with the rest of func_02044120. Nothing
   converts units here any more: the whole frame is scene units, which is
   what an actor's own Render writes. */
extern "C" void port_actor_render(void)
{
    data_02099f24[0] = 5;
    func_02043fdc(data_020a4b98);
    data_02099f24[0] = 0;
}

/* Phase 1: the scene tree. Priority re-sorts, parent flag propagation and the
   deferred list insertions -- the housekeeping that closes the ROM's frame. */
extern "C" void port_actor_scene_pass(void)
{
    data_02099f24[0] = 1;
    func_020441cc(data_020a4b6c);
    data_02099f24[0] = 0;
}

/* How many actors are currently on each list -- the read-back that says the
   spawn spine really linked them -- plus the AREA table, because an actor
   whose area is not showing has Actor::BeforeBehavior set its 0x38 flags and
   Actor::BeforeRender refuses to draw it. Every object in a sub-table carries
   the sub-table's index as its area id (LoadClsnAndObjects passes it as the
   loader's second argument; only the main table's objects get -1, which means
   "not area-bound"), so on the castle grounds that is area 0. */
extern "C" {
extern signed char data_02092120;         /* the area currently shown */
unsigned char IsAreaShowing(int idx);
}

extern "C" void port_actor_positions(void);
extern "C" void port_actor_collision_probe(void);

extern "C" void port_actor_lists_probe(void)
{
    std::printf("[area] shown %d, showing:", data_02092120);
    for (int i = 0; i < 4; ++i)
        std::printf(" [%d]=%u", i, IsAreaShowing(i));
    std::printf("\n");
    static const struct { const char *n; int *l; int step; } lists[] = {
        {"behaviour", data_020a4b78, 1}, {"render", data_020a4b98, 1},
        {"pending", data_020a4b88, 1}, {"cleanup", data_020a4ba8, 1},
    };
    for (int i = 0; i < 4; ++i) {
        int n = 0;
        for (int *node = (int *)(size_t)lists[i].l[0]; node && n < 4096;
             node = (int *)(size_t)node[1])
            ++n;
        std::printf("[lists] %-9s %d\n", lists[i].n, n);
    }
    port_actor_collision_probe();
    port_actor_positions();
}

/* THE COLLISION REGISTRY, which is what every Player probe walks.
   MeshColliderBase::Enable parks a collider in the first free slot of
   data_020a0c80[24]; RaycastGround::DetectClsn then walks all 24 and, for
   every slot past 0, applies a DISTANCE CULL before it will even ask the
   collider: when the owner actor's +0xb0 carries bit 1, the ray is skipped
   unless it is within (owner+0xb8 << 3) of the owner's position and no lower
   than owner.y + owner+0xb4 minus the same. owner+0xb8 is the Clipper's own
   cull radius in SCENE units, which is why the shift is there.
   So a collider that reads right in every other way can still be invisible to
   a probe standing on top of it, and this is the line that says so. */
extern "C" { extern void *data_020a0c80[24]; }

/* A registered collider's own KCL, in the world units its owner's matrix puts
   it in: MeshCollider+0x20 is the file, its vertex array runs up to the normal
   table, and every position is stored at 1/64 of a Fix12i (the <<6 the ITCM
   walk does on read). For a MovingMeshCollider the matrix translation is the
   owner's position, which the line above already prints. A probe that finds no
   water when it is standing in the moat is answered here: either the mesh is
   somewhere else, or it is exactly where it should be and the walk is the
   problem. */
static void port_clsn_kcl_bounds(const char *o)
{
    const char *const *f = (const char *const *)(o + 0x20);
    if (!*f) { std::printf("[clsnreg]      (no KCL)\n"); return; }
    const int (*pos)[3] = *(const int (**)[3])(*f + 0x00);
    const char *norm = *(const char *const *)(*f + 0x04);
    long n = (norm - (const char *)pos) / 12;
    if (n <= 0 || n > 65536) { std::printf("[clsnreg]      (KCL %ld?)\n", n);
                               return; }
    int lo[3] = {1 << 30, 1 << 30, 1 << 30};
    int hi[3] = {-(1 << 30), -(1 << 30), -(1 << 30)};
    for (long i = 0; i < n; ++i)
        for (int k = 0; k < 3; ++k) {
            int v = pos[i][k] << 6;
            if (v < lo[k]) lo[k] = v;
            if (v > hi[k]) hi[k] = v;
        }
    std::printf("[clsnreg]      KCL %ld verts, local world bounds "
                "x[%.0f..%.0f] y[%.0f..%.0f] z[%.0f..%.0f]\n", n,
                lo[0] / 4096.0, hi[0] / 4096.0, lo[1] / 4096.0, hi[1] / 4096.0,
                lo[2] / 4096.0, hi[2] / 4096.0);
}

extern "C" void port_actor_collision_probe(void)
{
    std::printf("[clsnreg] slot owner        id   pos                 "
                "flags    cull(+0xb8) yoff(+0xb4) +0x0c\n");
    for (int i = 0; i < 24; ++i) {
        const char *o = (const char *)data_020a0c80[i];
        if (!o) continue;
        const char *a = *(const char *const *)(o + 4);
        if (!a) {
            std::printf("[clsnreg] %2d   (level, owner NULL)\n", i);
            continue;
        }
        std::printf("[clsnreg] %2d   %p %4u (%6d,%6d,%6d) %08x %6d %6d %8d\n",
                    i, (const void *)a, *(const unsigned short *)(a + 0xc),
                    *(const int *)(a + 0x5c) >> 12,
                    *(const int *)(a + 0x60) >> 12,
                    *(const int *)(a + 0x64) >> 12,
                    *(const unsigned *)(a + 0xb0), *(const int *)(a + 0xb8),
                    *(const int *)(a + 0xb4), *(const int *)(o + 0x0c));
        port_clsn_kcl_bounds(o);
    }
}

/* Every live actor's own position, read back off the behaviour list in world
   units and grouped by class. The level's object tables are the reference:
   an actor that spawned but reads back somewhere else has had its record
   misread, and a class whose count here is short of the census has destroyed
   instances (which for the Tree is the ROM's own design -- see the gate-16
   commit). Actor pos is the Vector3 at +0x5c; the class is the id at +0xc,
   which the ActorBase constructor copied out of the spawn context. */
/* The global water level, which is not a harness number any more: since
   gate 17 CASTLE_WATER's own InitResources clamps data_0209f32c to its own Y
   minus 100 units, and the Player's state machine reads that word to decide
   it is swimming. */
extern "C" { extern int data_0209f32c[]; }

extern "C" void port_actor_positions(void)
{
    std::printf("[water] level %d (%.1f units)\n", data_0209f32c[0],
                data_0209f32c[0] / 4096.0f);
    for (const PortActorClass *k = port_actor_classes; k->name; ++k) {
        int n = 0;
        for (int *node = (int *)(size_t)data_020a4b78[0]; node && n < 4096;
             node = (int *)(size_t)node[1]) {
            char *o = (char *)(size_t)node[2];
            if (!o || *(unsigned short *)(o + 0xc) != k->id)
                continue;
            if (!n++)
                std::printf("[pos] %-17s", k->name);
            const int *p = (const int *)(o + 0x5c);
            std::printf(" (%d,%d,%d)", p[0] >> 12, p[1] >> 12, p[2] >> 12);
        }
        if (n)
            std::printf("  [%d]\n", n);
    }
}
