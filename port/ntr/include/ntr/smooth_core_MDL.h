// Curved PN-triangle subdivision -- the pure geometry kernel.
//
// This is the C++ port of the parked tangOS-SM64DS smoother
// (app/src/subdiv.rs, docs/SMOOTHING.md): curved point-normal triangles,
// Vlachos et al. 2001. Each input triangle becomes a tessellated cubic Bezier
// patch whose shape is driven by the three corner normals.
//
// WHAT IT GUARANTEES, and what the selftest pins:
//   * The three corners evaluate to the ORIGINAL vertices EXACTLY. The scheme
//     interpolates: silhouettes grow, nothing shrinks or drifts.
//   * A triangle whose three normals agree is EXACTLY flat afterwards (the
//     tangent-plane projections are no-ops and the centre lift collapses to
//     the centroid). Curvature appears only where the normals disagree.
//   * An edge's four control points depend ONLY on that edge's two endpoints
//     and their normals, so two triangles that share an edge tessellate it to
//     the same curve whichever way round they name it -- watertight, with no
//     weld pass and no cross-triangle state.
//   * Sub-triangle winding matches the parent's.
//   * No allocation, no global state, deterministic: the whole grid lives in
//     one fixed stack array, so this is callable from inside the geometry
//     stage with nothing buffered.
//
// WHERE THE NORMALS COME FROM IN THE PORT. Unlike the Rust original, which had
// to weld normals out of geometry because a GLB part carries none, the DS
// display list carries the model's OWN authored per-vertex normal in the
// NORMAL command (0x21) -- that is what the hardware lights with. gx.cpp
// already transforms it by the VECTOR matrix, which is the POSITION matrix
// without translation, so a stream normal and a view-space position live in
// the same space and need no reconciliation. Welding is therefore a follow-up
// (see smooth_core_MDL.cpp's NORMAL SOURCES note), not a prerequisite.
//
// THE SPACE. Everything here is VIEW SPACE: after the DS position matrix,
// before the projection, before the perspective divide and before near
// clipping. Subdividing after the divide would curve in a non-linear space
// and bend straight edges by the near plane's distance.

#ifndef NTR_SMOOTH_CORE_MDL_H
#define NTR_SMOOTH_CORE_MDL_H

#include <stdint.h>

namespace ntr {

// tf = 1 << level. Level 1 = 4x triangles, 2 = 16x, 3 = 64x.
enum {
    SMOOTH_MAX_LEVEL = 3,
    SMOOTH_MAX_TF    = 1 << SMOOTH_MAX_LEVEL,                       // 8
    SMOOTH_MAX_GRID  = (SMOOTH_MAX_TF + 1) * (SMOOTH_MAX_TF + 1),   // 81
    // (tf+1)(tf+2)/2 grid points, tf*tf sub-triangles at the top level.
    SMOOTH_MAX_TRIS  = SMOOTH_MAX_TF * SMOOTH_MAX_TF                // 64
};

// One corner handed to the smoother.
struct SmoothVertex {
    float x, y, z;        // view-space position
    float nx, ny, nz;     // view-space normal; need not be unit length
    float u, v;           // texel coordinates, as GxVertex carries them
    uint32_t color;       // 0xAARRGGBB, lighting already baked per vertex
};

// The ten control points of the cubic patch, ordered exactly as the Rust
// original: b300, b030, b003, b210, b120, b021, b012, b201, b102, b111.
struct SmoothPatch {
    float b[10][3];
};

// How hard to subdivide, and what to leave alone. Every field is data, not a
// hand list of model names: see THE SUBDIVISION RULE in smooth_core_MDL.cpp.
struct SmoothPolicy {
    int level;         // 0 = off; clamped to SMOOTH_MAX_LEVEL
    // Corner normals at least this aligned (dot product of the unit normals)
    // count as one flat face: the patch would be flat anyway, so the triangle
    // passes through untouched and its output is bit-identical to its input.
    float flat_cos;
    // Curvature cap, in the same world units as the positions. A triangle
    // whose tightest implied radius of curvature (edge length / normal turn)
    // is LARGER than this is a big gently-curved surface -- stage ground,
    // a water plane -- where the PN centre lift reads as a pillow rather than
    // as roundness. 0 disables the cap.
    float max_radius;
    // Skip a triangle whose longest edge is at least this long in view-space
    // units, whatever its curvature. 0 disables the cap.
    float max_edge;
};

// Build the patch for corners p1, p2, p3.
void smooth_build_patch(const SmoothVertex &p1, const SmoothVertex &p2,
                        const SmoothVertex &p3, SmoothPatch &out);

// Evaluate at barycentric (a, b, c) for (p1, p2, p3); a + b + c == 1.
void smooth_eval_patch(const SmoothPatch &patch, float a, float b, float c,
                       float out[3]);

// The tessellation factor this triangle earns under `pol`: 1 means "emit it
// unchanged", anything higher means tf*tf sub-triangles. Pure predicate, no
// side effects, safe on degenerate input.
int smooth_tess_factor(const SmoothVertex &p1, const SmoothVertex &p2,
                       const SmoothVertex &p3, const SmoothPolicy &pol);

// Emit the tessellation. `sink` is called once per sub-triangle, in a
// deterministic order, winding preserved. Returns the number of sub-triangles
// emitted; tf <= 1 emits the input triangle once, unchanged.
typedef void (*SmoothSink)(void *ctx, const SmoothVertex &a,
                           const SmoothVertex &b, const SmoothVertex &c);
int smooth_subdivide(const SmoothVertex &p1, const SmoothVertex &p2,
                     const SmoothVertex &p3, int tf,
                     SmoothSink sink, void *ctx);

// How many sub-triangles a given tf produces. Pure arithmetic, for the
// counters that must size storage before emitting.
inline int smooth_tri_count(int tf) { return tf <= 1 ? 1 : tf * tf; }

}  // namespace ntr

#endif  // NTR_SMOOTH_CORE_MDL_H
