/* HOST COPY of MONTY_MOLE's (MontyMole / daChoropu_c, actor 310, ov080)
 * Behavior -- the only TU that reads its pointer-to-member state table -- and
 * the seat of the table it reads.
 *
 * MontyMole::Behavior drives a six-state machine through data_ov080_02128438,
 * six single {function, delta} records that __sinit_ov080_021278c0 copies out of
 * six source statics at ov080 0x02127f80..0x02127fa8. Unlike the Crate's
 * {init, main} pairs, each MontyMole state is one record and Behavior dispatches
 * it every frame by the object's own idx (at +0x17c):
 *
 *   MontyMole::Behavior(c)   calls state c->idx's function, every frame
 *
 * Behavior is the OneUpMushroom / WHOMP case and must be a host copy for the
 * same two reasons:
 *
 *   1. MSVC forms a pointer-to-member of the INCOMPLETE `struct C;` the matched
 *      Behavior declares as the four-word general representation, which strides
 *      the table at 0x10 instead of 0x8 and dispatches a neighbour's body.
 *      Reading the record as a plain { function, 0 } and calling the function
 *      with `this` is the fix.
 *
 *   2. The words the sinit copies are the overlay image's own -- DS CODE
 *      ADDRESSES, the ovdata contract, matched against ov080's relocs. The seat
 *      below rewrites each SOURCE static's first word with its host body BEFORE
 *      the sinit copies it into the runtime table (the WaterBomb / Crate
 *      reading: one less mapping to get wrong), each checked against the ROM
 *      address the body was compiled from, so a mount pointing at the wrong
 *      bytes says so instead of calling into the overlay image.
 *
 * The table is the mole's OWN -- only MontyMole::Behavior references
 * data_ov080_02128438 -- and all six targets are matched src (slice_gate174.txt),
 * so there are no state traps. All six .delta halves are 0 (non-virtual
 * complete-class form, per the relocated overlay bytes).
 *
 * The rest of the mole (Init, Cleanup, Render, both destructors, the mid-life
 * helpers func_ov080_02124088/02124208/02124360/021243d8 and the six state
 * bodies) is matched src in the slice; only Behavior lands here.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {
/* the two per-frame helpers Behavior calls after the dispatch; matched src */
int func_ov080_02124208(void *c);
void func_ov080_021243d8(char *c);
/* engine faces Behavior calls; all in the map */
void _ZN5Actor19MakeVanishLuigiWorkER12CylinderClsn(void *self, void *cyl);
void _ZN12CylinderClsn5ClearEv(void *c);
void _ZN12CylinderClsn6UpdateEv(void *c);

/* the six state functions, table order; all matched src */
void func_ov080_02123fcc(char *c);   /* state 0 */
void func_ov080_02123ecc(char *c);   /* state 1 */
void func_ov080_02123c24(char *c);   /* state 2 */
void func_ov080_02123a34(char *c);   /* state 3 */
void func_ov080_02123924(char *c);   /* state 4 */
void func_ov080_02123860(char *c);   /* state 5 */

struct PortPmf { unsigned fn; int delta; };
/* the RUNTIME table Behavior reads (bss, filled by the sinit) */
extern PortPmf data_ov080_02128438[6];
/* the six SOURCE statics __sinit_ov080_021278c0 copies from. The sinit's copy
   order maps table index -> source: 0<-fa0, 1<-f88, 2<-fa8, 3<-f80, 4<-f98,
   5<-f90. */
extern PortPmf data_ov080_02127fa0[], data_ov080_02127f88[],
    data_ov080_02127fa8[], data_ov080_02127f80[], data_ov080_02127f98[],
    data_ov080_02127f90[];
}  /* extern "C" */

/* PORT_HOST_ABI: mwcc pointer-to-member stride on the incomplete class. The
   matched Behavior forms `(((C*)this)->*data_ov080_02128438[j].pmf[0])()`; here
   the record is read as a plain { fn, 0 } and the fn called with `this`. */
extern "C" int _ZN9MontyMole8BehaviorEv(void *self)
{
    char *p = (char *)self;
    _ZN5Actor19MakeVanishLuigiWorkER12CylinderClsn(p, p + 0x138);
    int j = *(int *)(p + 0x17c);
    ((void (*)(char *))(size_t)data_ov080_02128438[j].fn)(p);
    func_ov080_02124208(p);
    func_ov080_021243d8(p);
    _ZN12CylinderClsn5ClearEv(p + 0x138);
    _ZN12CylinderClsn6UpdateEv(p + 0x138);
    return 1;
}

static const struct { PortPmf *slot; unsigned rom; void (*host)(char *); }
g_monty_mole_states[] = {
    {data_ov080_02127fa0, 0x02123fcc, func_ov080_02123fcc},  /* state 0 */
    {data_ov080_02127f88, 0x02123ecc, func_ov080_02123ecc},  /* state 1 */
    {data_ov080_02127fa8, 0x02123c24, func_ov080_02123c24},  /* state 2 */
    {data_ov080_02127f80, 0x02123a34, func_ov080_02123a34},  /* state 3 */
    {data_ov080_02127f98, 0x02123924, func_ov080_02123924},  /* state 4 */
    {data_ov080_02127f90, 0x02123860, func_ov080_02123860},  /* state 5 */
};

extern "C" void port_monty_mole_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_monty_mole_states /
                             sizeof g_monty_mole_states[0]; ++i) {
        PortPmf *p = g_monty_mole_states[i].slot;
        if (p->fn != g_monty_mole_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: MontyMole state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta,
                         g_monty_mole_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_monty_mole_states[i].host;
    }
}
