/* HOST COPY of src/_ZN4Coin8BehaviorEv.cpp -- the coin's behaviour-state
 * dispatch read as a plain function pointer, and the table seated with host
 * addresses.
 *
 * Coin is one class serving three actor ids (288 Coin, 289 RedCoin, 290
 * BlueCoin) and nine behaviour states, and Behavior picks the state it is in
 * by indexing data_ov002_0210dc70 with mBehaviorType (+0x3a4).
 * data_ov002_0210dc70 is ov002 BSS, 0x48 bytes -- nine mwcc POINTER-TO-MEMBER
 * pairs -- and __sinit_ov002_02100c50 fills it out of the nine statics at
 * ov002 0x02108730..0x02108770. Every one is the plain nonvirtual
 * { function, 0 } form, so the host reads the first word back as a function
 * taking `this` and ignores the second: the OneUpMushroom's treatment
 * (port/unmatched/OneUpMushroom_Behavior.cpp), which carries the long version
 * of why MSVC cannot be handed the source line as written.
 *
 * THE SEAT IS THE SAME TWO-PART CHECK. The sinit really does run -- it is
 * matched src and it is on the gate-10 boot list -- but what it copies are
 * the ov002 image's own words, which are DS CODE ADDRESSES. The seat below
 * rewrites each pair's first word with the host body after checking that the
 * word the sinit left is the ROM address that body was compiled from, so a
 * mount pointing at the wrong bytes says so instead of calling into the
 * overlay image.
 *
 * TWO OF THE NINE ENTRIES ARE DUPLICATES and that is the ROM's own table, not
 * a transcription slip: states 2 and 6 are both ov002 0x020b1cc0, and states
 * 1 and 7 are both 0x020b20b4. Seven distinct bodies, nine slots.
 *
 * The body below is the matched source's control flow line for line: the
 * pickup-sound flag, the blue-coin area retire, the two early outs, the spin,
 * the state call, and the two cylinder clear/update arms.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

void _ZN5Sound9PlayBank3EjRK7Vector3(unsigned id, void *pos);
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN12CylinderClsn6UpdateEv(void *self);
int LenVec3(void *v);
extern unsigned char data_0209f2d8;      /* the mega-char state byte */

int func_ov002_020b10a0(void *self);     /* "am I collected / gone" */
int func_ov002_020b12ec(void *self);     /* "am I in the retire path" */
void func_ov002_020b14d8(void *self);
void func_ov002_020b10e4(void *self);
int func_ov002_020b19dc(void *self);

/* the nine states, in the order __sinit_ov002_02100c50 seats them */
void func_ov002_020b2150(void *);
void func_ov002_020b20b4(void *);
void func_ov002_020b1cc0(void *);
void func_ov002_020b1bfc(void *);
void func_ov002_020b2070(void *);
void func_ov002_020b1ad4(void *);
void func_ov002_020b1a60(void *);

struct PortPmf { unsigned fn; int delta; };
extern PortPmf data_ov002_0210dc70[];

}  /* extern "C" */

enum { PORT_COIN_STATES = 9 };

static const struct { unsigned rom; void (*host)(void *); } g_states[] = {
    {0x020b2150, func_ov002_020b2150},   /* 0 <- 0x02108730 */
    {0x020b20b4, func_ov002_020b20b4},   /* 1 <- 0x02108770 */
    {0x020b1cc0, func_ov002_020b1cc0},   /* 2 <- 0x02108748 */
    {0x020b1bfc, func_ov002_020b1bfc},   /* 3 <- 0x02108760 */
    {0x020b2070, func_ov002_020b2070},   /* 4 <- 0x02108740 */
    {0x020b1ad4, func_ov002_020b1ad4},   /* 5 <- 0x02108758 */
    {0x020b1cc0, func_ov002_020b1cc0},   /* 6 <- 0x02108750 (same as 2) */
    {0x020b20b4, func_ov002_020b20b4},   /* 7 <- 0x02108768 (same as 1) */
    {0x020b1a60, func_ov002_020b1a60},   /* 8 <- 0x02108738 */
};

extern "C" void port_coin_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (int i = 0; i < PORT_COIN_STATES; ++i) {
        if (data_ov002_0210dc70[i].fn != g_states[i].rom) {
            std::fprintf(stderr, "FATAL: coin state %d: the sinit left %08x, "
                         "the ROM's own table says %08x -- WRONG BYTES\n", i,
                         data_ov002_0210dc70[i].fn, g_states[i].rom);
            std::abort();
        }
        data_ov002_0210dc70[i].fn = (unsigned)(size_t)g_states[i].host;
        data_ov002_0210dc70[i].delta = 0;
    }
}

extern "C" int _ZN4Coin8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    unsigned char *f3ae = (unsigned char *)(c + 0x3ae);

    if (*f3ae & 2) {                       /* the pickup sound is pending */
        *f3ae &= (unsigned char)~2;
        _ZN5Sound9PlayBank3EjRK7Vector3(0x30, c + 0x74);
    }
    /* A collected BLUE coin retires by leaving its area rather than by being
       destroyed: mAreaId = -1 makes Actor::BeforeBehavior refuse it. */
    if (*(unsigned short *)(c + 0xc) == 0x122 && (*f3ae & 1))
        *(signed char *)(c + 0xcc) = -1;

    if (func_ov002_020b10a0(c) != 0)
        return 1;
    *(short *)(c + 0x8e) += 0xc00;         /* the spin */
    if (func_ov002_020b12ec(c) != 0) {
        func_ov002_020b14d8(c);
        _ZN12CylinderClsn5ClearEv(c + 0x178);
        return 1;
    }
    func_ov002_020b10e4(c);
    *(int *)(c + 0xd0) = 0;                /* mEatingPlayer */
    if (func_ov002_020b19dc(c) != 0)
        return 1;

    {
        unsigned st = (unsigned)*(int *)(c + 0x3a4);   /* mBehaviorType */
        if (st >= PORT_COIN_STATES) {
            std::fprintf(stderr, "FATAL: coin behaviour state %u out of "
                         "range\n", st);
            std::abort();
        }
        ((void (*)(void *))(size_t)data_ov002_0210dc70[st].fn)(c);
    }

    if (data_0209f2d8 != 1 && (*(int *)(c + 0xb0) & 8) != 0) {
        _ZN12CylinderClsn5ClearEv(c + 0x178);
        if (*(unsigned char *)(c + 0x3aa) == 0 &&
            LenVec3(c + 0x74) < 0x64000) {
            if (*(int *)(c + 0x3a0) != 1 || *(unsigned char *)(c + 0x3b0) == 0)
                _ZN12CylinderClsn6UpdateEv(c + 0x178);
        }
    } else {
        func_ov002_020b14d8(c);
        _ZN12CylinderClsn5ClearEv(c + 0x178);
        if (*(unsigned char *)(c + 0x3aa) == 0) {
            if (*(int *)(c + 0x3a0) != 1 || *(unsigned char *)(c + 0x3b0) == 0)
                _ZN12CylinderClsn6UpdateEv(c + 0x178);
        }
    }
    return 1;
}
