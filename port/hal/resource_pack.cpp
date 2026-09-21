#include "resource_pack.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <cstdlib>

extern "C" {
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
}

namespace sm64ds::packs {
namespace fs = std::filesystem;

namespace {
std::vector<Character> g_characters;
std::vector<TextureReplacement> g_textures;

struct LoadContext {
    fs::path directory;
    std::string id;
    std::vector<Character> characters;
    std::vector<TextureReplacement> textures;
};

LoadContext *context(lua_State *L)
{
    return static_cast<LoadContext *>(lua_touserdata(L, lua_upvalueindex(1)));
}

int fail(lua_State *L, const char *message) { return luaL_error(L, "%s", message); }

std::string required_string(lua_State *L, int table, const char *field)
{
    lua_getfield(L, table, field);
    size_t length = 0;
    const char *value = lua_tolstring(L, -1, &length);
    if (!value || !length) luaL_error(L, "%s must be a non-empty string", field);
    std::string result(value, length);
    lua_pop(L, 1);
    return result;
}

int optional_int(lua_State *L, int table, const char *field, int fallback)
{
    lua_getfield(L, table, field);
    int result = fallback;
    if (!lua_isnil(L, -1)) {
        int exact = 0;
        const lua_Integer value = lua_tointegerx(L, -1, &exact);
        if (!exact) luaL_error(L, "%s must be an integer", field);
        result = static_cast<int>(value);
    }
    lua_pop(L, 1);
    return result;
}

float optional_number(lua_State *L, int table, const char *field, float fallback)
{
    lua_getfield(L, table, field);
    float result = fallback;
    if (!lua_isnil(L, -1)) {
        int numeric = 0;
        result = static_cast<float>(lua_tonumberx(L, -1, &numeric));
        if (!numeric || result <= 0.0f || result > 10000.0f)
            luaL_error(L, "%s must be a number in (0, 10000]", field);
    }
    lua_pop(L, 1);
    return result;
}

std::string asset_path(lua_State *L, LoadContext *ctx, const std::string &relative,
                       const char *field, const char *extension)
{
    fs::path rel(relative);
    if (rel.empty() || rel.is_absolute() || relative.find(':') != std::string::npos)
        luaL_error(L, "%s must be a relative path", field);
    std::error_code ec;
    const fs::path base = fs::weakly_canonical(ctx->directory, ec);
    if (ec) luaL_error(L, "cannot resolve pack directory: %s", ec.message().c_str());
    const fs::path normalized = fs::weakly_canonical(ctx->directory / rel, ec);
    if (ec) luaL_error(L, "%s cannot be resolved: %s", field, ec.message().c_str());
    auto mismatch = std::mismatch(base.begin(), base.end(), normalized.begin(),
                                  normalized.end());
    if (mismatch.first != base.end()) luaL_error(L, "%s escapes its pack", field);
    if (extension && normalized.extension().string() != extension)
        luaL_error(L, "%s must reference a %s file", field, extension);
    if (!fs::is_regular_file(normalized, ec) || ec)
        luaL_error(L, "%s does not exist: %s", field, relative.c_str());
    return normalized.string();
}

int api_character(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    LoadContext *ctx = context(L);
    Character item;
    item.pack_id = ctx->id;
    item.id = optional_int(L, 1, "id", -1);
    item.base_character = optional_int(L, 1, "base", 0);
    item.name = required_string(L, 1, "name");
    if (item.id < 4 || item.id > 255) return fail(L, "character id must be 4..255");
    if (item.base_character < 0 || item.base_character > 3)
        return fail(L, "base must be 0..3 (Mario, Luigi, Wario, Yoshi)");
    const auto duplicate = std::find_if(ctx->characters.begin(), ctx->characters.end(),
        [&](const Character &c) { return c.id == item.id; });
    if (duplicate != ctx->characters.end()) return fail(L, "duplicate character id in pack");

    item.body_model = asset_path(L, ctx, required_string(L, 1, "body"), "body", ".bmd");
    item.head_cap_model = asset_path(L, ctx, required_string(L, 1, "head_cap"),
                                     "head_cap", ".bmd");
    item.head_no_cap_model = asset_path(L, ctx, required_string(L, 1, "head_no_cap"),
                                        "head_no_cap", ".bmd");

    lua_getfield(L, 1, "hitbox");
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        item.hitbox.radius = optional_number(L, -1, "radius", item.hitbox.radius);
        item.hitbox.height = optional_number(L, -1, "height", item.hitbox.height);
        item.hitbox.hurt_radius = optional_number(L, -1, "hurt_radius", item.hitbox.radius);
        item.hitbox.hurt_height = optional_number(L, -1, "hurt_height", item.hitbox.height);
    }
    lua_pop(L, 1);

