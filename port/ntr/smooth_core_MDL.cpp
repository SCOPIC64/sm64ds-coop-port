// Curved PN-triangle subdivision -- the pure geometry kernel.
// Contract, guarantees and the reason this is view-space-only: smooth_core_MDL.h.
//
// PORTED FROM app/src/subdiv.rs in the tangOS-SM64DS project
// (393 lines, unit-tested, parked; docs/SMOOTHING.md explains it). The maths
// is line for line the same. Three things are deliberately different, and each
// one is because this runs inside a live frame instead of an offline export:
//
//  1. NORMAL SOURCES. The Rust welded smooth normals out of geometry
//     (welded_normals(): quantise the position, accumulate area-weighted face
//     normals, normalise) because a GLB part carries no normals of its own.
//     The DS display list DOES carry them: the NORMAL command (0x21) is how
//     the hardware lights a vertex, and gx.cpp already turns it into a
//     view-space unit normal. Those are the normals Nintendo authored for
//     exactly this shading, so this kernel takes them as input and does no
//     welding at all. That removes the weld pass, the hash map, the per-batch
//     buffer and -- the part that actually mattered -- every question about
//     draw order, because nothing has to be held back to the end of a model.
//     Welding stays possible: the kernel takes normals per call, so a caller
//     that wants welded ones can compute them and pass them instead.
//
//  2. NO OUTPUT WELD. The Rust deduplicated output vertices by quantised
//     (position, uv, bone) so the exported index buffer stayed small. There is
//     no index buffer here -- the geometry stage emits independent triangles,
//     exactly as the DS display lists do -- so the dedup map is gone and with
//     it its per-frame allocation. Shared edges still land on the same curve
//     (see the edge-control-point argument in the header), which is the
//     property the dedup was protecting.
//
//  3. NO BONES. The Rust picked one bone per new vertex (pick_bone, ties to
//     the smaller bone id so a shared edge midpoint resolved the same way in
//     both neighbours). Here the bone has ALREADY been applied: the position
//     arrives after the DS POSITION matrix, which is the bone matrix a
//     MTX_RESTORE put there. The tie-break has no work left to do, and its
//     job -- "an edge point must come out identical in both triangles that
//     share the edge" -- is now a purely geometric property of the edge
//     control points, which is what the selftest measures instead.
//
// THE SUBDIVISION RULE (which triangles get curved), all of it data:
//   * No authored normal on a corner  -> leave alone. An unlit polygon (the
//     HUD's own 3D geometry, a billboard, anything drawn with COLOR instead
//     of NORMAL) has no surface to curve and must come out exactly as it went
//     in.
//   * All three normals agree         -> leave alone. The patch would be flat
//     anyway; skipping is not an approximation of that, it is the same answer
//     for no work, and it is what keeps a lone flat quad (a coin, a sign, a
//     stage face) bit-identical.
//   * Tightest implied curvature radius above max_radius, or longest edge
//     above max_edge -> leave alone. This is the pillow guard. PN lifts the
//     centre of a large, gently-curved face, which reads as a bulge in stage
//     ground where it reads as roundness on a character. Curvature radius
//     (edge length / normal turn across that edge) is the scale-free way to
//     tell those apart: a faceted sphere turns its normals a lot over a short
//     edge, a rolling hill turns them a little over a long one.

#include "ntr/smooth_core_MDL.h"

#include <math.h>

