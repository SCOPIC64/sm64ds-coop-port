// Disk-backed save state for the PC port (lane lk7). Makes the single lk6 slot
// survive the game closing and reopening.
//
// WHAT THIS ADDS ON TOP OF lk6
// ----------------------------
// hal/lk6_savestate.cpp keeps one IN-MEMORY slot: the hosted DS arena, the
// .dsstate section, and the hardware content stores, valid for the life of the
// process. lk7 writes that same content to a file next to the exe on every
// successful save, and reads it back at startup so the slot is populated from
// disk. Everything about WHAT is captured stays in lk6; lk7 only moves those
// bytes to and from disk and guards the move with a header.
//
// THE HARD PART: ABSOLUTE POINTERS ACROSS A RESTART
// -------------------------------------------------
// The snapshot is full of raw absolute pointers, and they come in TWO kinds.
// Pointers INTO THE ARENA (actor-list heads, model records, intrusive links)
// relocate only if the arena comes up at the same base, which is why
// hal/os_arena.cpp pins it at a fixed address (0x30000000) with VirtualAlloc,
// the same technique ntr/io.cpp uses for the DS ranges. Pointers INTO THE EXE
// IMAGE (every vtable pointer in every actor, every code pointer in a hosted
// table) relocate only if the IMAGE comes up at the same base -- and ASLR
// rebases a Windows exe once per boot, so a state written before a reboot
// would come back with every vtable pointing into the void. The original lane
// draft never checked that; the header now carries the image base and refuses
// a mismatch, and the build links the window targets with /DYNAMICBASE:NO so
// the base is the same every boot and the refusal never fires in practice.
//
// THE HEADER, AND WHAT EACH FIELD REFUSES
// ---------------------------------------
// A disk state is only loaded if every header field matches this process:
//   magic         wrong bytes           -> not our file, refuse
//   format        version bump          -> old on-disk layout, refuse
//   gittip        different build       -> another version's world, refuse
//   image_base    exe rebased           -> code pointers would dangle, refuse
//   arena_base    arena at a diff base  -> arena pointers would dangle, refuse
//   arena_size    arena a diff size     -> lengths would not line up, refuse
//   dsstate_base  section moved         -> hosted globals moved, refuse
//   dsstate_size  section grew/shrank   -> another layout's blob, refuse
//   hw_size       regions differ        -> textures would not line up, refuse
// A refusal names the field on stderr and leaves the file untouched. A gittip
// mismatch is the common one: it is how a state written by an older build of
// the game is turned away instead of loaded into a world it no longer
// describes. (gittip, image_base and dsstate_base overlap in what they catch;
// they are all kept because each refusal message tells the reader something
// different about WHY the file cannot load.)

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

typedef unsigned int u32;

extern "C" {
// os_arena window + whether it is at the fixed base.
void *port_arena_base(void);
void *port_arena_end(void);
void *port_arena_cursor(void);
void  port_arena_set_cursor(void *p);
int   port_arena_is_fixed(void);

// the .dsstate section bounds (hal/dsstate_seg.cpp sentinels).
extern char dsstate_lo;
extern char dsstate_hi;

// the hardware content stores (ntr/io.cpp).
unsigned port_hw_regions_size(void);
void port_hw_regions_copy_out(void *dst);
void port_hw_regions_copy_in(const void *src);

// lk6: the in-memory slot this layer mirrors to and from disk.
int lk6_savestate_save(void);
int lk6_savestate_load(void);
int lk6_savestate_has(void);

// the build's git tip, embedded by CMake (host-src/port_gittip.c). Weakly
// aliased in fault_probe.h for builds without it; here we only read it.
extern const char port_build_gittip[];
}

