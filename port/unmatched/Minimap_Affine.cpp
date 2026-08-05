// HOST COPY of the minimap's PER-FRAME AFFINE CALLBACK -- func_020297f4 (arm9
// 0x020297f4, 0x44 bytes) and the register writer it tails into, func_02055278
// (0x02055278, 0xb4 bytes). NEITHER HAS SOURCE ANYWHERE: both are holes in the
// arm9 delink table, so this is transcribed from the ROM's own instructions.
//
// WHY IT MATTERS: BG3-sub in BG mode 3 is an extended AFFINE layer, and the
// minimap is drawn on it. Every one of its pixels goes through BG3PA..BG3PD
// and the BG3X/BG3Y reference point before it picks a map entry. The port has
// been seeding those to the identity once at boot (hal/sub_screen.cpp) because
// nothing was writing them per frame -- so the minimap drew at exactly 1:1 and
// never followed the level's own scale. That is the "unscaled minimap".
//
// WHERE THE NUMBERS COME FROM. Minimap::Behavior ends in
//
//     UpdateMinimap(&self->f50, self->f60, self->f64,
//                   self->f60 - 0x80, self->f64 - 0x60)
//
// and UpdateMinimap (matched, gate 28) copies that sixteen-byte descriptor to
// data_0209f3c8 and the four scalars to data_0209f3c4 + 0x14..0x20 -- which
// land inside data_0209f3c8's own bytes, because f3c4 and f3c8 are one object
// (hal/sub_actors.cpp keeps the run contiguous). So the block is
//
//     +0x00  the 2x2 matrix, four 20.12 words
//     +0x10  x, y     the map-space point to put under the reference
//     +0x18  cx, cy   that point less (128, 96), the screen centre
//
// HOW THE ROM APPLIES IT. Stage::InitResources stores &data_0209f3c4 into
// data_0209d4a8, the current scene's graphics block, and func_02019144 -- the
// per-frame graphics beat -- opens by calling that block's vtable slot 2. For
// the Stage that slot is func_020297f4, whose whole body is
//
//     func_02055278(0x04001030, self + 4, self[0x14], self[0x18],
//                   self[0x1c], self[0x20]);  return 1;
//
// and func_02055278 packs the matrix into the four 8.8 halfwords the hardware
// wants and solves the reference point so that (x, y) lands at the centre:
//
//     BG3PA|BG3PB = (u16)((m0 << 12) >> 16) | (u16)((m1 << 12) >> 16) << 16
//     BG3PC|BG3PD = same for m2, m3
//     BG3X = (m0 * (cx - x) + m1 * (cy - y) + (x << 12)) >> 4
//     BG3Y = (m2 * (cx - x) + m3 * (cy - y) + (y << 12)) >> 4
//
// The port does not host func_02019144 itself: its tail is the layer-mask
// publish and the OAM upload, which hal/sub_screen.cpp already does its own
// way. Only the callback was missing, so that is what this is, called from the
// same place in the frame and behind the same gate the ROM uses -- the block
// pointer still being the Stage's. Stage::CleanupResources zeroes it, and then
// this does nothing, exactly as the ROM's null check does.
#include <stdio.h>
#include <stdlib.h>

extern "C" {
extern unsigned char data_0209f3c8[];   /* the block's 0x20 bytes of state */
}

/* func_02055278. `reg` is the first of the four affine words: for BG3-sub that
   is 0x04001030, i.e. PA|PB, PC|PD, X, Y. The two shifts on each matrix entry
   are the ROM's own (lsl #12 then asr #16, then a 16-bit zero-extend), which
   is a 20.12 to 8.8 narrowing that keeps only the low sixteen bits. */
static unsigned pack_8_8(int v)
{
    const int narrowed = (int)((unsigned)v << 12) >> 16;
    return (unsigned)(unsigned short)narrowed;
}

static void port_bg_affine_set(volatile unsigned *reg, const int *m,
                               int x, int y, int cx, int cy)
{
    const int dx = cx - x;
    const int dy = cy - y;
    reg[0] = pack_8_8(m[0]) | (pack_8_8(m[1]) << 16);
    reg[1] = pack_8_8(m[2]) | (pack_8_8(m[3]) << 16);
    reg[2] = (m[0] * dx + m[1] * dy + (int)((unsigned)x << 12)) >> 4;
    reg[3] = (m[2] * dx + m[3] * dy + (int)((unsigned)y << 12)) >> 4;
}

/* func_020297f4.
   THE OWNERSHIP CHECK IS NOT THE ROM'S, and cannot be: func_02019144 runs this
   only while data_0209d4a8 points at the Stage's block, and nothing in the port
   seats that word -- Stage::InitResources, the function that writes it, is in
   no slice. The block itself answers the same question. It is zeroed storage
   until Minimap::Behavior calls UpdateMinimap, and an all-zero matrix is
   degenerate anyway: every screen pixel would map to map pixel (0,0) and the
   layer would sample one tile. So until the minimap has spoken, leave the
   identity hal/sub_screen.cpp seeded at boot. */
extern "C" void port_minimap_affine_update(void)
{
    const int *m = (const int *)data_0209f3c8;
    const int x = *(const int *)(data_0209f3c8 + 0x10);
    const int y = *(const int *)(data_0209f3c8 + 0x14);
    const int cx = *(const int *)(data_0209f3c8 + 0x18);
    const int cy = *(const int *)(data_0209f3c8 + 0x1c);

    if ((m[0] | m[1] | m[2] | m[3]) == 0)
        return;

    port_bg_affine_set((volatile unsigned *)0x04001030, m, x, y, cx, cy);

    /* SM64DS_MINIMAP_TRACE=1: the matrix and the reference point it solved,
       once a second, for checking the scale is the level's rather than 1:1. */
    static int trace = -1, frame;
    if (trace < 0) trace = getenv("SM64DS_MINIMAP_TRACE") != 0;
    if (trace && (frame++ % 30) == 0)
        fprintf(stderr, "[mmaf] m=(%d,%d,%d,%d) at=(%d,%d) centre=(%d,%d) "
                "PA|PB=%08x PC|PD=%08x X=%d Y=%d\n",
                m[0], m[1], m[2], m[3], x, y, cx, cy,
                *(volatile unsigned *)0x04001030,
                *(volatile unsigned *)0x04001034,
                *(volatile int *)0x04001038,
                *(volatile int *)0x0400103c);
}
