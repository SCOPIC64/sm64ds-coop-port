#include "rom_locator.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#define RCK(msg) do { fprintf(stderr, "[rck] %s\n", msg); fflush(stderr); } while (0)

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
    RCK("pre-gmfn");
    DWORD length = GetModuleFileNameA(nullptr, buffer, MAX_PATH);
    RCK("post-gmfn");
    if (length == 0 || length >= MAX_PATH) return {};
    std::filesystem::path p(buffer);
    RCK("path-made");
    std::filesystem::path pp = p.parent_path();
    RCK("parent-made");
    return pp;
}

}

bool locate_rom_next_to_exe(RomInfo &out, std::string &error)
{
    RCK("enter");
    const std::filesystem::path directory = executable_directory();
    RCK("have-dir");
    if (directory.empty()) {
        error = "cannot determine the executable directory";
        return false;
    }

    std::filesystem::path found;
    RCK("pre-iterate");
    for (const auto &entry : std::filesystem::directory_iterator(directory)) {
        RCK("entry");
        if (!entry.is_regular_file() || !has_nds_extension(entry.path())) continue;
        if (!found.empty()) {
            error = "more than one .nds file is next to the executable; leave only your SM64DS dump there";
            return false;
        }
        found = entry.path();
    }
    if (found.empty()) {
        RCK("no-rom");
        error = "no .nds file is next to the executable; copy your own SM64DS dump beside walk_window.exe";
        return false;
    }

    std::ifstream rom(found, std::ios::binary);
    RCK("opened");
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