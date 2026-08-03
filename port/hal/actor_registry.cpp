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
    for (const PortActorClass *k = port_actor_classes; k->name; ++k) {
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

extern "C" void port_actor_tick(void)
{
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
   host's render frame rather than with the rest of func_02044120. */
/* Nonzero while list 5 is walking. hal/cxxname_bridge.cpp reads it at the
   Model render seam to convert each actor's ROM-convention (scene-unit) model
   matrix into the harness's world units for the duration of the bucket. */
extern "C" int port_actor_bucket_depth;   /* storage: hal/cxxname_bridge.cpp */

extern "C" void port_actor_render(void)
{
    data_02099f24[0] = 5;
    ++port_actor_bucket_depth;
    func_02043fdc(data_020a4b98);
    --port_actor_bucket_depth;
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
}
