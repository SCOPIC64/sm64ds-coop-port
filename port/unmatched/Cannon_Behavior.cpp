/* HOST COPY of src/_ZN6Cannon8BehaviorEv.cpp -- the mwcc pointer-to-member
 * dispatch read as a plain function pointer, and the table seated with host
 * addresses. The gate-16 case for the fourth time
 * (OneUpMushroom_Behavior.cpp, SignPost_StateDispatch.cpp,
 * LakituBro_Behavior.cpp), and the reasons are unchanged:
 *
 *   * MSVC forms a pointer-to-member of an INCOMPLETE class as the four-word
 *     general representation, which doubles this table's stride and
 *     dispatches a neighbour's body. The source TU declares `struct C;`
 *     before the typedef, so it hits that.
 *   * __sinit_ov098_0213c214 copies four {function, delta} statics into
 *     data_ov098_0213c8fc, and those are the overlay image's own words --
 *     DS CODE ADDRESSES. The seat rewrites the four STATICS before the sinit
 *     runs (the LakituBro reading: same guarantee, one less mapping to get
 *     wrong), each checked against the ROM address its host body was compiled
 *     from.
 *
 * ov098 0x0213ade8 (state 1, the lid opening) is MATCHED SRC now:
 * src/func_ov098_0213ade8.c went byte-perfect at 2004/b56 (PR #1227,
 * linkcheck VERIFIED) and the gate-19 slice compiles it. This file no
 * longer carries any state body -- only the PMF-dispatch frame and the
 * seat, which remain PORT_HOST_ABI for the incomplete-class PMF reason
 * above. The seat routes all four states at their ROM addresses to the
 * linked matched bodies.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

int func_ov098_0213a984(void *self);
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN12CylinderClsn6UpdateEv(void *self);

struct PortPmf { unsigned fn; int delta; };
/* the four statics __sinit_ov098_0213c214 copies into data_ov098_0213c8fc */
extern PortPmf data_ov098_0213c644[], data_ov098_0213c64c[],
    data_ov098_0213c654[], data_ov098_0213c65c[];
extern PortPmf data_ov098_0213c8fc[];

void func_ov098_0213aa28(void *);   /* state 3 */
void func_ov098_0213ad08(void *);   /* state 2, the closed lid */
void func_ov098_0213b0a4(void *);   /* state 0, the aim */
void func_ov098_0213ade8(void *);   /* state 1, the lid opening (matched src) */

}  /* extern "C" */

static const struct { PortPmf *slot; unsigned rom; void (*host)(void *); }
g_cannon_states[] = {
    {data_ov098_0213c644, 0x0213aa28, func_ov098_0213aa28},
    {data_ov098_0213c64c, 0x0213b0a4, func_ov098_0213b0a4},
    {data_ov098_0213c654, 0x0213ad08, func_ov098_0213ad08},
    {data_ov098_0213c65c, 0x0213ade8, func_ov098_0213ade8},
};

extern "C" void port_cannon_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_cannon_states / sizeof g_cannon_states[0];
         ++i) {
        PortPmf *p = g_cannon_states[i].slot;
        if (p->fn != g_cannon_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: Cannon state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta, g_cannon_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_cannon_states[i].host;
    }
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch; MSVC's PMF over an
 * incomplete class is the wider general representation. See the header. */
extern "C" int _ZN6Cannon8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(unsigned char *)(c + 0x184) != 1) {
        unsigned idx = *(unsigned *)(c + 0x180);
        if (idx >= 4) {
            std::fprintf(stderr, "FATAL: Cannon state %u out of range\n", idx);
            std::abort();
        }
        ((void (*)(void *))(size_t)data_ov098_0213c8fc[idx].fn)(c);
    }
    func_ov098_0213a984(c);
    _ZN12CylinderClsn5ClearEv(c + 0x124);
    _ZN12CylinderClsn6UpdateEv(c + 0x124);
    return 1;
}
