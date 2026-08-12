// Host OS arena: the memory region SetupRootHeap carves the root heap from.
//
// On the DS, data_020a0ea4 is the OS globals object and the func_02058*
// family are the arena accessors (get-low, get-high, align-allocate,
// set-low). On host the arena is one big malloc'd block, initialized on
// first use so the smoke needs no setup call.
#include <stdlib.h>
#include "dsstate_seg.h"
#include <stdio.h>

#if defined(_WIN32)
#include <windows.h>
#endif

typedef unsigned int u32;

enum { HOST_ARENA = 8 << 20 };   /* 8 MB: larger than the DS main-RAM arena */

static char *g_lo, *g_hi;

/* WHY THE ARENA WANTS A FIXED BASE (lane lk7, save states that outlive the run)
   ---------------------------------------------------------------------------
   The in-memory save state (hal/lk6_savestate.cpp) memcpy's the whole arena and
   restores it verbatim, which works because the block never moves for the life
   of the process: a pointer captured at save still addresses the same arena
   offset after restore. A save state written to DISK has to survive the process
   ENDING. On the next launch the arena is a fresh allocation, and a plain calloc
   lands wherever the host allocator puts it, which is a different absolute
   address every run. The disk state is full of raw absolute pointers into the
   arena (actor-list heads, model-matrix pointers, intrusive next-links), so it
   only loads correctly if the arena comes up at the SAME base it had when the
   state was written.

   So the arena is reserved at a fixed host address with VirtualAlloc, the same
   technique ntr/io.cpp uses to pin the DS ranges. The base sits above every DS
   range (main RAM 0x02000000..0x02400000, the shared block at 0x027ff000, and
   the MMIO/VRAM ranges up to 0x07000800) and well below 2GB so the truncating
   32-bit round-trips the loaders do stay exact. If the reservation fails at
   boot (something else holds the range), the arena falls back to calloc and the
   persist layer refuses disk load/save for the run; the in-memory slot still
   works exactly as before. port_arena_is_fixed() reports which path we took. */
#if defined(_WIN32)
enum : size_t { ARENA_FIXED_BASE = 0x30000000u };  /* 768 MB, above the DS ranges */
static int g_fixed_base;                            /* 1 = VirtualAlloc'd at ARENA_FIXED_BASE */
#endif

/* THE ARENA HAS TO BE DETERMINISTIC. It was a plain malloc, which left two
   things different on every run: the contents (whatever the host allocator
   last had there) and the absolute base. Both reach the game. Anything that
   reads a field before writing it picks up the old contents, and the carve in
   func_02058cd0 rounds `lo` up against the ABSOLUTE address, so a base that
   is not aligned to at least the largest alignment the game asks for shifts
   every later allocation by a different amount.
   The symptom was a walk_window selftest that produced five different final
   frames in six identical runs -- Mario ended up somewhere else each time,
   which makes any before/after comparison of the renderer meaningless.
   calloc zeroes it (the DS arena is cleared main RAM) and the 64K alignment
   makes every carve offset identical from one run to the next. */
enum { ARENA_ALIGN = 0x10000 };

/* The aligned base of the arena, i.e. g_lo as it stood the instant arena_init
   ran, before any carve advanced it. g_lo itself is the live carve cursor and
   moves; g_base does not. The save-state code (hal/lk6_savestate.cpp) needs the
   fixed base and the fixed ceiling to bound the region it snapshots, plus the
   live cursor as a value it captures and restores like any other allocator
   state. */
static char *g_base;

