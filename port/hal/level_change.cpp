// Gate 31: the level HANDOFF -- choosing a level, leaving the one you are in,
// and the teardown in between.
//
// ---- what was already here, and what was missing ---------------------------
//
// The whole game-side handoff already links. LoadLevel, LoadLevelNoReturn,
// SetNextLevel, ExitLevel, KillPlayer, HitDeathPlane and
// StartExitCharacterWipe are all matched src in slice_gate10.txt, and the
// fader wipes they end on are staged in hal/fader_wipes.cpp. Calling
// ExitLevel() in the port has therefore always "worked": it writes
//
//     data_02092110 = 1        the next level  (castle grounds)
//     data_0209f268 = 0xd      the next entrance
//
// and snaps a wipe. What was missing is the OTHER HALF. On the ROM those two
// words are read back by Stage::InitResources, which is the level boot; the
// port boots a level through port_stage_a_boot in hal/level_boot.cpp instead,
// and nothing anywhere read data_02092110. Every level change in the game
// wrote its request into a word no one was listening to.
//
// This file is the listener. It polls the request, tears the current level
// down, latches the request the same four lines Stage::InitResources latches
// it with, and boots the new level.
//
// ---- the seam the level-boot stream meets ----------------------------------
//
// A level id becomes an LVL_Overlay through port_level_overlay(). The ROM's
// own answer is one array index:
//
//     data_02092208[level]     -> the LVL_Overlay inside the level overlay
//     data_020758c8[level]     -> which overlay that is (it is level + 8)
//
// Both are arm9 .data and both are dumped by tools/romdata.py, so the table
// is Nintendo's rather than a guess. What a host cannot use directly is the
// VALUE: every level overlay is linked over the same 0x0211xxxx window (only
// one is resident at a time on the DS), so data_02092208[2] and
// data_02092208[1] are addresses in two different files that happen to
// overlap. Turning one into host bytes is a mount, and a mount is per-level
// work: ovdata --whole for that overlay, its relocations applied, its own
// static initialisers run.
//
// So the seam is a REGISTRY, not a table lookup:
//
//     port_level_mount_register(level, fn)   the mount stream registers
//     port_level_overlay(level)              the handoff calls
//
// `fn` returns the host address of that level's LVL_Overlay and must be
// idempotent -- the handoff calls it on every entry to the level, and a
// second call must not re-apply the overlay's relocations. hal/level_boot.cpp
// registers every row of its own level table through port_level_mounts_install,
// each against a thunk over a per-level mount cache, which is what gives that
// guarantee even when a session goes 1 -> 6 -> 1. A level not in that table
// answers "not mounted in this build" and the handoff declines the change with
// a message instead of booting into null.
//
// port_level_ds_overlay() is there so a mount can check itself: it returns
// the DS address the ROM's own table holds for that level, which is what the
// mount's LVL_Overlay symbol has to be.
//
// ---- teardown --------------------------------------------------------------
//
// The teardown is the ROM's per-actor path, driven from the port's side.
// ActorBase::MarkForDestruction on every live actor, then the cleanup phase
// pumped until the lists drain: each actor's own CleanupResources runs, its
// dedicated heap is destroyed, its slot-16 destructor runs and the object
// goes back to the game heap (port/unmatched/ActorBase_AfterCleanupResources
// .cpp is that path, hosted). Nothing here frees an actor by hand.
//
// What this file DOES own is the host storage the game's teardown cannot know
// about, because the game never allocated it: the per-level statics in
// hal/level_boot.cpp (the LoadFile handle table, the entrance record cache)
// and the engine globals the port's boot seats by hand. port_level_reset_host
// is that half, and hal/level_boot.cpp implements it next to the statics it
// clears.
//
// NOT torn down, deliberately: the Stage. On the ROM the Stage actor is
// respawned per level (scene 3), because on the ROM the level boot IS
// Stage::InitResources. The port's boot is port_stage_a_boot, a hand-written
// subset of it, and Stage::CleanupResources is the symmetric undo of the
// FULL one -- it releases twelve SharedFilePtrs that only Stage::InitResources
// fills, unloads archives the port never loaded, and calls
// Sound::ResetPlayerVoiceGroup. Running it against the port's boot would tear
// down things that were never built. So the Stage object persists and its two
// level-owned sub-objects are re-seated in place, which is what
// port_level_stage_reseat does. When the boot becomes Stage::InitResources
// for real, this is the piece that goes away.
//
// WHAT THE HANDOFF NOW GETS RIGHT that it did not: the boot it drives mounts
// the level it warped TO. The latch writes the new level into data_0209f2f8;
// this file hands the same id to port_level_set_target before port_stage_a_boot,
// so the boot's overlay mount and its sound-row seat resolve to the warped-to
// level. Before that the boot resolved through the SM64DS_LEVEL-cached desc and
// re-booted the castle grounds -- the census came back the castle's, doubled,
// and this line said "level 1 up" after a select of level 6.
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

