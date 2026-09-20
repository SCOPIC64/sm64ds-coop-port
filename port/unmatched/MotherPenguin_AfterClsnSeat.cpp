/* MOTHER_PENGUIN's six-cell code-pointer table seat, the SoundObject/Cap_
 * StateDispatch treatment applied to a plain function-pointer table instead
 * of a PMF dispatch.
 *
 * THE TABLE. __sinit_ov018_02112c80 copies six 8-byte cells (data_ov018_
 * 0211394c..02113974, static const in the overlay's own .rodata) verbatim
 * into the mutable instance table data_ov018_02113c4c (0211394c..0211397c,
 * confirmed by reloc: each source cell's first word is a relocated DS CODE
 * ADDRESS -- the ovdata "romdata pointer" contract: a named symbol carrying
 * a relocated word holds a DS address until something patches it). Later,
 * func_ov018_021123d0(self, i) stores &data_ov018_02113c4c[i] at self+0x370
 * and func_ov018_02112398 immediately dereferences it as a pointer-to-
 * member-function and calls through it -- so the DS code address in cell i
 * gets CALLED directly unless something rewrites it to the host body first.
 *
 * ONLY CELL 0 IS REACHABLE from this gate: MotherPenguin::InitResources
 * (the HOST COPY, MotherPenguin_InitResources.cpp) is the only caller in
 * this slice, and it always passes i=0. The other five cells' own callers
 * (func_ov018_02111968/02111b3c/02111fac, MotherPenguin's own OTHER methods
 * not in this slice) are not reachable without hosting more of the class's
 * behavior tree, so cells 1-5 trap loudly instead of silently calling into
 * the DS overlay image -- the same reading gate 190's ccm_trap slots take
 * for unfilled vtable entries.
 *
 * Cell 0's target, func_ov018_021122ec ("AfterClsn" per its own recovered-
 * name comment, itself carrying the SAME dsd-era class-identity mislabel
 * as the eight _ZN7SkiLift* files -- it is MotherPenguin's own body), is
 * carried byte-identical from main HEAD: its own data_ov027/ov030
 * cross-overlay spellings (the SAME #1308-#1310 trap family InitResources
 * had) are fixed there to data_ov018_02113be8/02113bf0 -- the exact cells
 * this gate already mounts for MotherPenguin's own TextureSequence
 * (data_ov018_02113bf8/02113be8) and Animation (data_ov018_02113bf0/
 * 02113c08) SharedFilePtrs, so func_ov018_021122ec's own reads land on
 * bytes this gate's mount already provides -- no new mount rows needed.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {
struct PortCodePair { unsigned fn; int delta; };
extern PortCodePair data_ov018_02113c4c[6];   /* the six mutable cells */
int func_ov018_021122ec(char *self);          /* cell 0 -- AfterClsn, HOSTED */
}

enum { PORT_MPG_CELLS = 6 };

typedef int (*PortMpgFn)(char *);

static int __cdecl mpg_trap_cell(char *self)
{
    std::fprintf(stderr, "FATAL: MotherPenguin AfterClsn-table cell "
                 "dispatched with no host filler (self=%p) -- only cell 0 "
                 "is hosted this gate\n", (void *)self);
    std::abort();
    return 0;
}

/* {ROM code address the sinit's own source cell carries, host body} --
   verified against the ROM's own record before the rewrite, the Cap seat
   shape: a mount pointing at the wrong bytes aborts instead of calling
   into the overlay image. */
/* __sinit_ov018_02112c80's copy is NOT in address order (.a=0211394c,
   .b=02113974, .c=02113954, .d=02113964, .e=0211395c, .f=0211396c) -- each
   row here is that source cell's own reloc target, in the sinit's own field
   order, not sorted by source address. */
static const struct { unsigned rom; PortMpgFn host; } g_mpg_cells[PORT_MPG_CELLS] = {
    {0x021122ec, (PortMpgFn)func_ov018_021122ec},   /* cell 0 (.a): AfterClsn, HOSTED */
    {0x02112234, mpg_trap_cell},                     /* cell 1 (.b): not in this slice */
    {0x021121dc, mpg_trap_cell},                     /* cell 2 (.c): not in this slice */
    {0x02111fac, mpg_trap_cell},                     /* cell 3 (.d): not in this slice */
    {0x02111f1c, mpg_trap_cell},                     /* cell 4 (.e): not in this slice */
    {0x02111e28, mpg_trap_cell},                     /* cell 5 (.f): not in this slice */
};

extern "C" void port_mother_penguin_afterclsn_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (int i = 0; i < PORT_MPG_CELLS; ++i) {
        if (data_ov018_02113c4c[i].fn != g_mpg_cells[i].rom ||
            data_ov018_02113c4c[i].delta != 0) {
            std::fprintf(stderr, "FATAL: MotherPenguin AfterClsn-table cell "
                         "%d: the sinit left %08x/%d, the ROM's own record "
                         "says %08x/0 -- WRONG BYTES\n", i,
                         data_ov018_02113c4c[i].fn, data_ov018_02113c4c[i].delta,
                         g_mpg_cells[i].rom);
            std::abort();
        }
        data_ov018_02113c4c[i].fn = (unsigned)(size_t)g_mpg_cells[i].host;
    }
}
