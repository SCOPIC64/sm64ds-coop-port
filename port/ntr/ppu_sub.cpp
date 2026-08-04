// The DS BOTTOM SCREEN, rasterised whole.
//
// ppu.cpp scans one engine's text backgrounds and one engine's sprites into a
// Framebuffer, which is SCREEN_W x SCREEN_H -- the tier the top screen is
// rendered at (512x384 in the window, 1024x768 in the hires clone). The bottom
// screen cannot use that. It is not a 3D scene that can be rendered at any
// resolution: it is a tile raster whose every address is a DS pixel, so the
// only size it exists at is 256x192. Scaling it is the compositor's job, not
// the rasteriser's.
//
// So this file is a second, complete engine-B compositor at fixed 256x192.
// "Complete" as against ppu.cpp's two independent passes: the DS resolves
// sprites and backgrounds together by priority, and the bottom screen is where
// that matters -- the minimap sits on BG3 at priority 2 and the HUD's sprites
// have to land either side of it depending on their own priority word.
//
// What is here and what is not:
//
//   - text BGs, affine BGs, and EXTENDED AFFINE with 16-bit map entries (the
//     mode the minimap runs in: 8bpp tiles, per-tile palette select, rotated
//     and scaled by the BG3 matrix).
//   - BG EXTENDED PALETTES. Minimap::InitResources loads its colours through
//     GXS::LoadBGExtPltt and nothing else, so without this the minimap is a
//     black rectangle. The base address is the one that function writes to,
//     0x06898000, and the slot stride is the 0x2000 its own 0x6000 argument
//     implies for BG3.
//   - the WINDOW unit (WIN0/WIN1/WININ/WINOUT), which is what Message boxes
//     drive.
//   - MASTER BRIGHTNESS, which is how the sub screen fades.
//
//   - no OBJ window (mode 3 sprites are skipped, as in ppu.cpp)
//   - no BLDCNT alpha blending: the second-target machinery is a bigger
//     rewrite than the bottom screen has needed so far, and master brightness
//     covers the fades. Semi-transparent sprites draw opaque.
//   - no bitmap BGs, no mosaic.
//
// Address arithmetic is taken from the decomp's own byte-verified getters
// rather than from docs wherever one exists -- G2S::GetBG0CharPtr computes
//
//     0x6200000 + ((BG0CNT_SUB & 0x3c) >> 2 << 14)
//
// which is a FOUR-bit character base field, not the two bits a GBA-shaped
// reader would use. ppu.cpp had two, which put every engine-B tile fetch at
// character base 0.

#include "ntr/ppu.h"

#include <cstdio>
#include <cstring>