/* ---- the ROM's request words ---------------------------------------------
   data_02092110 is arm9 .data and the ROM ships it as -1; romdata.py carries
   it, so the port starts with "no level pending" for the same reason the DS
   does. */
extern signed char data_02092110;    /* next level, -1 = nothing pending */
extern unsigned char data_0209f268;  /* next entrance */
extern unsigned char data_0209f1f0;  /* next star */
extern signed char data_0209f2f8;    /* current level */
extern int data_0209f264[];          /* current entrance */
extern int data_0209f220[];          /* current star */
extern unsigned char data_0209f26c;  /* why we are entering (1 fresh, 2 death) */

/* the ROM tables, from romdata.py */
extern unsigned char data_02092208[];   /* level -> LVL_Overlay (DS address) */
extern unsigned char data_020758c8[];   /* level -> overlay id */

/* the engine pieces the change drives */
void _ZN9ActorBase18MarkForDestructionEv(void *self);
void port_actor_tick(void);
void port_actor_scene_pass(void);
void *port_stage_object(void);
void *port_stage_a_boot(void *mc, int spawn);
void port_level_reset_host(void);        /* hal/level_boot.cpp */
void port_level_set_target(int level);   /* hal/level_boot.cpp: which level the
                                            next port_stage_a_boot mounts */
void CleanCommonModelDataArr(void);
void port_model_vram_reset(void);   /* hal/model_host.cpp */
void port_level_stage_reseat(void *stage);
unsigned _ZN22ExpandingHeapAllocator10MemoryLeftEv(void *self);
extern void *data_020a0eac;              /* Memory::gameHeapPtr */

extern int data_020a4b6c[];   /* scene tree     {head, cb, 0} */
extern int data_020a4b78[];   /* behaviour list {head, tail, cb, 0} */
extern int data_020a4b88[];   /* pending list */
extern int data_020a4b98[];   /* render list */
extern int data_020a4ba8[];   /* cleanup list */

}  /* extern "C" */

// ---- deleting a model-family object without the ambiguous slot -----------
//
// THE PROBLEM. mwcc emits two destructor slots (D1 at 0, D0 at 1) where MSVC
// emits one, so every synthetic vtable array in this file is one slot short of
// the ROM's from DoSetFile onward. A TU that dispatches through a LOCAL SHADOW
// CLASS counts ROM slots; a TU compiled against Model.h counts MSVC's. Render
// happens to be servable both ways (slot 5 is double-filled above) and the
// rest are not: ROM slot 1, the DELETING destructor, is MSVC slot 1, DoSetFile.
//
// Two matched TUs delete a model that way -- Player::CleanupResources and
// daDoor_c::CleanupResources, both of them `obj->v1()` on a two-virtual shadow
// -- and both landed on Model::DoSetFile with no arguments and faulted inside
// Model::AddToCommonModelDataArr. Neither was reachable until the port had a
// level teardown to run them (gate 31).
//
// THE FIX is to spell the delete rather than dispatch it, and to pick the body
// by the object's OWN vtable pointer rather than by what the call site thinks
// it holds. The three model classes are distinguishable that way and nothing
// else is guessed: an object carrying a fourth vptr aborts by name instead of
// running the wrong destructor over it.
extern "C" {
extern void *_ZTV5Model[8];
extern void *_ZTV9ModelAnim[10];
extern void *_ZTV10ModelAnim2[12];
void *_ZN5ModelD1Ev(void *self);
void *_ZN9ModelAnimD1Ev(void *self);
void *_ZN10ModelAnim2D1Ev(void *self);
void _ZdlPv(void *p);

void port_model_family_delete(void *obj)
{
    if (!obj)
        return;
    void *vt = *(void **)obj;
    if (vt == (void *)_ZTV5Model)
        _ZN5ModelD1Ev(obj);
    else if (vt == (void *)_ZTV9ModelAnim)
        _ZN9ModelAnimD1Ev(obj);
    else if (vt == (void *)_ZTV10ModelAnim2)
        _ZN10ModelAnim2D1Ev(obj);
    else {
        fprintf(stderr, "FATAL: port_model_family_delete: %p carries vtable "
                "%p, which is not Model, ModelAnim or ModelAnim2\n", obj, vt);
        abort();
    }
    _ZdlPv(obj);
}
}

