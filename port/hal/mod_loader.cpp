#include "mod_loader.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>

#ifdef SM64DS_HAS_LUA
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#endif

namespace sm64ds::mods {

namespace {
// -1 = untouched by any Lua mod this boot.
int g_want_character = -1;
int g_want_speed = -1;
int g_want_jump = -1;
int g_want_sub = -1;
int g_want_wide = -1;
int g_want_arena = -1;
int g_want_camera = -1;
int g_want_color = -1;
}  // namespace

int requested_character(void) { return g_want_character; }
int requested_speed_pct(void) { return g_want_speed; }
int requested_jump_pct(void) { return g_want_jump; }
int requested_sub_scale(void) { return g_want_sub; }
int requested_widescreen(void) { return g_want_wide; }
int requested_arena(void) { return g_want_arena; }
int requested_camera(void) { return g_want_camera; }
int requested_color(void) { return g_want_color; }

bool installed(const std::string &mods_directory, const std::string &name)
{
    const std::filesystem::path mod =
        std::filesystem::path(mods_directory) / name;
    return std::filesystem::is_directory(mod) &&
           std::filesystem::is_regular_file(mod / "main.lua");
}

namespace {
// Toggleable native-backed mods, in MODS-menu order.
const ModInfo kKnownMods[] = {
    {"analog_controls", "Analog Controls"},
    {"character_select", "Character Select"},
    {"outfit", "Outfit"},
    {"speed_boost", "Speed Boost"},
    {"high_jump", "High Jump"},
    {"bottom_compact", "Compact Bottom"},
    {"widescreen", "Widescreen"},
    {"small_arena", "Small Arena"},
    {"sm64_movement", "SM64 Movement"},
};
std::string g_mod_dirs[9];   // actual folder name per mod ("" = missing)
bool g_mods_dir_seen;
}  // namespace

int mod_count(void) { return 9; }

ModInfo mod_info(int i)
{
    if (i < 0 || i >= mod_count()) return {"", ""};
    return kKnownMods[i];
}

static void mod_scan_dirs(const std::string &mods_directory)
{
    namespace fs = std::filesystem;
    g_mods_dir_seen = fs::exists(fs::path(mods_directory));
    for (int i = 0; i < mod_count(); ++i) {
        g_mod_dirs[i].clear();
        if (fs::is_directory(fs::path(mods_directory) / kKnownMods[i].id))
            g_mod_dirs[i] = kKnownMods[i].id;
        else if (fs::is_directory(fs::path(mods_directory) /
                                  ("off_" + std::string(kKnownMods[i].id))))
            g_mod_dirs[i] = "off_" + std::string(kKnownMods[i].id);
    }
}

/* public rescan for the MODS Refresh row: picks up folders added or
   removed by hand without a restart (enabling still needs one, since
   Lua scripts run at boot). */
void mod_rescan(const std::string &mods_directory)
{
    mod_scan_dirs(mods_directory);
    std::printf("[mods] rescanned %s\n", mods_directory.c_str());
}

bool mod_enabled(const char *id)
{
    if (!g_mods_dir_seen) return true;
    for (int i = 0; i < mod_count(); ++i) {
        if (std::string(kKnownMods[i].id) == id)
            return !g_mod_dirs[i].empty() &&
                   g_mod_dirs[i].rfind("off_", 0) != 0;
    }
    return true;
}

bool mod_set_enabled(const std::string &mods_directory, const char *id,
                     bool on)
{
    namespace fs = std::filesystem;
    mod_scan_dirs(mods_directory);
    for (int i = 0; i < mod_count(); ++i) {
        if (std::string(kKnownMods[i].id) != id) continue;
        const std::string want = on ? id : ("off_" + std::string(id));
        if (g_mod_dirs[i] == want) return true;
        if (g_mod_dirs[i].empty()) return false;   // uninstalled: nothing to rename
        std::error_code ec;
        fs::rename(fs::path(mods_directory) / g_mod_dirs[i],
                   fs::path(mods_directory) / want, ec);
        if (ec) {
            std::fprintf(stderr, "[mods] cannot %s %s\n",
                         on ? "enable" : "disable", id);
            return false;
        }
        std::printf("[mods] %s %s (restart applies its script)\n",
                    on ? "enabled" : "disabled", id);
        g_mod_dirs[i] = want;
        return true;
    }
    return false;
}

#ifdef SM64DS_HAS_LUA
static int mod_log(lua_State *state)
{
    size_t length = 0;
    const char *message = lua_tolstring(state, 1, &length);
    if (message) std::printf("[lua] %.*s\n", (int)length, message);
    return 0;
}

static int mod_get_character(lua_State *state)
{
    lua_pushinteger(state, g_want_character);
    return 1;
}

static int mod_set_character(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 0 && v <= 3) {
        g_want_character = (int)v;
        std::printf("[lua] requested character %d\n", g_want_character);
    } else {
        std::printf("[lua] set_character: want 0..3 (0 Mario, 1 Luigi, 2 Wario, 3 Yoshi)\n");
    }
    return 0;
}

static int mod_set_speed(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 25 && v <= 300) {
        g_want_speed = (int)v;
        std::printf("[lua] requested speed %d%%\n", g_want_speed);
    } else {
        std::printf("[lua] set_speed_pct: want 25..300\n");
    }
    return 0;
}

