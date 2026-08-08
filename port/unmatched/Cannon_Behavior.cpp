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
 * ov098 0x0213ade8 IS NOT DECOMPILED -- state 1, the one the lid runs while
 * it is opening -- and it is seated by name. The castle grounds' cannon takes
 * state 0 or state 2 out of its own InitResources depending on the spawn
 * record's low two bits, and its Behavior does not dispatch at all while
 * those bits read 1, so on this level the trap is never reached.
 *
 * A LEVEL DID GET THERE. Bob-omb Battlefield carries eight cannons and one of
 * them enters state 1 inside the first forty frames, which used to abort the
 * whole run before the level could be walked at all, then held the hole open
 * as a one-shot report while the lid stopped part-way through opening.
 *
 * IT NOW HAS A HOST BODY. port_cannon_state_0213ade8 below is a
 * LOGIC-VERIFIED NEAR-MISS host copy of ov098 0x0213ade8, NOT matched src:
 * the near-miss C is six instruction-schedule divergences from byte-perfect
 * (nearmiss/db.jsonl, source log_attempt, div=6), semantics verified, and the
 * byte-perfect match is still open. It reads as a plain function over `self`
 * (the same PORT_HOST_ABI treatment the rest of this file's dispatch uses):
 * the aim-timer decrement, the closest-player range gate, the state==0 reset,
 * the state==0x3c bomb-spawn arm that reads the sine table and launches the
 * fired bob-omb, and the lid-angle integrator otherwise. Every symbol it
 * closes over is compiled by the gate-19 slice or seated
 * (data_ov098_0213c63c in port/ov098_syms.txt, data_02082214 via
 * hal/cxx_aliases.cpp). Restore the matched-src note the moment 0x0213ade8 is
 * byte-perfect; the near-miss is a hole held open with the right logic, not a
 * decision.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

int func_ov098_0213a984(void *self);
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN12CylinderClsn6UpdateEv(void *self);

/* ---- what port_cannon_state_0213ade8 (ov098 0x0213ade8) closes over ---- */
struct PortV16 { unsigned short x, y, z; };
extern short data_02082214[];              /* the arm9 sine/cosine table */
extern struct PortV16 data_ov098_0213c63c; /* the fired bob-omb's rot template */
int _ZN5Actor13ClosestPlayerEv(void *self);
int Vec3_Dist(void *a, void *b);
void func_ov098_0213b15c(void *self);
void _ZN5Sound9PlayBank3EjRK7Vector3(unsigned int id, void *pos);
void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
    unsigned int a, unsigned int b, void *pos, void *rot, int area, int death);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *self, void *c);
void _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
    unsigned int slot, unsigned int unk, int x, int y, int z, void *rot,
    void *cb);

struct PortPmf { unsigned fn; int delta; };
/* the four statics __sinit_ov098_0213c214 copies into data_ov098_0213c8fc */
extern PortPmf data_ov098_0213c644[], data_ov098_0213c64c[],
    data_ov098_0213c654[], data_ov098_0213c65c[];
extern PortPmf data_ov098_0213c8fc[];

void func_ov098_0213aa28(void *);   /* state 3 */
void func_ov098_0213ad08(void *);   /* state 2, the closed lid */
void func_ov098_0213b0a4(void *);   /* state 0, the aim */

}  /* extern "C" */

