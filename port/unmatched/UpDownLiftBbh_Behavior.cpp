/* HOST COPY of the UP_DOWN_LIFT_HMC's (UpDownLiftBbh / daUdlift_c, actor 33
 * and its Bbh/Rr siblings, ov095) Behavior, and the seat of the state table it
 * reads.
 *
 * UpDownLiftBbh::Behavior drives a five-state machine through
 * data_ov095_02137910, five {function, delta} records that
 * __sinit_ov095_0213722c copies out of five SOURCE statics at ov095
 * 0x0213757c..0x0213759c. Unlike the Crate, the records DO NOT pair up: the
 * ROM indexes data_ov095_02137910[mState] directly, one 8-byte record per
 * state, and calls that state's function every frame. The class has ONE
 * dispatcher, Behavior itself, and it must be a host copy for the two reasons:
 *
 *   1. MSVC forms a pointer-to-member of the INCOMPLETE `struct Plat;` the
 *      source declares (`typedef void (Plat::*PMF)();`) as the four-word
 *      general representation, which strides the table at 0x10 instead of 0x08
 *      and dispatches a neighbour's body. Reading the record as a plain
 *      { function, 0 } and calling the function with `this` is the fix.
 *
 *   2. The words the sinit copies are the overlay image's own -- DS CODE
 *      ADDRESSES, the ovdata contract, matched against ov095's relocs
 *      (0213757c..0213759c load to 02136090/104/178/298/368). The seat below
 *      rewrites each SOURCE static's first word with its host body BEFORE the
 *      sinit copies it into the runtime table (the WaterBomb / Crate reading:
 *      one less mapping to get wrong), each checked against the ROM address the
 *      body was compiled from, so a mount pointing at the wrong bytes says so
 *      instead of calling into the overlay image.
 *
 * The table is the lift's OWN -- only UpDownLiftBbh::Behavior references
 * data_ov095_02137910 -- and all five targets are matched src
 * (slice_gate173.txt), so there are no state traps. All five .delta halves are
 * 0 (non-virtual complete-class form, per the overlay bytes). The state table
 * is indexed by mState, whose values 0..4 are the five records copied in
 * order: state 0 = record .a (0x02137594 -> 02136368), state 1 = .b
 * (0x0213757c -> 02136298), state 2 = .c (0x02137584 -> 02136178), state 3 = .d
 * (0x0213758c -> 02136090), state 4 = .e (0x0213759c -> 02136104).
 *
 * The host copy is transcribed line for line from
 * src/_ZN13UpDownLiftBbh8BehaviorEv.cpp; only the PMF dispatch is changed. It
 * is C-named (extern "C" _ZN13UpDownLiftBbh8BehaviorEv) rather than the mangled
 * MSVC method so it does not collide with the qualified `UpDownLiftBbh::Behavior`
 * call the vtable fill makes for the class's OTHER methods -- the Coin/WaterBomb
 * reading. Member accesses are kept exactly as the source spells them, over a
 * char* self, so no header is needed and it compiles standalone.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {
/* the five state bodies, table order = mState order; all matched src */
void func_ov095_02136368(char *c);   /* state 0 (0x02137594) */
void func_ov095_02136298(char *c);   /* state 1 (0x0213757c) */
void func_ov095_02136178(char *c);   /* state 2 (0x02137584) */
void func_ov095_02136090(char *c);   /* state 3 (0x0213758c) */
void func_ov095_02136104(char *c);   /* state 4 (0x0213759c) */

/* the Platform helpers Behavior calls, all matched src / linked base */
void *_ZN5Actor13ClosestPlayerEv(void *c);
void _ZN8Platform21UpdateModelPosAndRotYEv(void *c);
int _ZN8Platform13IsClsnInRangeE5Fix12IiES1_(void *c, int a, int b);
void _ZN8Platform19UpdateClsnPosAndRotEv(void *c);
int _ZN6Player7IsInAirEv(void *c);

struct PortPmf { unsigned fn; int delta; };
/* the RUNTIME table the dispatcher reads (bss, filled by the sinit) */
extern PortPmf data_ov095_02137910[5];
/* the five SOURCE statics __sinit_ov095_0213722c copies from, in the sinit's
   copy order = state order 0..4 */
extern PortPmf data_ov095_02137594[], data_ov095_0213757c[],
    data_ov095_02137584[], data_ov095_0213758c[], data_ov095_0213759c[];
}  /* extern "C" */

/* PORT_HOST_ABI: mwcc pointer-to-member stride on the incomplete class. */
extern "C" int _ZN13UpDownLiftBbh8BehaviorEv(void *this_)
{
    char *self = (char *)this_;
    int old;
    *(void **)(self + 0x324) = _ZN5Actor13ClosestPlayerEv(self);
    old = *(int *)(self + 0x32c);
    ((void (*)(char *))(size_t)data_ov095_02137910[old].fn)(self);
    *(unsigned short *)(self + 0x344) += 1;
    if (old != *(int *)(self + 0x32c)) *(unsigned short *)(self + 0x344) = 0;
    _ZN8Platform21UpdateModelPosAndRotYEv(self);
    if (_ZN8Platform13IsClsnInRangeE5Fix12IiES1_(self, 0, 0) != 0)
        _ZN8Platform19UpdateClsnPosAndRotEv(self);
    if (*(int *)(self + 0x32c) == 0 ||
        (unsigned)(*(int *)(self + 0x32c) - 3) <= 1) {
        void *pl = *(void **)(self + 0x320);
        if (pl != 0) {
            if (_ZN6Player7IsInAirEv(pl) == 0) {
                if (*(unsigned char *)(self + 0x348) == 0) {
                    *(int *)(self + 0x320) = 0;
                    *(unsigned char *)(self + 0x347) = 1;
                }
            }
        }
    }
    *(unsigned char *)(self + 0x348) = 0;
    if (*(void **)(self + 0x324) != 0)
        *(int *)(self + 0x330) = *(int *)(*(char **)(self + 0x324) + 0x60);
    return 1;
}

static const struct { PortPmf *slot; unsigned rom; void (*host)(char *); }
g_updownlift_states[] = {
    {data_ov095_02137594, 0x02136368, func_ov095_02136368},   /* state 0 */
    {data_ov095_0213757c, 0x02136298, func_ov095_02136298},   /* state 1 */
    {data_ov095_02137584, 0x02136178, func_ov095_02136178},   /* state 2 */
    {data_ov095_0213758c, 0x02136090, func_ov095_02136090},   /* state 3 */
    {data_ov095_0213759c, 0x02136104, func_ov095_02136104},   /* state 4 */
};

extern "C" void port_updownlift_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_updownlift_states /
                             sizeof g_updownlift_states[0]; ++i) {
        PortPmf *p = g_updownlift_states[i].slot;
        if (p->fn != g_updownlift_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: UpDownLiftBbh state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta,
                         g_updownlift_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_updownlift_states[i].host;
    }
}
