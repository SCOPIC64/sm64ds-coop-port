#pragma once

#include <string>

namespace sm64ds::port {

struct RomInfo {
    std::string path;
    std::string title;
    std::string game_code;
};

bool locate_rom_next_to_exe(RomInfo &out, std::string &error);

}