/* LOGIC-VERIFIED NEAR-MISS host copy of ov098 0x0213ade8 (Cannon state 1, the
   lid opening), div=6, semantics verified; byte-perfect match still open. The
   near-miss C is C99 with a char-pointer self; adapted here to the file's
   void-pointer dispatch shape. Not matched src. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
static void port_cannon_state_0213ade8(void *selfv)
{
    char *self = (char *)selfv;
    struct PortV16 rot;
    struct { int x, y, z; } pos;
    volatile int vel[3];
    int state;
    int closest;

    if (*(int *)(self + 0x174) != 0) {
        int *p = (int *)(int)(((long long)(int)(self + 0x174)) & 0xFFFFFFFFFFFFFFFFLL);
        *p = *p - 1;
    }

    closest = _ZN5Actor13ClosestPlayerEv(self);
    if (Vec3_Dist(self + 0x5c, (char *)closest + 0x5c) >= 0x800000) {
        return;
    }

    state = *(int *)(self + 0x174);
    if (state == 0) {
        *(int *)(self + 0x180) = 0;
        func_ov098_0213b15c(self);
        _ZN5Sound9PlayBank3EjRK7Vector3(0x14c, self + 0x74);
        return;
    }

    if (state == 0x3c) {
        char *a;
        short *tbl;
        int k;
        short ang;
        int idx;

        pos.x = *(int *)(self + 0x5c);
        pos.y = *(int *)(self + 0x60);
        pos.z = *(int *)(self + 0x64);
        pos.y = pos.y + 0x80000;

        {
            void *sp = _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
                0xd0, 3, &pos, 0, *(signed char *)(self + 0xcc), -1);
            short *t = data_02082214;
            int kk = 0x64;
            short pitch = *(short *)(self + 0x92);
            int tval = (short)(0x4000 - pitch);
            int i = (unsigned short)tval >> 4;
            int s = t[i * 2];
            tbl = t;
            k = kk;
            a = (char *)sp;
            *(int *)(a + 0xa4) = 0;
            *(int *)(a + 0xa8) = s * kk;
            *(int *)(a + 0xac) = 0;
        }

        ang = (short)(0x4000 - *(short *)(self + 0x92));
        idx = (unsigned short)ang >> 4;
        *(int *)(a + 0x98) = tbl[idx * 2 + 1] * k;

        {
            short yaw = *(short *)(self + 0x94);
            *(short *)(a + 0x92) = 0;
            *(short *)(a + 0x94) = yaw;
            *(short *)(a + 0x96) = 0;
        }

        _ZN5Actor9UpdatePosEP12CylinderClsn(a, 0);

        {
            int *vp = (int *)(int)(((long long)(int)(a + 0xa4)) & 0xFFFFFFFFFFFFFFFFLL);
            int vx = *vp;
            int px = pos.x;
            vel[0] = vx;
            int vy = *(vp + 1);
            int py = pos.y;
            vel[1] = vy;
            int vz = *(vp + 2);
            int y = py + (vy << 1);
            int pz = pos.z;
            vel[2] = vz;
            int x = px + (vx << 1);
            *(int *)(self + 0x16c) = y;  /* store y first */
            *(int *)(self + 0x168) = x;
            *(int *)(self + 0x170) = pz + (vz << 1);
        }

        rot = data_ov098_0213c63c;
        rot.x = (unsigned short)data_02082214[(*(unsigned short *)(self + 0x94) >> 4) * 2];
        rot.z = (unsigned short)data_02082214[(*(unsigned short *)(self + 0x94) >> 4) * 2 + 1];

        _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
            0, 7, *(int *)(self + 0x168), *(int *)(self + 0x16c), *(int *)(self + 0x170),
            &rot, 0);
        _ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
            0, 8, *(int *)(self + 0x168), *(int *)(self + 0x16c), *(int *)(self + 0x170),
            &rot, 0);

        _ZN5Sound9PlayBank3EjRK7Vector3(0xd4, self + 0x74);
        *(int *)(self + 0x18c) = -0x1800;
        return;
    }

    {
        int *p188 = (int *)(int)(((long long)(int)(self + 0x188)) & 0xFFFFFFFFFFFFFFFFLL);
        int *p18c = (int *)(int)(((long long)(int)(self + 0x18c)) & 0xFFFFFFFFFFFFFFFFLL);
        *p188 = *p188 + *(int *)(self + 0x18c);
        *p18c = *p18c + 0xc00;
    }
    if (*(int *)(self + 0x18c) >= 0x800) {
        *(int *)(self + 0x18c) = 0x800;
    }
    if (*(int *)(self + 0x188) >= 0) {
        *(int *)(self + 0x18c) = 0;
        *(int *)(self + 0x188) = *(int *)(self + 0x18c);
    }
}

static const struct { PortPmf *slot; unsigned rom; void (*host)(void *); }
g_cannon_states[] = {
    {data_ov098_0213c644, 0x0213aa28, func_ov098_0213aa28},
    {data_ov098_0213c64c, 0x0213b0a4, func_ov098_0213b0a4},
    {data_ov098_0213c654, 0x0213ad08, func_ov098_0213ad08},
    {data_ov098_0213c65c, 0x0213ade8, port_cannon_state_0213ade8},
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
