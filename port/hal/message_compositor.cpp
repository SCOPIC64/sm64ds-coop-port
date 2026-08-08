// Engine-A 2D-over-3D compositor, host side.
//
// WHAT THE DS DOES AND THE PORT DID NOT. The top screen is engine A. Its 2D
// unit (four tile BGs, the OBJ sprite layer, the window unit) composites over
// the 3D engine's output in hardware every frame: 3D is effectively "BG0 in 3D
// mode", and the 2D BGs and sprites draw over it by priority. walk_window
// rendered ONLY the 3D frame to the top-screen framebuffer and never scanned
// engine A's 2D layers, so anything the game drew into engine A's 2D -- the
// dialogue box (BG3 + the cursor OBJ) above all -- was invisible.
//
// port/ntr/ppu.cpp already has the raster (ppu_scanout / ppu_scanout_obj), but
// ppu_scanout OVERWRITES the whole framebuffer with a backdrop-filled 2D image,
// which would erase the 3D scene. This file is the COMPOSITE form: it walks the
// same engine-A register state and writes ONLY the pixels a 2D layer actually
// covers, leaving the 3D frame showing everywhere the 2D is transparent. That
// is exactly what the DS's layer compositor produces.
//
// WHAT IT IMPLEMENTS (faithful to the dialogue box's needs):
//   - the four engine-A text BGs, by priority, lower priority in front;
//   - the window unit (WIN0/WIN1, WININ/WINOUT), because the box uses WIN0 to
//     reveal BG3 as it animates open -- without it the box tilemap would show
//     full-screen the instant it is enabled;
//   - the engine-A OBJ layer (the message cursor arrows), composited last;
//   - the DS 256x192 space mapped to the host framebuffer by integer scale.
//
// WHAT IT SKIPS (documented, none of it on the dialogue-box path):
//   - BLDCNT alpha blending and per-pixel colour effects (bit 5 of the window
//     masks): the box does not alpha-blend its tiles; its dimming is the
//     master-brightness / BLDY path, which walk_window's own fade composite
//     already applies after this;
//   - affine and bitmap BGs (engine A's box is text-mode);
//   - mosaic.
//
// The register/VRAM addresses are engine A's, cross-checked against ppu.cpp
// (same source of truth: the decomp's own G2::GetBG*Ptr shifts) and ppu_sub.cpp
// (the window unit, mirrored here for engine A's 0x04000000 register base).
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "ntr/ppu.h"

namespace {

// Engine A only.
constexpr uint32_t kRegBase  = 0x04000000u;
constexpr uint32_t kVramBase = 0x06000000u;
constexpr uint32_t kPlttBase = 0x05000000u;
constexpr uint32_t kOamBase  = 0x07000000u;
constexpr uint32_t kObjVram  = 0x06400000u;

inline uint16_t rd16(uint32_t a) { return *reinterpret_cast<volatile uint16_t *>(a); }
inline uint32_t rd32(uint32_t a) { return *reinterpret_cast<volatile uint32_t *>(a); }

inline uint32_t bgr555(uint16_t c) {
    const uint32_t r = (c >> 0) & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8)
           | (b << 3 | b >> 2);
}

struct BgConfig {
    bool enabled;
    int priority;
    uint32_t screen;   // tilemap address
    uint32_t chars;    // tile data address
    bool bpp8;
    int tiles_w, tiles_h;
    int hofs, vofs;
};

BgConfig read_bg(int bg, uint32_t dispcnt) {
    BgConfig c{};
    c.enabled = (dispcnt >> (8 + bg)) & 1;
    if (!c.enabled) return c;
    const uint16_t cnt = rd16(kRegBase + 0x08 + bg * 2);
    c.priority = cnt & 3;
    c.bpp8 = (cnt >> 7) & 1;
    // Engine A has the DISPCNT char/screen base offsets (bits 24-26 / 27-29).
    const uint32_t char_off = ((dispcnt >> 24) & 7) << 16;
    const uint32_t scr_off = ((dispcnt >> 27) & 7) << 16;
    c.screen = kVramBase + scr_off + (((cnt >> 8) & 0x1F) << 11);
    c.chars = kVramBase + char_off + (((cnt & 0x3c) >> 2) << 14);
    const unsigned sz = (cnt >> 14) & 3;
    c.tiles_w = (sz & 1) ? 64 : 32;
    c.tiles_h = (sz & 2) ? 64 : 32;
    c.hofs = rd16(kRegBase + 0x10 + bg * 4) & 0x1FF;
    c.vofs = rd16(kRegBase + 0x12 + bg * 4) & 0x1FF;
    return c;
}

