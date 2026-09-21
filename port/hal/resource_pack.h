#pragma once

#include <map>
#include <cstdint>
#include <string>
#include <vector>

namespace sm64ds::packs {

struct Hitbox {
    float radius = 50.0f;
    float height = 100.0f;
    float hurt_radius = 50.0f;
    float hurt_height = 100.0f;
};

struct Character {
    int id = -1;
    int base_character = 0;
    std::string pack_id;
    std::string name;
    std::string body_model;
    std::string head_cap_model;
    std::string head_no_cap_model;
    std::map<std::string, std::string> animations;
    Hitbox hitbox;
};

struct TextureReplacement {
    std::string pack_id;
    std::string target;
    std::uint64_t target_hash = 0;
    std::string source;
};

// Loads every enabled <root>/<pack>/pack.lua. A broken pack is rejected without
// preventing the remaining packs from loading. Returns false when any pack was
// rejected and places a complete diagnostic in `error`.
bool load_all(const std::string &root, std::string &error);
void clear();

const Character *character(int id);
const std::vector<Character> &characters();
const std::vector<TextureReplacement> &textures();

}  // namespace sm64ds::packs
