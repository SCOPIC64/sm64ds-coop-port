/* HOST COPIES of the PAINTING's three per-mode dispatchers
 * (daPicGate_c, actor 307, ov080):
 *
 *     src/func_ov080_02126ca0.cpp   InitResources
 *     src/func_ov080_02126c60.c pp  Behavior
 *     src/func_ov080_02126c20.cpp   Render
 *
 * All three matched TUs declare `struct C;` and form a pointer-to-member
 * while C is INCOMPLETE -- the same case Door_Behavior.cpp documents: mwcc
 * makes that PMF the ROM's 8-byte {fn, delta} record, MSVC widens it to the
 * 16-byte general form. Two distinct breaks follow: &data_ov080_02128628[mi]
 * strides 0x20 instead of 0x18 (Init picks the wrong mode entry), and the
 * dispatch reads the two words PAST the pair as this-adjustments (the call
 * runs on a shifted this). The registry note (actor_registry.cpp, gate 50)
 * recorded the fix as deferred because level 2's paintings "survived by luck
 * of the layout" -- the luck was literally which host addresses the linker
 * put in the words after each pair, and the 2026-08-08 door-warp rebuilds
 * moved them: the star door to BoB's room started faulting in the wave loop
 * (func_ov080_021269b8+0x46, this shifted so +0x1a0 read 8). Debt due.
 *
 * The twelve state statics are seated with host bodies before
 * __sinit_ov080_02127b2c copies them into the BSS table (actor_overlays.cpp),
 * so every pair here carries a host fn and delta 0; the dispatch below is the
 * ROM encoding over that, stride spelled explicitly.
 *
 * With these hosted, the registry's port_host_abi_blocked list (levels
 * 4/5/13) can come off next time those levels are walked.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

struct PtPmf { unsigned fn; int delta; };
/* one per mode, the ROM's 0x18 stride: {setup, update, render} */
struct PtEntry { PtPmf setup, update, render; };

extern unsigned char data_ov080_02128628[];   /* the BSS dispatch table */
extern unsigned char data_ov080_02127714[];   /* wave grid sizes */
extern int data_ov080_02127834[];
extern int data_0209caa0[];

void *_ZN6Memory13operator_new2Ej(unsigned size);
int func_ov080_02125630(char *c, int texParam);
void _ZN5Actor9SetRangesE5Fix12IiES1_S1_S1_(void *self, int a, int b, int c, int d);
int func_ov080_0212555c(char *c);

}  /* extern "C" */

typedef int (*PtFn)(void *);

static void pt_call(const PtPmf *p, char *self)
{
    if (p->fn == 0)
        return;
    char *adj = self + (p->delta >> 1);
    PtFn f;
    if (p->delta & 1)
        f = *(PtFn *)(*(char **)adj + p->fn);
    else
        f = (PtFn)(size_t)p->fn;
    f(adj);
}

static PtEntry *pt_entry(unsigned mi)
{
    return (PtEntry *)(data_ov080_02128628 + mi * 0x18);
}

/* src/func_ov080_02126ca0.cpp: grid size from the param nibbles, the wave
   buffer, the texture load, the mode-entry seat and its setup dispatch. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" int func_ov080_02126ca0(char *c)
{
    unsigned f = *(unsigned *)(c + 8);
    unsigned m = (f >> 13) & 3;

    if (m >= 2) {
        c[0x1bb] = 2;
        c[0x1ba] = c[0x1bb];
    } else {
        unsigned idx = (unsigned char)(f & 0xf) + 1;
        static const signed char slot[17] = {
            -1, -1, -1, 0, 1, 2, 3, 4, 5, -1, -1, -1, -1, -1, -1, -1, 6 };
        if (idx <= 16 && slot[idx] >= 0) {
            c[0x1bb] = data_ov080_02127714[slot[idx]];
            c[0x1ba] = c[0x1bb];
        }
    }
    *(unsigned short *)(c + 0x1b8) =
        (unsigned short)((unsigned char)c[0x1ba] * (unsigned char)c[0x1bb]);

    *(void **)(c + 0x1a0) = _ZN6Memory13operator_new2Ej(
        (unsigned)*(unsigned short *)(c + 0x1b8) * 0x18u);
    *(int *)(c + 0x1a8) = func_ov080_02125630(c, (int)((f >> 8) & 0x1f));
    *(void **)(c + 0x1ac) = data_ov080_02127834;

    {
        unsigned mi = (f >> 13) & 3;
        PtEntry *e = pt_entry(mi);
        *(void **)(c + 0x1a4) = e;
        pt_call(&e->setup, c);
    }

    {
        int lo = (int)((unsigned char)(f & 0xf) + 1);
        int hi = (int)((unsigned char)((f >> 4) & 0xf) + 1);
        int a = lo * 0x64000;
        int b = hi * 0x64000;
        _ZN5Actor9SetRangesE5Fix12IiES1_S1_S1_(c, b / 2, (a / 2) + 0xc8000,
                                               0x1964000, 0x1964000);
    }
    if ((unsigned char)((f >> 8) & 0x1f) == 4 &&
        (unsigned char)((f >> 13) & 3) == 1) {
        *(int *)(c + 0xb0) &= ~3;
    }
    if ((unsigned char)((f >> 8) & 0x1f) == 7 &&
        (unsigned char)((f >> 13) & 3) == 1) {
        *(int *)(c + 0x1b0) = *(int *)(c + 0x5c);
        if (data_0209caa0[2] & 0x40000)
            *(int *)(c + 0x5c) += 0x802000;
    }
    func_ov080_0212555c(c);
    return 1;
}

/* src/func_ov080_02126c60.cpp: the per-frame half -- entry's update pair. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" int func_ov080_02126c60(char *c)
{
    PtEntry *e = *(PtEntry **)(c + 0x1a4);
    pt_call(&e->update, c);
    return 1;
}

/* src/func_ov080_02126c20.cpp: the render half -- entry's render pair. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" int func_ov080_02126c20(char *c)
{
    PtEntry *e = *(PtEntry **)(c + 0x1a4);
    pt_call(&e->render, c);
    return 1;
}
