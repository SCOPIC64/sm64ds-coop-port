/* HOST COPY of src/_ZN4Fish8BehaviorEv.cpp -- the mwcc pointer-to-member
 * dispatch read as a plain function pointer, and the table seated with host
 * addresses. The gate-16 case for the fifth time (OneUpMushroom_Behavior.cpp,
 * SignPost_StateDispatch.cpp, LakituBro_Behavior.cpp, Cannon_Behavior.cpp),
 * and the reasons are unchanged:
 *
 *   * MSVC forms a pointer-to-member of an INCOMPLETE class as the four-word
 *     general representation, which quadruples this table's stride and
 *     dispatches a neighbour's body. The source TU writes `struct C;` before
 *     the typedef and only defines C further down, so it hits that squarely.
 *   * __sinit_ov100_02147bc0 copies seven {function, delta} statics from
 *     ov100 0x02148448..0x02148478 into data_ov100_02148a1c, and those are
 *     the overlay image's own words -- DS CODE ADDRESSES. The seat rewrites
 *     the seven STATICS before the sinit runs (the LakituBro reading: same
 *     guarantee, one less mapping to get wrong), each checked against the ROM
 *     address its host body was compiled from.
 *
 * ALL SEVEN STATES ARE MATCHED SRC -- there is no hole here and no trap. The
 * fish has nowhere to fall through to.
 *
 * The body is the matched source's line for line; only the dispatch is
 * spelled as a plain call through the pair's first word.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

struct PortVec3 { int x, y, z; };
struct PortM48 { int w[12]; };
extern PortM48 data_020a0e68;

int *_ZN5Actor10FindWithIDEj(unsigned id);
void _ZN9ActorBase18MarkForDestructionEv(void *self);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *clsn);
void Vec3_Asr(PortVec3 *d, PortVec3 *s, int sh);
void Matrix4x3_FromTranslation(PortM48 *m, int x, int y, int z);
void Matrix4x3_ApplyInPlaceToRotationY(PortM48 *m, short angY);
void _ZN9Animation7AdvanceEv(void *anim);
int func_ov100_0214639c(void *other);

struct PortPmf { unsigned fn; int delta; };
/* the seven statics __sinit_ov100_02147bc0 copies into data_ov100_02148a1c */
extern PortPmf data_ov100_02148448[], data_ov100_02148450[],
    data_ov100_02148458[], data_ov100_02148460[], data_ov100_02148468[],
    data_ov100_02148470[], data_ov100_02148478[];
extern PortPmf data_ov100_02148a1c[];

void func_ov100_021463b0(void *);
void func_ov100_02146468(void *);
void func_ov100_021464f4(void *);
void func_ov100_02146640(void *);
void func_ov100_021467d4(void *);
void func_ov100_021467e8(void *);
void func_ov100_02146828(void *);

}  /* extern "C" */

static const struct { PortPmf *slot; unsigned rom; void (*host)(void *); }
g_fish_states[] = {
    {data_ov100_02148448, 0x021467d4, func_ov100_021467d4},
    {data_ov100_02148450, 0x02146828, func_ov100_02146828},
    {data_ov100_02148458, 0x021464f4, func_ov100_021464f4},
    {data_ov100_02148460, 0x021467e8, func_ov100_021467e8},
    {data_ov100_02148468, 0x02146640, func_ov100_02146640},
    {data_ov100_02148470, 0x02146468, func_ov100_02146468},
    {data_ov100_02148478, 0x021463b0, func_ov100_021463b0},
};

extern "C" void port_fish_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_fish_states / sizeof g_fish_states[0];
         ++i) {
        PortPmf *p = g_fish_states[i].slot;
        if (p->fn != g_fish_states[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: Fish state %u: the mount holds "
                         "%08x/%d, the ROM's own table says %08x/0 -- WRONG "
                         "BYTES\n", i, p->fn, p->delta, g_fish_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_fish_states[i].host;
    }
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch; MSVC's PMF over an
 * incomplete class is the wider general representation. See the header. */
extern "C" int _ZN4Fish8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    PortVec3 v;
    int *r;
    unsigned idx = (unsigned)*(int *)(c + 0x14c);
    if (idx >= 7) {
        std::fprintf(stderr, "FATAL: Fish state %u out of range\n", idx);
        std::abort();
    }
    if (*(unsigned char *)(c + 0x159) != 0) {
        ((void (*)(void *))(size_t)data_ov100_02148a1c[idx].fn)(c);
    } else {
        r = _ZN5Actor10FindWithIDEj(*(unsigned *)(c + 0x13c));
        if (r == 0 || func_ov100_0214639c(r) != 0) {
            _ZN9ActorBase18MarkForDestructionEv(c);
        } else {
            ((void (*)(void *))(size_t)data_ov100_02148a1c[idx].fn)(c);
            _ZN5Actor9UpdatePosEP12CylinderClsn(c, 0);
            *(int *)(c + 0x150) += 1;
        }
        Vec3_Asr(&v, (PortVec3 *)(c + 0x5c), 3);
        Matrix4x3_FromTranslation(&data_020a0e68, v.x, v.y, v.z);
        *(short *)(c + 0x8e) = *(short *)(c + 0x94);
        Matrix4x3_ApplyInPlaceToRotationY(&data_020a0e68, *(short *)(c + 0x8e));
        *(PortM48 *)(c + 0xf0) = data_020a0e68;
        _ZN9Animation7AdvanceEv(c + 0x124);
    }
    return 1;
}
