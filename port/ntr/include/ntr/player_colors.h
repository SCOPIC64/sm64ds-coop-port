#pragma once
#include <cstdint>
#include <cstddef>
namespace ntr {
struct PlayerColorTexture { uint32_t offset; const char* name; };
// Render context only. No palette, material or actor in DS memory is edited.
void gx_player_colors_begin(int slot,int character,const PlayerColorTexture* textures,size_t count);
void gx_player_colors_end();
}
