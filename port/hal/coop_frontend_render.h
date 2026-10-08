#pragma once
#include "coop_frontend.h"
#include <cstdint>
#include <cmath>
#include <cstring>

namespace coop_frontend {
// Decoded from the player's extracted SM64DS message-font tiles (file 0x980e).
// This object contains no game bytes until load() is called at runtime.
struct RomFont {
    uint8_t rows[95][16] = {};
    uint8_t advances[95] = {};
    uint8_t present[95] = {};
    bool ready = false;

    static int game_code(unsigned char ascii) {
        if (ascii >= '0' && ascii <= '9') return ascii - '0';
        if (ascii >= 'A' && ascii <= 'Z') return 0x0a + ascii - 'A';
        if (ascii >= 'a' && ascii <= 'z') return 0x2d + ascii - 'a';
        if (ascii == ' ') return 0x4d;
        return -1;
    }

    bool load(const uint8_t* tiles, size_t tile_bytes,
              const uint8_t* widths = nullptr, size_t width_count = 0) {
        std::memset(rows, 0, sizeof rows);
        std::memset(advances, 0, sizeof advances);
        std::memset(present, 0, sizeof present);
        ready = false;
        if (!tiles || tile_bytes < 0x4000) return false;
        for (int ascii = 32; ascii <= 126; ++ascii) {
            const int code = game_code(static_cast<unsigned char>(ascii));
            if (code < 0) continue;
            const size_t top_off = static_cast<size_t>(
                ((code & 0x1f) + ((code & 0xe0) << 1)) << 5);
            const size_t bottom_off = top_off + 0x400;
            if (bottom_off + 32 > tile_bytes) return false;
            int right = 0;
            for (int row = 0; row < 16; ++row) {
                const uint8_t* source = tiles + (row < 8 ? top_off : bottom_off) +
                                        (row & 7) * 4;
                uint8_t bits = 0;
                for (int x = 0; x < 8; ++x) {
                    const uint8_t packed = source[x >> 1];
                    const uint8_t ink = (x & 1) ? packed >> 4 : packed & 0x0f;
                    if (ink) { bits |= static_cast<uint8_t>(0x80 >> x); right = x + 1; }
                }
                rows[ascii - 32][row] = bits;
            }
            int advance = widths && static_cast<size_t>(code) < width_count
                              ? widths[code] : right + 1;
            if (advance < 1 || advance > 16) advance = right ? right + 1 : 4;
            advances[ascii - 32] = static_cast<uint8_t>(advance);
            present[ascii - 32] = 1;
        }
        ready = present['A' - 32] != 0;
        return ready;
    }
};

// The caller supplies the existing port font. Draw into the existing game
// framebuffer; no browser, window, texture assets or second renderer is needed.
struct Canvas {
    uint32_t* pixels;
    int width, height, stride;
    const unsigned char (*font)[8];
    const RomFont* rom_font = nullptr;
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
    int glyph_height() const { return rom_font && rom_font->ready ? 16 : 8; }
    int advance(unsigned char ch) const {
        if (rom_font && rom_font->ready && ch >= 32 && ch <= 126 &&
            rom_font->present[ch - 32])
            return rom_font->advances[ch - 32];
        return 6;
    }
    int measure(const char* str, int size) const {
        int result = 0;
        for (; *str; ++str) result += advance(static_cast<unsigned char>(*str)) * size;
        return result;
    }
    void text(int x, int y, const char* str, uint32_t color, int size) const {
        for (; *str; ++str) {
            unsigned char ch = static_cast<unsigned char>(*str);
            if (ch < 32 || ch > 126) continue;
            const bool use_rom = rom_font && rom_font->ready && rom_font->present[ch - 32];
            const int glyph_rows = use_rom ? 16 : 8;
            for (int r = 0; r < glyph_rows; ++r)
                for (int c = 0; c < 8; ++c)
                    if ((use_rom ? rom_font->rows[ch - 32][r] : font[ch - 32][r]) &
                        (0x80 >> c))
                        box(x + c * size, y + r * size, size, size, color);
            x += advance(ch) * size;
        }
    }
    void centered(int y, const char* str, uint32_t color, int size) const {
        int x = (width - measure(str, size)) / 2;
        text(x + size, y + size * 2, str, 0xff07101eu, size);
        text(x, y, str, color, size);
    }
};

// Drawing and pointer input share these regions, including the Controls back button.
inline int panel_width(const Canvas& c, const Menu& m) {
    return m.page == HOME ? c.width * 42 / 100 : c.width * 90 / 100;
}
inline int panel_left(const Canvas& c, const Menu& m) {
    return m.page == HOME ? c.width / 30 : (c.width - panel_width(c, m)) / 2;
}
inline int button_left(const Canvas& c, const Menu& m) { return panel_left(c, m) + c.width / 50; }
inline int button_width(const Canvas& c, const Menu& m) { return panel_width(c, m) - c.width / 25; }
inline int row_top(const Canvas& c, const Menu& m) {
    return c.height * (m.page == HOME ? 39 : m.page == CONTROLS ? 77 : 27) / 100;
}
inline int row_height(const Canvas& c) { int h = c.height * 8 / 100; return h > 0 ? h : 1; }
inline int hit_row(const Canvas& c, const Menu& m, int x, int y) {
    if (!c.valid() || x < 0 || y < 0 || x >= c.width || y >= c.height) return -1;
    if (x < button_left(c, m) || x >= button_left(c, m) + button_width(c, m) ||
        y < row_top(c, m)) return -1;
    int row = (y - row_top(c, m)) / row_height(c);
    if (row >= m.rows() || (y - row_top(c, m)) % row_height(c) >= row_height(c) * 4 / 5) return -1;
    return row;
}
inline void panel_text(const Canvas& c, const Menu& m, int y, const char* text,
                       uint32_t color, int size) {
    const int x = panel_left(c, m) + (panel_width(c, m) - c.measure(text, size)) / 2;
    c.text(x, y, text, color, size);
}
inline void draw(const Canvas& c, const Menu& menu, unsigned frame,
                 const char* loading = nullptr) {
    if (!c.valid()) return;
    (void)frame;
    const int s = c.scale();
    // Asset-free backdrop until the engine can render its own live title scene.
    for (int y = 0; y < c.height; ++y) {
        const unsigned r = 46 + y * 20 / c.height;
        const unsigned g = 103 + y * 28 / c.height;
        const unsigned b = 158 + y * 31 / c.height;
        c.box(0, y, c.width, 1, 0xff000000u | (r << 16) | (g << 8) | b);
    }
    const int px = panel_left(c, menu), pw = panel_width(c, menu);
    c.box(px, c.height / 12, pw, c.height * 79 / 100, 0xff141414u);
    c.box(px, c.height / 12, pw, 2 * s, 0xff4b4b4bu);
    if (menu.page == HOME) {
        const char* title = "SUPER MARIO";
        const uint32_t colors[] = { 0xfff34b46u, 0xff5dbef7u, 0xffffd542u, 0xff60cd65u };
        const int size = s * 2;
        int x = px + (pw - c.measure(title, size)) / 2;
        for (int i = 0; title[i]; ++i) {
            char letter[] = { title[i], 0 };
            c.text(x + size, c.height * 15 / 100 + size, letter, 0xff000000u, size);
            c.text(x, c.height * 15 / 100, letter, colors[i % 4], size);
            x += c.advance(static_cast<unsigned char>(title[i])) * size;
        }
        panel_text(c, menu, c.height * 23 / 100, "64 DS CO-OP", 0xffffffffu, size);
    } else {
        panel_text(c, menu, c.height * 16 / 100, menu.title(), 0xffffd542u, s * 2);
    }
    if (loading) {
        panel_text(c, menu, c.height * 43 / 100, loading, 0xffffffffu, s);
        panel_text(c, menu, c.height * 56 / 100, "LOADING...", 0xffddddddU, s);
    } else {
        if (menu.page == CONTROLS) {
            const char* lines[] = {
                "WASD / ARROWS: MOVE   SPACE: JUMP",
                "X: ATTACK   CTRL: CROUCH   SHIFT: RUN",
                "Q/E: TURN   R/F: TILT CAMERA",
                "RIGHT MOUSE: LOOK   WHEEL: ZOOM",
                "LEFT STICK: MOVE   RIGHT STICK: CAMERA",
                "A: JUMP   B: ATTACK   RT: CROUCH",
                "F5: GAME OPTIONS   F12: FULLSCREEN"
            };
            for (int i = 0; i < 7; ++i)
                panel_text(c, menu, c.height * (29 + i * 6) / 100, lines[i], 0xffeeeeeeu, s);
        }
        for (int i = 0; i < menu.rows(); ++i) {
            const bool selected = i == menu.row;
            int y = row_top(c, menu) + i * row_height(c);
            int height = row_height(c) * 4 / 5;
            c.box(button_left(c, menu), y, button_width(c, menu), height,
                  selected ? 0xff0078d7u : 0xff4b4b4bu);
            c.box(button_left(c, menu) + s, y + s, button_width(c, menu) - 2 * s, height - 2 * s,
                  selected ? 0xffe5f1fbu : 0xffdededeu);
            char label[80]; menu.label(i, label, sizeof label);
            int text_size = s;
            while (text_size > 1 && c.measure(label, text_size) > button_width(c, menu) - 4 * s) --text_size;
            panel_text(c, menu, y + (height - c.glyph_height() * text_size) / 2,
                       label, 0xff0b0b0bu, text_size);
        }
    }
    const char* footer = menu.message[0] ? menu.message :
        menu.editing ? "TYPE ADDRESS / PORT - ENTER TO FINISH" :
        (menu.page == HOST || menu.page == JOIN) ? "LAN CO-OP - MATCHING VERSIONS REQUIRED" :
#ifdef SM64DS_HANDHELD
        "D-PAD: CHOOSE   A: SELECT   B: BACK";
#else
        "ARROWS: CHOOSE   ENTER: SELECT   ESC: BACK";
#endif
    c.centered(c.height * 91 / 100, footer, menu.message[0] ? 0xffffad8eu : 0xffffffffu, s);
    c.centered(c.height * 96 / 100, "SM64DS CO-OP / 0.5.4 PREVIEW", 0xffddddddU, s);
}
} // namespace coop_frontend