inline uint32_t screen_entry_addr(const BgConfig &c, int tx, int ty) {
    const int bx = tx >> 5, by = ty >> 5;
    const int blocks_w = c.tiles_w >> 5;
    const int block = by * blocks_w + bx;
    return c.screen + block * 0x800 + (((ty & 31) * 32 + (tx & 31)) * 2);
}

// false where the BG is transparent at this pixel.
bool sample_bg(const BgConfig &c, int x, int y, uint32_t &out) {
    const int px = (x + c.hofs) & (c.tiles_w * 8 - 1);
    const int py = (y + c.vofs) & (c.tiles_h * 8 - 1);
    const uint16_t se = rd16(screen_entry_addr(c, px >> 3, py >> 3));
    const int tile = se & 0x3FF;
    int fx = px & 7, fy = py & 7;
    if (se & 0x400) fx = 7 - fx;
    if (se & 0x800) fy = 7 - fy;
    uint32_t index;
    if (c.bpp8) {
        index = *reinterpret_cast<volatile uint8_t *>(c.chars + tile * 64 + fy * 8 + fx);
        if (index == 0) return false;
    } else {
        const uint8_t pair =
            *reinterpret_cast<volatile uint8_t *>(c.chars + tile * 32 + fy * 4 + (fx >> 1));
        index = (fx & 1) ? (pair >> 4) : (pair & 0xF);
        if (index == 0) return false;
        index += ((se >> 12) & 0xF) * 16;
    }
    out = bgr555(rd16(kPlttBase + index * 2));
    return true;
}

// ---- the window unit (engine A, register base 0x04000000) ------------------
// Mirror of ppu_sub.cpp's read_windows/window_mask. Bits 0..3 BG0..BG3, bit 4
// OBJ, bit 5 colour effect. Edges are exclusive; a right/bottom at or before
// the left/top means "to end of line" (a zero register = full width).
struct Windows {
    bool any;
    int x1[2], x2[2], y1[2], y2[2];
    bool on[2];
    unsigned in[2], out;
};

void read_windows(uint32_t dispcnt, Windows &w) {
    w.on[0] = (dispcnt >> 13) & 1;
    w.on[1] = (dispcnt >> 14) & 1;
    w.any = w.on[0] || w.on[1];
    if (!w.any) return;
    for (int i = 0; i < 2; ++i) {
        const uint16_t hh = rd16(kRegBase + 0x40 + i * 2);
        const uint16_t vv = rd16(kRegBase + 0x44 + i * 2);
        w.x1[i] = hh >> 8;
        w.x2[i] = hh & 0xFF;
        w.y1[i] = vv >> 8;
        w.y2[i] = vv & 0xFF;
        if (w.x2[i] <= w.x1[i]) w.x2[i] = 256;
        if (w.y2[i] <= w.y1[i]) w.y2[i] = 192;
    }
    const uint16_t winin = rd16(kRegBase + 0x48);
    w.in[0] = winin & 0x3F;
    w.in[1] = (winin >> 8) & 0x3F;
    w.out = rd16(kRegBase + 0x4A) & 0x3F;
}

inline unsigned window_mask(const Windows &w, int x, int y) {
    if (!w.any) return 0x3F;
    for (int i = 0; i < 2; ++i)
        if (w.on[i] && x >= w.x1[i] && x < w.x2[i] && y >= w.y1[i] && y < w.y2[i])
            return w.in[i];
    return w.out;
}

// The DS 256x192 image the 2D unit produces, per pixel: colour + "did any 2D
// layer write here" flag. Only flagged pixels overwrite the 3D framebuffer.
struct Cell { uint32_t color; bool hit; };
Cell g_a[192][256];

