// See logo.h.
#include "logo.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool port_logo_load(PortLogo &logo)
{
    logo.px = 0;
    logo.w = logo.h = 0;
    char path[MAX_PATH];
    if (!GetModuleFileNameA(0, path, sizeof path)) return false;
    std::string p(path);
    const size_t slash = p.find_last_of("\\/");
    p = (slash == std::string::npos ? std::string("logo.bmp")
                                    : p.substr(0, slash + 1) + "logo.bmp");
    FILE *f = std::fopen(p.c_str(), "rb");
    if (!f) return false;

    unsigned char fileHead[14], infoHead[40];
    if (std::fread(fileHead, 1, 14, f) != 14 || fileHead[0] != 'B' ||
        fileHead[1] != 'M') {
        std::fclose(f);
        return false;
    }
    if (std::fread(infoHead, 1, 40, f) != 40) {
        std::fclose(f);
        return false;
    }
    const int w = infoHead[4] | (infoHead[5] << 8) | (infoHead[6] << 16) |
                  (infoHead[7] << 24);
    int h = infoHead[8] | (infoHead[9] << 8) | (infoHead[10] << 16) |
            (infoHead[11] << 24);
    const int planes = infoHead[12] | (infoHead[13] << 8);
    const int bpp = infoHead[14] | (infoHead[15] << 8);
    const int comp = infoHead[16] | (infoHead[17] << 8) |
                     (infoHead[18] << 16) | (infoHead[19] << 24);
    const int dataOff = fileHead[10] | (fileHead[11] << 8) |
                        (fileHead[12] << 16) | (fileHead[13] << 24);
    if (w <= 0 || w > 2048 || h == 0 || h > 2048 || h < -2048 || planes != 1 ||
        (bpp != 24 && bpp != 32) || comp != 0) {
        std::fclose(f);
        return false;
    }
    const int flip = h < 0 ? 1 : 0;
    const int ah = h < 0 ? -h : h;
    const int stride = ((w * (bpp / 8) + 3) & ~3);
    unsigned *px = (unsigned *)std::malloc((size_t)w * ah * 4);
    unsigned char *row = (unsigned char *)std::malloc((size_t)stride);
    if (!px || !row) {
        std::free(px);
        std::free(row);
        std::fclose(f);
        return false;
    }
    std::fseek(f, dataOff, SEEK_SET);
    for (int y = 0; y < ah; ++y) {
        if (std::fread(row, 1, stride, f) != (size_t)stride) {
            std::free(px);
            std::free(row);
            std::fclose(f);
            return false;
        }
        const int dst_y = flip ? y : (ah - 1 - y);
        for (int x = 0; x < w; ++x) {
            unsigned r, g, b, a, stored;
            if (bpp == 32) {
                b = row[x * 4];
                g = row[x * 4 + 1];
                r = row[x * 4 + 2];
                stored = row[x * 4 + 3];
            } else {
                b = row[x * 3];
                g = row[x * 3 + 1];
                r = row[x * 3 + 2];
                stored = 255;
            }
            /* flattened-white despill: fully transparent stays so;
               fully opaque stays so (white art highlights survive);
               partial alpha is re-cut from the white page -- the source
               was flattened onto white, so its edge pixels are white
               blends, and only a re-cut removes the halo. */
            if (stored == 0) {
                a = 0;
            } else if (stored == 255 && bpp == 32) {
                a = 255;
            } else {
                /* 225..240 ramp off the white page (both depths) */
                unsigned m = r < g ? r : g;
                m = m < b ? m : b;
                const int t = (int)m - 225;
                a = t <= 0 ? 255 : (t >= 15 ? 0 : 255 - t * 17);
            }
            px[dst_y * w + x] = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
    std::free(row);
    std::fclose(f);
    /* inpaint flattened-white edges: the art was flattened onto white,
       so partial-alpha edge texels carry white-blended rgb that would
       halo against the dark rail. Replace their color with the nearest
       fully-opaque neighbour's (letters/outlines), keeping alpha: the
       downscale then composites true edge colors. */
    for (int y = 0; y < ah; ++y)
        for (int x = 0; x < w; ++x) {
            const unsigned p = px[y * w + x];
            const unsigned a = p >> 24;
            if (a == 0 || a == 255) continue;
            int done = 0;
            for (int r = 1; r <= 4 && !done; ++r)
                for (int ky = -r; ky <= r && !done; ++ky)
                    for (int kx = -r; kx <= r; ++kx) {
                        const int nx = x + kx, ny = y + ky;
                        if (nx < 0 || ny < 0 || nx >= w || ny >= ah)
                            continue;
                        const unsigned q = px[ny * w + nx];
                        if ((q >> 24) == 255) {
                            px[y * w + x] = (a << 24) | (q & 0x00ffffffu);
                            done = 1;
                            break;
                        }
                    }
        }
    logo.px = px;
    logo.w = w;
    logo.h = ah;
    std::printf("[logo] %s: %dx%d %dbpp\n", p.c_str(), w, ah, bpp);
    return true;
}

void port_logo_free(PortLogo &logo)
{
    std::free(logo.px);
    logo.px = 0;
    logo.w = logo.h = 0;
}

void port_logo_blit(unsigned *dst, int dst_w, int dst_h, const PortLogo &logo,
                    int x0, int y0, int draw_w, int draw_h)
{
    if (!logo.px || draw_w <= 0 || draw_h <= 0) return;
    /* footprint box filter: average every source texel the destination
       pixel covers (premultiplied by alpha), then blend over what is
       already there. Downscales come out smooth instead of chunky, and
       soft art edges composite instead of haloing. Menu-only; the cost
       is one small blit a frame while the menu is up. */
    for (int y = 0; y < draw_h; ++y) {
        const int fy = y0 + y;
        if (fy < 0 || fy >= dst_h) continue;
        int sy0 = (int)((long long)y * logo.h / draw_h);
        int sy1 = (int)((long long)(y + 1) * logo.h / draw_h);
        if (sy1 <= sy0) sy1 = sy0 + 1;
        if (sy0 < 0) sy0 = 0;
        if (sy1 > logo.h) sy1 = logo.h;
        for (int x = 0; x < draw_w; ++x) {
            const int fx = x0 + x;
            if (fx < 0 || fx >= dst_w) continue;
            int sx0 = (int)((long long)x * logo.w / draw_w);
            int sx1 = (int)((long long)(x + 1) * logo.w / draw_w);
            if (sx1 <= sx0) sx1 = sx0 + 1;
            if (sx0 < 0) sx0 = 0;
            if (sx1 > logo.w) sx1 = logo.w;
            /* premultiplied footprint average over the eroded matte:
               transparent texels contribute nothing, survivors average
               true, then blend over the framebuffer */
            long long sr = 0, sg = 0, sb = 0, sa = 0;
            const long long n =
                (long long)(sx1 - sx0) * (sy1 - sy0);
            for (int sy = sy0; sy < sy1; ++sy)
                for (int sx = sx0; sx < sx1; ++sx) {
                    const unsigned p = logo.px[sy * logo.w + sx];
                    const unsigned a = p >> 24;
                    sa += a;
                    sr += ((p >> 16) & 0xff) * a;
                    sg += ((p >> 8) & 0xff) * a;
                    sb += (p & 0xff) * a;
                }
            const unsigned a8 = (unsigned)(sa / n);
            if (!a8) continue;
            const unsigned r8 = (unsigned)(sr / sa);
            const unsigned g8 = (unsigned)(sg / sa);
            const unsigned b8 = (unsigned)(sb / sa);
            const unsigned d = dst[fy * dst_w + fx];
            const unsigned ia = 255 - a8;
            dst[fy * dst_w + fx] =
                0xFF000000u |
                (((r8 * a8 + ((d >> 16) & 0xff) * ia) / 255) << 16) |
                (((g8 * a8 + ((d >> 8) & 0xff) * ia) / 255) << 8) |
                ((b8 * a8 + (d & 0xff) * ia) / 255);
        }
    }
}
