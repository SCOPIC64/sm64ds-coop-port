// THE MODEL SMOOTHING CONFIGURATION SEAM.
//
// This header is the SEAM ONLY, for the reason ntr/hdtex.h states: the
// settings key and the CMake rows land in one small inert commit ahead of
// the geometry work that gives them meaning. The stub below stores what it
// is told and nothing reads the answer, so a build carrying this file draws
// exactly the picture a build without it draws.
//
// THE CONTRACT. smooth_configure is called once, at boot, beside
// ntr::configure_aspect, from tests/walk_window.cpp's main. `level` is the
// SmoothModels settings key: 0 is off and is the default and the ROM's own
// geometry, 1..3 are subdivision levels, and anything outside 0..3 has
// already been clamped by host_settings before it arrives. It is clamped
// again here rather than trusted, because a level is about to size work per
// polygon.
//
// Lane MDL of run hd1 owns this file after the seam commit and is free to
// rewrite the whole of it, this note included.

#ifndef NTR_SMOOTH_H
#define NTR_SMOOTH_H

namespace ntr {

// Store the subdivision level. Call once at boot, before any geometry.
void smooth_configure(int level);

// What smooth_configure was told, clamped into 0..3. 0 is the ROM.
int smooth_level(void);

}  // namespace ntr

#endif  // NTR_SMOOTH_H