// ---- OBJ (sprites): the cursor arrows -------------------------------------
// Engine-A OAM at 0x07000000, OBJ VRAM at 0x06400000, OBJ palette pltt+0x200.
// Composites into g_a (only non-transparent texels), on top of the BGs, same
// as the DS at equal-or-lower priority. Plain and affine, 16/256-colour, 1D
// mapping -- the same subset ppu_scanout_obj implements.
void raster_obj(uint32_t dispcnt) {
    static const int kSizes[3][4][2] = {
        {{8, 8}, {16, 16}, {32, 32}, {64, 64}},
        {{16, 8}, {32, 8}, {32, 16}, {64, 32}},
        {{8, 16}, {8, 32}, {16, 32}, {32, 64}},
    };
    const uint32_t obj_pltt = kPlttBase + 0x200u;
    const uint32_t boundary = 32u << ((dispcnt >> 20) & 3);

    for (int i = 127; i >= 0; --i) {
        const uint16_t a0 = rd16(kOamBase + i * 8u);
        const uint16_t a1 = rd16(kOamBase + i * 8u + 2);
        const uint16_t a2 = rd16(kOamBase + i * 8u + 4);
        const bool affine = a0 & 0x100;
        if (!affine && (a0 & 0x200)) continue;
        if (((a0 >> 10) & 3) == 3) continue;
        const int shape = (a0 >> 14) & 3;
        if (shape == 3) continue;
        const int size = (a1 >> 14) & 3;
        const int w = kSizes[shape][size][0], h = kSizes[shape][size][1];
        const bool dbl = affine && (a0 & 0x200);
        const int bw = dbl ? w * 2 : w, bh = dbl ? h * 2 : h;
        int x = a1 & 0x1FF, y = a0 & 0xFF;
        if (x >= 256) x -= 512;
        if (y >= 192 && y >= 256 - bh) y -= 256;
        const bool c256 = a0 & 0x2000;
        const bool hflip = !affine && (a1 & 0x1000);
        const bool vflip = !affine && (a1 & 0x2000);
        const uint32_t tile = a2 & 0x3FF;
        const uint32_t pal = (a2 >> 12) & 0xF;

        int pa = 256, pb = 0, pc = 0, pd = 256;
        if (affine) {
            const int grp = (a1 >> 9) & 0x1F;
            pa = (int16_t)rd16(kOamBase + (grp * 4 + 0) * 8u + 6);
            pb = (int16_t)rd16(kOamBase + (grp * 4 + 1) * 8u + 6);
            pc = (int16_t)rd16(kOamBase + (grp * 4 + 2) * 8u + 6);
            pd = (int16_t)rd16(kOamBase + (grp * 4 + 3) * 8u + 6);
        }

        for (int sy = 0; sy < bh; ++sy) {
            const int py = y + sy;
            if (py < 0 || py >= 192) continue;
            for (int sx = 0; sx < bw; ++sx) {
                const int px = x + sx;
                if (px < 0 || px >= 256) continue;
                int tx, ty;
                if (affine) {
                    const int cx = sx - bw / 2, cy = sy - bh / 2;
                    tx = ((pa * cx + pb * cy) >> 8) + w / 2;
                    ty = ((pc * cx + pd * cy) >> 8) + h / 2;
                    if (tx < 0 || tx >= w || ty < 0 || ty >= h) continue;
                } else {
                    tx = hflip ? w - 1 - sx : sx;
                    ty = vflip ? h - 1 - sy : sy;
                }
                const int tcol = tx >> 3, trow = ty >> 3;
                const int fx = tx & 7, fy = ty & 7;
                const int tiles_per_row = w / 8;
                const uint32_t tno = trow * tiles_per_row + tcol;
                uint32_t index;
                if (c256) {
                    const uint32_t addr = kObjVram + tile * boundary + tno * 64u + fy * 8u + fx;
                    index = *reinterpret_cast<volatile uint8_t *>(addr);
                    if (!index) continue;
                    g_a[py][px].color = bgr555(rd16(obj_pltt + index * 2u));
                } else {
                    const uint32_t addr = kObjVram + tile * boundary + tno * 32u + fy * 4u + fx / 2;
                    const uint8_t b = *reinterpret_cast<volatile uint8_t *>(addr);
                    index = (fx & 1) ? (b >> 4) : (b & 0xF);
                    if (!index) continue;
                    g_a[py][px].color = bgr555(rd16(obj_pltt + (pal * 16u + index) * 2u));
                }
                g_a[py][px].hit = true;
            }
        }
    }
}

}  // namespace

