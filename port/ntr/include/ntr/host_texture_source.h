#pragma once
#include "texture.h"
#include <vector>
#include <cstdint>
namespace ntr {
// Borrowed immutable model payloads, retained by the mod asset owner. These
// descriptors never consume or enlarge the native DS common-model/VRAM arrays.
struct HostTextureSource {TextureDesc texture;std::uint32_t flags=0;};
struct HostPaletteSource {const std::uint8_t* data=nullptr;int size=0;};
bool gx_host_textures_register(const void* owner,const std::vector<HostTextureSource>&,const std::vector<HostPaletteSource>&);
void gx_host_textures_forget(const void* owner);
const void* gx_host_textures_scope(const void* owner);
}