// It lives here rather than beside the vtables it reads because the three
// destructors it names are in gate slices only the level-carrying targets
// link; in hal/cxxname_bridge.cpp it broke smoke_actor's link.

/* ---- the mount registry ---------------------------------------------------
   Fifty-two levels; the array is the ROM's own count so a registration for a
   level the ROM does not have is caught rather than stored. */
enum { PORT_LEVEL_COUNT = 52 };

typedef void *(*PortLevelMount)(void);

static PortLevelMount g_mount[PORT_LEVEL_COUNT];

extern "C" int port_level_mount_register(int level, PortLevelMount fn)
{
    if (level < 0 || level >= PORT_LEVEL_COUNT || !fn) {
        std::fprintf(stderr, "  [lvl] REGISTER REFUSED: level %d is outside "
                     "the ROM's 0..%d\n", level, PORT_LEVEL_COUNT - 1);
        return 0;
    }
    g_mount[level] = fn;
    return 1;
}

extern "C" unsigned port_level_ds_overlay(int level)
{
    if (level < 0 || level >= PORT_LEVEL_COUNT)
        return 0;
    return *(const unsigned *)(data_02092208 + level * 4);
}

extern "C" int port_level_overlay_id(int level)
{
    if (level < 0 || level >= PORT_LEVEL_COUNT)
        return -1;
    return *(const int *)(data_020758c8 + level * 4);
}

extern "C" void *port_level_overlay(int level)
{
    if (level < 0 || level >= PORT_LEVEL_COUNT || !g_mount[level])
        return 0;
    return g_mount[level]();
}

extern "C" int port_level_is_mounted(int level)
{
    return level >= 0 && level < PORT_LEVEL_COUNT && g_mount[level] != 0;
}

/* ---- what the game is asking for ------------------------------------------ */

extern "C" int port_level_change_pending(void)
{
    return data_02092110 >= 0;
}

/* ---- teardown -------------------------------------------------------------
   Two passes matter and the loop bounds both. Marking an actor does not
   remove it: the phase-1 scene pass (func_02043880) is what moves a marked
   actor onto the cleanup list, and the phase-4 cleanup walk is what runs its
   Process. An actor whose own CleanupResources spawns or marks something --
   the castle grounds' trees mark their cylinder owners -- needs another
   round, so this alternates the two until the lists are empty or the budget
   runs out. Sixteen is far past what the castle grounds needs (it settles in
   three) and small enough that a teardown that cannot converge says so
   instead of hanging. */
static int port_level_live_count(void)
{
    int n = 0;
    void *stage = port_stage_object();
    for (int *node = (int *)(size_t)data_020a4b78[0]; node && n < 8192;
         node = (int *)(size_t)node[1])
        if (node[2] && (void *)(size_t)node[2] != stage)
            ++n;
    for (int *node = (int *)(size_t)data_020a4b88[0]; node && n < 8192;
         node = (int *)(size_t)node[1])
        if (node[2] && (void *)(size_t)node[2] != stage)
            ++n;
    return n;
}

/* The census the level cycle reports: how many actors are alive that are not
   the Stage. Two runs of the same cycle have to produce the same number, and
   an actor that survives a teardown shows up here as the count going up. */
extern "C" int port_actor_live_count(void) { return port_level_live_count(); }