namespace ntr {
namespace {

inline void v_sub(const float a[3], const float b[3], float o[3]) {
    o[0] = a[0] - b[0]; o[1] = a[1] - b[1]; o[2] = a[2] - b[2];
}
inline float v_dot(const float a[3], const float b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// The Rust `edge()`: the control point one third along i->j, pulled back onto
// i's tangent plane. With ni perpendicular to (pj - pi) -- a flat face -- the
// pull-back term is zero and this is the flat cubic's own control point,
// which is why flat input stays exactly flat.
void pn_edge(const float pi[3], const float pj[3], const float ni[3],
             float out[3]) {
    float d[3];
    v_sub(pj, pi, d);
    const float w = v_dot(d, ni);
    for (int k = 0; k < 3; ++k)
        out[k] = (pi[k] * 2.0f + pj[k] - ni[k] * w) * (1.0f / 3.0f);
}

void unit_normal(const SmoothVertex &v, float out[3]) {
    const float l2 = v.nx * v.nx + v.ny * v.ny + v.nz * v.nz;
    if (l2 > 1e-16f) {
        const float inv = 1.0f / sqrtf(l2);
        out[0] = v.nx * inv; out[1] = v.ny * inv; out[2] = v.nz * inv;
    } else {
        out[0] = out[1] = out[2] = 0.0f;
    }
}

// Interpolate a packed 0xAARRGGBB colour linearly in barycentrics, byte by
// byte, the same rounding gx.cpp's own clip_lerp uses. At a corner (one weight
// 1, the others 0) this returns that corner's colour bit for bit.
uint32_t bary_color(uint32_t c0, uint32_t c1, uint32_t c2,
                    float a, float b, float c) {
    uint32_t out = 0;
    for (int s = 0; s < 32; s += 8) {
        const float ch = (float)((c0 >> s) & 0xFF) * a
                       + (float)((c1 >> s) & 0xFF) * b
                       + (float)((c2 >> s) & 0xFF) * c;
        const int i = (int)(ch + 0.5f);
        out |= (uint32_t)(i < 0 ? 0 : (i > 255 ? 255 : i)) << s;
    }
    return out;
}

}  // namespace

void smooth_build_patch(const SmoothVertex &p1, const SmoothVertex &p2,
                        const SmoothVertex &p3, SmoothPatch &out) {
    const float P1[3] = {p1.x, p1.y, p1.z};
    const float P2[3] = {p2.x, p2.y, p2.z};
    const float P3[3] = {p3.x, p3.y, p3.z};
    float N1[3], N2[3], N3[3];
    unit_normal(p1, N1);
    unit_normal(p2, N2);
    unit_normal(p3, N3);

    for (int k = 0; k < 3; ++k) {
        out.b[0][k] = P1[k];      // b300
        out.b[1][k] = P2[k];      // b030
        out.b[2][k] = P3[k];      // b003
    }
    pn_edge(P1, P2, N1, out.b[3]);   // b210
    pn_edge(P2, P1, N2, out.b[4]);   // b120
    pn_edge(P2, P3, N2, out.b[5]);   // b021
    pn_edge(P3, P2, N3, out.b[6]);   // b012
    pn_edge(P1, P3, N1, out.b[7]);   // b201
    pn_edge(P3, P1, N3, out.b[8]);   // b102

    // The standard PN centre lift: E is the mean of the six edge points, V the
    // vertex centroid, b111 = E + (E - V)/2.
    for (int k = 0; k < 3; ++k) {
        const float e = (out.b[3][k] + out.b[4][k] + out.b[5][k]
                       + out.b[6][k] + out.b[7][k] + out.b[8][k]) * (1.0f / 6.0f);
        const float v = (P1[k] + P2[k] + P3[k]) * (1.0f / 3.0f);
        out.b[9][k] = e + (e - v) * 0.5f;
    }
}

void smooth_eval_patch(const SmoothPatch &patch, float a, float b, float c,
                       float out[3]) {
    const float w[10] = {
        a * a * a,          // b300
        b * b * b,          // b030
        c * c * c,          // b003
        3.0f * a * a * b,   // b210
        3.0f * a * b * b,   // b120
        3.0f * b * b * c,   // b021
        3.0f * b * c * c,   // b012
        3.0f * a * a * c,   // b201
        3.0f * a * c * c,   // b102
        6.0f * a * b * c    // b111
    };
    out[0] = out[1] = out[2] = 0.0f;
    for (int i = 0; i < 10; ++i)
        for (int k = 0; k < 3; ++k)
            out[k] += patch.b[i][k] * w[i];
}

int smooth_tess_factor(const SmoothVertex &p1, const SmoothVertex &p2,
                       const SmoothVertex &p3, const SmoothPolicy &pol) {
    if (pol.level <= 0) return 1;

    float N[3][3];
    unit_normal(p1, N[0]);
    unit_normal(p2, N[1]);
    unit_normal(p3, N[2]);
    // A corner with no authored normal: nothing to curve towards.
    for (int i = 0; i < 3; ++i)
        if (N[i][0] == 0.0f && N[i][1] == 0.0f && N[i][2] == 0.0f) return 1;

    const float P[3][3] = {{p1.x, p1.y, p1.z}, {p2.x, p2.y, p2.z},
                           {p3.x, p3.y, p3.z}};
    static const int E0[3] = {0, 1, 2};
    static const int E1[3] = {1, 2, 0};

    bool curved = false;
    float tightest = 3.4e38f;   // smallest implied radius over the three edges
    float longest = 0.0f;
    for (int e = 0; e < 3; ++e) {
        const int i = E0[e], j = E1[e];
        const float cosij = v_dot(N[i], N[j]);
        if (cosij < pol.flat_cos) curved = true;

        float d[3];
        v_sub(P[j], P[i], d);
        const float len = sqrtf(v_dot(d, d));
        if (len > longest) longest = len;

        // Chord between the unit normals: 2*sin(theta/2), which IS the turn
        // angle to within a percent over the range that matters and costs no
        // trigonometry. Radius = arc length / turn.
        float nd[3];
        v_sub(N[j], N[i], nd);
        const float turn = sqrtf(v_dot(nd, nd));
        if (turn > 1e-6f) {
            const float r = len / turn;
            if (r < tightest) tightest = r;
        }
    }
    // Three normals that agree: the patch is flat, so leave the triangle
    // exactly as it arrived.
    if (!curved) return 1;
    if (pol.max_edge > 0.0f && longest >= pol.max_edge) return 1;
    if (pol.max_radius > 0.0f && tightest > pol.max_radius) return 1;

    int level = pol.level;
    if (level > SMOOTH_MAX_LEVEL) level = SMOOTH_MAX_LEVEL;
    return 1 << level;
}

int smooth_subdivide(const SmoothVertex &p1, const SmoothVertex &p2,
                     const SmoothVertex &p3, int tf,
                     SmoothSink sink, void *ctx) {
    if (tf <= 1) {
        sink(ctx, p1, p2, p3);
        return 1;
    }
    if (tf > SMOOTH_MAX_TF) tf = SMOOTH_MAX_TF;

    SmoothPatch patch;
    smooth_build_patch(p1, p2, p3, patch);

    // The whole tessellation lives in this one stack array: 81 vertices at the
    // top level, no allocation on any path.
    SmoothVertex grid[SMOOTH_MAX_GRID];
    const int stride = tf + 1;
    // tf is a power of two, so 1/tf is exact and ia*inv lands on 1.0 exactly
    // at the corner -- which is what makes corner preservation bit-exact
    // rather than merely close.
    const float inv = 1.0f / (float)tf;

    for (int ia = 0; ia <= tf; ++ia) {
        for (int ib = 0; ib <= tf - ia; ++ib) {
            const float a = (float)ia * inv;
            const float b = (float)ib * inv;
            float c = 1.0f - a - b;
            if (c < 0.0f) c = 0.0f;

            SmoothVertex &o = grid[ia * stride + ib];
            float pos[3];
            smooth_eval_patch(patch, a, b, c, pos);
            o.x = pos[0]; o.y = pos[1]; o.z = pos[2];
            // Linear in barycentrics: textures never swim, and a corner gets
            // its own coordinate back bit for bit.
            o.u = p1.u * a + p2.u * b + p3.u * c;
            o.v = p1.v * a + p2.v * b + p3.v * c;
            o.color = bary_color(p1.color, p2.color, p3.color, a, b, c);
            // Carried for a future relighting pass; nothing reads it today
            // (the colour above is the lighting, already baked at NORMAL).
            o.nx = p1.nx * a + p2.nx * b + p3.nx * c;
            o.ny = p1.ny * a + p2.ny * b + p3.ny * c;
            o.nz = p1.nz * a + p2.nz * b + p3.nz * c;
        }
    }

    int n = 0;
    for (int ia = 0; ia < tf; ++ia) {
        for (int ib = 0; ib < tf - ia; ++ib) {
            // Upward sub-triangle: winding matches the parent's.
            sink(ctx, grid[ia * stride + ib],
                      grid[(ia + 1) * stride + ib],
                      grid[ia * stride + ib + 1]);
            ++n;
            if (ib < tf - ia - 1) {
                sink(ctx, grid[(ia + 1) * stride + ib],
                          grid[(ia + 1) * stride + ib + 1],
                          grid[ia * stride + ib + 1]);
                ++n;
            }
        }
    }
    return n;
}

}  // namespace ntr
