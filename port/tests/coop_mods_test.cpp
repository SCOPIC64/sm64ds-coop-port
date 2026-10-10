#include "coop_mods.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs = std::filesystem;
static void write(const fs::path& p, const char* s) {
    fs::create_directories(p.parent_path()); std::ofstream(p) << s;
}
static std::size_t index(const char* id) {
    for (std::size_t i=0;i<sm64ds::mods::count();++i)
        if (sm64ds::mods::info(i).id==id) return i;
    assert(false); return 0;
}
int main() {
    using namespace sm64ds::mods;
    const fs::path root=fs::absolute("lua-mod-fixture");
    assert(!fs::exists(root)); // Tests never delete a pre-existing user folder.
    fs::create_directories(root/"resource-packs");
    write(root/"bounded/main.lua", "-- name: Bounded\nsm64ds.hook('walk',function(p) p.horizontal_speed=9999; p.gravity=0/0; p.action=0 end)");
    write(root/"broken/main.lua", "sm64ds.hook('walk',function(p) p.horizontal_speed=8; error('intentional') end)");
    write(root/"loop/main.lua", "sm64ds.hook('walk',function(p) while true do end end)");
    write(root/"memory/main.lua", "local x=string.rep('x',8*1024*1024)");
    write(root/"meta/main.lua", "sm64ds.hook('walk',function(p) setmetatable(p,{__index=function() while true do end end}); p.horizontal_speed=nil end)");
    write(root/"sandbox/main.lua", "assert(os==nil and io==nil and package==nil and debug==nil and load==nil and math.random==nil)");
    write(root/"multi/main.lua", "order='main'; function adjust(p) p.horizontal_speed=7 end");
    write(root/"multi/10-helper.lua", "assert(order=='main'); order=order..',helper'");
    write(root/"multi/20-hook.lua", "assert(order=='main,helper'); sm64ds.hook('walk',adjust)");
    write(root/"multi-broken/main.lua", "sm64ds.hook('walk',function(p) p.horizontal_speed=99 end)");
    write(root/"multi-broken/extra.lua", "error('second script failed')");
    const std::string large = "--" + std::string(600000, 'x');
    write(root/"multi-limit/main.lua", large.c_str());
    write(root/"multi-limit/extra.lua", large.c_str());
    write(root/"many-files/main.lua", "-- file count check");
    for (int i=0;i<32;++i) write(root/"many-files"/("part-"+std::to_string(i)+".lua"), "-- helper");
    const fs::path unicode = fs::u8path(u8"caf\u00e9");
    write(root/unicode/"main.lua", "sm64ds.hook('player_update',function(p) p.vertical_speed=9 end)");
    std::string error; assert(load(root.u8string(),true,error));
    assert(count()==12 && enabled("sm64-movement"));
    PlayerState p; p.stick=1; p.launch_speed=32; p.gravity=-4;
    assert(dispatch(Event::walk,p) && p.horizontal_speed>1 && p.horizontal_speed<2);
    p.action=0x020e127c; p.airborne=true;
    assert(dispatch(Event::state,p) && p.vertical_speed==30 && p.horizontal_speed==48 && p.gravity==-2);
    assert(set_enabled(index("sm64-movement"),false,error));
    assert(set_enabled(index("multi"),true,error));
    p.horizontal_speed=0; assert(dispatch(Event::walk,p) && p.horizontal_speed==7);
    assert(set_enabled(index("multi"),false,error));
    assert(!set_enabled(index("multi-broken"),true,error));
    p.horizontal_speed=3; assert(!dispatch(Event::walk,p) && p.horizontal_speed==3);
    assert(!set_enabled(index("multi-limit"),true,error) && error.find("1 MiB")!=std::string::npos);
    assert(!set_enabled(index("many-files"),true,error) && error.find("32 Lua files")!=std::string::npos);
    assert(set_enabled(index(u8"caf\u00e9"),true,error));
    p.vertical_speed=0; assert(dispatch(Event::after_update,p) && p.vertical_speed==9);
    assert(set_enabled(index(u8"caf\u00e9"),false,error));
    assert(set_enabled(index("bounded"),true,error));
    p.action=123; p.horizontal_speed=0; p.gravity=-4;
    assert(dispatch(Event::walk,p) && p.horizontal_speed==512 && p.gravity==-4 && p.action==123);
    assert(set_enabled(index("bounded"),false,error));
    assert(set_enabled(index("broken"),true,error));
    p.horizontal_speed=3; assert(!dispatch(Event::walk,p) && p.horizontal_speed==3);
    assert(info(index("broken")).failed);
    assert(set_enabled(index("loop"),true,error));
    assert(!dispatch(Event::walk,p) && info(index("loop")).failed);
    assert(!set_enabled(index("memory"),true,error));
    assert(set_enabled(index("sandbox"),true,error));
    assert(set_enabled(index("meta"),true,error));
    assert(!dispatch(Event::walk,p) && !info(index("meta")).failed);
    assert(set_enabled(index("broken"),false,error));
    assert(set_enabled(index("loop"),false,error));
    assert(load(root.u8string(),true,error) && !enabled("sm64-movement") && enabled("meta"));
    clear(); fs::remove_all(root);
    std::cout << "Lua mods: movement, limits, error recovery and saved selection PASS\n";
}
