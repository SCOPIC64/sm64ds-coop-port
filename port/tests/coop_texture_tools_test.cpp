#include "hal/coop_texture_tools.h"
#include "hal/resource_pack.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
namespace fs=std::filesystem;
int main() {
    const fs::path root=fs::absolute("texture-tools-fixture");
    assert(!fs::exists(root));fs::create_directories(root/"capture");
    std::string made,error;
    assert(!sm64ds::textures::create_pack((root/"capture").u8string(),(root/"packs").u8string(),made,error));
    const fs::path fixture=fs::path(COOP_MEDIA_FIXTURES)/"menu-colors.png";
    assert(fs::exists(fixture));
    fs::copy_file(fixture,root/"capture"/"0123456789abcdef.png");
    assert(sm64ds::textures::create_pack((root/"capture").u8string(),(root/"packs").u8string(),made,error));
    assert(sm64ds::packs::load_all((root/"packs").u8string(),error));
    assert(sm64ds::packs::textures().size()==1 && sm64ds::packs::textures()[0].target_hash==0x0123456789abcdefULL);
    const std::string first=made;
    auto selection=sm64ds::packs::selections((root/"packs").u8string(),error);
    assert(error.empty()&&selection.size()==1&&selection[0].enabled);
    const std::string folder=selection[0].directory;
    assert(sm64ds::packs::set_enabled((root/"packs").u8string(),folder,false,error));
    assert(fs::exists(fs::u8path(first)/"pack.lua")); // Active asset paths remain intact.
    assert(sm64ds::packs::load_all((root/"packs").u8string(),error)&&sm64ds::packs::textures().empty());
    assert(!sm64ds::packs::selections((root/"packs").u8string(),error)[0].enabled);
    assert(!sm64ds::packs::set_enabled((root/"packs").u8string(),"../escape",true,error));
    assert(sm64ds::packs::set_enabled((root/"packs").u8string(),folder,true,error));
    assert(sm64ds::packs::load_all((root/"packs").u8string(),error)&&sm64ds::packs::textures().size()==1);
    assert(sm64ds::textures::create_pack((root/"capture").u8string(),(root/"packs").u8string(),made,error));
    assert(first!=made && fs::exists(fs::u8path(first)/"pack.lua"));
    std::ofstream(root/"capture"/"ffffffffffffffff.png")<<"broken";
    assert(!sm64ds::textures::create_pack((root/"capture").u8string(),(root/"packs").u8string(),made,error));
    fs::remove_all(root);std::cout<<"Texture tools: capture recipe, native registry and non-overwrite PASS\n";
}
