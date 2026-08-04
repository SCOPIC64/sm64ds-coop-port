// DS 2D engine scan-out.
//
// The 2D engines are pure register state: DISPCNT and BGxCNT say where the
// tilemaps and tiles live, and VRAM holds them. Nothing about drawing is
// write-triggered, so the host can simply read the register file once per frame
// and rasterise -- no interception needed, which is why 68% of the decomp's
// hardware accesses need no more than a working memory map to behave correctly.
//
// See notes/assessment.md section 2a and docs/mmio-inventory.md.

#ifndef NTR_PPU_H
#define NTR_PPU_H

#include <stdint.h>

namespace ntr {

#ifdef NTR_HIRES
constexpr int SCREEN_W = 1024;   /* 4x the DS panel; the smokes and their
                                    reference pixel counts stay on 256x192
                                    (the plain ntr lib) */
constexpr int SCREEN_H = 768;
#elif defined(NTR_HIRES2)
constexpr int SCREEN_W = 512;    /* 2x: the interactive window's tier */
constexpr int SCREEN_H = 384;
#else
constexpr int SCREEN_W = 256;
constexpr int SCREEN_H = 192;
#endif

enum Engine { ENGINE_A = 0, ENGINE_B = 1 };

struct Framebuffer {
    uint32_t px[SCREEN_H][SCREEN_W];   // 0xAARRGGBB
};

// Rasterise one engine's current register state into fb. Text-mode BGs only for
// now; affine, bitmap, sprites and the blend/window units are not implemented.
void ppu_scanout(Engine eng, Framebuffer &fb);

// Rasterise one engine's OBJ layer (sprites) from its OAM + OBJ VRAM over
// whatever fb already holds. Plain and affine sprites, 16- and 256-color
// tiles, 1D mapping; bitmap OBJs, mosaic, windows and blending are not
// implemented. Transparent color-0 texels leave fb untouched.
void ppu_scanout_obj(Engine eng, Framebuffer &fb);

// Debug output, so a frame can be inspected without a window yet.
bool ppu_write_bmp(const char *path, const Framebuffer &fb);

// ---- the bottom screen ------------------------------------------------------
//
// The sub engine is a tile raster whose every address is a DS pixel, so unlike
// the 3D top screen it exists at exactly one size. It gets its own fixed
// framebuffer and its own compositor (port/ntr/ppu_sub.cpp), and is scaled --
// or, as the port does it, not scaled -- only when it reaches the window.
constexpr int SUB_W = 256;
constexpr int SUB_H = 192;

struct SubFramebuffer {
    uint32_t px[SUB_H][SUB_W];   // 0xAARRGGBB
};

// Engine B, whole: text/affine/extended-affine backgrounds with extended
// palettes, sprites, the window unit and master brightness, all resolved
// together by priority. No OBJ window, no BLDCNT blending, no bitmap BGs.
void ppu_scanout_sub(SubFramebuffer &fb);

bool ppu_write_bmp_sub(const char *path, const SubFramebuffer &fb);

// Blit the bottom screen 1:1 into the bottom-right corner of a dst_w x dst_h
// ARGB buffer, `margin` pixels in from both edges, with a one-pixel frame.
void ppu_compose_sub(const SubFramebuffer &sub, uint32_t *dst, int dst_w,
                     int dst_h, int margin);

}  // namespace ntr

#endif  // NTR_PPU_H
