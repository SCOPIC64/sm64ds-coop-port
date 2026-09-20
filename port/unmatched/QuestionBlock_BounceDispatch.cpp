/* HOST COPY of src/func_ov102_021498e0.cpp -- the QUESTION_BLOCK's state-1
 * main half, the block bouncing after Mario hits it (daObjHatenaBlock_c,
 * actor 20, ov102), plus the seat for the two pointer-to-member CONTENT tables
 * it dispatches through.
 *
 * WHY A HOST COPY. The matched TU forms `struct C; typedef void (C::*PMF)();`
 * and then reads `data_ov102_0214e870[content].pmf[ch]` /
 * `data_ov102_0214e8c0[content].pmf[ch]` while C is INCOMPLETE. mwcc answers
 * an incomplete-class PMF with the ROM's 8-byte {fn, delta} record; MSVC
 * widens it to its 16-byte general representation, so compiling that dispatch
 * on the host reads the tables at the wrong stride and calls through garbage.
 * The Door / Painting treatment: transcribe the body for the host with the PMF
 * dispatch respelled to read the ROM-layout table explicitly. The CONTENT
 * bodies are NOT copied -- they are matched src TUs, sliced and linked as-is
 * (port/slice_gate180.txt); only this PMF-dispatching frame gets hosted.
 *
 * THE TABLE LAYOUT (verified against the overlay image + the matched
 * __sinit_ov102_0214d908). data_ov102_0214e870 (0x20) and
 * data_ov102_0214e8c0 (0x100) are BSS -- the overlay image ends at 0x0214e6e0,
 * they carry no static relocs, and __sinit_ov102_0214d908 (gate 23) fills them
 * at boot by copying source .data statics. Each content row is 32 bytes --
 * FOUR 8-byte {fn, delta} records, one per ch -- straight from the ROM's own
 * dispatch math at 0x021498e0+0x2a8: `add r0, r1, r0, lsl #5` (base +
 * content*32) then `add r3, r0, r4, lsl #3` (+ ch*8). Flat pair index =
 * content*4 + ch; 0214e8c0 is 8 content rows exactly (0x100/32), 0214e870 is
 * one. The record is mwcc's PMF: word0 = fn (or the vtable offset), word1 =
 * (delta<<1)|virtual-bit. Every record these tables ship is {fn, 0} --
 * non-virtual, zero delta -- so each pair is just a function pointer.
 *
 * THE SEATED STATICS. The .fn word each source static ships is the overlay
 * image's own DS CODE ADDRESS (the ovdata contract, e.g. 0214e310 -> 021492d4,
 * confirmed against config/arm9/overlays/ov102/relocs.txt). Copied into the
 * BSS and called, that jumps into unbuilt DS memory. port_..._content_seat()
 * rewrites the 36 source statics that feed the two content tables to the host
 * addresses of the linked bodies BEFORE __sinit copies them -- the WaterBomb /
 * Cap reading, one less mapping to get wrong than reseating the runtime table
 * after. Each is checked against the ROM address its host body was compiled
 * from, so a mount pointing at the wrong bytes says so instead of calling into
 * the overlay image.
 *
 * THE CASE-0 TABLE-CHOICE FIX is preserved exactly: switch case 0 reads
 * data_ov102_0214e8c0 (NOT 0214e870); only case 2 reads 0214e870. This was a
 * real bug fixed during matching and the relocs confirm it.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

struct PortPmf { unsigned fn; int delta; };

/* the two runtime content tables __sinit_ov102_0214d908 fills (BSS). Read here
 * as flat {fn,delta} pairs: row `content` at +content*32, pmf[ch] at +ch*8. */
extern PortPmf data_ov102_0214e870[];   /* 1 row  x 4 pmfs = 4 pairs  (0x20)  */
extern PortPmf data_ov102_0214e8c0[];   /* 8 rows x 4 pmfs = 32 pairs (0x100) */

/* the 36 source statics __sinit_ov102_0214d908 copies into those two tables.
 * ov102 relocs ship each as {DS-code-addr, 0}; reseated to host bodies below. */
extern PortPmf
    data_ov102_0214e388[], data_ov102_0214e380[], data_ov102_0214e378[],
    data_ov102_0214e370[], data_ov102_0214e368[], data_ov102_0214e360[],
    data_ov102_0214e358[], data_ov102_0214e350[], data_ov102_0214e348[],
    data_ov102_0214e340[], data_ov102_0214e320[], data_ov102_0214e288[],
    data_ov102_0214e338[], data_ov102_0214e330[], data_ov102_0214e280[],
    data_ov102_0214e318[], data_ov102_0214e2a8[], data_ov102_0214e2b0[],
    data_ov102_0214e270[], data_ov102_0214e298[], data_ov102_0214e2a0[],
    data_ov102_0214e2f8[], data_ov102_0214e2f0[], data_ov102_0214e2e8[],
    data_ov102_0214e390[], data_ov102_0214e2d8[], data_ov102_0214e2d0[],
    data_ov102_0214e2c8[], data_ov102_0214e2c0[], data_ov102_0214e2b8[],
    data_ov102_0214e2e0[], data_ov102_0214e328[],
    data_ov102_0214e310[], data_ov102_0214e308[], data_ov102_0214e300[],
    data_ov102_0214e290[];

