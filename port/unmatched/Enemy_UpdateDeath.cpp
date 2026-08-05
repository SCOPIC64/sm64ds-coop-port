/* HOST COPY of src/_ZN5Enemy11UpdateDeathER12WithMeshClsn.cpp -- the mwcc
 * pointer-to-member dispatch, for the eighth time in this port and the first
 * one that belongs to a BASE class rather than to a single actor.
 *
 * The matched source is
 *
 *     typedef int (Enemy::*PMF)(WithMeshClsn&);
 *     extern PMF data_ov002_0210dbc0[];
 *     ...
 *     ret = (thiz->*data_ov002_0210dbc0[deathType - 1])(*clsn);
 *
 * and the typedef is formed while `Enemy` is still INCOMPLETE (its shape is
 * declared four lines further down). MSVC answers an incomplete class with the
 * four-word GENERAL representation, so the array it indexes gets a 16-byte
 * stride where the ROM's is 8, and entry 3 of an eight-entry table reads out
 * of the middle of entry 1. mwccarm's is {function, this-delta}, two words.
 *
 * So the table is read here as what it is: eight {fn, delta} pairs, the ROM's
 * own bytes, with the same virtual-bit test the Butterfly's matched source
 * spells out for itself. Every delta in the ROM's copy is 0 and no entry has
 * the virtual bit; the seat below asserts both rather than assuming them.
 *
 * ---- and the eight statics have to be seated ------------------------------
 *
 * __sinit_ov002_02100938 (matched, in slice_gate10) copies eight {fn, delta}
 * statics out of ov002 .data into data_ov002_0210dbc0. Those statics are the
 * ROM's own words -- DS code addresses -- so the host bodies go over the
 * SOURCE side before the sinit copies them, and the sinit propagates host
 * addresses wherever the ROM propagates DS ones. This is the Rabbit's
 * treatment (hal/actor_overlays.cpp) applied to ov002.
 *
 * DEATH TYPE, one line each, in the order the table stores them:
 *   1  020ae64c  shrink while the timer runs, then the shared tail
 *   2  020ae608  wait for the ground, spawn the coin, kill and track
 *   3  020ae608  the same body (four of the eight entries share it)
 *   4  020ae4cc  the particle burst, then the coin
 *   5  020ae608
 *   6  020ae608
 *   7  020ae454  the sink-and-fade
 *   8  020aea24  the plain kill
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

void DecIfAbove0_Short(short *p);
int _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *clsn);
int _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *self, void *clsn,
                                             unsigned sel);

/* the eight ROM statics the sinit copies, and the bss table it copies into */
struct PortEnemyPmf { unsigned fn; int delta; };
extern PortEnemyPmf data_ov002_021081b0[], data_ov002_021081a8[],
    data_ov002_02108148[], data_ov002_02108198[], data_ov002_02108190[],
    data_ov002_02108188[], data_ov002_02108180[], data_ov002_02108178[];
extern PortEnemyPmf data_ov002_0210dbc0[];

int func_ov002_020ae64c(void *self, void *clsn);
int func_ov002_020ae608(void *self, void *clsn);
int func_ov002_020ae4cc(void *self, void *clsn);
int func_ov002_020ae454(void *self, void *clsn);
int func_ov002_020aea24(void *self, void *clsn);

}  /* extern "C" */

typedef int (*PortEnemyDeathFn)(void *, void *);

static const struct { PortEnemyPmf *slot; unsigned rom; PortEnemyDeathFn host; }
g_enemy_death[] = {
    {data_ov002_021081b0, 0x020ae64c, func_ov002_020ae64c},
    {data_ov002_021081a8, 0x020ae608, func_ov002_020ae608},
    {data_ov002_02108148, 0x020ae608, func_ov002_020ae608},
    {data_ov002_02108198, 0x020ae4cc, func_ov002_020ae4cc},
    {data_ov002_02108190, 0x020ae608, func_ov002_020ae608},
    {data_ov002_02108188, 0x020ae608, func_ov002_020ae608},
    {data_ov002_02108180, 0x020ae454, func_ov002_020ae454},
    {data_ov002_02108178, 0x020aea24, func_ov002_020aea24},
};

/* Both sides are seated, and that is deliberate. The sinit runs from the
   window's own boot and the registry runs later, so by the time the first
   Enemy-family class asks for this the COPY has already happened -- seating
   only the statics would leave the bss table full of DS addresses. Seating
   only the bss table would break if the sinit were ever run again. Each slot
   is checked against the ROM address it is supposed to hold and left alone if
   it is already the host body, so the order stops mattering. */
static void port_enemy_death_seat_one(PortEnemyPmf *p, unsigned rom,
                                      PortEnemyDeathFn host, const char *where,
                                      unsigned i)
{
    if (p->fn == (unsigned)(size_t)host)
        return;
    if (p->fn != rom || p->delta != 0) {
        std::fprintf(stderr, "FATAL: Enemy death state %u (%s): the mount holds "
                     "%08x/%d, the ROM's own table says %08x/0 -- WRONG BYTES\n",
                     i, where, p->fn, p->delta, rom);
        std::abort();
    }
    p->fn = (unsigned)(size_t)host;
}

extern "C" void port_enemy_death_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_enemy_death / sizeof g_enemy_death[0];
         ++i) {
        port_enemy_death_seat_one(g_enemy_death[i].slot, g_enemy_death[i].rom,
                                  g_enemy_death[i].host, "static", i);
        port_enemy_death_seat_one(&data_ov002_0210dbc0[i], g_enemy_death[i].rom,
                                  g_enemy_death[i].host, "table", i);
    }
}

extern "C" int _ZN5Enemy11UpdateDeathER12WithMeshClsn(void *thiz, void *clsn)
{
    char *c = (char *)thiz;
    int ret;
    int type = *(int *)(c + 0x10c);
    if (type == 0)
        return 0;
    DecIfAbove0_Short((short *)(c + 0x102));
    {
        const PortEnemyPmf *m = &data_ov002_0210dbc0[type - 1];
        if (m->fn & 1) {
            std::fprintf(stderr, "FATAL: Enemy death type %d is a VIRTUAL "
                         "member pointer (%08x/%d); no class in this port "
                         "stores one there\n", type, m->fn, m->delta);
            std::abort();
        }
        ret = ((PortEnemyDeathFn)(size_t)m->fn)(c + m->delta, clsn);
    }
    _ZN5Actor9UpdatePosEP12CylinderClsn(thiz, 0);
    _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(thiz, clsn, 0);
    return ret;
}