namespace ntr {
namespace {

constexpr uint32_t kRegBase = 0x04001000u;   // engine B
constexpr uint32_t kVramBase = 0x06200000u;  // engine B BG VRAM
constexpr uint32_t kPlttBase = 0x05000400u;  // engine B BG palette
constexpr uint32_t kObjVram = 0x06600000u;   // engine B OBJ VRAM
constexpr uint32_t kObjPltt = 0x05000600u;   // engine B OBJ palette
constexpr uint32_t kOamBase = 0x07000400u;   // engine B OAM
// GXS::LoadBGExtPltt's own destination base; slots are 0x2000 apart (its
// caller passes 0x6000 for BG3).
constexpr uint32_t kBgExtPltt = 0x06898000u;

inline uint16_t rd16(uint32_t a) { return *reinterpret_cast<volatile uint16_t *>(a); }
inline uint32_t rd32(uint32_t a) { return *reinterpret_cast<volatile uint32_t *>(a); }
inline uint8_t rd8(uint32_t a) { return *reinterpret_cast<volatile uint8_t *>(a); }

inline uint32_t bgr555(uint16_t c) {
    const uint32_t r = (c >> 0) & 0x1F, g = (c >> 5) & 0x1F, b = (c >> 10) & 0x1F;
    return 0xFF000000u | ((r << 3 | r >> 2) << 16) | ((g << 3 | g >> 2) << 8)
           | (b << 3 | b >> 2);
}

// ---- backgrounds ------------------------------------------------------------

enum BgKind { BG_OFF, BG_TEXT, BG_AFFINE, BG_EXT_AFFINE };

struct BgLayer {
    BgKind kind;
    int prio;
    uint32_t screen;   // map address
    uint32_t chars;    // tile data address
    bool bpp8;
    int map_w, map_h;  // text: tiles; affine: pixels
    bool wrap;         // affine display-area overflow
    int hofs, vofs;    // text scroll
    int pa, pb, pc, pd;
    int refx, refy;    // 8.8 fixed
    uint32_t ext;      // extended palette slot base, 0 = not in use
};

// The DS BG-mode table, minus the 3D column engine B does not have. Each entry
// is the kind of BG0..BG3 in that mode. Mode 6 (large bitmap) is engine A only
// and is left off.
const unsigned char kModeKinds[6][4] = {
    /* 0 */ {BG_TEXT, BG_TEXT, BG_TEXT, BG_TEXT},
    /* 1 */ {BG_TEXT, BG_TEXT, BG_TEXT, BG_AFFINE},
    /* 2 */ {BG_TEXT, BG_TEXT, BG_AFFINE, BG_AFFINE},
    /* 3 */ {BG_TEXT, BG_TEXT, BG_TEXT, BG_EXT_AFFINE},
    /* 4 */ {BG_TEXT, BG_TEXT, BG_AFFINE, BG_EXT_AFFINE},
    /* 5 */ {BG_TEXT, BG_TEXT, BG_EXT_AFFINE, BG_EXT_AFFINE},
};

void read_bg(BgLayer &c, int bg, uint32_t dispcnt) {
    c.kind = BG_OFF;
    if (!((dispcnt >> (8 + bg)) & 1))
        return;
    const unsigned mode = dispcnt & 7;
    if (mode > 5)
        return;
    BgKind kind = (BgKind)kModeKinds[mode][bg];

    const uint16_t cnt = rd16(kRegBase + 0x08 + bg * 2);
    c.prio = cnt & 3;
    c.bpp8 = (cnt >> 7) & 1;

    // FOUR bits of character base, 16K units -- G2S::GetBG0CharPtr's own
    // (v & 0x3c) >> 2 << 14. Engine B has no DISPCNT base offset.
    c.chars = kVramBase + (((cnt & 0x3c) >> 2) << 14);
    c.screen = kVramBase + (((cnt & 0x1f00) >> 8) << 11);

    const int sz = (cnt >> 14) & 3;
    if (kind == BG_EXT_AFFINE && c.bpp8) {
        // bit 7 set in an extended BG selects a bitmap, which is not hosted.
        c.kind = BG_OFF;
        return;
    }
    if (kind == BG_TEXT) {
        c.map_w = (sz & 1) ? 64 : 32;
        c.map_h = (sz & 2) ? 64 : 32;
        c.hofs = rd16(kRegBase + 0x10 + bg * 4) & 0x1FF;
        c.vofs = rd16(kRegBase + 0x12 + bg * 4) & 0x1FF;
    } else {
        c.map_w = c.map_h = 128 << sz;   // 128 / 256 / 512 / 1024 pixels
        c.wrap = (cnt >> 13) & 1;
        // Affine registers: BG2 at 0x20, BG3 at 0x30 (engine-relative).
        const uint32_t ab = kRegBase + 0x20 + (bg - 2) * 0x10;
        c.pa = (int16_t)rd16(ab + 0);
        c.pb = (int16_t)rd16(ab + 2);
        c.pc = (int16_t)rd16(ab + 4);
        c.pd = (int16_t)rd16(ab + 6);
        // BGxX/BGxY are 28-bit signed 20.8 values.
        c.refx = (int)(rd32(ab + 8) << 4) >> 4;
        c.refy = (int)(rd32(ab + 12) << 4) >> 4;
        // Extended affine with 16-bit entries uses 8bpp tiles whatever bit 7
        // said; bit 7 selected bitmap-vs-map, and we rejected bitmap above.
        if (kind == BG_EXT_AFFINE)
            c.bpp8 = true;
    }

    // Extended palettes. DISPCNT bit 30 arms them; they apply to 256-colour
    // BGs only. BG0/BG1 pick slot 0/1 or 2/3 through BGxCNT bit 13; BG2/BG3
    // are hardwired to slots 2/3.
    c.ext = 0;
    if (((dispcnt >> 30) & 1) && c.bpp8) {
        const int slot = bg < 2 ? (bg + (((cnt >> 13) & 1) ? 2 : 0)) : bg;
        c.ext = kBgExtPltt + (uint32_t)slot * 0x2000u;
    }
    c.kind = kind;
}

inline uint32_t map_entry_addr(const BgLayer &c, int tx, int ty) {
    const int bx = tx >> 5, by = ty >> 5;
    const int blocks_w = c.map_w >> 5;
    return c.screen + (by * blocks_w + bx) * 0x800 + (((ty & 31) * 32 + (tx & 31)) * 2);
}

// One BG pixel. Returns false where the layer is transparent here.
bool sample_bg(const BgLayer &c, int x, int y, uint32_t &out) {
    int px, py;
    uint16_t se;
    int tile, fx, fy;

    if (c.kind == BG_TEXT) {
        px = (x + c.hofs) & (c.map_w * 8 - 1);
        py = (y + c.vofs) & (c.map_h * 8 - 1);
        se = rd16(map_entry_addr(c, px >> 3, py >> 3));
        tile = se & 0x3FF;
        fx = px & 7;
        fy = py & 7;
        if (se & 0x400) fx = 7 - fx;
        if (se & 0x800) fy = 7 - fy;
    } else {
        // Affine: texture coordinate = ref + x*P + y*P, in 8.8.
        px = (c.refx + c.pa * x + c.pb * y) >> 8;
        py = (c.refy + c.pc * x + c.pd * y) >> 8;
        if (c.wrap) {
            px &= c.map_w - 1;
            py &= c.map_h - 1;
        } else if (px < 0 || px >= c.map_w || py < 0 || py >= c.map_h) {
            return false;
        }
        const int tw = c.map_w >> 3;
        if (c.kind == BG_AFFINE) {
            // 8-bit map entries: a bare tile number, no flip, no palette.
            tile = rd8(c.screen + (py >> 3) * tw + (px >> 3));
            se = 0;
            fx = px & 7;
            fy = py & 7;
        } else {
            // Extended affine, 16-bit entries: laid out as one flat tw*th
            // array rather than the text BG's 32x32 blocks.
            se = rd16(c.screen + ((py >> 3) * tw + (px >> 3)) * 2);
            tile = se & 0x3FF;
            fx = px & 7;
            fy = py & 7;
            if (se & 0x400) fx = 7 - fx;
            if (se & 0x800) fy = 7 - fy;
        }
    }

    uint32_t index;
    if (c.bpp8) {
        index = rd8(c.chars + (uint32_t)tile * 64 + fy * 8 + fx);
        if (index == 0) return false;
        if (c.ext) {
            out = bgr555(rd16(c.ext + (((se >> 12) & 0xF) * 256 + index) * 2));
            return true;
        }
    } else {
        const uint8_t pair = rd8(c.chars + (uint32_t)tile * 32 + fy * 4 + (fx >> 1));
        index = (fx & 1) ? (pair >> 4) : (pair & 0xF);
        if (index == 0) return false;
        index += ((se >> 12) & 0xF) * 16;
    }
    out = bgr555(rd16(kPlttBase + index * 2));
    return true;
}

// ---- sprites ----------------------------------------------------------------
//
// Rasterised into a whole-screen buffer first, because priority has to be
// resolved against the backgrounds pixel by pixel and a sprite's priority is
// its own, not its layer's. Sprite-vs-sprite order is by OAM index (low wins)
// regardless of priority, which is why the walk runs 127 down to 0 and lets
// later writes overwrite.

struct ObjPixel {
    uint32_t color;
    uint8_t prio;
    uint8_t hit;
};

ObjPixel g_obj[192][256];

void raster_obj(uint32_t dispcnt) {
    static const int kSizes[3][4][2] = {
        {{8, 8}, {16, 16}, {32, 32}, {64, 64}},
        {{16, 8}, {32, 8}, {32, 16}, {64, 32}},
        {{8, 16}, {8, 32}, {16, 32}, {32, 64}},
    };
    std::memset(g_obj, 0, sizeof g_obj);
    if (!((dispcnt >> 12) & 1))
        return;
    const uint32_t boundary = 32u << ((dispcnt >> 20) & 3);
    // DISPCNT bit 4: 1 = one-dimensional mapping (a sprite's tiles are
    // consecutive), 0 = two-dimensional (OBJ VRAM is a 32-tile-wide matrix and
    // the next row of a sprite is 32 tiles on). Stage::InitResources clears it
    // for the sub engine -- `*p1 &= 0xFFCFFFEF` -- so the bottom screen is 2D
    // and a 1D-only reader smears every HUD sprite.
    const bool map1d = (dispcnt >> 4) & 1;

    for (int i = 127; i >= 0; --i) {
        const uint16_t a0 = rd16(kOamBase + i * 8u);
        const uint16_t a1 = rd16(kOamBase + i * 8u + 2);
        const uint16_t a2 = rd16(kOamBase + i * 8u + 4);
        const bool affine = a0 & 0x100;
        if (!affine && (a0 & 0x200)) continue;          // disabled
        if (((a0 >> 10) & 3) == 3) continue;            // OBJ window: not hosted
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
        const uint8_t prio = (a2 >> 10) & 3;

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
                // Where this 8x8 cell lives, in 32-byte tile slots from the
                // sprite's base. 1D: consecutive. 2D: a 32-slot-wide matrix,
                // and a 256-colour cell is two slots wide.
                const uint32_t slot =
                    map1d ? (c256 ? (uint32_t)(trow * (w / 8) + tcol) * 2u
                                  : (uint32_t)(trow * (w / 8) + tcol))
                          : (uint32_t)(trow * 32 + (c256 ? tcol * 2 : tcol));
                const uint32_t cell = kObjVram + tile * boundary + slot * 32u;
                uint32_t color;
                if (c256) {
                    const uint32_t idx = rd8(cell + fy * 8u + fx);
                    if (!idx) continue;
                    color = bgr555(rd16(kObjPltt + idx * 2u));
                } else {
                    const uint8_t b = rd8(cell + fy * 4u + fx / 2);
                    const uint32_t idx = (fx & 1) ? (b >> 4) : (b & 0xF);
                    if (!idx) continue;
                    color = bgr555(rd16(kObjPltt + (pal * 16u + idx) * 2u));
                }
                g_obj[py][px].color = color;
                g_obj[py][px].prio = prio;
                g_obj[py][px].hit = 1;
            }
        }
    }
}