namespace {

// Bumping any field layout below is a FORMAT_VERSION bump; an older file is
// then refused on the format field rather than misread. Format 2 is the
// .dsstate-section world (format 1 was the never-shipped hand-list draft).
const char   MAGIC[8]        = { 'S','M','6','4','D','S','S','T' };
const u32    FORMAT_VERSION  = 2;
const u32    GITTIP_FIELD    = 64;   // fixed-width, NUL-padded, always NUL-terminated

#pragma pack(push, 1)
struct Header {
    char     magic[8];
    u32      format;
    char     gittip[GITTIP_FIELD];   // stays at offset 12: smoke_persist pokes it there
    uint64_t image_base;             // exe base: vtable/code pointers relocate iff equal
    uint64_t arena_base;
    uint64_t arena_size;
    uint64_t arena_cursor;   // low-water carve pointer at save time (host state)
    uint64_t dsstate_base;   // &dsstate_lo: where the hosted globals sit this build
    uint64_t dsstate_size;   // &dsstate_hi - &dsstate_lo
    uint64_t hw_size;        // palette + video + sprite memory, 0 if not reserved
};
#pragma pack(pop)

size_t dsstate_size_live()
{
    char *lo = &dsstate_lo, *hi = &dsstate_hi;
    return hi > lo ? (size_t)(hi - lo) : 0;
}

// Fill hdr for THIS process. Returns 0 (and leaves hdr zeroed) if there is no
// usable arena window, which is the same guard lk6_savestate_save uses.
int fill_header(Header *hdr)
{
    memset(hdr, 0, sizeof *hdr);
    char *base = (char *)port_arena_base();
    char *end  = (char *)port_arena_end();
    if (!base || end <= base) return 0;
    memcpy(hdr->magic, MAGIC, sizeof hdr->magic);
    hdr->format = FORMAT_VERSION;
    snprintf(hdr->gittip, sizeof hdr->gittip, "%s", port_build_gittip);
#if defined(_WIN32)
    hdr->image_base = (uint64_t)(uintptr_t)GetModuleHandleA(NULL);
#endif
    hdr->arena_base   = (uint64_t)(uintptr_t)base;
    hdr->arena_size   = (uint64_t)(size_t)(end - base);
    hdr->arena_cursor = (uint64_t)(uintptr_t)port_arena_cursor();
    hdr->dsstate_base = (uint64_t)(uintptr_t)&dsstate_lo;
    hdr->dsstate_size = (uint64_t)dsstate_size_live();
    hdr->hw_size      = (uint64_t)port_hw_regions_size();
    return 1;
}

// The state file path: next to the exe, the folder the launcher puts the bundle
// in (same idea as host_settings.cpp find_settings). Returns 0 if it cannot be
// determined. Non-Windows never gets here because port_arena_is_fixed is 0
// there, so this is Windows-only.
int state_path(char *out, size_t cap)
{
#if defined(_WIN32)
    char exe[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (n > 0 && n < MAX_PATH) {
        char *slash = strrchr(exe, '\\');
        char *fwd   = strrchr(exe, '/');
        if (fwd && (!slash || fwd > slash)) slash = fwd;
        if (slash) {
            *slash = '\0';
            if (strlen(exe) + 16 < cap) {
                snprintf(out, cap, "%s\\savestate.bin", exe);
                return 1;
            }
        }
    }
#endif
    (void)out; (void)cap;
    return 0;
}

// Compare the on-disk header field by field against this process. On the first
// mismatch, name the field on stderr and return 0. Returns 1 only if every
// field matches.
int header_matches(const Header *disk, const Header *self)
{
    if (memcmp(disk->magic, self->magic, sizeof disk->magic) != 0) {
        fprintf(stderr, "[savestate] disk state refused: magic mismatch (not a save file)\n");
        return 0;
    }
    if (disk->format != self->format) {
        fprintf(stderr, "[savestate] disk state refused: format mismatch (file %u, build %u)\n",
                disk->format, self->format);
        return 0;
    }
    if (strncmp(disk->gittip, self->gittip, GITTIP_FIELD) != 0) {
        fprintf(stderr, "[savestate] disk state refused: gittip mismatch (file %.*s, build %.*s)\n",
                (int)GITTIP_FIELD, disk->gittip, (int)GITTIP_FIELD, self->gittip);
        return 0;
    }
    if (disk->image_base != self->image_base) {
        fprintf(stderr, "[savestate] disk state refused: exe image base mismatch "
                        "(file 0x%llx, this run 0x%llx) -- code pointers would "
                        "not relocate\n",
                (unsigned long long)disk->image_base,
                (unsigned long long)self->image_base);
        return 0;
    }
    if (disk->arena_base != self->arena_base) {
        fprintf(stderr, "[savestate] disk state refused: arena base mismatch "
                        "(file 0x%llx, this run 0x%llx)\n",
                (unsigned long long)disk->arena_base,
                (unsigned long long)self->arena_base);
        return 0;
    }
    if (disk->arena_size != self->arena_size) {
        fprintf(stderr, "[savestate] disk state refused: arena size mismatch "
                        "(file %llu, this run %llu)\n",
                (unsigned long long)disk->arena_size,
                (unsigned long long)self->arena_size);
        return 0;
    }
    if (disk->dsstate_base != self->dsstate_base) {
        fprintf(stderr, "[savestate] disk state refused: dsstate base mismatch "
                        "(file 0x%llx, this run 0x%llx)\n",
                (unsigned long long)disk->dsstate_base,
                (unsigned long long)self->dsstate_base);
        return 0;
    }
    if (disk->dsstate_size != self->dsstate_size) {
        fprintf(stderr, "[savestate] disk state refused: dsstate size mismatch "
                        "(file %llu, this run %llu)\n",
                (unsigned long long)disk->dsstate_size,
                (unsigned long long)self->dsstate_size);
        return 0;
    }
    if (disk->hw_size != self->hw_size) {
        fprintf(stderr, "[savestate] disk state refused: hardware-store size mismatch "
                        "(file %llu, this run %llu)\n",
                (unsigned long long)disk->hw_size,
                (unsigned long long)self->hw_size);
        return 0;
    }
    return 1;
}

} // namespace

extern "C" {

// 1 if disk states are usable this run: Windows and the arena is at its fixed
// base. When 0, save and load are memory-only (the caller stays on lk6).
int lk7_persist_available(void)
{
    return port_arena_is_fixed();
}

// Write the current live world to <exedir>\savestate.bin. The caller invokes
// this right after a successful lk6_savestate_save, so live memory IS the slot
// content. Returns 1 if the file was written, 0 otherwise (disk states off, no
// arena, path or IO failure). A failure to write the disk copy never fails the
// in-memory save.
int lk7_persist_write(void)
{
    if (!lk7_persist_available()) return 0;

    Header hdr;
    if (!fill_header(&hdr)) return 0;

    char path[512];
    if (!state_path(path, sizeof path)) {
        fprintf(stderr, "[savestate] could not resolve disk state path; disk save skipped\n");
        return 0;
    }

    char  *base = (char *)port_arena_base();
    size_t asz  = (size_t)hdr.arena_size;
    size_t dsz  = (size_t)hdr.dsstate_size;
    size_t hsz  = (size_t)hdr.hw_size;

    // The hardware stores go through the same copy-out the slot uses, into one
    // temporary blob, so the file's byte order is the hook's fixed region order.
    char *hbuf = 0;
    if (hsz) {
        hbuf = (char *)malloc(hsz);
        if (!hbuf) { fprintf(stderr, "[savestate] out of memory for disk save; skipped\n"); return 0; }
        port_hw_regions_copy_out(hbuf);
    }

    // Write to a temp file then rename, so a crash mid-write cannot leave a
    // truncated savestate.bin that the next launch would try to load.
    char tmp[540];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "wb");
    if (!f) {
        fprintf(stderr, "[savestate] cannot open %s for write; disk save skipped\n", tmp);
        free(hbuf);
        return 0;
    }
    int ok = 1;
    ok &= (fwrite(&hdr, sizeof hdr, 1, f) == 1);
    ok &= (asz == 0 || fwrite(base, 1, asz, f) == asz);
    ok &= (dsz == 0 || fwrite(&dsstate_lo, 1, dsz, f) == dsz);
    ok &= (hsz == 0 || fwrite(hbuf, 1, hsz, f) == hsz);
    fclose(f);
    free(hbuf);
    if (!ok) {
        fprintf(stderr, "[savestate] write to %s failed; disk save skipped\n", tmp);
        remove(tmp);
        return 0;
    }

    remove(path);          // MoveFile/rename won't overwrite on Windows
    if (rename(tmp, path) != 0) {
        fprintf(stderr, "[savestate] rename %s -> %s failed; disk save skipped\n", tmp, path);
        remove(tmp);
        return 0;
    }
    fprintf(stderr, "[savestate] wrote disk state: %zu arena + %zu dsstate + "
                    "%zu hw bytes -> %s\n", asz, dsz, hsz, path);
    return 1;
}

// Read <exedir>\savestate.bin and, if its header matches this process in every
// field, copy the arena, the .dsstate section and the hardware stores straight
// into place and hand the world to lk6 (cursor, cache drops, audio reset, all
// of it) via lk6_savestate_save + lk6_savestate_load. Returns 1 if a disk state
// was loaded, 0 if there was no file, disk states are off, or the header was
// refused (a refusal names the field and leaves the file alone).
int lk7_persist_read(void)
{
    if (!lk7_persist_available()) return 0;

    char path[512];
    if (!state_path(path, sizeof path)) return 0;

    FILE *f = fopen(path, "rb");
    if (!f) return 0;   // no saved disk state; silent, this is the normal case

    Header disk;
    if (fread(&disk, sizeof disk, 1, f) != 1) {
        fprintf(stderr, "[savestate] disk state refused: file too short for a header\n");
        fclose(f);
        return 0;
    }

    Header self;
    if (!fill_header(&self)) { fclose(f); return 0; }
    if (!header_matches(&disk, &self)) { fclose(f); return 0; }

    size_t asz = (size_t)disk.arena_size;
    size_t dsz = (size_t)disk.dsstate_size;
    size_t hsz = (size_t)disk.hw_size;
    char  *base = (char *)port_arena_base();

    // Copy the arena and the section straight in. The bases matched the header,
    // so every absolute pointer in these bytes addresses what it did at save.
    if (asz && fread(base, 1, asz, f) != asz) {
        fprintf(stderr, "[savestate] disk state refused: arena body truncated\n");
        fclose(f);
        return 0;
    }
    if (dsz && fread(&dsstate_lo, 1, dsz, f) != dsz) {
        fprintf(stderr, "[savestate] disk state refused: dsstate body truncated\n");
        fclose(f);
        return 0;
    }
    if (hsz) {
        char *hbuf = (char *)malloc(hsz);
        if (!hbuf) { fprintf(stderr, "[savestate] out of memory reading disk state\n"); fclose(f); return 0; }
        if (fread(hbuf, 1, hsz, f) != hsz) {
            fprintf(stderr, "[savestate] disk state refused: hardware-store body truncated\n");
            free(hbuf);
            fclose(f);
            return 0;
        }
        port_hw_regions_copy_in(hbuf);   /* also drops the texture decode cache */
        free(hbuf);
    }
    fclose(f);

    // Put the arena carve cursor back where it stood at save. It is host state
    // in os_arena.cpp, NOT part of the arena bytes, so a fresh boot left it
    // wherever this session's own allocations reached; without this the lk6
    // snapshot below would capture the wrong high-water mark and the next
    // allocation would stomp restored data.
    port_arena_set_cursor((void *)(uintptr_t)disk.arena_cursor);

    // The bytes and the cursor are in place. Snapshot them into the lk6 slot so
    // lk6 owns the world exactly as if F8 had run this session, then load it:
    // that path re-asserts the cursor, drops the wave cache and resets audio,
    // and it leaves the slot valid so the menu shows a state is present and F9
    // works without touching the disk again.
    lk6_savestate_save();
    lk6_savestate_load();

    fprintf(stderr, "[savestate] loaded disk state: %zu arena + %zu dsstate + "
                    "%zu hw bytes from %s\n", asz, dsz, hsz, path);
    return 1;
}

}
