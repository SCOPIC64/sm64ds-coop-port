// Disk save-state verification (lane lk7).
//
// This proves the save state survives the process ending. It reuses the actor
// world smoke_savestate builds (SetupRootHeap, spawn one real actor, tick it),
// then drives the DISK path in hal/lk7_persist.cpp across a real process
// boundary:
//
//   PHASE "save" (the parent):
//     1. boots the actor world and ticks it so real state lives in the arena;
//     2. saves the in-memory slot (lk6) and mirrors it to savestate.bin
//        (lk7_persist_write);
//     3. records the arena hash and prints it;
//     4. re-execs ITSELF with SM64DS_PERSIST_PHASE=load and the expected hash in
//        SM64DS_PERSIST_HASH, then returns the child's exit code.
//
//   PHASE "load" (the child, a genuinely separate process):
//     1. boots the SAME way but does NOT rebuild the evolved actor state;
//     2. calls lk7_persist_read, which validates the header, copies the arena
//        and globals into place and hands the world to lk6;
//     3. asserts the restored arena hash equals the parent's saved hash. A
//        second process loading the first one's disk state and landing on the
//        identical arena hash is the core proof.
//
// Two refusal cases run in-process in the parent after the child returns, so
// they need no second boot:
//   - a corrupted header (magic byte flipped) is refused cleanly, file left;
//   - a gittip mismatch (the build-tip field overwritten) is refused cleanly.
//
// The arena is pinned at a fixed host base (hal/os_arena.cpp), so both the
// parent and the child bring the arena up at the SAME base and the saved
// pointers relocate. If that pin ever fails, lk7_persist_available returns 0 and
// this test reports the run as skipped rather than failing, because a disk state
// is legitimately off when the arena is not fixed.

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "ntr/gx.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"

#include "fault_probe.h"

typedef unsigned int u32;

extern "C" {
int *ArrowSignRight_Spawn(void);
void *_ZN4Heap13SetupRootHeapEv(void);
extern int data_0209b3ec[12];       /* camera matrix */
struct SharedFilePtrC { unsigned short fileID; unsigned char numRefs;
                        unsigned char pad; void *filePtr; };
SharedFilePtrC *_ZN13SharedFilePtr9ConstructEj(SharedFilePtrC *s, u32 id);
extern unsigned short data_020a4b54;    /* pending actor ID */
extern void **data_020a4bb8;            /* actorID -> SpawnInfo* */
void *data_ov098_0213c380[6];
char data_ov098_0213c384[0x18];
extern void *data_020a0eac_c;           /* actor heap = root heap */
extern void *data_020a0ea0;             /* defaultHeapPtr */
extern void *data_0209f394[];           /* the player array */
extern unsigned char data_0209f21c;     /* player count */
void hal_fill_model_vtable(void);
void hal_fill_shadow_vtable(void);
void hal_fill_mmc_vtable(void);

// the in-memory + disk save-state layers under test
int lk6_savestate_save(void);
int lk6_savestate_load(void);
int lk6_savestate_has(void);
int lk7_persist_write(void);
int lk7_persist_read(void);
int lk7_persist_available(void);

// arena window
void *port_arena_base(void);
void *port_arena_end(void);
void *port_arena_cursor(void);
int   port_arena_is_fixed(void);
extern int LCG_STATE_0204da4c;
}

// lk7_persist_read hands off to lk6_savestate_load, which calls the sdat
// resets. A headless smoke opens no device, so they are no-ops here, the same
// four smoke_savestate stubs. The hardware-store hooks need no stubs: this
// smoke links ntr, so the real port_hw_regions_* in ntr/io.cpp serve both lk6
// and lk7 against the real reservations.
void sd_seq_reset(void) {}
void sd_mix_reset(void) {}
void sd_consumer_reset(void) {}
void sd_waves_reset(void) {}
void sd_sdat_reseat(void) {}

typedef int (__thiscall *Fn0)(void *);
static int vcall0(void *actor, int slot)
{
    void **vt = *(void ***)actor;
    return ((Fn0)vt[slot])(actor);
}

static int g_failures;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void ident_fx(void *m)
{
    memset(m, 0, 48);
    ((int *)m)[0] = ((int *)m)[4] = ((int *)m)[8] = 0x1000;
}

static void reset_scene()
{
    ntr::gx_reset();
    NTR_MMIO(uint32_t, 0x04000440) = 0;
    NTR_MMIO(uint32_t, 0x04000454) = 0;
    NTR_MMIO(uint32_t, 0x04000440) = 1;
    NTR_MMIO(uint32_t, 0x04000454) = 0;
    NTR_MMIO(uint32_t, 0x04000580) = 0u | (0u << 8) | (255u << 16) | (191u << 24);
    ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
    ntr::gx_enable_lights(0x1);
}