static int port_level_mark_all(void)
{
    int n = 0;
    void *stage = port_stage_object();
    /* Snapshot first. MarkForDestruction runs the actor's OnPendingDestroy,
       and an OnPendingDestroy is allowed to mark other actors -- which
       relinks nodes under the walk. */
    static void *victim[4096];
    int v = 0;
    for (int *node = (int *)(size_t)data_020a4b78[0]; node && v < 4096;
         node = (int *)(size_t)node[1])
        if (node[2] && (void *)(size_t)node[2] != stage)
            victim[v++] = (void *)(size_t)node[2];
    for (int *node = (int *)(size_t)data_020a4b88[0]; node && v < 4096;
         node = (int *)(size_t)node[1])
        if (node[2] && (void *)(size_t)node[2] != stage)
            victim[v++] = (void *)(size_t)node[2];
    for (int i = 0; i < v; ++i) {
        char *o = (char *)victim[i];
        if (*(unsigned char *)(o + 0xf))   /* already marked */
            continue;
        _ZN9ActorBase18MarkForDestructionEv(o);
        ++n;
    }
    return n;
}

extern "C" int port_level_teardown(void)
{
    const int trace = std::getenv("SM64DS_TRACE_LEVEL") != 0;
    int rounds = 0;
    for (; rounds < 16; ++rounds) {
        int marked = port_level_mark_all();
        /* phase 1 moves the marked onto the cleanup list, phase 4 runs it */
        port_actor_scene_pass();
        port_actor_tick();
        port_actor_scene_pass();
        int left = port_level_live_count();
        if (trace)
            std::printf("  [lvl] teardown round %d: marked %d, %d left\n",
                        rounds, marked, left);
        if (!left)
            break;
    }
    int left = port_level_live_count();
    if (left) {
        std::fprintf(stderr, "  [lvl] TEARDOWN DID NOT CONVERGE: %d actors "
                     "still live after %d rounds\n", left, rounds);
        return 0;
    }
    /* The four processing lists have to be genuinely empty, not just free of
       actors this walk could see. A stale head is a dangling node the next
       level's phase walk would step through. */
    struct { const char *name; int *list; } lists[4] = {
        {"behaviour", data_020a4b78}, {"pending", data_020a4b88},
        {"render", data_020a4b98}, {"cleanup", data_020a4ba8}};
    void *stage = port_stage_object();
    int bad = 0;
    for (int i = 0; i < 4; ++i) {
        for (int *n = (int *)(size_t)lists[i].list[0]; n;
             n = (int *)(size_t)n[1]) {
            if ((void *)(size_t)n[2] == stage)
                continue;
            std::fprintf(stderr, "  [lvl] %s LIST NOT EMPTY: node %p actor "
                         "%p\n", lists[i].name, (void *)n,
                         (void *)(size_t)n[2]);
            ++bad;
            break;
        }
    }
    return bad == 0;
}

/* ---- the change ----------------------------------------------------------- */

static unsigned port_level_heap_free(void)
{
    if (!data_020a0eac)
        return 0;
    /* Heap's first word is its allocator (src/_ZN4HeapC1EPvjP4Heap.c). */
    void *alloc = *(void **)((char *)data_020a0eac + 4);
    if (!alloc)
        return 0;
    return _ZN22ExpandingHeapAllocator10MemoryLeftEv(alloc);
}

extern "C" unsigned port_level_heap_free_bytes(void)
{ return port_level_heap_free(); }

/* Runs the four lines Stage::InitResources runs, in its order:
       prev      = data_0209f2f8
       current   = pending
       entrance  = next entrance
       star      = next star
   and then clears the request, which is Stage::InitResources' own last
   statement (data_02092110 = -1). Everything between those two in the ROM is
   the level boot itself. */
static void port_level_latch(void)
{
    data_0209f2f8 = data_02092110;
    data_0209f264[0] = data_0209f268;
    data_0209f220[0] = data_0209f1f0;
    data_02092110 = -1;
}

