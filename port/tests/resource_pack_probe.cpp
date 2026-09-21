#include "resource_pack.h"

#include <cstdio>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: resource_pack_probe <pack-root>\n");
        return 2;
    }
    std::string error;
    if (!sm64ds::packs::load_all(argv[1], error)) {
        std::fprintf(stderr, "%s", error.c_str());
        return 1;
    }
    const auto *character = sm64ds::packs::character(4);
    if (!character || character->name != "Probe" || character->base_character != 2)
        return 3;
    if (character->animations.size() != 2 || character->hitbox.radius != 61.0f ||
        character->hitbox.hurt_height != 117.0f)
        return 4;
    if (sm64ds::packs::textures().size() != 1 ||
        sm64ds::packs::textures()[0].target_hash != 0x0123456789abcdefULL)
        return 5;
    std::puts("resource_pack_probe: PASS");
    return 0;
}
