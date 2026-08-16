// The OBJ window, and the bit-contract bug it was hiding behind.
//
// OAM attribute 0 bits 10-11 are the object mode: 0 normal, 1 semi-transparent,
// 2 OBJ WINDOW, 3 prohibited (bitmap on the DS). ntr/ppu.cpp and ntr/ppu_sub.cpp
// both used to skip mode 3 under the comment "OBJ window", which is the wrong
// mode in both directions: it threw away bitmap sprites and it DREW window
// sprites, which on hardware are invisible and contribute only a mask.
//
// This programs engine B by hand -- no game code, no ROM data -- and checks the
// three behaviours that separate a modelled OBJ window from the old one:
//
//   A. CONTROL. A mode-0 sprite over a full-screen BG draws its own colour.
//      Without this the "no sprite colour anywhere" check in B would pass
//      vacuously, for instance if the sprite were mispositioned or the palette
//      were wrong.
//   B. A mode-2 sprite paints NOTHING, and its silhouette is instead the region
//      where WINOUT's upper half decides what shows. With WINOUT set to "BG
//      inside the OBJ window, nothing outside", the BG appears in the sprite's
//      box and the backdrop everywhere else.
//   C. A mode-3 sprite draws nothing and masks nothing. Bitmap OBJs are not
//      hosted, so the honest answer is absence, not a wrong-format smear.
//
// Nothing in the port's measured runs uses any of these modes today
// (port/ppu_gap_audit.txt), which is exactly why the contract needs a test
// rather than a scene.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ntr/mmio.h"
#include "ntr/ppu.h"

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

// Engine B.
static const u32 kReg = 0x04001000u;
static const u32 kVram = 0x06200000u;
static const u32 kBgPltt = 0x05000400u;
static const u32 kObjVram = 0x06600000u;
static const u32 kObjPltt = 0x05000600u;
static const u32 kOam = 0x07000400u;

static const u32 kRed = 0xFFFF0000u;    // BGR555 0x001F
static const u32 kGreen = 0xFF00FF00u;  // BGR555 0x03E0
static const u32 kBlue = 0xFF0000FFu;   // BGR555 0x7C00

// The sprite: 16x16 square, tile 2, at (40,60).
static const int kSprX = 40, kSprY = 60, kSprW = 16, kSprH = 16;

static int g_failures;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        std::fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

// SM64DS_OBJWIN_BMP=1 writes each case out as objwin_<case>.bmp next to the
// exe. The pixel counts below are the assertion; these are for a human, since
// this lane is not allowed to judge its own rendering.
static void dump(const ntr::SubFramebuffer &fb, const char *tag) {
    if (!std::getenv("SM64DS_OBJWIN_BMP")) return;
    char path[64];
    std::snprintf(path, sizeof path, "objwin_%s.bmp", tag);
    if (ntr::ppu_write_bmp_sub(path, fb)) std::printf("  wrote %s\n", path);
}

static void wr16(u32 a, u16 v) { *reinterpret_cast<volatile u16 *>(a) = v; }
static void wr32(u32 a, u32 v) { *reinterpret_cast<volatile u32 *>(a) = v; }
static void wr8(u32 a, u8 v) { *reinterpret_cast<volatile u8 *>(a) = v; }

// Count pixels of one exact colour, and report one position, so a failure says
// where rather than only how many.
static int count_color(const ntr::SubFramebuffer &fb, u32 c, int *fx, int *fy) {
    int n = 0;
    for (int y = 0; y < ntr::SUB_H; ++y)
        for (int x = 0; x < ntr::SUB_W; ++x)
            if (fb.px[y][x] == c) {
                if (!n && fx) { *fx = x; *fy = y; }
                ++n;
            }
    return n;
}

// One OAM entry, plus every other entry disabled.
static void set_sprite(unsigned objmode) {
    for (int i = 0; i < 128; ++i) wr16(kOam + i * 8u, 0x0200);   // bit 9 = disable
    // attr0: y, plain (bits 8-9 = 0), object mode at 10-11, square (14-15 = 0).
    wr16(kOam + 0, (u16)((kSprY & 0xFF) | (objmode << 10)));
    // attr1: x, size 1 (bits 14-15) = 16x16 for a square shape.
    wr16(kOam + 2, (u16)((kSprX & 0x1FF) | (1u << 14)));
    // attr2: tile 2, priority 0, palette 0.
    wr16(kOam + 4, 2);
}

