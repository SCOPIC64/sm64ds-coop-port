/* HOST COPY of src/_ZN10BulletBill8BehaviorEv.cpp -- the mwcc pointer-to-member
 * dispatch read as a plain function pointer, and the two-entry state table
 * seated with host addresses. Whomp_Behavior's twin one class over, same
 * reasons:
 *
 *   * MSVC forms a pointer-to-member of an INCOMPLETE class as the four-word
 *     general representation. The matched source writes `struct Klass;` and
 *     `typedef void (Klass::*PMF)()`, then indexes `data_ov079_021282e0[which]`
 *     (an mwcc {function, delta} pair) and calls `(this->*(m->pmf))()`. Read
 *     here as the pair's first word, called as a plain function pointer.
 *   * __sinit_ov079_021279d4 copies two {function, delta} statics from ov079
 *     0x02127ea4 (entry 0) and 0x02127e9c (entry 1) into the bss table
 *     data_ov079_021282e0. Those statics are the overlay image's own words --
 *     DS CODE ADDRESSES -- so the seat rewrites the two STATICS before the
 *     sinit runs, each checked against the ROM address its host body was
 *     compiled from. Both deltas are 0.
 *
 * BOTH STATES ARE MATCHED SRC (func_ov079_0212682c and func_ov079_02126794).
 *
 * The body is the matched source line for line; only the dispatch is spelled
 * as a plain call through the pair's first word.
 */
#include <cstdio>
#include <cstdlib>

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

struct PortVec3 { int x, y, z; };

extern "C" {
void func_0200f760(void *self, void *cc);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *cc);
void *_ZN5Actor10FindWithIDEj(u32 id);
void _ZN6Player16IncMegaKillCountEv();
void _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(u32, int, int, int);
void _ZN9ActorBase18MarkForDestructionEv(void *self);
void func_02012694(int a, void *b);
void _ZN5Sound9PlayBank0EjRK7Vector3(u32 id, const PortVec3 *pos);
void _ZN6Player4HurtERK7Vector3j5Fix12IiEjjj(void *self, const PortVec3 *pos,
    u32 a, int b, u32 c, u32 d, u32 e);
void func_ov079_02126704(char *c);
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(
    void *self, const PortVec3 *pos);
void _ZN12CylinderClsn6UpdateEv(void *self);

struct PortPmf { unsigned fn; int delta; };
extern PortPmf data_ov079_021282e0[];       /* the bss table, dispatched each frame */
extern PortPmf data_ov079_02127e9c[];       /* entry 1 source static */
extern PortPmf data_ov079_02127ea4[];       /* entry 0 source static */

void func_ov079_0212682c(void *);           /* state 0 */
void func_ov079_02126794(void *);           /* state 1 */
}

static const struct { PortPmf *slot; unsigned rom; void (*host)(void *); }
g_bullet_bill_states[] = {
    {data_ov079_02127ea4, 0x0212682c, func_ov079_0212682c},
    {data_ov079_02127e9c, 0x02126794, func_ov079_02126794},
};

extern "C" void port_bullet_bill_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0;
         i < sizeof g_bullet_bill_states / sizeof g_bullet_bill_states[0]; ++i) {
        PortPmf *p = g_bullet_bill_states[i].slot;
        if (p->fn != g_bullet_bill_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: BulletBill state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta,
                         g_bullet_bill_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_bullet_bill_states[i].host;
    }
}

extern "C" int _ZN10BulletBill8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    int flags;
    u32 which;
    u32 id;

    func_0200f760(c, c + 0x110);
    _ZN5Actor9UpdatePosEP12CylinderClsn(c, 0);

    which = *(u32 *)(c + 0x3d4);
    ((void (*)(void *))(size_t)data_ov079_021282e0[which].fn)(c);

    id = *(u32 *)(c + 0x134);
    if (id != 0) {
        flags = *(int *)(c + 0x130);
        if (flags & 0x10) {
            _ZN5Actor10FindWithIDEj(id);
            _ZN6Player16IncMegaKillCountEv();
            _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(
                0x8f, *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64));
            _ZN9ActorBase18MarkForDestructionEv(c);
            {
                void *p = c + 0x74;
                int n = 0x78;
                func_02012694(n, p);
            }
        } else if (flags & 0x3c0) {
            *(int *)(c + 0x3d4) = 1;
            {
                PortVec3 *pos = (PortVec3 *)(c + 0x74);
                _ZN5Sound9PlayBank0EjRK7Vector3(0xb5, pos);
            }
        } else {
            void *o = _ZN5Actor10FindWithIDEj(id);
            if (o != 0) {
                int eq = (*(u16 *)((char *)o + 0xc) == 0xbf);
                if (eq != 0) {
                    if (*(u8 *)((char *)o + 0x6fb) == 0) {
                        PortVec3 pos;
                        pos.x = *(int *)(c + 0x5c);
                        pos.y = *(int *)(c + 0x60);
                        pos.z = *(int *)(c + 0x64);
                        _ZN6Player4HurtERK7Vector3j5Fix12IiEjjj(
                            o, &pos, 3, 0xc000, 1, 0, 1);
                        *(int *)(c + 0x3d4) = 1;
                        {
                            PortVec3 *p2 = (PortVec3 *)(c + 0x74);
                            _ZN5Sound9PlayBank0EjRK7Vector3(0xb5, p2);
                        }
                    }
                }
            }
        }
    }

    {
        u16 *h = (u16 *)(c + 0x100);
        *h = *h + 1;
        if (which != *(u32 *)(c + 0x3d4)) {
            *h = 0;
        }
    }

    func_ov079_02126704(c);

    _ZN12CylinderClsn5ClearEv(c + 0x110);
    {
        PortVec3 pos;
        pos.x = 0;
        pos.y = -0x50000;
        pos.z = 0;
        _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(
            c + 0x110, &pos);
    }
    _ZN12CylinderClsn6UpdateEv(c + 0x110);

    *(short *)(c + 0x92) = *(short *)(c + 0x8c);
    *(short *)(c + 0x94) = *(short *)(c + 0x8e);
    *(short *)(c + 0x96) = *(short *)(c + 0x90);
    return 1;
}
