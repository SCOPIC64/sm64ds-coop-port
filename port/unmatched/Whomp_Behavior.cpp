/* HOST COPY of src/_ZN5Whomp8BehaviorEv.cpp -- the mwcc pointer-to-member
 * dispatch read as a plain function pointer, and the twelve-entry state table
 * seated with host addresses. The Fish/Door case for gates 64-69's first
 * class, and the reasons are the ones every prior seat gives:
 *
 *   * MSVC forms a pointer-to-member of an INCOMPLETE class as the four-word
 *     general representation. The matched source writes `struct Actor;` and
 *     `typedef int (Actor::*PMF)()` before Actor is defined, then indexes
 *     `data_ov079_02128280[idx]` (an mwcc {function, delta} pair, 8 bytes) and
 *     calls `(this->**pmf)()`. Compiled by MSVC that reads a 16-byte stride and
 *     dispatches a neighbour's body. So the table is read here as what it is:
 *     the pair's first word, called as a plain function pointer.
 *   * __sinit_ov079_02127618 copies twelve {function, delta} statics from ov079
 *     0x02127bc0..0x02127c20 into the bss table data_ov079_02128280, and those
 *     statics are the overlay image's own words -- DS CODE ADDRESSES. The seat
 *     rewrites the twelve STATICS before the sinit runs (the LakituBro reading:
 *     one less mapping to get wrong), each checked against the ROM address its
 *     host body was compiled from. All twelve deltas are 0 in the ROM.
 *
 * ALL TWELVE STATES ARE MATCHED SRC -- there is no hole and no trap. WHOMP and
 * WHOMP_KING share this class and this table.
 *
 * The body is the matched source line for line; only the dispatch is spelled
 * as a plain call through the pair's first word.
 */
#include <cstdio>
#include <cstdlib>

struct PortWithMeshClsn;
struct PortCylinderClsn;

extern "C" {
int _ZN5Actor13DistToCPlayerEv(void *self);
void func_ov079_02123f34(void *self);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *c);
int _ZN5Enemy15IsGoingOffCliffER12WithMeshClsn5Fix12IiEsbbS3_(
    void *self, void *w, int a, short b, int c, int d, void *e);
void _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *self, void *w, unsigned n);
void func_ov079_02124188(void *self);
int func_ov079_021243e0(char *c, int r4);
int func_ov079_02123a8c(void *self);
void func_ov079_02124008(void *self);

extern int data_0209f318;

struct PortPmf { unsigned fn; int delta; };
/* the bss table the sinit fills, dispatched every frame */
extern PortPmf data_ov079_02128280[];

/* the twelve statics __sinit_ov079_02127618 copies into data_ov079_02128280 */
extern PortPmf data_ov079_02127bc0[], data_ov079_02127bc8[],
    data_ov079_02127bd0[], data_ov079_02127bd8[], data_ov079_02127be0[],
    data_ov079_02127be8[], data_ov079_02127bf8[], data_ov079_02127c00[],
    data_ov079_02127c08[], data_ov079_02127c10[], data_ov079_02127c18[],
    data_ov079_02127c20[];

void func_ov079_02124530(void *); void func_ov079_02124638(void *);
void func_ov079_021246d8(void *); void func_ov079_021246dc(void *);
void func_ov079_021249f0(void *); void func_ov079_02124b08(void *);
void func_ov079_02125240(void *); void func_ov079_0212538c(void *);
void func_ov079_021254b4(void *); void func_ov079_021256d4(void *);
void func_ov079_021258fc(void *); void func_ov079_02125b44(void *);
}

/* Seated over the SOURCE side, one row per static, checked against the ROM
   address the host body was compiled from. All twelve deltas are 0. */
static const struct { PortPmf *slot; unsigned rom; void (*host)(void *); }
g_whomp_states[] = {
    {data_ov079_02127bc0, 0x0212538c, func_ov079_0212538c},
    {data_ov079_02127bc8, 0x02125240, func_ov079_02125240},
    {data_ov079_02127bd0, 0x021258fc, func_ov079_021258fc},
    {data_ov079_02127bd8, 0x021254b4, func_ov079_021254b4},
    {data_ov079_02127be0, 0x02124638, func_ov079_02124638},
    {data_ov079_02127be8, 0x021246dc, func_ov079_021246dc},
    {data_ov079_02127bf8, 0x021246d8, func_ov079_021246d8},
    {data_ov079_02127c00, 0x02125b44, func_ov079_02125b44},
    {data_ov079_02127c08, 0x02124530, func_ov079_02124530},
    {data_ov079_02127c10, 0x021256d4, func_ov079_021256d4},
    {data_ov079_02127c18, 0x02124b08, func_ov079_02124b08},
    {data_ov079_02127c20, 0x021249f0, func_ov079_021249f0},
};

extern "C" void port_whomp_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_whomp_states / sizeof g_whomp_states[0];
         ++i) {
        PortPmf *p = g_whomp_states[i].slot;
        if (p->fn != g_whomp_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: Whomp state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta, g_whomp_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_whomp_states[i].host;
    }
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch; MSVC's PMF over an
 * incomplete class is the wider general representation. See the header. */
extern "C" int _ZN5Whomp8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;

    if (*(unsigned char *)(c + 0x414) != 0 && *(int *)(c + 0x3b0) != 9) {
        if (_ZN5Actor13DistToCPlayerEv(selfv) < 0x1770000) {
            *(int *)(*(int *)&data_0209f318 + 0x114) = (int)(size_t)selfv;
        }
    }

    func_ov079_02123f34(selfv);
    _ZN5Actor9UpdatePosEP12CylinderClsn(selfv, 0);

    if (*(int *)(c + 0x98) != 0) {
        if (_ZN5Enemy15IsGoingOffCliffER12WithMeshClsn5Fix12IiEsbbS3_(
                selfv, c + 0x110, 0x3c000, (short)0x2888, 0, 0,
                (void *)0x32000)) {
            *(int *)(c + 0x5c) = *(int *)(c + 0x3d4);
            *(int *)(c + 0x60) = *(int *)(c + 0x3d8);
            *(int *)(c + 0x64) = *(int *)(c + 0x3dc);
        } else {
            *(int *)(c + 0x3d4) = *(int *)(c + 0x5c);
            *(int *)(c + 0x3d8) = *(int *)(c + 0x60);
            *(int *)(c + 0x3dc) = *(int *)(c + 0x64);
        }
    } else {
        *(int *)(c + 0x3d4) = *(int *)(c + 0x5c);
        *(int *)(c + 0x3d8) = *(int *)(c + 0x60);
        *(int *)(c + 0x3dc) = *(int *)(c + 0x64);
    }

    _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(selfv, c + 0x110, 0);

    {
        int idx = *(int *)(c + 0x3b0);
        ((void (*)(void *))(size_t)data_ov079_02128280[idx].fn)(selfv);

        {
            unsigned short *ctr = (unsigned short *)(c + 0x100);
            *ctr = *ctr + 1;
            if (idx != *(int *)(c + 0x3b0)) {
                *ctr = 0;
                *(unsigned char *)(c + 0x40c) = 0;
            }
        }
    }

    func_ov079_02124188(selfv);

    if (func_ov079_021243e0(c, 0) == 0 || func_ov079_02123a8c(selfv) != 0) {
        func_ov079_02124008(selfv);
    }

    *(unsigned char *)(c + 0x403) = 0;
    return 1;
}
