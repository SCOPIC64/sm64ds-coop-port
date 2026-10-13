#pragma once
#include <string>
namespace sm64ds::textures {
// Creates a fresh, editable resource pack; an existing pack is never replaced.
bool create_pack(const std::string& capture, const std::string& packs,
                 std::string& created, std::string& error);
}