// ---- the window unit --------------------------------------------------------
//
// WIN0H/WIN1H hold X1<<8|X2 and WIN0V/WIN1V Y1<<8|Y2, all edges exclusive on
// the right/bottom. WININ's low six bits are what shows inside window 0 and
// its next six what shows inside window 1; WINOUT's low six are what shows
// outside both. Bits 0..3 are BG0..BG3, bit 4 is OBJ, bit 5 the colour effect.

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
        // A right/bottom edge at or before the left/top one means the window
        // runs to the end of the line, which is how the hardware behaves when
        // the register holds 0 for a full-width window.
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

// ---- master brightness ------------------------------------------------------

struct Bright { int mode, factor; };

inline uint32_t apply_bright(uint32_t c, const Bright &b) {
    if (!b.factor || b.mode == 0 || b.mode == 3) return c;
    int r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, bl = c & 0xFF;
    if (b.mode == 1) {
        r += (255 - r) * b.factor / 16;
        g += (255 - g) * b.factor / 16;
        bl += (255 - bl) * b.factor / 16;
    } else {
        r -= r * b.factor / 16;
        g -= g * b.factor / 16;
        bl -= bl * b.factor / 16;
    }
    return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)bl;
}

}  // namespace

void ppu_scanout_sub(SubFramebuffer &fb)
{
    const uint32_t dispcnt = rd32(kRegBase);
    const unsigned disp_mode = (dispcnt >> 16) & 3;
    const bool forced_blank = (dispcnt >> 7) & 1;

    Bright br;
    {
        const uint16_t mb = rd16(kRegBase + 0x6C);
        br.factor = mb & 0x1F;
        if (br.factor > 16) br.factor = 16;
        br.mode = (mb >> 14) & 3;
    }

    if (disp_mode == 0 || forced_blank) {
        // Display off is white on a DS panel, not black.
        for (int y = 0; y < SUB_H; ++y)
            for (int x = 0; x < SUB_W; ++x) fb.px[y][x] = 0xFFFFFFFFu;
        return;
    }

    BgLayer bgs[4];
    for (int i = 0; i < 4; ++i) read_bg(bgs[i], i, dispcnt);

    Windows win;
    read_windows(dispcnt, win);

    raster_obj(dispcnt);

    const uint32_t backdrop = bgr555(rd16(kPlttBase));

    for (int y = 0; y < SUB_H; ++y) {
        for (int x = 0; x < SUB_W; ++x) {
            const unsigned mask = window_mask(win, x, y);
            uint32_t c = backdrop;
            // Priority 0 is nearest. At equal priority a sprite is above a
            // background, and among backgrounds the lower number wins.
            for (int prio = 0; prio < 4; ++prio) {
                const ObjPixel &o = g_obj[y][x];
                if (o.hit && o.prio == prio && (mask & 0x10)) {
                    c = o.color;
                    goto done;
                }
                for (int bg = 0; bg < 4; ++bg) {
                    if (bgs[bg].kind == BG_OFF || bgs[bg].prio != prio) continue;
                    if (!(mask & (1u << bg))) continue;
                    uint32_t s;
                    if (sample_bg(bgs[bg], x, y, s)) {
                        c = s;
                        goto done;
                    }
                }
            }
        done:
            fb.px[y][x] = apply_bright(c, br);
        }
    }
}