/* --- the CONTENT bodies the two tables reach (matched src, linked as-is) --- */
void  func_ov102_02149684(int *dst, void *src);
void *func_ov102_02149220(void *c);
void  func_ov102_02149288(void *c);
void  func_ov102_021492d4(void *c);
void *func_ov102_02149384(void *c);
void  func_ov102_021493dc(void *c);
void  func_ov102_02149428(void *c);
void  func_ov102_02149478(void *c);
int   func_ov102_021494cc(void *c);
void  func_ov102_0214953c(void *c, int p1, int p2);

/* --- the rest of the matched body's closure (already linked) --- */
int  DecIfAbove0_Short(short *p);
void func_ov002_020f0438(void *a);
void _ZN5Sound9PlayBank3EjRK7Vector3(int bank, void *pos);
void _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(unsigned id, int x, int y, int z);
int  _ZN8SaveData16HasPlayerLostCapEv(void);
void func_ov102_02149da8(void *c, int i);

extern short       data_02082214[];
extern signed char data_0209f2f8;

}  /* extern "C" */

/* ------------------------------------------------------------------------- */
/* Seat: rewrite the 36 content-table source statics to host bodies, then let */
/* __sinit_ov102_0214d908 copy them into the BSS tables. Runs BEFORE that     */
/* __sinit from hal/actor_overlays.cpp.                                       */
/* ------------------------------------------------------------------------- */
static const struct { PortPmf *slot; unsigned rom; void *host; }
g_qblock_content_statics[] = {
    /* --- 0214e8c0 feeders. Which table offset each lands at is the matched
       __sinit's business, not the seat's: the seat only rewrites each source
       static's fn word, checked against its own reloc destination. --- */
    { data_ov102_0214e388, 0x021494cc, (void *)func_ov102_021494cc },
    { data_ov102_0214e380, 0x021494cc, (void *)func_ov102_021494cc },
    { data_ov102_0214e378, 0x021494cc, (void *)func_ov102_021494cc },
    { data_ov102_0214e370, 0x021494cc, (void *)func_ov102_021494cc },
    { data_ov102_0214e368, 0x02149478, (void *)func_ov102_02149478 },
    { data_ov102_0214e360, 0x02149478, (void *)func_ov102_02149478 },
    { data_ov102_0214e358, 0x02149478, (void *)func_ov102_02149478 },
    { data_ov102_0214e350, 0x02149478, (void *)func_ov102_02149478 },
    { data_ov102_0214e348, 0x02149428, (void *)func_ov102_02149428 },
    { data_ov102_0214e340, 0x02149428, (void *)func_ov102_02149428 },
    { data_ov102_0214e320, 0x02149428, (void *)func_ov102_02149428 },
    { data_ov102_0214e288, 0x02149428, (void *)func_ov102_02149428 },
    { data_ov102_0214e338, 0x02149384, (void *)func_ov102_02149384 },
    { data_ov102_0214e330, 0x02149384, (void *)func_ov102_02149384 },
    { data_ov102_0214e280, 0x02149384, (void *)func_ov102_02149384 },
    { data_ov102_0214e318, 0x02149384, (void *)func_ov102_02149384 },
    { data_ov102_0214e2a8, 0x021493dc, (void *)func_ov102_021493dc },
    { data_ov102_0214e2b0, 0x021493dc, (void *)func_ov102_021493dc },
    { data_ov102_0214e270, 0x021493dc, (void *)func_ov102_021493dc },
    { data_ov102_0214e298, 0x021493dc, (void *)func_ov102_021493dc },
    { data_ov102_0214e2a0, 0x021492d4, (void *)func_ov102_021492d4 },
    { data_ov102_0214e2f8, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2f0, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2e8, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e390, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2d8, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2d0, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2c8, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2c0, 0x02149220, (void *)func_ov102_02149220 },
    { data_ov102_0214e2b8, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e2e0, 0x02149288, (void *)func_ov102_02149288 },
    { data_ov102_0214e328, 0x02149288, (void *)func_ov102_02149288 },
    /* --- 0214e870 feeders --- */
    { data_ov102_0214e310, 0x021492d4, (void *)func_ov102_021492d4 },
    { data_ov102_0214e308, 0x021492d4, (void *)func_ov102_021492d4 },
    { data_ov102_0214e300, 0x021492d4, (void *)func_ov102_021492d4 },
    { data_ov102_0214e290, 0x021492d4, (void *)func_ov102_021492d4 },
};

