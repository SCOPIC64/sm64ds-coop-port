#include "coop_mods.h"
#include "coop_mod_builtin.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

namespace sm64ds::mods {
namespace fs = std::filesystem;
namespace {
constexpr std::size_t max_mods = 48, max_script = 1024 * 1024;
constexpr std::size_t per_mod_memory = 4 * 1024 * 1024;
constexpr std::size_t total_memory = 32 * 1024 * 1024;
std::size_t used_memory = 0;
struct Mod {
    Info info;
    fs::path script;
    lua_State* lua = nullptr;
    std::size_t memory = 0;
    bool loading = false;
    std::vector<int> hooks[4];
    ~Mod() { if (lua) lua_close(lua); }
};
std::vector<std::unique_ptr<Mod>> registry;
fs::path mod_root;

void* allocate(void* user, void* ptr, size_t old_size, size_t new_size) {
    Mod* mod = static_cast<Mod*>(user);
    if (!ptr) old_size = 0; // Lua supplies a type tag for new allocations.
    if (!new_size) {
        std::free(ptr);
        mod->memory -= (std::min)(mod->memory, old_size);
        used_memory -= (std::min)(used_memory, old_size);
        return nullptr;
    }
    if (new_size > old_size &&
        (new_size - old_size > per_mod_memory - mod->memory ||
         new_size - old_size > total_memory - used_memory)) return nullptr;
    void* next = std::realloc(ptr, new_size);
    if (!next) return nullptr;
    mod->memory = mod->memory - old_size + new_size;
    used_memory = used_memory - old_size + new_size;
    return next;
}
void instruction_limit(lua_State* L, lua_Debug*) {
    luaL_error(L, "mod exceeded its instruction budget");
}
Mod* owner(lua_State* L) {
    return static_cast<Mod*>(lua_touserdata(L, lua_upvalueindex(1)));
}
int api_hook(lua_State* L) {
    const char* name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);
    int event = -1;
    for (int i = 0; i < 4; ++i)
        if (!std::strcmp(name, event_name(static_cast<Event>(i)))) event = i;
    if (event < 0) return luaL_error(L, "unknown player event: %s", name);
    Mod* mod = owner(L);
    if (!mod->loading) return luaL_error(L, "register hooks while loading main.lua");
    if (mod->hooks[event].size() >= 16) return luaL_error(L, "too many hooks");
    lua_pushvalue(L, 2);
    mod->hooks[event].push_back(luaL_ref(L, LUA_REGISTRYINDEX));
    return 0;
}
int api_log(lua_State* L) {
    const char* message = luaL_checkstring(L, 1);
    std::fprintf(stderr, "[mod:%s] %.512s\n", owner(L)->info.id.c_str(), message);
    return 0;
}
void sandbox(lua_State* L, Mod* mod) {
    luaL_requiref(L, "_G", luaopen_base, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);
    const char* removed[] = { "dofile", "loadfile", "load", "collectgarbage", "print" };
    for (const char* name : removed) { lua_pushnil(L); lua_setglobal(L, name); }
    lua_getglobal(L, "math");
    lua_pushnil(L); lua_setfield(L, -2, "random");
    lua_pushnil(L); lua_setfield(L, -2, "randomseed"); lua_pop(L, 1);
    lua_newtable(L);
    lua_pushinteger(L, 1); lua_setfield(L, -2, "api_version");
    lua_pushlightuserdata(L, mod); lua_pushcclosure(L, api_hook, 1);
    lua_setfield(L, -2, "hook");
    lua_pushlightuserdata(L, mod); lua_pushcclosure(L, api_log, 1);
    lua_setfield(L, -2, "log");
    lua_newtable(L);
    lua_pushinteger(L, 0x020e22c0); lua_setfield(L, -2, "JUMP_INIT");
    lua_pushinteger(L, 0x020e127c); lua_setfield(L, -2, "LONG_JUMP_INIT");
    lua_pushinteger(L, 0x020e200c); lua_setfield(L, -2, "FALL_INIT");
    lua_pushinteger(L, 0x020e2118); lua_setfield(L, -2, "FALL_MAIN");
    lua_setfield(L, -2, "actions");
    lua_setglobal(L, "sm64ds");
}
int prepare(lua_State* L) {
    sandbox(L, static_cast<Mod*>(lua_touserdata(L, 1)));
    return 0;
}
bool start(Mod& mod) {
    if (mod.lua) { lua_close(mod.lua); mod.lua = nullptr; }
    for (auto& hooks : mod.hooks) hooks.clear();
    mod.info.failed = false; mod.info.error.clear();
    mod.lua = lua_newstate(allocate, &mod);
    if (!mod.lua) { mod.info.error = "Cannot allocate Lua state"; mod.info.failed = true; return false; }
    lua_sethook(mod.lua, instruction_limit, LUA_MASKCOUNT, 100000);
    lua_pushcfunction(mod.lua, prepare); lua_pushlightuserdata(mod.lua, &mod);
    if (lua_pcall(mod.lua, 1, 0, 0) != LUA_OK) {
        mod.info.error = "Cannot initialize mod API"; mod.info.failed = true;
        lua_pop(mod.lua, 1); return false;
    }
    mod.loading = true;
    const int loaded = luaL_loadfilex(mod.lua, mod.script.string().c_str(), "t");
    const int result = loaded == LUA_OK ? lua_pcall(mod.lua, 0, 0, 0) : loaded;
    mod.loading = false;
    if (result != LUA_OK) {
        const char* message = lua_tostring(mod.lua, -1);
        mod.info.error = message ? message : "Lua initialization failed";
        mod.info.failed = true; lua_pop(mod.lua, 1);
        std::fprintf(stderr, "[mod:%s] disabled: %s\n", mod.info.id.c_str(), mod.info.error.c_str());
        return false;
    }
    std::fprintf(stderr, "[mod:%s] loaded\n", mod.info.id.c_str());
    return true;
}
bool save_selection(std::string& error) {
    const fs::path dest = mod_root / "enabled.txt", temporary = mod_root / "enabled.txt.tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        if (!file) { error = "Cannot save mod selection"; return false; }
        for (const auto& mod : registry) if (mod->info.enabled) file << mod->info.id << '\n';
        file.close();
        if (!file) { error = "Cannot write mod selection"; return false; }
    }
#ifdef _WIN32
    if (!MoveFileExW(temporary.c_str(), dest.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "Cannot replace mod selection"; return false;
    }
#else
    std::error_code ec; fs::rename(temporary, dest, ec);
    if (ec) { error = "Cannot replace mod selection: " + ec.message(); return false; }
#endif
    return true;
}
void number(lua_State* L, const char* key, double value) {
    lua_pushnumber(L, value); lua_setfield(L, -2, key);
}
void boolean(lua_State* L, const char* key, bool value) {
    lua_pushboolean(L, value); lua_setfield(L, -2, key);
}
void push_player(lua_State* L, const PlayerState& p) {
    lua_createtable(L, 0, 24);
#define NUMBER(field) number(L, #field, p.field)
    NUMBER(character); NUMBER(action); NUMBER(jump_stage); NUMBER(facing_yaw);
    NUMBER(desired_yaw); NUMBER(previous_yaw); NUMBER(horizontal_speed);
    NUMBER(vertical_speed); NUMBER(gravity); NUMBER(terminal_velocity);
    NUMBER(stick); NUMBER(floor_normal); NUMBER(sink_depth); NUMBER(launch_speed);
#undef NUMBER
#define BOOLEAN(field) boolean(L, #field, p.field)
    BOOLEAN(airborne); BOOLEAN(mega); BOOLEAN(wings); BOOLEAN(no_control);
    BOOLEAN(multiplayer); BOOLEAN(cutscene); BOOLEAN(jump_flag);
#undef BOOLEAN
}
float read_number(lua_State* L, int table, const char* key, float fallback, float low, float high) {
    lua_pushstring(L, key); lua_rawget(L, table);
    int valid = 0; const double value = lua_tonumberx(L, -1, &valid); lua_pop(L, 1);
    return valid && std::isfinite(value) ? static_cast<float>(std::clamp(value, double(low), double(high))) : fallback;
}
void read_player(lua_State* L, int table, PlayerState& p) {
    p.horizontal_speed = read_number(L, table, "horizontal_speed", p.horizontal_speed, -512, 512);
    p.vertical_speed = read_number(L, table, "vertical_speed", p.vertical_speed, -512, 512);
    p.gravity = read_number(L, table, "gravity", p.gravity, -64, 64);
    p.terminal_velocity = read_number(L, table, "terminal_velocity", p.terminal_velocity, -512, 512);
    p.previous_yaw = static_cast<int>(read_number(L, table, "previous_yaw", float(p.previous_yaw), -32768, 32767));
}
bool changed(const PlayerState& a, const PlayerState& b) {
    return a.horizontal_speed != b.horizontal_speed || a.vertical_speed != b.vertical_speed ||
        a.gravity != b.gravity || a.terminal_velocity != b.terminal_velocity || a.previous_yaw != b.previous_yaw;
}
struct Call { PlayerState* player; int ref; bool handled=false; };
int invoke(lua_State* L) {
    Call* call = static_cast<Call*>(lua_touserdata(L, 1));
    push_player(L, *call->player); const int table = lua_gettop(L);
    lua_rawgeti(L, LUA_REGISTRYINDEX, call->ref); lua_pushvalue(L, table);
    lua_call(L, 1, 1);
    call->handled=lua_isboolean(L,-1) && lua_toboolean(L,-1);
    lua_pop(L,1);
    read_player(L, table, *call->player);
    return 0;
}
}
const char* event_name(Event e) {
    static const char* names[] = { "before_player_update", "player_update", "walk", "state" };
    return names[static_cast<int>(e)];
}
void clear() { registry.clear(); mod_root.clear(); }
std::size_t count() { return registry.size(); }
Info info(std::size_t i) { return i < registry.size() ? registry[i]->info : Info{}; }
bool enabled(const char* id) {
    for (const auto& mod : registry)
        if (mod->info.id == id) return mod->info.enabled && !mod->info.failed;
    return false;
}
bool load(const std::string& root, bool migrate, std::string& error) try {
    clear(); error.clear(); mod_root = fs::u8path(root);
    std::error_code ec; fs::create_directories(mod_root, ec);
    if (ec) { error = ec.message(); return false; }
    // The executable supplies the editable example only when it is absent.
    // Never replace a player's changed script.
    const fs::path builtin = mod_root / "sm64-movement" / "main.lua";
    if (!fs::exists(builtin, ec)) {
        fs::create_directories(builtin.parent_path(), ec);
        if (ec) { error = ec.message(); return false; }
        std::ofstream output(builtin, std::ios::binary);
        output << builtin_movement;
        output.close();
        if (!output) { error = "Cannot create SM64 movement mod"; return false; }
    }
    std::vector<std::string> selected;
    const bool selection_exists = fs::exists(mod_root / "enabled.txt", ec);
    std::ifstream selection(mod_root / "enabled.txt");
    std::string line; while (std::getline(selection, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        selected.push_back(line);
    }
    std::vector<fs::path> scripts;
    for (fs::directory_iterator it(mod_root, ec), end; !ec && it != end; it.increment(ec)) {
        const std::string id = it->path().filename().u8string();
        if (id.empty() || id.size() > 80 || id.rfind("off_", 0) == 0 || id.find_first_of("\r\n") != std::string::npos) continue;
        std::error_code probe;
        if (it->is_directory(probe)) {
            if (fs::is_regular_file(it->path() / "main.lua", probe)) scripts.push_back(it->path() / "main.lua");
        } else if (it->is_regular_file(probe) && it->path().extension() == ".lua") scripts.push_back(it->path());
    }
    if (ec) { error = "Cannot scan mods: " + ec.message(); return false; }
    std::sort(scripts.begin(), scripts.end());
    for (const fs::path& script : scripts) {
        if (registry.size() == max_mods) { error += "Too many mods (maximum 48)\n"; break; }
        if (fs::file_size(script, ec) > max_script || ec) { error += script.string() + ": script exceeds 1 MiB\n"; ec.clear(); continue; }
        auto mod = std::make_unique<Mod>(); mod->script = script;
        mod->info.id = script.filename() == "main.lua" ? script.parent_path().filename().u8string() : script.filename().u8string();
        mod->info.name = mod->info.id;
        std::ifstream text(script); int lines = 0;
        while (lines++ < 32 && std::getline(text, line)) {
            for (const char* field : { "name", "description" }) {
                const std::string prefix = std::string("-- ") + field + ":";
                if (line.rfind(prefix, 0) != 0) continue;
                const std::size_t first = line.find_first_not_of(" \t", prefix.size());
                const std::string value = first == std::string::npos ? "" : line.substr(first, 240);
                if (std::string(field) == "name") mod->info.name = value;
                else mod->info.description = value;
            }
        }
        mod->info.enabled = std::find(selected.begin(), selected.end(), mod->info.id) != selected.end() ||
            (!selection_exists && migrate && mod->info.id == "sm64-movement");
        if (mod->info.enabled && !start(*mod)) error += mod->info.id + ": " + mod->info.error + '\n';
        registry.push_back(std::move(mod));
    }
    if (!selection_exists && !save_selection(error)) return false;
    return error.empty();
} catch (const std::exception& e) { error = e.what(); return false; }
bool set_enabled(std::size_t i, bool value, std::string& error) {
    error.clear(); if (i >= registry.size()) { error = "Unknown mod"; return false; }
    Mod& mod = *registry[i]; const bool previous = mod.info.enabled;
    if (value && !start(mod)) { error = mod.info.error; return false; }
    mod.info.enabled = value;
    if (!save_selection(error)) { mod.info.enabled = previous; return false; }
    if (!value && mod.lua) { lua_close(mod.lua); mod.lua = nullptr; }
    return true;
}
bool dispatch(Event e, PlayerState& player) {
    const PlayerState before = player;
    bool handled=false;
    for (auto& item : registry) {
        Mod& mod = *item;
        if (!mod.info.enabled || mod.info.failed || !mod.lua) continue;
        lua_State* L = mod.lua;
        for (int ref : mod.hooks[static_cast<int>(e)]) {
            const int top = lua_gettop(L);
            const PlayerState callback_before = player;
            Call call = { &player, ref };
            lua_pushcfunction(L, invoke); lua_pushlightuserdata(L, &call);
            lua_sethook(L, instruction_limit, LUA_MASKCOUNT, 50000);
            if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
                const char* message = lua_tostring(L, -1);
                mod.info.error = message ? message : "Lua callback failed";
                mod.info.failed = true;
                player = callback_before;
                std::fprintf(stderr, "[mod:%s] disabled: %s\n", mod.info.id.c_str(), mod.info.error.c_str());
                lua_settop(L, top); break;
            }
            handled=handled || call.handled;
            lua_settop(L, top);
        }
    }
    return handled || changed(before, player);
}
}