static void arena_init(void)
{
    if (!g_lo) {
        /* soaks and tools can ask for more than the DS ever had */
        const char *env = getenv("SM64DS_HOST_ARENA_MB");
        size_t mb = env ? (size_t)atoi(env) : 0;
        size_t size = mb ? mb << 20 : (size_t)HOST_ARENA;

#if defined(_WIN32)
        /* Try the fixed base first. ARENA_FIXED_BASE is 64K-aligned, so the
           reservation starts exactly there and no post-alignment is needed --
           the base is deterministic across runs, which is what the disk save
           state leans on. VirtualAlloc zeroes committed pages, matching the
           calloc'd DS arena. */
        void *fixed = VirtualAlloc((void *)ARENA_FIXED_BASE, size,
                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (fixed) {
            g_lo = (char *)fixed;
            g_hi = g_lo + size;
            g_base = g_lo;
            g_fixed_base = 1;
            return;
        }
        /* Fell through: something holds 0x30000000. Say so once and keep the
           game running on a calloc'd arena; the persist layer reads
           port_arena_is_fixed() and turns disk states off for the run. */
        fprintf(stderr, "[savestate] arena not at fixed base; disk states off this run\n");
#endif
        char *base = (char *)calloc(size + ARENA_ALIGN, 1);
        if (!base) return;
        g_lo = (char *)(((size_t)base + (ARENA_ALIGN - 1))
                        & ~(size_t)(ARENA_ALIGN - 1));
        g_hi = g_lo + size;
        g_base = g_lo;
    }
}

extern "C" {
/* Read-only arena window accessors for the save-state layer. base and end are
   fixed for the process lifetime; cursor is the live low-water carve pointer
   that func_02058cd0/func_02058d58 move. All three return 0 before the first
   carve forces arena_init, which is why the save-state code calls
   port_arena_base() (it runs arena_init) before reading any of them. */
void *port_arena_base(void)   { arena_init(); return g_base; }
void *port_arena_end(void)    { arena_init(); return g_hi; }
void *port_arena_cursor(void) { arena_init(); return g_lo; }
void  port_arena_set_cursor(void *p) { g_lo = (char *)p; }

/* 1 if the arena came up at its fixed host base (VirtualAlloc succeeded), 0 if
   it fell back to calloc. The disk save state (hal/lk7_persist.cpp) is only
   loadable and savable when this is 1, because a disk state carries absolute
   pointers into the arena and only a fixed base makes them valid across a
   restart. On non-Windows the arena is always calloc'd, so this is 0 and disk
   states are off (the shipped port is Windows-only). */
int port_arena_is_fixed(void)
{
    arena_init();
#if defined(_WIN32)
    return g_fixed_base;
#else
    return 0;
#endif
}
}

extern "C" {
// the OS globals object; the accessors ignore it, but the address must exist
DSSTATE_BEGIN
char data_020a0ea4[4];
DSSTATE_END

/* PORT_HOST_ABI: DS OS-arena globals live at unmapped 0x27ffda0; host substitutes
   a deterministic calloc'd arena. */
int func_02058ea0(void *) { arena_init(); return (int)(size_t)g_lo; }   /* arena lo */
/* PORT_HOST_ABI: DS OS-arena globals live at unmapped 0x27ffdc4; host arena. */
int func_02058eb4(void *) { arena_init(); return (int)(size_t)g_hi; }   /* arena hi */

/* align `lo` up by `align`, bounded by hi -- mirrors OS_AllocFromArenaLo's
 * pre-alignment step as SetupRootHeap uses it
 * PORT_HOST_ABI: DS OS-arena state (0x27ffda0) unmapped; host arena carve. */
int func_02059040(void *, int lo, int hi, int align)
{
    (void)hi;
    return (lo + align - 1) & ~(align - 1);
}

/* PORT_HOST_ABI: DS OS-arena globals live at unmapped 0x27ffda0; host arena. */
void func_02058d58(void *, int newLo) { g_lo = (char *)(size_t)newLo; }  /* set lo */

/* carve `size` bytes aligned `align` from the low side
   PORT_HOST_ABI: DS OS-arena state (0x27ffda0) unmapped; host arena carve. */
void *func_02058cd0(void *, int size, int align)
{
    arena_init();
    char *p = (char *)(((size_t)g_lo + align - 1) & ~(size_t)(align - 1));
    if (p + size > g_hi)
        return 0;
    g_lo = p + size;
    return p;
}
}
