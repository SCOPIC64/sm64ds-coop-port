#pragma once
#include "coop_frontend.h"
#include <cstdint>
#include <cmath>

namespace coop_frontend {
// The caller supplies the existing port font. Draw into the existing game
// framebuffer; no browser, window, texture assets or second renderer is needed.
struct Canvas {
    uint32_t* pixels;
    int width, height, stride;
    const unsigned char (*font)[8];
    bool valid() const { return pixels && font && width > 0 && height > 0 && stride >= width; }
    int scale() const {
        int s = height / 256;
        if (s > width / 384) s = width / 384;
        return s < 1 ? 1 : s;
    }
    void box(int x, int y, int w, int h, uint32_t color) const {
        for (int py = y < 0 ? 0 : y; py < y + h && py < height; ++py)
            for (int px = x < 0 ? 0 : x; px < x + w && px < width; ++px)
                pixels[py * stride + px] = color;
    }
    void text(int x, int y, const char* str, uint32_t color, int size) const {
        for (; *str; ++str, x += 6 * size) {
            unsigned char ch = static_cast<unsigned char>(*str);
            if (ch < 32 || ch > 126) continue;
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c)
                    if (font[ch - 32][r] & (0x80 >> c))
                        box(x + c * size, y + r * size, size, size, color);
        }
    }
    void centered(int y, const char* str, uint32_t color, int size) const {
        int x = (width - static_cast<int>(std::strlen(str)) * 6 * size) / 2;
        text(x + size, y + size * 2, str, 0xff07101eu, size);
        text(x, y, str, color, size);
    }
};

inline int row_top(const Canvas& c) { return c.height * 40 / 100; }
inline int row_height(const Canvas& c) { int h = c.height * 6 / 100; return h > 0 ? h : 1; }
inline int hit_row(const Canvas& c, const Menu& m, int x, int y) {
    if (!c.valid() || y >= c.height) return -1;
    if (x < c.width / 6 || x >= c.width * 5 / 6 || y < row_top(c)) return -1;
    int row = (y - row_top(c)) / row_height(c);
    return row < m.rows() ? row : -1;
}

inline void draw(const Canvas& c, const Menu& menu, unsigned frame,
                 const char* loading = nullptr) {
    if (!c.valid()) return;
    const int s = c.scale();
    for (int y = 0; y < c.height; ++y) {
        for (int x = 0; x < c.width; ++x) {
            const int checker = ((x / (40 * s)) + (y / (40 * s))) & 1;
            const unsigned r = 10 + checker * 3 + y * 10 / c.height;
            const unsigned g = 22 + checker * 5 + y * 16 / c.height;
            const unsigned b = 48 + checker * 8 + y * 25 / c.height;
            c.pixels[y * c.stride + x] = 0xff000000u | (r << 16) | (g << 8) | b;
        }
    }
    // Small gold star silhouettes, generated geometry rather than game assets.
    for (int star = 0; star < 10; ++star) {
        int x = (star * 137 + 37) % c.width;
        int y = (star * 89 + 23) % c.height;
        int radius = (star % 3 + 2) * s;
        unsigned color = (star + frame / 45) % 3 ? 0xff456187u : 0xffc7a44fu;
        for (int dy = -radius; dy <= radius; ++dy) {
            int span = radius - std::abs(dy);
            c.box(x - span, y + dy, span * 2 + 1, 1, color);
        }
    }
    const char* words[] = { "SUPER", "MARIO" };
    const uint32_t colors[] = { 0xffffc84bu, 0xffff655bu };
    int logo_size = s * 3;
    if (c.width < 500) logo_size = s * 2;
    int x = (c.width - 11 * 6 * logo_size) / 2;
    int y = c.height * 8 / 100;
    for (int i = 0; i < 2; ++i) {
        c.text(x + logo_size, y + 2 * logo_size, words[i], 0xff050d20u, logo_size);
        c.text(x, y, words[i], colors[i], logo_size);
        x += 6 * 6 * logo_size;
    }
    c.centered(c.height * 18 / 100, "64DS CO-OP", 0xff83dcffu, logo_size);
    c.centered(c.height * 31 / 100, menu.title(), 0xffffdf82u, s);
    if (loading) {
        c.centered(c.height * 53 / 100, loading, 0xffffffffu, s);
        c.centered(c.height * 64 / 100, "LOADING YOUR ADVENTURE...", 0xff92aed1u, s);
    } else if (menu.page == CONTROLS) {
        const char* lines[] = {
            "WASD / ARROWS: MOVE    SPACE: JUMP",
            "X: ATTACK    CTRL: CROUCH    SHIFT: RUN",
            "Q/E: CAMERA TURN    R/F: CAMERA TILT",
            "RIGHT MOUSE: LOOK    WHEEL: ZOOM",
            "GAMEPAD: LEFT STICK MOVE / RIGHT STICK CAMERA",
            "A: JUMP    B: ATTACK    RT: CROUCH",
            "F5: GAME OPTIONS    F12: FULLSCREEN",
            "ENTER / B / ESC: BACK"
        };
        for (int i = 0; i < 8; ++i)
            c.centered(row_top(c) + i * row_height(c), lines[i], 0xffe7efffu, s);
    } else {
        for (int i = 0; i < menu.rows(); ++i) {
            int ry = row_top(c) + i * row_height(c);
            if (i == menu.row) {
                c.box(c.width / 6, ry - 3 * s, c.width * 2 / 3, row_height(c) - s, 0xff23466eu);
                c.box(c.width / 6, ry - 3 * s, 3 * s, row_height(c) - s, 0xffffd061u);
            }
            char label[80]; menu.label(i, label, sizeof label);
            c.centered(ry, label, i == menu.row ? 0xffffdf82u : 0xffe7efffu, s);
        }
    }
    const char* footer = menu.message[0] ? menu.message :
        menu.editing ? "TYPE WITH KEYBOARD - ENTER TO FINISH" :
        (menu.page == HOST || menu.page == JOIN) ? "LAN CO-OP - BOTH PLAYERS NEED THIS VERSION" :
#ifdef SM64DS_HANDHELD
        "D-PAD: CHOOSE   A: SELECT   B: BACK";
#else
        "ARROWS / D-PAD: CHOOSE   ENTER / A: SELECT   ESC / B: BACK";
#endif
    c.centered(c.height * 91 / 100, footer, menu.message[0] ? 0xffffad8eu : 0xffa4bad6u, s);
    c.centered(c.height * 96 / 100, "SM64DS CO-OP  /  0.5.4 PREVIEW", 0xff6b88b0u, s);
}
} // namespace coop_frontend