int main(void)
{
    if (!ntr::io_init()) { std::fprintf(stderr, "io_init failed\n"); return 2; }

    // ---- a full-screen 16-colour text BG on BG0 -----------------------------
    // BG0CNT 0x0104: priority 0, character base 1 (bits 2-5 -> 0x4000),
    // screen base 1 (bits 8-12 -> 0x800), 16-colour, size 0 (32x32 tiles).
    wr16(kReg + 0x08, 0x0104);
    for (int i = 0; i < 32 * 32; ++i) wr16(kVram + 0x800 + i * 2u, 1);  // tile 1
    for (int i = 0; i < 32; ++i) wr8(kVram + 0x4000 + 32 + i, 0x11);    // index 1
    wr16(kBgPltt + 0, 0x7C00);   // backdrop blue
    wr16(kBgPltt + 2, 0x001F);   // BG colour red

    // The sprite's tiles: 16x16 at 16 colours is four 32-byte tiles, and DISPCNT
    // bit 4 below selects 1D mapping so they are consecutive from tile 2.
    for (int i = 0; i < 4 * 32; ++i) wr8(kObjVram + 2 * 32 + i, 0x22);  // index 2
    wr16(kObjPltt + 4, 0x03E0);  // OBJ colour green

    wr16(kReg + 0x6C, 0);        // master brightness off
    wr16(kReg + 0x48, 0);        // WININ unused (windows 0 and 1 stay off)

    // DISPCNT: mode 0, 1D OBJ mapping (bit 4), BG0 on (bit 8), OBJ on (bit 12),
    // graphics display (bits 16-17 = 1).
    const u32 kBaseDispcnt = 0x00011110u;

    ntr::SubFramebuffer fb;
    int fx = -1, fy = -1;

    // ---- A. control: a normal sprite really does draw ----------------------
    wr32(kReg, kBaseDispcnt);
    wr16(kReg + 0x4A, 0x003F);   // WINOUT: everything shows (no window armed)
    set_sprite(0);
    ntr::ppu_scanout_sub(fb);
    const int a_green = count_color(fb, kGreen, &fx, &fy);
    const int a_red = count_color(fb, kRed, 0, 0);
    dump(fb, "a_control");
    std::printf("  A control      green=%d (first %d,%d)  red=%d\n",
                a_green, fx, fy, a_red);
    CHECK(a_green == kSprW * kSprH);              // the sprite drew, whole
    CHECK(fx == kSprX && fy == kSprY);            // and where it was put
    CHECK(a_red == ntr::SUB_W * ntr::SUB_H - a_green);   // BG everywhere else

    // ---- B. a mode-2 sprite is a mask, not a picture ------------------------
    // OBJ window on (DISPCNT bit 15). WINOUT low six = 0 (nothing shows outside
    // any window), bits 8-13 = 0x3F (everything shows inside the OBJ window).
    wr32(kReg, kBaseDispcnt | (1u << 15));
    wr16(kReg + 0x4A, 0x3F00);
    set_sprite(2);
    ntr::ppu_scanout_sub(fb);
    const int b_green = count_color(fb, kGreen, 0, 0);
    const int b_red = count_color(fb, kRed, &fx, &fy);
    const int b_blue = count_color(fb, kBlue, 0, 0);
    dump(fb, "b_objwindow");
    std::printf("  B obj window   green=%d  red=%d (first %d,%d)  blue=%d\n",
                b_green, b_red, fx, fy, b_blue);
    // The whole point: the window sprite contributes no colour of its own.
    // This is the check that fails against the old `== 3` test.
    CHECK(b_green == 0);
    // Its silhouette is where the BG is allowed through, exactly.
    CHECK(b_red == kSprW * kSprH);
    CHECK(fx == kSprX && fy == kSprY);
    // and the backdrop owns everything outside it.
    CHECK(b_blue == ntr::SUB_W * ntr::SUB_H - kSprW * kSprH);
    // Corner check: the BG is visible inside the box and hidden just outside.
    CHECK(fb.px[kSprY + 8][kSprX + 8] == kRed);
    CHECK(fb.px[kSprY + 8][kSprX + kSprW + 4] == kBlue);

    // ---- C. a mode-3 sprite neither draws nor masks --------------------------
    // Bitmap OBJs are not hosted. With the same WINOUT as B, an unhosted sprite
    // that wrongly masked would open a red box; one that wrongly drew would show
    // green. The honest answer is a screen of pure backdrop.
    set_sprite(3);
    ntr::ppu_scanout_sub(fb);
    const int c_green = count_color(fb, kGreen, 0, 0);
    const int c_red = count_color(fb, kRed, 0, 0);
    const int c_blue = count_color(fb, kBlue, 0, 0);
    dump(fb, "c_bitmapobj");
    std::printf("  C bitmap obj   green=%d  red=%d  blue=%d\n",
                c_green, c_red, c_blue);
    CHECK(c_green == 0);
    CHECK(c_red == 0);
    CHECK(c_blue == ntr::SUB_W * ntr::SUB_H);

    // ---- D. the OBJ window enable bit is honoured ---------------------------
    // Same mode-2 sprite, DISPCNT bit 15 clear. With no window armed at all the
    // whole window unit is off and the BG shows everywhere, so the mask must not
    // leak through as a region.
    wr32(kReg, kBaseDispcnt);
    set_sprite(2);
    ntr::ppu_scanout_sub(fb);
    const int d_red = count_color(fb, kRed, 0, 0);
    const int d_green = count_color(fb, kGreen, 0, 0);
    dump(fb, "d_winobj_off");
    std::printf("  D winobj off   green=%d  red=%d\n", d_green, d_red);
    CHECK(d_green == 0);
    CHECK(d_red == ntr::SUB_W * ntr::SUB_H);

    if (g_failures) {
        std::fprintf(stderr, "smoke_objwin: %d FAILURES\n", g_failures);
        return 1;
    }
    std::printf("smoke_objwin: ok\n");
    return 0;
}