bool ppu_write_bmp_sub(const char *path, const SubFramebuffer &fb)
{
    std::FILE *f = std::fopen(path, "wb");
    if (!f) return false;

    const uint32_t stride = SUB_W * 3;
    const uint32_t pad = (4 - (stride & 3)) & 3;
    const uint32_t img = (stride + pad) * SUB_H;
    const uint32_t off = 54;

    uint8_t hdr[54] = {};
    hdr[0] = 'B'; hdr[1] = 'M';
    const uint32_t total = off + img;
    std::memcpy(hdr + 2, &total, 4);
    std::memcpy(hdr + 10, &off, 4);
    const uint32_t dib = 40;
    std::memcpy(hdr + 14, &dib, 4);
    const int32_t w = SUB_W, h = SUB_H;
    std::memcpy(hdr + 18, &w, 4);
    std::memcpy(hdr + 22, &h, 4);
    const uint16_t planes = 1, bpp = 24;
    std::memcpy(hdr + 26, &planes, 2);
    std::memcpy(hdr + 28, &bpp, 2);
    std::memcpy(hdr + 34, &img, 4);
    std::fwrite(hdr, 1, sizeof hdr, f);

    const uint8_t zero[3] = {0, 0, 0};
    for (int y = SUB_H - 1; y >= 0; --y) {
        for (int x = 0; x < SUB_W; ++x) {
            const uint32_t c = fb.px[y][x];
            const uint8_t bgr[3] = {static_cast<uint8_t>(c), static_cast<uint8_t>(c >> 8),
                                    static_cast<uint8_t>(c >> 16)};
            std::fwrite(bgr, 1, 3, f);
        }
        if (pad) std::fwrite(zero, 1, pad, f);
    }
    std::fclose(f);
    return true;
}

