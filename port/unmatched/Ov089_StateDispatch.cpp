/* THE BOSS REWARD'S STATE MACHINE -- ov089's one pointer-to-member dispatch
 * table, seated with host bodies, and a host copy of the method that reads it.
 *
 * run linkw wave 6, lane w6-A. The long version of the reading is in
 * port/unmatched/Ov060_StateDispatch.cpp and port/unmatched/Crate_StateDispatch
 * .cpp; this is the same shape one overlay over.
 *
 * ==== THE TABLE =============================================================
 *
 * data_ov089_02132cec, bss, EIGHT 8-byte {code, adj} records, filled by
 * __sinit_ov089_021328d4 out of eight .data source statics at 0x02132b00..
 * 0x02132b38, and read by Key::Behavior with `PMFTABLE[mState].pmf`. Read from
 * extracted/overlays/overlay_0089.bin (base 0x02130f00) and cross-checked
 * against config/arm9/overlays/ov089/relocs.txt: all eight adj halves are 0,
 * and the eight records name only THREE distinct bodies --
 *
 *   state 0,1,2,4,5,6  func_ov089_02131b18   (the idle/hover body)
 *   state 3            func_ov089_0213162c
 *   state 7            func_ov089_021311c0
 *
 * -- all three matched src, all three in port/slice_w6a.txt.
 *
 * ==== WHY Key::Behavior IS A HOST COPY ======================================
 *
 * src/_ZN3Key8BehaviorEv.cpp forms its pointer-to-member over
 * `struct C { virtual void dummy(); };` -- a COMPLETE single-inheritance class
 * with a vtable, for which MSVC's representation is FOUR bytes against the ROM
 * record's eight. Measured, the same way the ov060 five were: state 0 reads
 * correctly and every other state calls through an adj word (zero). On top of
 * the stride, a pointer-to-member call is thiscall and every state body here
 * is a cdecl func_ov089_xxxxxxxx(char *).
 *
 * The body below is the matched source's control flow line for line, offsets
 * from include/Key.h; only the one dispatch at its line 114 is respelled.
 * BOTH KEY (282) and LAST_STAR (283) run it -- they share _ZTV3Key, the only
 * vtable ov089 defines, and the mActorID == 0x11a test at the end is how the
 * body itself tells the two apart.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

struct PortPmf { unsigned fn; int adj; };

/* the eight source statics, in the sinit's copy order = table order 0..7 */
extern PortPmf data_ov089_02132b28[], data_ov089_02132b38[],
    data_ov089_02132b00[], data_ov089_02132b30[], data_ov089_02132b20[],
    data_ov089_02132b10[], data_ov089_02132b08[], data_ov089_02132b18[];
/* the runtime table (bss, filled by the sinit) */
extern PortPmf data_ov089_02132cec[];

/* the three state bodies, all matched src */
void func_ov089_02131b18(char *c);
void func_ov089_0213162c(char *c);
void func_ov089_021311c0(char *c);

/* what the host copy calls */
int _ZN9Animation8FinishedEv(void *a);
void _ZN9Animation7AdvanceEv(void *a);
void Matrix4x3_FromTranslation(void *m, int x, int y, int z);
void Matrix4x3_ApplyInPlaceToRotationY(void *m, int ang);
void MulMat4x3Mat4x3(void *d, void *a, void *b);
void SubVec3(void *d, void *a, void *b);
void Vec3_LslInPlace(void *v, int sh);
void AddVec3(void *d, void *a, void *b);
void *_ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
    unsigned a, unsigned b, int x, int y, int z, const void *v, void *cb);
void _ZN9ActorBase18MarkForDestructionEv(void *c);
int _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(void *c, void *w);
void _ZN12CylinderClsn5ClearEv(void *c);
void _ZN12CylinderClsn6UpdateEv(void *c);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *c, void *cyl);
void _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(
    void *c, void *v);
void func_ov089_02131f54(char *c);
extern char data_020a0e68;
extern int data_ov089_02132c40[];
extern int data_ov089_02132b40[];
extern int data_ov089_02132ca4[];

/* PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
int _ZN3Key8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    int vec[3];
    int p7[3];
    int pe[3];
    int v = *(int *)(c + 0x448);

    if (v != 0) {
        if (v == 3) {
            char *o = *(char **)(c + 0x110);
            if (o != 0) {
                int *s = (int *)(o + 0x5c);
                *(int *)(c + 0x5c) = s[0];
                *(int *)(c + 0x60) = s[1];
                *(int *)(c + 0x64) = s[2];
                *(short *)(c + 0x8e) =
                    *(short *)(*(char **)(c + 0x110) + 0x8e);
            }
            if (_ZN9Animation8FinishedEv(c + 0x164) == 0) {
                Matrix4x3_FromTranslation(&data_020a0e68, *(int *)(c + 0x5c),
                                          *(int *)(c + 0x60),
                                          *(int *)(c + 0x64));
                Matrix4x3_ApplyInPlaceToRotationY(&data_020a0e68,
                                                  *(short *)(c + 0x8e));
                MulMat4x3Mat4x3(*(void **)(c + 0x128), &data_020a0e68,
                                &data_020a0e68);
                {
                    char *m = &data_020a0e68;
                    vec[2] = *(int *)(m + 0x2c);
                    vec[0] = *(int *)(m + 0x24);
                    vec[1] = *(int *)(m + 0x28);
                }
                SubVec3(vec, c + 0x5c, vec);
                Vec3_LslInPlace(vec, 3);
                AddVec3(vec, c + 0x5c, vec);
                vec[1] = *(int *)(*(char **)(c + 0x124) + 0xc) * 0x23 + vec[1];
                vec[1] = vec[1] - 0x48000;
                *(void **)(c + 0x464) =
                    _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
                        *(unsigned *)(c + 0x464), 0x82, vec[0], vec[1], vec[2],
                        0, 0);
                *(void **)(c + 0x468) =
                    _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
                        *(unsigned *)(c + 0x468), 0x83, vec[0], vec[1], vec[2],
                        0, 0);
            }
        }

        _ZN9Animation7AdvanceEv(c + 0x164);
        func_ov089_02131f54(c);
        if (_ZN9Animation8FinishedEv(c + 0x164)) {
            if (*(unsigned short *)(c + 0xc) == 0x11a) {
                if (*(int *)(c + 0x174) != data_ov089_02132c40[1])
                    _ZN9ActorBase18MarkForDestructionEv(c);
            }
        }
        return 1;
    }

    if (_ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(c, c + 0x260)) {
        func_ov089_02131f54(c);
        _ZN12CylinderClsn5ClearEv(c + 0x220);
        return 1;
    }
    *(int *)(c + 0xd0) = 0;
    if (*(short *)(c + 0x440) > 0x400)
        *(short *)(c + 0x440) = (short)(*(short *)(c + 0x440) - 0x100);
    else if (*(short *)(c + 0x440) == 0)
        *(short *)(c + 0x440) = 0x400;
    *(short *)(c + 0x8e) = (short)(*(short *)(c + 0x8e) +
                                   *(short *)(c + 0x440));
    _ZN5Actor9UpdatePosEP12CylinderClsn(c, 0);
    {
        PortPmf *e = &data_ov089_02132cec[*(int *)(c + 0x444)];
        ((void (*)(char *))(size_t)e->fn)(c + (e->adj >> 1));
    }
    func_ov089_02131f54(c);
    _ZN12CylinderClsn5ClearEv(c + 0x220);
    if (*(int *)(c + 0x444) == 7) {
        p7[0] = data_ov089_02132b40[0];
        p7[1] = data_ov089_02132b40[1];
        p7[2] = data_ov089_02132b40[2];
        _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(
            c + 0x220, p7);
    } else {
        pe[0] = data_ov089_02132ca4[0];
        pe[1] = data_ov089_02132ca4[1];
        pe[2] = data_ov089_02132ca4[2];
        _ZN25MovingCylinderClsnWithPos21SetPosRelativeToActorERK7Vector3(
            c + 0x220, pe);
    }
    _ZN12CylinderClsn6UpdateEv(c + 0x220);
    return 1;
}

}  /* extern "C" */

namespace {
struct Seat { PortPmf *slot; unsigned rom; void (*host)(char *); const char *tab; };
const Seat g_ov089_states[] = {
    {data_ov089_02132b28, 0x02131b18, func_ov089_02131b18, "cec[0]"},
    {data_ov089_02132b38, 0x02131b18, func_ov089_02131b18, "cec[1]"},
    {data_ov089_02132b00, 0x02131b18, func_ov089_02131b18, "cec[2]"},
    {data_ov089_02132b30, 0x0213162c, func_ov089_0213162c, "cec[3]"},
    {data_ov089_02132b20, 0x02131b18, func_ov089_02131b18, "cec[4]"},
    {data_ov089_02132b10, 0x02131b18, func_ov089_02131b18, "cec[5]"},
    {data_ov089_02132b08, 0x02131b18, func_ov089_02131b18, "cec[6]"},
    {data_ov089_02132b18, 0x021311c0, func_ov089_021311c0, "cec[7]"},
};
}  /* namespace */

/* Seats the SOURCE statics before __sinit_ov089_021328d4 copies them. That
   sinit runs from hal/actor_overlays.cpp's ov089 block, which is where the
   call goes -- ov089 is the one overlay in this lane that was already brought
   up, for the castle doors' key models (port/ov089_syms.txt's header). */
extern "C" void port_ov089_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_ov089_states / sizeof g_ov089_states[0];
         ++i) {
        PortPmf *p = g_ov089_states[i].slot;
        if (p->fn != g_ov089_states[i].rom || p->adj != 0) {
            std::fprintf(stderr, "FATAL: ov089 state %s: the mount holds "
                         "%08x/%d, the ROM's own record says %08x/0 -- WRONG "
                         "BYTES\n", g_ov089_states[i].tab, p->fn, p->adj,
                         g_ov089_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_ov089_states[i].host;
    }
}
