#pragma once

// Front-menu logo (the SM64 DS COOP wordmark), host side.
//
// Loads logo.bmp from beside the executable (24-bit BMP, white background
// chroma-keyed to transparent at load). Missing or unreadable file = no
// logo; the menu falls back to its text wordmark. Nothing here is game
// code: it is a file reader plus a framebuffer blitter.

struct PortLogo {
    unsigned *px;   // ARGB, 0 alpha = transparent
    int w, h;
};

bool port_logo_load(PortLogo &logo);
void port_logo_free(PortLogo &logo);

// Nearest-neighbor blit, transparent-safe, clipped to the framebuffer.
void port_logo_blit(unsigned *dst, int dst_w, int dst_h, const PortLogo &logo,
                    int x0, int y0, int draw_w, int draw_h);