// ---- the corner panel -------------------------------------------------------
//
// Composited at 1:1 DS pixels whatever tier the top screen is drawn at, which
// is the whole reason this file exists. A one-pixel frame around it so the
// panel reads as a panel and not as a corruption of the 3D view.
void ppu_compose_sub(const SubFramebuffer &sub, uint32_t *dst, int dst_w,
                     int dst_h, int margin, int div)
{
    if (div < 1) div = 1;
    const int out_w = SUB_W / div, out_h = SUB_H / div;
    const int x0 = dst_w - out_w - margin;
    const int y0 = dst_h - out_h - margin;
    if (x0 < 1 || y0 < 1) return;      // no room; leave the frame alone

    for (int x = x0 - 1; x <= x0 + out_w; ++x) {
        dst[(y0 - 1) * dst_w + x] = 0xFF000000u;
        dst[(y0 + out_h) * dst_w + x] = 0xFF000000u;
    }
    for (int y = y0 - 1; y <= y0 + out_h; ++y) {
        dst[y * dst_w + (x0 - 1)] = 0xFF000000u;
        dst[y * dst_w + (x0 + out_w)] = 0xFF000000u;
    }
    if (div == 1) {
        for (int y = 0; y < SUB_H; ++y)
            std::memcpy(dst + (y0 + y) * dst_w + x0, sub.px[y], SUB_W * 4);
        return;
    }
    const int n = div * div;
    for (int y = 0; y < out_h; ++y)
        for (int x = 0; x < out_w; ++x) {
            unsigned r = 0, g = 0, b = 0;
            for (int sy = 0; sy < div; ++sy)
                for (int sx = 0; sx < div; ++sx) {
                    const uint32_t p = sub.px[y * div + sy][x * div + sx];
                    r += (p >> 16) & 0xFF;
                    g += (p >> 8) & 0xFF;
                    b += p & 0xFF;
                }
            dst[(y0 + y) * dst_w + (x0 + x)] =
                0xFF000000u | ((r / n) << 16) | ((g / n) << 8) | (b / n);
        }
}

}  // namespace ntr
