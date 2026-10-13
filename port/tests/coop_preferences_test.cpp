#define WIN32_LEAN_AND_MEAN
#include "hal/coop_preferences.h"
#include "hal/coop_color_transform.h"
#include <cassert>
#include <cstdio>

int main(int argc,char** argv) {
    assert(argc==2);_putenv_s("APPDATA",argv[1]);CreateDirectoryA(argv[1],nullptr);
    coop_prefs::Preferences p;p.load();p.name="Player_64";
    p.custom_controls=true;p.bindings[coop_prefs::ATTACK]={'Y','U',-0x1000};
    p.colors[0][0]="00ffaa";p.colors[1][1]="ff6600";
    const std::string id=p.id;assert(id.size()==32&&p.save());
    coop_prefs::Preferences round;round.load();assert(round.name==p.name&&round.id==id);
    assert(round.bindings[coop_prefs::ATTACK]==p.bindings[coop_prefs::ATTACK]);
    assert(round.colors[0][0]=="00ffaa"&&round.colors[1][1]=="ff6600"&&round.colors[2][1].empty());
    assert(!coop_prefs::Preferences::valid_name(" trailing "));
    assert(!coop_prefs::Preferences::valid_name("a,b"));
    assert(!coop_prefs::Preferences::valid_name(std::string(17,'x')));
    std::ofstream(round.path)<<"{\"bindings\":[[999]],\"palettes\":[1],\"player_id\":\"invalid\"}";
    coop_prefs::Preferences damaged;damaged.load();assert(!damaged.custom_controls&&damaged.id.size()==32);
    assert(damaged.bindings[coop_prefs::JUMP][0]==VK_SPACE&&damaged.colors[0][0].empty());
    coop_colors::Palette palette;assert(coop_colors::parse("00ff00:::::::ffffff",palette));
    assert(!coop_colors::parse("00ff00::::::",palette));assert(!coop_colors::parse("zz0000:::::::",palette));
    assert(coop_colors::part(0,"mario_eye_1",0,0,16,32,0xff000000)==-1);
    assert(coop_colors::part(0,"mario_eye_1",0,0,16,32,0xffffffff)==-1);
    assert(coop_colors::part(0,"mario_head",4,4,32,64,0xffff0000)==0);
    assert(coop_colors::part(0,"mario_body",18,56,64,64,0xffbd8c63)==4);
    assert(coop_colors::part(0,"mario_body",4,56,64,64,0xffbd8c63)==5);
    assert(coop_colors::part(1,"luigi_body_tr_1",48,28,64,64,0xffbd8c63)==4);
    assert(coop_colors::part(0,"mario_body",4,4,64,64,0xff0020ff)==2);
    std::vector<uint32_t> pixels={0x00ff0000,0xffff0000,0xff880000,0xff000000};
    auto original=pixels;coop_colors::recolor(pixels,2,2,0,"mario_head",palette);
    assert(pixels[0]==original[0]);assert(pixels[1]!=original[1]&&pixels[2]!=original[2]);
    assert((pixels[1]>>24)==255&&pixels[3]==original[3]);
    auto green=pixels;coop_colors::Palette other;assert(coop_colors::parse("0000ff:::::::",other));
    pixels=original;coop_colors::recolor(pixels,2,2,0,"mario_head",other);assert(pixels[1]!=green[1]);
    puts("PASS: three bindings, AppData profile round-trip, damaged-profile recovery and independent render colors");
}
