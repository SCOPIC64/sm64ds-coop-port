#include "hal/coop_frontend.h"
#include "hal/coop_frontend_render.h"
#include "hal/coop_touch.h"
#include "overlay_font.h"
#include <cassert>
#include <vector>
#include <cstdio>

using namespace coop_frontend;
int main(int argc, char** argv) {
    assert(valid_address("192.168.1.2"));
    assert(valid_address("127.0.0.1"));
    const char* bad[] = { "", "1.2.3", "1.2.3.", "256.0.0.1", "1.2.3.4.5",
        "1.2.3.4:1234", "1.2.3.-1", "0000.1.1.1", "1.2.3.4x" };
    for (const char* text : bad) assert(!valid_address(text));
    assert(valid_port("1024") && valid_port("65534"));
    assert(!valid_port("65535") && !valid_port("0") && !valid_port("1023"));
    assert(!valid_port("999999999999") && !valid_port("-1") && !valid_port("51x"));
    Menu m;
    m.key(UP); assert(m.row == 5); m.key(DOWN); assert(m.row == 0);
    m.key(ACCEPT); assert(m.page == PLAY);
    m.key(LEFT); assert(m.slot == 2);
    m.key(DOWN); assert(m.key(ACCEPT) == START_SOLO);
    m.key(BACK); m.row = 1; m.key(ACCEPT); assert(m.page == HOST);
    m.row = 2; assert(m.key(ACCEPT) == START_HOST);
    m.open(JOIN); m.row = 3; assert(m.key(ACCEPT) == START_JOIN);
    m.row = 0; m.key(ACCEPT); m.type('5'); m.key(BACK);
    assert(std::strcmp(m.address, "192.168.1.2") == 0);
    m.row = 0; m.key(ACCEPT); m.type('2'); m.type('5'); m.type('6');
    m.key(ACCEPT); m.row = 3; assert(m.key(ACCEPT) == NONE); assert(m.message[0]);
    m.row = 0; m.key(ACCEPT);
    for (char ch : "127.0.0.1") m.type(ch);
    m.type('x'); m.key(ACCEPT); m.row = 3; assert(m.key(ACCEPT) == START_JOIN);
    m.open(OPTIONS); m.row = 5;
    for (int i = 0; i < 20; ++i) m.key(LEFT);
    assert(m.volume == 0);
    for (int i = 0; i < 20; ++i) m.key(RIGHT);
    assert(m.volume == 100);
    m.row = 6; assert(m.key(ACCEPT) == SAVE_OPTIONS);
    m.open(HOME); m.key(BACK); assert(m.page == EXIT_CONFIRM);
    m.key(ACCEPT); assert(m.page == HOME);
    m.key(BACK); m.row = 1; assert(m.key(ACCEPT) == QUIT);

    TouchControls touch; touch.resize(1000, 600);
    touch.down(11, 150, 450); touch.move(11, 250, 400);
    touch.down(22, 950, 450); touch.down(33, 790, 450);
    auto state = touch.state();
    assert(state.move_x > 0 && state.move_y > 0);
    assert(state.buttons == (0x1000 | 0x2000));
    touch.up(22); assert(touch.state().buttons == 0x2000);
    touch.down(44, 790, 450); touch.up(33); assert(touch.state().buttons == 0x2000);
    touch.move(44, 0, 0); assert(touch.state().buttons == 0);
    touch.cancel(); assert(touch.state().move_x == 0 && touch.state().buttons == 0);
    touch.down(100, 600, 450); touch.move(100, 640, 440);
    assert(touch.state().look_x > 0);
    touch.resize(600, 1000); assert(touch.state().look_x == 0);
    touch.resize(0, 0); touch.down(1, 10, 10); assert(touch.state().buttons == 0);
    touch.resize(1000, 600, 20, 30, 20, 30);
    touch.down(9, 950, 450); assert(touch.state().buttons == 0x1000);

    Canvas empty = { nullptr, 0, 0, 0, OVL_FONT };
    draw(empty, m, 0); assert(hit_row(empty, m, 0, 0) == -1);
    uint32_t tiny[3] = { 0xdeadbeef, 0, 0xdeadbeef };
    Canvas small = { tiny + 1, 1, 1, 1, OVL_FONT };
    draw(small, m, 0); assert(tiny[0] == 0xdeadbeef && tiny[2] == 0xdeadbeef);
    // Guard pixels ensure drawing respects both extent and stride at all sizes.
    for (auto dims : { std::pair<int,int>{512,384}, {1024,576}, {1280,800}, {800,1280} }) {
        int w = dims.first, h = dims.second, stride = w + 16;
        std::vector<uint32_t> pixels(stride * h + 64, 0xdeadbeef);
        Canvas c = { pixels.data() + 32, w, h, stride, OVL_FONT };
        for (int page = HOME; page <= EXIT_CONFIRM; ++page) {
            m.open(static_cast<Page>(page)); draw(c, m, 100);
            assert(hit_row(c, m, w / 2, row_top(c)) == 0);
        }
        for (int i = 0; i < 32; ++i) assert(pixels[i] == 0xdeadbeef);
        for (int y = 0; y < h; ++y)
            for (int x = w; x < stride; ++x) assert(pixels[32 + y * stride + x] == 0xdeadbeef);
        for (size_t i = 32 + stride * h; i < pixels.size(); ++i) assert(pixels[i] == 0xdeadbeef);
    }
    if (argc == 2) {
        const int w = 1024, h = 576;
        std::vector<uint32_t> image(w * h);
        Canvas c = { image.data(), w, h, w, OVL_FONT }; m.open(HOME); draw(c, m, 0);
        FILE* f = std::fopen(argv[1], "wb"); assert(f);
        std::fprintf(f, "P6\n%d %d\n255\n", w, h);
        for (uint32_t pixel : image) {
            unsigned char rgb[] = { static_cast<unsigned char>(pixel >> 16),
                static_cast<unsigned char>(pixel >> 8), static_cast<unsigned char>(pixel) };
            std::fwrite(rgb, 1, 3, f);
        }
        std::fclose(f);
    }
    std::puts("PASS: menu navigation, endpoint validation, render bounds, multi-touch and cancellation");
}
