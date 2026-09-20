#include "rom_locator.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace sm64ds::port {
namespace {

std::string trim_nul_field(const char *bytes, size_t length)
{
    size_t end = 0;
    while (end < length && bytes[end] != '\0') ++end;
    return std::string(bytes, end);
}

bool has_nds_extension(const std::filesystem::path &path)
{
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return (char)std::tolower(c); });
    return extension == ".nds";
}

std::filesystem::path executable_directory()
{
    char buffer[MAX_PATH];
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) return {};
    return std::filesystem::path(buffer).parent_path();
}

}

bool locate_rom_next_to_exe(RomInfo &out, std::string &error)
{
    const std::filesystem::path directory = executable_directory();
    if (directory.empty()) {
        error = "cannot determine the executable directory";
        return false;
    }

    std::filesystem::path found;
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        if (!entry.is_regular_file() || !has_nds_extension(entry.path())) continue;
        if (!found.empty()) {
            error = "more than one .nds file is next to the executable; leave only your SM64DS dump there";
            return false;
        }
        found = entry.path();
    }
    if (found.empty()) {
        error = "no .nds file is next to the executable; copy your own SM64DS dump beside walk_window.exe";
        return false;
    }

    std::ifstream rom(found, std::ios::binary);
    char header[0x80] = {};
    if (!rom.read(header, sizeof header)) {
        error = "the .nds file is too small to be a Nintendo DS cartridge dump";
        return false;
    }
    const std::string title = trim_nul_field(header, 12);
    const std::string code = trim_nul_field(header + 12, 4);
    if (title.empty() || code.size() != 4) {
        error = "the .nds file does not have a valid Nintendo DS header";
        return false;
    }

    out.path = std::filesystem::absolute(found).string();
    out.title = title;
    out.game_code = code;
    return true;
}

}