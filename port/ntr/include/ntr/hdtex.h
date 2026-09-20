// THE HD TEXTURE PACK'S CONFIGURATION SEAM.
//
// This header is the SEAM ONLY. It exists so the settings key, the CMake
// rows and the sampler's scale field can all land in one small inert commit,
// before the pack loader that gives them meaning is written. The stub below
// stores what it is told and answers when asked, and nothing in the port
// reads the answer yet, so a build carrying this file draws exactly the
// picture a build without it draws.
//
// THE CONTRACT THE LOADER INHERITS. hdtex_configure is called once, at boot,
// beside ntr::configure_aspect, from tests/walk_window.cpp's main. `enabled`
// is the HdTextures settings key (0 off, which is the default and the ROM's
// own textures; nonzero on). `pack_dir` is the directory the replacement
// images live in -- "textures_hd" under the asset root unless the player set
// SM64DS_HD_TEXTURES_DIR -- and it is a pointer the caller keeps alive for
// the whole run, because host_settings' own storage is a static buffer.
// Null or empty is "no directory", which reads as off whatever `enabled`
// says: a pack with nowhere to load from is not a pack.
//
// WHAT A REPLACEMENT IMAGE MAY BE. Any whole multiple of the DS texture's
// own pixel dimensions, and the multiple is carried to the raster in
// GxTriangle::tex_scale (ntr/gx.h): the sampler multiplies u and v by it and
// reads tw/th as the ACTUAL pixel dimensions of the bound buffer, so a 4x
// image of a 32x32 DS texture is bound as 128x128 with tex_scale 4 and every
// wrap, clamp and flip rule keeps working on the DS's own texel grid. A pack
// that replaces nothing leaves tex_scale at 1, which is the arithmetic the
// raster has always done.
//
// Lane TEX of run hd1 owns this file after the seam commit and is free to
// rewrite the whole of it, this note included.

#ifndef NTR_HDTEX_H
#define NTR_HDTEX_H

namespace ntr {

// Store the pack's configuration. Call once at boot, before anything binds a
// texture. See the note above for the arguments.
void hdtex_configure(int enabled, const char *pack_dir);

// What hdtex_configure was told. 1 when the pack is on AND has a directory,
// else 0; the directory as given, or "" when there is none.
int hdtex_enabled(void);
const char *hdtex_pack_dir(void);

}  // namespace ntr

#endif  // NTR_HDTEX_H