// Rasterise engine A's 2D layers into the DS 256x192 hit buffer, then composite
// the covered pixels over the host 3D framebuffer (scaled by integer factor).
// Called from walk_window right after gx_render and before the fade composite.
extern "C" void port_message_composite_engine_a(void *fbp)
{
    ntr::Framebuffer &fb = *reinterpret_cast<ntr::Framebuffer *>(fbp);

    const uint32_t dispcnt = rd32(kRegBase);
    const unsigned disp_mode = (dispcnt >> 16) & 3;
    const bool forced_blank = (dispcnt >> 7) & 1;

    /* raw engine-A register dump on the first frame the box is active, before
       any early-return, so a headless run sees the real DISPCNT/BGxCNT the box
       setup produced even if no BG reads as enabled */
    if (std::getenv("SM64DS_MSG_COMPOSITE_DEBUG")) {
        extern unsigned char data_0209d660, data_0209d45c;
        static int dumped;
        if (data_0209d660 && !dumped) {
            dumped = 1;
            std::fprintf(stderr, "[msgcomp] raw engA DISPCNT=%08x mode=%u fblank=%d "
                         "BG0CNT=%04x BG1CNT=%04x BG2CNT=%04x BG3CNT=%04x "
                         "d45c=%02x\n", dispcnt, disp_mode, forced_blank,
                         rd16(kRegBase + 0x08), rd16(kRegBase + 0x0a),
                         rd16(kRegBase + 0x0c), rd16(kRegBase + 0x0e),
                         (unsigned)data_0209d45c);
        }
    }

    /* NOTE: we do NOT gate on DISPCNT display mode here. On the DS, engine A's
       display-mode field (bits 16-17) selects display off / graphics / VRAM /
       main-memory. The PORT renders 3D to engine A WITHOUT driving engine A's
       DISPCNT (the 3D path writes the framebuffer directly), so this field
       stays 0 ("display off") for the whole run even while the box setup
       (func_0201f32c) enables BG3 + WIN0 in the SAME register. Gating on mode
       here would drop every real box. Forced blank (bit 7) is honoured because
       the game does set it deliberately. The per-BG enable bits below are the
       real signal for what 2D to composite. */
    if (forced_blank)
        return;

    BgConfig bgs[4];
    bool any_bg = false;
    for (int i = 0; i < 4; ++i) { bgs[i] = read_bg(i, dispcnt); any_bg |= bgs[i].enabled; }
    const bool obj_on = (dispcnt >> 12) & 1;
    if (!any_bg && !obj_on)
        return;   // nothing 2D enabled: the box is not up, leave the 3D frame

    Windows win;
    read_windows(dispcnt, win);

    // Clear the hit buffer, then BGs by priority (lower priority drawn last so
    // it wins), honouring the per-pixel window mask.
    for (int y = 0; y < 192; ++y)
        for (int x = 0; x < 256; ++x) g_a[y][x].hit = false;

    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            const unsigned mask = window_mask(win, x, y);
            for (int prio = 3; prio >= 0; --prio) {
                for (int bg = 3; bg >= 0; --bg) {
                    if (!bgs[bg].enabled || bgs[bg].priority != prio) continue;
                    if (!(mask & (1u << bg))) continue;
                    uint32_t s;
                    if (sample_bg(bgs[bg], x, y, s)) { g_a[y][x].color = s; g_a[y][x].hit = true; }
                }
            }
        }
    }

    if (obj_on)
        raster_obj(dispcnt);

    // One-shot diagnostic (SM64DS_MSG_COMPOSITE_DEBUG): the engine-A 2D state
    // the frame the compositor first finds any 2D enabled, so a headless run
    // can confirm the box BG is on and how many pixels it covers.
    if (std::getenv("SM64DS_MSG_COMPOSITE_DEBUG")) {
        int hits = 0;
        for (int y = 0; y < 192; ++y)
            for (int x = 0; x < 256; ++x) if (g_a[y][x].hit) ++hits;
        extern unsigned char data_0209d6bc;   /* Message::Update state */
        static int maxhits = -1;
        /* Print each time the composited-pixel count reaches a new maximum OR
           the box reaches its settled prompt/idle state (d6bc >= 4), so a
           headless run sees the box grow, its peak coverage, and the tilemap at
           the settled frame -- plus BG config and the window rect. */
        static int dumped_settled;
        const bool settled = (data_0209d6bc >= 4 && !dumped_settled);
        if (settled) dumped_settled = 1;
        if ((any_bg || obj_on) && (hits > maxhits || settled)) {
            if (hits > maxhits) maxhits = hits;
            std::fprintf(stderr, "[msgcomp] engine A: DISPCNT=%08x bg_en=%d%d%d%d "
                         "obj=%d win=%d(x0 %d..%d y0 %d..%d in0=%02x out=%02x), "
                         "composited %d/%d px\n", dispcnt,
                         bgs[0].enabled, bgs[1].enabled, bgs[2].enabled,
                         bgs[3].enabled, obj_on, win.any,
                         win.any ? win.x1[0] : 0, win.any ? win.x2[0] : 0,
                         win.any ? win.y1[0] : 0, win.any ? win.y2[0] : 0,
                         win.any ? win.in[0] : 0, win.any ? win.out : 0,
                         hits, 256 * 192);
            for (int b = 0; b < 4; ++b)
                if (bgs[b].enabled)
                    std::fprintf(stderr, "[msgcomp]   BG%d prio %d chars %08x "
                                 "screen %08x bpp%d %dx%d ofs(%d,%d)\n", b,
                                 bgs[b].priority, bgs[b].chars, bgs[b].screen,
                                 bgs[b].bpp8 ? 8 : 4, bgs[b].tiles_w, bgs[b].tiles_h,
                                 bgs[b].hofs, bgs[b].vofs);
            /* dump a slice of BG3's tilemap (nonzero screen entries) and whether
               its char data has any nonzero bytes, to see if the box content was
               uploaded and where the nonzero tiles sit on screen */
            if (bgs[3].enabled) {
                int nz = 0; uint32_t first = 0;
                for (int ty = 0; ty < 24 && nz < 8; ++ty)
                    for (int tx = 0; tx < 32; ++tx) {
                        uint16_t se = rd16(bgs[3].screen + (ty * 32 + tx) * 2);
                        if ((se & 0x3FF) != 0) {
                            if (!nz) std::fprintf(stderr, "[msgcomp]   BG3 nonzero tiles:");
                            if (nz < 8) std::fprintf(stderr, " (%d,%d)=t%u", tx, ty, se & 0x3FF);
                            ++nz; first = se & 0x3FF;
                        }
                    }
                std::fprintf(stderr, "\n[msgcomp]   BG3 tilemap nonzero entries (first 24 rows): %d\n", nz);
                /* is tile `first`'s char data nonzero? */
                int cnz = 0;
                for (int k = 0; k < 32; ++k)
                    if (*reinterpret_cast<volatile uint8_t *>(bgs[3].chars + first * 32 + k)) ++cnz;
                std::fprintf(stderr, "[msgcomp]   BG3 tile %u char nonzero bytes: %d/32\n", first, cnz);
                /* scan which tiles across the char region actually have data,
                   to locate where the uploaded glyphs/frame really live */
                int filled = 0, firstfilled = -1;
                for (int t = 0; t < 1024; ++t) {
                    int any = 0;
                    for (int k = 0; k < 32; ++k)
                        if (*reinterpret_cast<volatile uint8_t *>(bgs[3].chars + t * 32 + k)) { any = 1; break; }
                    if (any) { ++filled; if (firstfilled < 0) firstfilled = t; }
                }
                std::fprintf(stderr, "[msgcomp]   BG3 char region: %d/1024 tiles have data, first at t%d\n",
                             filled, firstfilled);
                /* what do the tilemap entries look like across the top rows
                   (where func_0201f32c writes the box frame starting at +0x42 =
                   row1 col1) -- is it all 895 or a mix incl. glyph tiles 0x200+? */
                for (int ry = 0; ry < 4; ++ry) {
                    std::fprintf(stderr, "[msgcomp]   BG3 row%d:", ry);
                    for (int tx = 0; tx < 20; ++tx)
                        std::fprintf(stderr, " %u", rd16(bgs[3].screen + (ry * 32 + tx) * 2) & 0x3FF);
                    std::fprintf(stderr, "\n");
                }
            }
        }
    }

    // Composite the covered 2D pixels over the 3D framebuffer. The window's OBJ
    // and colour-effect bits are already resolved above for BGs; the sprite
    // raster is drawn wherever OAM places it (the cursor is inside the box
    // window in practice). Integer scale from DS 256x192 to SCREEN_W x SCREEN_H.
    const int sx = ntr::SCREEN_W / 256;
    const int sy = ntr::SCREEN_H / 192;
    for (int y = 0; y < 192; ++y) {
        for (int x = 0; x < 256; ++x) {
            if (!g_a[y][x].hit) continue;
            const uint32_t c = g_a[y][x].color;
            for (int dy = 0; dy < sy; ++dy)
                for (int dx = 0; dx < sx; ++dx)
                    fb.px[y * sy + dy][x * sx + dx] = c;
        }
    }
}