    lua_getfield(L, 1, "animations");
    if (!lua_isnil(L, -1)) {
        luaL_checktype(L, -1, LUA_TTABLE);
        lua_pushnil(L);
        while (lua_next(L, -2)) {
            if (lua_type(L, -2) != LUA_TSTRING || lua_type(L, -1) != LUA_TSTRING)
                return fail(L, "animations must map names to .bca paths");
            const std::string key = lua_tostring(L, -2);
            const std::string value = lua_tostring(L, -1);
            item.animations[key] = asset_path(L, ctx, value, "animation", ".bca");
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    ctx->characters.push_back(std::move(item));
    return 0;
}

int api_texture(lua_State *L)
{
    luaL_checktype(L, 1, LUA_TTABLE);
    LoadContext *ctx = context(L);
    TextureReplacement item;
    item.pack_id = ctx->id;
    item.target = required_string(L, 1, "target");
    if (item.target.size() != 16 ||
        item.target.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
        return fail(L, "texture target must be a 16-digit content hash");
    item.target_hash = std::strtoull(item.target.c_str(), nullptr, 16);
    item.source = asset_path(L, ctx, required_string(L, 1, "source"), "source", ".png");
    ctx->textures.push_back(std::move(item));
    return 0;
}

int api_log(lua_State *L)
{
    size_t length = 0;
    const char *message = luaL_checklstring(L, 1, &length);
    std::fprintf(stderr, "[resource-pack:%s] %.*s\n", context(L)->id.c_str(),
                 static_cast<int>(length), message);
    return 0;
}

void instruction_limit(lua_State *L, lua_Debug *)
{
    luaL_error(L, "pack.lua exceeded its instruction budget");
}

void open_sandbox(lua_State *L)
{
    luaL_requiref(L, "_G", luaopen_base, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1); lua_pop(L, 1);
    luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1); lua_pop(L, 1);
    lua_pushnil(L); lua_setglobal(L, "dofile");
    lua_pushnil(L); lua_setglobal(L, "loadfile");
    lua_pushnil(L); lua_setglobal(L, "load");
}

void register_api(lua_State *L, LoadContext *ctx)
{
    lua_createtable(L, 0, 4);
    lua_pushlightuserdata(L, ctx); lua_pushcclosure(L, api_character, 1);
    lua_setfield(L, -2, "character");
    lua_pushlightuserdata(L, ctx); lua_pushcclosure(L, api_texture, 1);
    lua_setfield(L, -2, "texture");
    lua_pushlightuserdata(L, ctx); lua_pushcclosure(L, api_log, 1);
    lua_setfield(L, -2, "log");
    lua_pushinteger(L, 1); lua_setfield(L, -2, "api_version");
    lua_setglobal(L, "sm64ds");
}

bool load_pack(const fs::path &directory, std::string &error)
{
    const fs::path script = directory / "pack.lua";
    if (!fs::is_regular_file(script)) return true;
    if (fs::file_size(script) > 1024 * 1024) {
        error = script.string() + ": pack.lua exceeds 1 MiB";
        return false;
    }
    LoadContext ctx{directory, directory.filename().string()};
    lua_State *L = luaL_newstate();
    if (!L) { error = ctx.id + ": cannot create Lua state"; return false; }
    open_sandbox(L);
    register_api(L, &ctx);
    lua_sethook(L, instruction_limit, LUA_MASKCOUNT, 1000000);
    const int loaded = luaL_loadfilex(L, script.string().c_str(), "t");
    const int called = loaded == LUA_OK ? lua_pcall(L, 0, 0, 0) : loaded;
    if (called != LUA_OK) {
        const char *message = lua_tostring(L, -1);
        error = ctx.id + ": " + (message ? message : "unknown Lua error");
        lua_close(L);
        return false;
    }
    lua_close(L);
    for (const Character &candidate : ctx.characters) {
        if (character(candidate.id)) {
            error = ctx.id + ": character id " + std::to_string(candidate.id) +
                    " is already owned by another pack";
            return false;
        }
    }
    for (const TextureReplacement &candidate : ctx.textures) {
        const auto duplicate = std::find_if(g_textures.begin(), g_textures.end(),
            [&](const TextureReplacement &item) {
                return item.target_hash == candidate.target_hash;
            });
        if (duplicate != g_textures.end()) {
            error = ctx.id + ": texture hash " + candidate.target +
                    " is already owned by another pack";
            return false;
        }
    }
    g_characters.insert(g_characters.end(), ctx.characters.begin(), ctx.characters.end());
    g_textures.insert(g_textures.end(), ctx.textures.begin(), ctx.textures.end());
    std::fprintf(stderr, "[resource-pack] loaded %s (%zu characters, %zu textures)\n",
                 ctx.id.c_str(), ctx.characters.size(), ctx.textures.size());
    return true;
}
}  // namespace

void clear() { g_characters.clear(); g_textures.clear(); }

bool load_all(const std::string &root, std::string &error)
{
    clear();
    error.clear();
    const fs::path base(root);
    if (!fs::exists(base)) return true;
    if (!fs::is_directory(base)) { error = root + " is not a directory"; return false; }
    bool ok = true;
    std::ostringstream errors;
    std::error_code ec;
    std::vector<fs::path> directories;
    for (const auto &entry : fs::directory_iterator(base, ec))
        if (entry.is_directory()) directories.push_back(entry.path());
    std::sort(directories.begin(), directories.end());
    for (const fs::path &directory : directories) {
        const std::string name = directory.filename().string();
        if (name.empty() || name[0] == '.' || name.rfind("off_", 0) == 0) continue;
        std::string one;
        if (!load_pack(directory, one)) { ok = false; errors << one << '\n'; }
    }
    if (ec) { ok = false; errors << root << ": " << ec.message() << '\n'; }
    error = errors.str();
    return ok;
}

const Character *character(int id)
{
    const auto found = std::find_if(g_characters.begin(), g_characters.end(),
        [id](const Character &item) { return item.id == id; });
    return found == g_characters.end() ? nullptr : &*found;
}
const std::vector<Character> &characters() { return g_characters; }
const std::vector<TextureReplacement> &textures() { return g_textures; }

}  // namespace sm64ds::packs