extern "C" int port_level_change_apply(void)
{
    if (data_02092110 < 0)
        return 0;

    const int want = data_02092110;
    const int from = data_0209f2f8;
    const unsigned free_before = port_level_heap_free();

    if (!port_level_is_mounted(want)) {
        std::fprintf(stderr,
                     "  [lvl] level %d (overlay %d, LVL_Overlay DS 0x%08x) is "
                     "NOT MOUNTED in this build -- the handoff is real, the "
                     "mount is the level-boot seam (port_level_mount_register)"
                     "\n", want, port_level_overlay_id(want),
                     port_level_ds_overlay(want));
        data_02092110 = -1;        /* consume it: a stuck request re-fires */
        return 0;
    }

    std::fprintf(stderr, "[lvl] change: level %d -> %d, entrance %u, reason %u\n",
                from, want, (unsigned)data_0209f268, (unsigned)data_0209f26c);

    if (!port_level_teardown()) {
        std::fprintf(stderr, "  [lvl] teardown failed; the change is "
                     "declined and the level stands\n");
        data_02092110 = -1;
        return 0;
    }

    /* THE COMMON-MODEL ARRAY, and it is the ROM's own line. Model::LoadFile
       registers every BMD it loads in data_0209cefc so that two actors asking
       for the same file share one parsed copy, and Stage::CleanupResources
       calls CleanCommonModelDataArr to empty it -- which is the half of the
       ROM's Stage teardown the port's boot really does owe, because the port
       DOES run the loader that fills it. Left behind, the array holds
       pointers to models the actors' own destructors have already freed, and
       the next level's first Model::LoadFile walks one: the second boot got
       as far as spawning the castle grounds' butterflies and then called
       through a freed entry. */
    CleanCommonModelDataArr();

    /* and the texture-VRAM cursors, which are the port's own expression of
       InitialiseVramGlobals -- the line Stage::InitResources opens a level
       boot with. Without it the second boot exhausts the arena and
       Model::GetVramOffset reaches the game's Crash(). */
    port_model_vram_reset();

    const unsigned free_torn = port_level_heap_free();
    port_level_reset_host();
    port_level_latch();
    /* Point the boot at the level the latch just made current. Without this the
       boot's mount resolved to the env-cached level and the warp re-booted the
       castle grounds -- the [lvl] line said "level 1 up" after a select of
       level 6, and the census came back the castle's, doubled. The latch put
       the new level in data_0209f2f8; hand the same id to the boot. */
    port_level_set_target((int)data_0209f2f8);

    void *stage = port_stage_object();
    if (!stage) {
        std::fprintf(stderr, "  [lvl] no Stage: the change needs the real "
                     "boot (SM64DS_LEGACY_BOOT is on)\n");
        return 0;
    }
    port_level_stage_reseat(stage);
    port_stage_a_boot((char *)stage + 0x91c, 1);

    const unsigned free_after = port_level_heap_free();
    std::fprintf(stderr, "[lvl] level %d up. heap free: %u before, %u torn down, %u "
                "after (net %+d)\n", (int)data_0209f2f8, free_before,
                free_torn, free_after, (int)free_after - (int)free_before);
    return 1;
}

/* The per-frame poll. Sits where Scene::SpawnIfNecessary sits in the ROM's
   own frame (func_020197b8 phase 3): after input, before the actor phases,
   so a level that comes up mid-frame gets its first tick from the same frame
   loop as any other. */
extern "C" int port_level_change_poll(void)
{
    if (data_02092110 < 0)
        return 0;
    return port_level_change_apply();
}

/* ---- the front door -------------------------------------------------------
   dScTitle_c's own selection table, and its own handoff call.
   func_ov003_020ad814 (the debug level select's Behavior) picks a row out of
   data_ov003_020b1180 -- 0x36 eight-byte rows, byte 0 the level id, byte 1
   the entrance -- and calls LoadLevelNoReturn(level, entrance, 1, 0). Two
   rows are sentinels: -1 means "back to the file select" and -2 "into the
   minigame menu", neither of which is a level.
   The port reads the SAME TABLE. What it does not run is the scene actor
   around it: dScTitle_c is an ov003 class and ov003 is not mounted, so the
   rendered grid, its cursor sprite and its music are not here. The row list
   and the call it ends in are Nintendo's; the presentation is the port's own
   menu (tests/walk_window.cpp). */