extern "C" void port_question_block_content_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_qblock_content_statics /
                             sizeof g_qblock_content_statics[0]; ++i) {
        PortPmf *p = g_qblock_content_statics[i].slot;
        if (p->fn != g_qblock_content_statics[i].rom || p->delta != 0) {
            std::fprintf(stderr, "FATAL: QuestionBlock content static %u: the "
                         "mount holds %08x/%d, the ROM's own table says "
                         "%08x/0 -- WRONG BYTES\n", i, p->fn, p->delta,
                         g_qblock_content_statics[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_qblock_content_statics[i].host;
    }
}

/* Read the two content tables at the ROM stride: content row is 32 bytes
 * (four {fn,delta} pairs, one per ch), pmf[ch] at +ch*8. Flat pair index =
 * content*4 + ch -- the ROM's own `lsl #5` / `lsl #3` at 0x021498e0+0x2a8. */
static void qblock_content_call(PortPmf *tbl, unsigned content, int ch,
                                void *self)
{
    PortPmf *p = &tbl[content * 4 + ch];
    /* every ROM .delta here is 0 (non-virtual, complete class): plain fn ptr */
    ((void (*)(void *))(size_t)p->fn)(self);
}

/* ------------------------------------------------------------------------- */
/* HOST COPY of src/func_ov102_021498e0.cpp -- semantics preserved exactly.   */
/* ------------------------------------------------------------------------- */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" void func_ov102_021498e0(void *selfv)
{
    char *c = (char *)selfv;
    int pos[3];
    int ch;
    unsigned short typ;
    void *held;

    func_ov102_02149684(pos, c);

    if (DecIfAbove0_Short((short *)(c + 0x3ee)) != 0) {
        short *pang = (short *)(c + 0x3ec);
        unsigned short ang = *(unsigned short *)(c + 0x3ec);
        short s = data_02082214[(ang >> 4) * 2];
        int t = (int)s + 0x1000;
        *(int *)(c + 0x84) =
            (int)(((long long)t * 0x999 + 0x800) >> 12) + 0x666;
        ang = *(unsigned short *)(c + 0x3ec);
        s = data_02082214[(ang >> 4) * 2];
        *(int *)(c + 0x80) =
            (int)(((long long)(0x1000 - (int)s) * 0x1000 + 0x800) >> 12)
            + 0x1000;
        *(int *)(c + 0x88) = *(int *)(c + 0x80);
        ang = *(unsigned short *)(c + 0x3ec);
        s = data_02082214[(ang >> 4) * 2];
        *(int *)(c + 0x3dc) = (0x1000 - (int)s) * 0xd;
        *pang += 0x1000;
        return;
    }

    held = *(void **)(c + 0x3f4);
    if (held != 0) {
        if (*(unsigned short *)((char *)held + 0xc) == 0x149)
            func_ov002_020f0438(held);
        *(void **)(c + 0x3f4) = 0;
    }

    _ZN5Sound9PlayBank3EjRK7Vector3(0, c + 0x74);
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0xb, pos[0], pos[1], pos[2]);
    _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0xd, pos[0], pos[1], pos[2]);

    typ = *(unsigned short *)(c + 0xc);
    switch (typ - 0x14) {
    case 1: case 2:
        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0x10, pos[0], pos[1], pos[2]);
        break;
    case 0:
        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(0xc, pos[0], pos[1], pos[2]);
        break;
    case 3: case 4: case 5:
        _ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(9, pos[0], pos[1], pos[2]);
        break;
    }

    ch = (int)*(unsigned char *)(c + 0x3f2);
    if (ch < 0) ch = 0; else if (ch > 3) ch = 3;

    typ = *(unsigned short *)(c + 0xc);
    switch (typ - 0x14) {
    case 1: {
        unsigned content = *(unsigned char *)(c + 0x3f3);
        qblock_content_call(data_ov102_0214e8c0, content, 0, c);
        break;
    }
    case 2: {
        unsigned content = *(unsigned char *)(c + 0x3f3);
        if (ch >= 4) ch = 0;
        qblock_content_call(data_ov102_0214e870, content, ch, c);
        break;
    }
    case 0:
        if (_ZN8SaveData16HasPlayerLostCapEv() == 0 || data_0209f2f8 == 0x1f) {
            unsigned content = *(unsigned char *)(c + 0x3f3);
            if (ch >= 4) ch = 0;
            qblock_content_call(data_ov102_0214e8c0, content, ch, c);
        } else {
            func_ov102_02149220(c);
        }
        break;
    case 3:
        if (_ZN8SaveData16HasPlayerLostCapEv() == 0) func_ov102_0214953c(c, 0, 0x12);
        else func_ov102_02149220(c);
        break;
    case 5:
        if (_ZN8SaveData16HasPlayerLostCapEv() == 0) func_ov102_0214953c(c, 1, 0x12);
        else func_ov102_02149220(c);
        break;
    case 4:
        if (_ZN8SaveData16HasPlayerLostCapEv() == 0) func_ov102_0214953c(c, 2, 0x12);
        else func_ov102_02149220(c);
        break;
    }

    func_ov102_02149da8(c, 2);
}