static uint64_t arena_hash()
{
    const unsigned char *b = (const unsigned char *)port_arena_base();
    const unsigned char *e = (const unsigned char *)port_arena_end();
    uint64_t h = 1469598103934665603ULL;
    for (; b < e; ++b) { h ^= *b; h *= 1099511628211ULL; }
    return h;
}

// Boot the shared actor world both phases stand on. Returns the spawned actor.
static int *boot_actor_world()
{
    CHECK(_ZN4Heap13SetupRootHeapEv() != NULL);
    ident_fx(data_0209b3ec);

    hal_fill_model_vtable();
    hal_fill_shadow_vtable();
    hal_fill_mmc_vtable();
    data_020a4b54 = 0x12b;
    static unsigned short spawn_info[4] = { 0, 0, 100, 100 };
    data_020a4bb8[0x12b] = spawn_info;
    data_020a0eac_c = data_020a0ea0;
    static SharedFilePtrC sign_model, sign_kcl;
    _ZN13SharedFilePtr9ConstructEj(&sign_model, 1177);
    _ZN13SharedFilePtr9ConstructEj(&sign_kcl, 1178);
    data_ov098_0213c380[0] = &sign_model;
    data_ov098_0213c380[1] = &sign_kcl;
    *(void **)(data_ov098_0213c384 + 0) = &sign_kcl;

    static char fake_player[0x800];
    data_0209f394[0] = fake_player;
    *(unsigned char *)&data_0209f21c = 1;

    int *actor = ArrowSignRight_Spawn();
    CHECK(actor != NULL);
    if (!actor) return NULL;
    CHECK(vcall0(actor, 0) == 1);           // InitResources
    reset_scene();
    return actor;
}

static const char *state_file_path(char *buf, size_t cap)
{
    char exe[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return NULL;
    char *slash = strrchr(exe, '\\');
    char *fwd = strrchr(exe, '/');
    if (fwd && (!slash || fwd > slash)) slash = fwd;
    if (!slash) return NULL;
    *slash = '\0';
    snprintf(buf, cap, "%s\\savestate.bin", exe);
    return buf;
}

// ---- PHASE "load": the spawned child --------------------------------------
static int phase_load()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }
    if (!port_arena_is_fixed()) {
        fprintf(stderr, "  [child] arena not at fixed base; disk load cannot run\n");
        return 3;   // signalled to the parent as a skip
    }

    const char *hs = getenv("SM64DS_PERSIST_HASH");
    if (!hs) { fprintf(stderr, "  [child] no expected hash passed\n"); return 4; }
    uint64_t expect = strtoull(hs, NULL, 16);

    int *actor = boot_actor_world();
    if (!actor) { fprintf(stderr, "  [child] actor world did not boot\n"); return 1; }

    // Load the disk state the parent wrote. This is the cross-process load: a
    // separate process picking up the first one's savestate.bin.
    int loaded = lk7_persist_read();
    CHECK(loaded == 1);
    CHECK(lk6_savestate_has() == 1);

    uint64_t got = arena_hash();
    printf("  [child] loaded disk state, arena hash=%016llx expected=%016llx\n",
           (unsigned long long)got, (unsigned long long)expect);
    CHECK(got == expect);

    // keep running after the load: the actor still dispatches
    for (int f = 0; f < 8; ++f) vcall0(actor, 6);
    (void)vcall0(actor, 9);
    printf("  [child] ran 8 frames after the cross-process load: no crash\n");

    if (g_failures) { fprintf(stderr, "  [child] %d FAILURE(S)\n", g_failures); return 1; }
    printf("  [child] cross-process load byte-exact: ok\n");
    return 0;
}