static int mod_set_jump(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 25 && v <= 300) {
        g_want_jump = (int)v;
        std::printf("[lua] requested jump %d%%\n", g_want_jump);
    } else {
        std::printf("[lua] set_jump_pct: want 25..300\n");
    }
    return 0;
}

static int mod_set_sub(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 1 && v <= 4) {
        g_want_sub = (int)v;
        std::printf("[lua] requested bottom-screen scale 1/%d\n", g_want_sub);
    } else {
        std::printf("[lua] set_sub_scale: want 1..4 (3 = small default)\n");
    }
    return 0;
}

static int mod_set_wide(lua_State *state)
{
    g_want_wide = lua_toboolean(state, 1) ? 1 : 0;
    std::printf("[lua] requested widescreen %s\n", g_want_wide ? "ON" : "off");
    return 0;
}

static int mod_set_arena(lua_State *state)
{
    g_want_arena = lua_toboolean(state, 1) ? 1 : 0;
    std::printf("[lua] requested arena %s\n", g_want_arena ? "ON" : "off");
    return 0;
}

static int mod_set_camera(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 0 && v <= 3) {
        g_want_camera = (int)v;
        std::printf("[lua] requested camera %d\n", g_want_camera);
    } else {
        std::printf("[lua] set_camera: want 0 analog, 1 sm64, 2 freecam, "
                    "3 DS-exact\n");
    }
    return 0;
}

static int mod_set_outfit(lua_State *state)
{
    int ok = 0;
    long long v = lua_tointegerx(state, 1, &ok);
    if (ok && v >= 0 && v <= 7) {
        g_want_color = (int)v;
        std::printf("[lua] requested outfit %d\n", g_want_color);
    } else {
        std::printf("[lua] set_outfit: want 0..7\n");
    }
    return 0;
}

static int mod_is_enabled(lua_State *state)
{
    size_t length = 0;
    const char *id = lua_tolstring(state, 1, &length);
    lua_pushboolean(state, id && mod_enabled(id));
    return 1;
}

static void register_api(lua_State *state)
{
    lua_createtable(state, 0, 14);
    lua_pushcclosure(state, mod_log, 0);
    lua_setfield(state, -2, "log");
    lua_pushcclosure(state, mod_get_character, 0);
    lua_setfield(state, -2, "get_character");
    lua_pushcclosure(state, mod_set_character, 0);
    lua_setfield(state, -2, "set_character");
    lua_pushcclosure(state, mod_set_speed, 0);
    lua_setfield(state, -2, "set_speed_pct");
    lua_pushcclosure(state, mod_set_jump, 0);
    lua_setfield(state, -2, "set_jump_pct");
    lua_pushcclosure(state, mod_set_sub, 0);
    lua_setfield(state, -2, "set_sub_scale");
    lua_pushcclosure(state, mod_set_wide, 0);
    lua_setfield(state, -2, "set_widescreen");
    lua_pushcclosure(state, mod_set_arena, 0);
    lua_setfield(state, -2, "set_arena");
    lua_pushcclosure(state, mod_set_camera, 0);
    lua_setfield(state, -2, "set_camera");
    lua_pushcclosure(state, mod_set_outfit, 0);
    lua_setfield(state, -2, "set_outfit");
    lua_pushcclosure(state, mod_is_enabled, 0);
    lua_setfield(state, -2, "mod_enabled");
    lua_pushstring(state, std::getenv("SM64DS_ROM"));
    lua_setfield(state, -2, "rom_path");
    lua_setglobal(state, "sm64ds");
}
#endif

bool load_all(const std::string &mods_directory, std::string &error)
{
    namespace fs = std::filesystem;
    const fs::path root(mods_directory);
    mod_scan_dirs(mods_directory);
    if (!fs::exists(root)) return true;

#ifndef SM64DS_HAS_LUA
    std::fprintf(stderr, "Lua mods found, but this build has no Lua 5.4 runtime; skipping %s\n",
                 root.string().c_str());
    return true;
#else
    lua_State *lua = luaL_newstate();
    if (!lua) {
        error = "could not create the Lua state";
        return false;
    }
    luaL_openlibs(lua);
    register_api(lua);
    for (const auto &entry : fs::directory_iterator(root)) {
        if (!entry.is_directory()) continue;
        /* opt-out convention: folders starting with "off_" (or ".") are
           skipped, so a shipped mod can be disabled by renaming it. */
        const std::string name = entry.path().filename().string();
        if (!name.empty() && (name[0] == '.' || name.rfind("off_", 0) == 0)) {
            std::printf("Skipped mod (disabled): %s\n", name.c_str());
            continue;
        }
        const fs::path script = entry.path() / "main.lua";
        if (!fs::is_regular_file(script)) continue;
        if (luaL_loadfilex(lua, script.string().c_str(), nullptr) != 0 ||
            lua_pcallk(lua, 0, 0, 0, 0, nullptr) != 0) {
            size_t length = 0;
            const char *message = lua_tolstring(lua, -1, &length);
            error = message ? std::string(message, length) : "unknown Lua error";
            lua_close(lua);
            return false;
        }
        std::printf("Loaded mod: %s\n", entry.path().filename().string().c_str());
    }
    lua_close(lua);
    return true;
#endif
}

}