extern "C" {
extern signed char data_ov003_020b1180[];   /* romdata: the level-select rows */
void LoadLevelNoReturn(int level, unsigned entrance, unsigned star,
                       unsigned reason);
void SetPlayerGlobals(void);
void SetNumPlayers(unsigned n);
extern unsigned char data_0209f2d8;         /* game mode */
/* Scene::StartSceneFade is matched src (slice_gate10): it records the pending
   scene id in data_02092664 and writes the fade colour into data_0209f5e8+0xc.
   It does NOT put the fade in motion -- on the ROM the Scene actor's own
   BeforeBehavior does that when it sees the pending scene. */
void _ZN5Scene14StartSceneFadeEjjt(unsigned actorID, unsigned param,
                                   unsigned short fadeColor);
extern unsigned short data_02092664;         /* Scene::SetSceneToSpawn's id */
extern unsigned short data_0209f5e8[];        /* the color fader (its +0xc word) */
/* hal/fader_wipes.cpp: put the color fader in motion for the port's frame loop
   to step, since the port has no Scene actor to arm data_0209d4b0. */
void port_fader_start_color(int frames, int toEnd, unsigned short color);
}

/* ---- the scene-fade request (gate 31, deliverable 3) ----------------------
   Scene::StartSceneFade parks a pending scene id in data_02092664 and a colour
   in the fader. On the ROM the Scene actor consumes the pending id (spawns the
   scene) once the fade has covered the screen. The port has no scene spawner
   for ov003 scenes, so it records the request here and the frame loop acts on
   it: run the colour fade, and when a scene handler exists (dScStarSel_c, the
   stretch), hand off to it. Until then the request is recorded and reported so
   the flow is visible and the fade renders.

   ---- WHAT BOOTING dScStarSel_c NEEDS (the star select, scene 4) -----------
   The boot chain is fully mapped and none of it is a guess:

     data_02092664 = 4  (StartSceneFade set it)
       -> Scene::SpawnIfNecessary  calls func_02013edc(4, param, 1)
       -> func_02042fe4 -> func_02043098(4, 0, param, 1)   the spawn spine
       -> (*(Fn*)data_020a4bb8[4])()   the factory for scene id 4
       -> StarSelect_Spawn  (ov003, 0x020b04f0)

   StarSelect_Spawn (src/StarSelect_Spawn.cpp) is small and portable-shaped:
     - ActorBase::operator new(0x13c), ActorBase ctor
     - vptr = data_ov003_020b1704   (the dScStarSel_c vtable, IN ov003)
     - flags +0x13 |= 1|4
     - func_020733a8(self+0x64, 2, 0x50, Model::ctor, Model::dtor)  two Models

   The dScStarSel_c vtable (data_ov003_020b1704, from ov003 relocs) is:
     slot 0  0x020af8a0   (a method, ov003)
     slot 3  0x020af86c   CleanupResources           (ov003)
     slot 6  0x020af038   Behavior                   (ov003, NONMATCHING src)
     slot 7  0x0202e3d4   Scene::BeforeBehavior      (MAIN -- port HAS it)
     slot 8  0x0202e3c8   Scene::AfterBehavior       (MAIN -- port HAS it)
     slot 9  0x020ae6f4   Render (0x944 bytes of OAM) (ov003)
     slot 12 0x020ae6f0   OnPendingDestroy           (ov003)
   So the framework slots (Before/AfterBehavior) are already hosted; the
   scene-specific slots (Behavior, Render, CleanupResources, InitResources)
   are ov003 code.

   WHAT IS TOO DEEP for a first pass, and why the port records-and-reports
   rather than boots:
     1. The vtable and the four ov003 methods are not mounted -- ov003 is
        mounted for ONE data table (the row list), not its .text. Booting the
        scene means mounting ov003 code + relocs, the same per-overlay work a
        level mount is.
     2. dScStarSel_c::Render is 0x944 bytes of OAM::Render calls -- the star
        grid, the course thumbnails, the cursor -- built on the 2D sprite
        engine and the star/coin SAVE DATA. The port's OAM path exists (gate
        25) but the star-select graphics (its SpawnInfo-loaded 2D resources in
        InitResources) are not staged.
     3. dScStarSel_c::Behavior (0x020af038, above in this file's evidence) ends
        in StartSceneFade(3,0,0) -- the star select's OWN handoff back into the
        level (scene 3, the Stage boot). So the port's current path (title ->
        level, with the fade) already produces the END STATE the star select
        would hand to; what is missing is the intermediate star-choice UI, not
        a different level outcome.

   So the stretch is a real sub-project (mount ov003 .text, stage the star-grid
   2D resources, host the OAM render), landed here as analysis. The fade flow
   that would drive it is live: the request is recorded, the screen fades, and
   the frame loop is the seam a real StarSelect_Spawn registration would plug
   into (register a host factory at data_020a4bb8[4], then spawn on cover). */