// ---- PHASE "save": the parent ---------------------------------------------
static int phase_save()
{
    PORT_INSTALL_FAULT_PROBE();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }

    if (!port_arena_is_fixed()) {
        // Legitimately off: the fixed base was taken. A disk state cannot
        // relocate, so report a skip rather than a failure.
        printf("smoke_persist: SKIPPED (arena not at fixed base; disk states off)\n");
        return 0;
    }
    CHECK(lk7_persist_available() == 1);

    char path[512];
    if (!state_file_path(path, sizeof path)) { fprintf(stderr, "no state path\n"); return 2; }
    remove(path);   // start from no file so the write is provably ours

    int *actor = boot_actor_world();
    if (!actor) { fprintf(stderr, "smoke_persist: no actor, abort\n"); return 1; }

    // evolve real state, then move a few words so there is unmistakable live
    // state to carry across the restart
    for (int f = 0; f < 8; ++f) vcall0(actor, 6);
    *(int *)((char *)actor + 0x5c) = 0x11110000;
    *(int *)((char *)actor + 0x60) = 0x22220000;
    *(int *)((char *)actor + 0x64) = 0x33330000;
    *(int *)((char *)actor + 0x70) = 0x0000abcd;
    LCG_STATE_0204da4c = 0x5eedf715;

    // save to the in-memory slot, then mirror to disk
    CHECK(lk6_savestate_save() == 1);
    CHECK(lk7_persist_write() == 1);

    // the file must now exist and be non-trivial
    FILE *chk = fopen(path, "rb");
    CHECK(chk != NULL);
    long fsz = 0;
    if (chk) { fseek(chk, 0, SEEK_END); fsz = ftell(chk); fclose(chk); }
    CHECK(fsz > (long)(8 << 20));   // header + 8MB arena + globals
    printf("  [parent] wrote savestate.bin (%ld bytes) at %s\n", fsz, path);

    uint64_t saved = arena_hash();
    printf("  [parent] saved arena hash=%016llx\n", (unsigned long long)saved);

    // ---- spawn a genuinely separate process to load it --------------------
    char exe[MAX_PATH];
    GetModuleFileNameA(NULL, exe, MAX_PATH);
    char hashenv[64];
    snprintf(hashenv, sizeof hashenv, "%016llx", (unsigned long long)saved);
    SetEnvironmentVariableA("SM64DS_PERSIST_PHASE", "load");
    SetEnvironmentVariableA("SM64DS_PERSIST_HASH", hashenv);

    STARTUPINFOA si; memset(&si, 0, sizeof si); si.cb = sizeof si;
    PROCESS_INFORMATION pi; memset(&pi, 0, sizeof pi);
    printf("  [parent] spawning a second process to load the disk state...\n");
    BOOL ok = CreateProcessA(exe, NULL, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    CHECK(ok);
    int child_rc = -1;
    if (ok) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD rc = 0; GetExitCodeProcess(pi.hProcess, &rc); child_rc = (int)rc;
        CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    }
    SetEnvironmentVariableA("SM64DS_PERSIST_PHASE", NULL);
    printf("  [parent] child exited %d\n", child_rc);
    if (child_rc == 3) {
        printf("smoke_persist: SKIPPED (child could not pin the fixed base)\n");
        return 0;
    }
    CHECK(child_rc == 0);

    // ---- refusal case A: a corrupted header is refused cleanly ------------
    // Flip the first magic byte, then try to read: the read must refuse and
    // return 0, and it must NOT touch the (already valid) in-memory slot.
    {
        FILE *f = fopen(path, "r+b");
        CHECK(f != NULL);
        if (f) { unsigned char bad = 'X'; fseek(f, 0, SEEK_SET); fwrite(&bad, 1, 1, f); fclose(f); }
        int r = lk7_persist_read();
        CHECK(r == 0);
        printf("  [parent] corrupted-magic header refused: r=%d\n", r);
    }

    // ---- refusal case B: a gittip mismatch is refused cleanly -------------
    // Rewrite a valid file (write() again), then overwrite the gittip field with
    // a different build string. Header magic/format/base/size still match, so
    // the ONLY thing that can turn it away is the gittip field.
    {
        CHECK(lk7_persist_write() == 1);
        // header layout: magic[8], format(u32), gittip[64] at offset 12
        FILE *f = fopen(path, "r+b");
        CHECK(f != NULL);
        if (f) {
            const char fake[] = "deadbeef-not-this-build";
            fseek(f, 12, SEEK_SET);
            fwrite(fake, 1, sizeof fake, f);   // includes the terminating NUL
            fclose(f);
        }
        int r = lk7_persist_read();
        CHECK(r == 0);
        printf("  [parent] gittip-mismatch header refused: r=%d\n", r);
    }

    // leave a clean valid file behind (tidy, and matches shipped behaviour)
    CHECK(lk7_persist_write() == 1);
    remove(path);   // do not leave a savestate.bin in the build tree

    if (g_failures) { fprintf(stderr, "smoke_persist: %d FAILURE(S)\n", g_failures); return 1; }
    printf("smoke_persist: all checks passed (wrote disk state, a second process "
           "loaded it byte-exact, corrupted and stale-build headers both refused)\n");
    return 0;
}

int main(void)
{
    const char *phase = getenv("SM64DS_PERSIST_PHASE");
    if (phase && strcmp(phase, "load") == 0) return phase_load();
    return phase_save();
}
