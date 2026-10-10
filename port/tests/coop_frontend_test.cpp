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
    m.key(UP); assert(m.row == 4); m.key(DOWN); assert(m.row == 0);
    m.key(ACCEPT); assert(m.page == HOST);
    m.row = 6; m.key(ACCEPT); assert(m.page == PLAY);
    m.key(LEFT); assert(m.slot == 2);
    m.key(DOWN); assert(m.key(ACCEPT) == START_SOLO);
    m.key(BACK); assert(m.page == HOST);
    m.row = 5; assert(m.key(ACCEPT) == START_HOST);
    m.open(JOIN); m.row = 3; assert(m.key(ACCEPT) == START_JOIN);
    m.row = 0; m.key(ACCEPT); m.type('5'); m.key(BACK);
    assert(std::strcmp(m.address, "192.168.1.2") == 0);
    m.row = 0; m.key(ACCEPT); m.type('2'); m.type('5'); m.type('6');
    m.key(ACCEPT); m.row = 3; assert(m.key(ACCEPT) == NONE); assert(m.message[0]);
    m.row = 0; m.key(ACCEPT);
    for (char ch : "127.0.0.1") m.type(ch);
    m.type('x'); m.key(ACCEPT); m.row = 3; assert(m.key(ACCEPT) == START_JOIN);
    m.open(OPTIONS); m.row = 3; m.key(ACCEPT); assert(m.page == DISPLAY);
    assert(m.fps == 0);
    char rate_label[80]; m.label(0, rate_label, sizeof rate_label);
    assert(std::strstr(rate_label, "NATIVE"));
    m.key(LEFT); assert(m.fps == 240);
    m.key(RIGHT); assert(m.fps == 0);
    for (int rate : { 60, 90, 120, 144, 240, 0 }) {
        m.key(RIGHT); assert(m.fps == rate);
    }
    m.fps = 165; m.key(RIGHT); assert(m.fps == 240);
    m.open(SOUND);
    for (int i = 0; i < 20; ++i) m.key(LEFT);
    assert(m.volume == 0);
    for (int i = 0; i < 20; ++i) m.key(RIGHT);
    assert(m.volume == 100);
    m.row = 1; m.key(LEFT); assert(m.music == MUSIC_OFF);
    m.key(RIGHT); assert(m.music == MUSIC_RANDOM);
    for (int i = 0; i < SONG_COUNT; ++i) { m.key(RIGHT); assert(m.music == i); }
    m.key(RIGHT); assert(m.music == MUSIC_OFF);
    m.row = 2; m.key(ACCEPT); assert(!m.menu_sounds);
    m.row = 3; assert(m.key(ACCEPT) == NEXT_SONG);
    m.row = 4; assert(m.key(ACCEPT) == IMPORT_MUSIC);
    m.row = 5; assert(m.key(ACCEPT) == RELOAD_MEDIA);
    m.row = 6; assert(m.key(ACCEPT) == SAVE_OPTIONS); assert(m.after_save == OPTIONS);
    m.custom_music_count=2; std::strcpy(m.custom_music[0],"My song");
    m.music=SONG_COUNT-1; m.row=1; m.key(RIGHT); assert(m.music==SONG_COUNT);
    m.label(1,rate_label,sizeof rate_label); assert(std::strstr(rate_label,"My song"));
    m.key(RIGHT); assert(m.music==SONG_COUNT+1);
    m.key(RIGHT); assert(m.music==MUSIC_RANDOM_ALL);
    m.key(RIGHT); assert(m.music==MUSIC_RANDOM_CUSTOM);
    m.custom_music_count=0; m.music=MUSIC_RANDOM_CUSTOM; m.key(RIGHT); assert(m.music==0);
    unsigned seed = 7; int previous = -1;
    for (int i = 0; i < 1000; ++i) {
        int next = random_song(seed, previous);
        assert(next >= 0 && next < SONG_COUNT && next != previous); previous = next;
    }
    for (const Song& song : songs) assert(song.sequence != 44 && song.sequence != 45 && song.sequence != 66 && song.sequence != 67);
    unsigned char chip[8192]; std::memset(chip,255,sizeof chip);
    assert(!preview_save(chip,sizeof chip,0).exists && !preview_save(chip,sizeof chip,0).damaged);
    unsigned char payload[68] = {}; std::memcpy(payload,"8000",4);
    payload[0x14]=11; payload[0x15]=3; payload[0x41]=1;
    // Golden cartridge record: five stars and Luigi, checksum independently
    // computed from the native format (including the "ds mario" tag).
    assert(save_checksum(payload)==0x71a1);
    for (int copy=0;copy<2;++copy) {
        unsigned char* record=chip+copy*4096+128;
        record[0]=0xa1; record[1]=0x71;
        std::memcpy(record+2,"ds mario",8); std::memcpy(record+10,payload,68);
    }
    SavePreview saved=preview_save(chip,sizeof chip,1);
    assert(saved.exists && saved.stars==5 && saved.character==1 && !saved.damaged);
    chip[128]^=1; saved=preview_save(chip,sizeof chip,1);
    assert(saved.exists && saved.stars==5); // good mirror survives bad primary
    chip[4096+128]^=1; saved=preview_save(chip,sizeof chip,1);
    assert(!saved.exists && saved.damaged);
    assert(preview_save(chip,sizeof chip-1,1).damaged);
    assert(!preview_save(chip,sizeof chip,3).exists);
    m.open(HOST); m.row = 2; m.key(ACCEPT); assert(m.slot == 2 && m.page == HOST);
    m.saves[2].damaged = true; m.row = 5; assert(m.key(ACCEPT) == NONE && m.message[0]);
    m.saves[2].damaged = false;
    m.open(DISPLAY); m.row = 2; m.key(LEFT); assert(m.background == BACKGROUND_BUILTIN_COUNT-1);
    m.key(RIGHT); assert(m.background == 0);
    m.custom_background_count=1; std::strcpy(m.custom_backgrounds[0],"My picture");
    m.key(LEFT); assert(m.background==BACKGROUND_BUILTIN_COUNT);
    m.label(2,rate_label,sizeof rate_label); assert(std::strstr(rate_label,"My picture"));
    m.row=4; assert(m.key(ACCEPT)==IMPORT_BACKGROUND);
    m.row=5; assert(m.key(ACCEPT)==RELOAD_MEDIA);
    m.custom_background_count=0; m.background=0;
    m.open(MISC); m.key(ACCEPT); assert(m.mouse_capture);
    assert(m.key(BACK) == SAVE_OPTIONS && m.after_save == OPTIONS);
    const Page categories[] = { PLAYER, CAMERA, CONTROLS, DISPLAY, SOUND, MISC };
    for (int i = 0; i < 6; ++i) {
        m.open(OPTIONS); m.row = i; m.key(ACCEPT); assert(m.page == categories[i]);
        if (m.page == CONTROLS) { m.key(BACK); assert(m.page == OPTIONS); }
        else assert(m.key(BACK) == SAVE_OPTIONS && m.after_save == OPTIONS);
    }
    m.open(OPTIONS); m.row = 6; assert(m.key(ACCEPT) == SAVE_OPTIONS && m.after_save == HOME);
    m.open(PLAYER);m.row=1;m.key(ACCEPT);assert(m.page==MODS);
    m.mod_count=6;m.open(MODS);m.row=0;assert(m.key(ACCEPT)==TOGGLE_MOD);
    m.row=4;m.key(RIGHT);assert(m.mod_page==1 && m.shown_mods()==2 && m.row==2);
    m.row=3;assert(m.key(ACCEPT)==RELOAD_MODS);
    m.row=4;m.key(ACCEPT);assert(m.page==TEXTURES);
    assert(m.key(ACCEPT)==TEXTURE_CAPTURE);m.row=1;assert(m.key(ACCEPT)==TEXTURE_PACK);
    m.in_game=true;m.open(PAUSE);assert(m.key(BACK)==RESUME);
    m.row=1;m.key(ACCEPT);assert(m.page==OPTIONS);
    assert(m.key(BACK)==SAVE_OPTIONS && m.after_save==PAUSE);
    m.open(MODS);m.key(BACK);assert(m.page==PAUSE);m.in_game=false;
    m.mod_page=0;m.mod_count=48;
    m.open(CAMERA);m.camera=2;m.key(RIGHT);assert(m.camera==3);
    m.label(0,rate_label,sizeof rate_label);assert(std::strstr(rate_label,"SM64 CAM"));
    m.key(RIGHT);assert(m.camera==0);m.key(LEFT);assert(m.camera==3);
    m.open(DISPLAY);m.row=3;
    for(int distance:{1,2,3,4,0}){m.key(RIGHT);assert(m.object_distance==distance);}
    m.key(LEFT);assert(m.object_distance==4);
    m.open(HOME); m.key(BACK); assert(m.page == EXIT_CONFIRM);
    m.key(ACCEPT); assert(m.page == HOME);
    m.key(BACK); m.row = 1; char quit_label[20]; m.label(1, quit_label, sizeof quit_label);
    assert(std::strcmp(quit_label, "QUIT") == 0); assert(m.key(ACCEPT) == QUIT);

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
        for (int page = HOME; page <= PAUSE; ++page) {
            m.open(static_cast<Page>(page)); draw(c, m, 100);
            for (int row = 0; row < m.rows(); ++row) {
                ButtonRect r = button_rect(c,m,row);
                assert(hit_row(c, m, r.x + r.w / 2, r.y + r.h / 2) == row);
            }
            assert(hit_row(c, m, -1, row_top(c, m)) == -1);
            assert(hit_row(c, m, button_left(c, m), row_top(c, m) - 1) == -1);
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