static int g_scene_fade_scene = -1;   /* pending scene id, -1 = none */

extern "C" int port_scene_fade_pending(int *sceneId)
{
    if (g_scene_fade_scene < 0)
        return 0;
    if (sceneId) *sceneId = g_scene_fade_scene;
    return 1;
}

extern "C" void port_scene_fade_clear(void) { g_scene_fade_scene = -1; }

enum { PORT_TITLE_ROWS = 0x36 };

extern "C" int port_title_rows(void) { return PORT_TITLE_ROWS; }

extern "C" int port_title_row(int i, int *level, int *entrance)
{
    if (i < 0 || i >= PORT_TITLE_ROWS)
        return 0;
    const signed char *r = data_ov003_020b1180 + i * 8;
    if (level) *level = r[0];
    if (entrance) *entrance = (unsigned char)r[1];
    return r[0] >= 0;         /* -1 / -2 are the two scene sentinels */
}

/* The else-branch of func_ov003_020ad814, now in the ROM's OWN order.
   FaderColor is staged (hal/fader_wipes.cpp), so LoadLevel's opening
   Scene::SetAndStopColorFader call is safe and the mount check no longer has to
   come first to dodge a null fader slot. The ROM branch runs verbatim, then the
   port refuses an unmounted row AFTER it -- which is what that function's
   old comment promised staging the color fader would allow. */
extern "C" int port_title_select(int i)
{
    int level = 0, entrance = 0;
    if (!port_title_row(i, &level, &entrance)) {
        std::fprintf(stderr, "  [title] row %d is a scene sentinel (%d), not "
                     "a level\n", i, level);
        return 0;
    }

    /* dScTitle_c::Behavior's confirm branch, in order (func_ov003_020ad814):
           data_0209f2d8 = 0;                       single player
           LoadLevelNoReturn(level, entrance, 1, 0);
           SetPlayerGlobals();
           SetNumPlayers(1);
           Scene::StartSceneFade(4, 0, 0);          hand to the star select
           data_0209f5e8[6] = 0x7fff;               fade to WHITE
       LoadLevelNoReturn opens with SetAndStopColorFader (safe now: the color
       fader is a real object), so this runs whether or not the port can mount
       the row. */
    data_0209f2d8 = 0;                 /* single player */
    LoadLevelNoReturn(level, (unsigned)entrance, 1, 0);
    SetPlayerGlobals();
    SetNumPlayers(1);
    /* Scene::StartSceneFade(4, 0, 0): records scene 4 (dScStarSel_c) as the
       pending scene and sets the fade colour. data_0209f5e8[6] (+0xc) = 0x7fff
       is the ROM's own next line: fade to WHITE, not black. */
    _ZN5Scene14StartSceneFadeEjjt(4, 0, 0);
    data_0209f5e8[6] = 0x7fff;
    /* Record the scene request for the frame loop, and put the colour fade in
       motion so it renders. 0x7fff (nonzero) is a white fade; 16 frames is the
       DS default for a scene transition. */
    g_scene_fade_scene = (int)data_02092664;
    port_fader_start_color(16, 1, 0x7fff);
    std::fprintf(stderr, "[title] row %d -> level %d entrance %d, scene fade to "
                 "%d (white)\n", i, level, entrance, (int)data_02092664);

    /* NOW the not-mounted refusal, after the ROM's own order. The level request
       (data_02092110) that LoadLevelNoReturn wrote is consumed by the change
       poll, which already declines an unmounted level with a message; but say
       so here too, so a row that cannot boot is legible at the point of the
       choice rather than only when the poll fires. The fade still ran, which is
       the intended feedback that the button was seen. */
    if (!port_level_is_mounted(level)) {
        std::fprintf(stderr, "  [title] row %d is level %d (overlay %d), which "
                     "is not mounted in this build -- the fade ran, the change "
                     "poll will decline the boot\n", i, level,
                     port_level_overlay_id(level));
        /* leave data_02092110 for the poll to consume and report */
    }
    return 1;
}
