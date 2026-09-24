// Host seam for the unenrolled middle of the actor hierarchy:
// fBase_c::SceneNode, dBase_c's ctor/AfterInitResources/Spawn, and the
// _ZTV7dBase_c table that d_camera/d_map/d_meter/d_s_stage install as
// their vptr.
//
// WHY THIS EXISTS: no src/ TU defines these yet (the decomp enrolls
// overlay-by-overlay), but every actor construction runs through them:
// dActor_c::C2 -> dBase_c::C2 -> fBase_c::C2, the spawn spine ends in
// dBase_c::Spawn, and the init Process dispatches slot 2
// (AfterInitResources) virtually. Leaving any of them to /FORCE stubs
// jumps to the image base before the level finishes booting.
//
// The table is NOT filled by hand: _ZTV7dBase_c aliases MSVC's own
// ??_7dBase_c@@6B@ (same 18-slot declaration order -- fBase_c.h: "in
// _ZTV7fBase_c order. Do not reorder"). Every slot dispatches to a real
// implementation, including dBase's own slot-2 override, because this TU
// defines AfterInitResources (the key function), which is also what emits
// the MSVC vtable. A missing implementation fails at LINK time, not boot.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include "fBase_c.h"
#include "dBase_c.h"
#include "View.h"
#include "Camera.h"

/* fBase_c::SceneNode: links zeroed; owner is (re)set by Manager's body
   right after this runs, so zeroing it here is harmless. */
fBase_c::SceneNode::SceneNode()
    : parent(0), child(0), prev(0), next(0), owner(0)
{
}

void fBase_c::SceneNode::Reset()
{
    parent = 0;
    child = 0;
    prev = 0;
    next = 0;
}

/* dBase_c adds no members: the implicit fBase_c base init (gated real TU)
   does the work (spawn-context reads, tree link, priorities). */
dBase_c::dBase_c()
{
}

/* dBase_c.h: "marks the actor for destruction when init failed, then
   chains." Both callees are real gated TUs. */
void dBase_c::AfterInitResources(u32 vfSuccess)
{
    if (!vfSuccess)
        MarkForDestruction();
    fBase_c::AfterInitResources(vfSuccess);
}

extern "C" int func_02042ffc(unsigned id, void *parent, unsigned param1,
                             int flags);

/* dBase_c.h: static veneer to func_02042ffc, returns what it built. */
fBase_c *dBase_c::Spawn(u32 actorID, fBase_c *parent, int a, int b)
{
    return (fBase_c *)(size_t)func_02042ffc(actorID, parent, (unsigned)a, b);
}

/* Minimal host ctors for classes with no enrolled C1 TU. The compiler runs
   the real base chain automatically (View -> dBase -> fBase, Camera ->
   View); only the derived part starts zeroed, for InitResources to fill.
   This is what makes the registry's Camera factory (and any View-derived
   construction) survive: previously the placement face called a missing
   ??0Camera@@QAE@XZ stub. */
View::View()
{
    std::memset((char *)this + sizeof(dBase_c), 0,
                sizeof(View) - sizeof(dBase_c));
}

Camera::Camera()
{
    std::memset((char *)this + sizeof(View), 0,
                sizeof(Camera) - sizeof(View));
}

/* The vtable alias. Data aliasing is convention-safe (heap_globals.cpp
   does the same); the decorated MSVC spelling is verbatim. */
#pragma comment(linker, "/alternatename:__ZTV7dBase_c=??_7dBase_c@@6B@")

extern "C" {
/* Itanium spelling of the base-object ctor for C callers (d_camera and
   friends): placement-construct the real thing. The MSVC C1/C2 come from
   the gated src/_ZN7fBase_cC2Ev.cpp. */
void _ZN7fBase_cC2Ev(void *self)
{ ::new (self) fBase_c(); }

/* ---- RaycastGround (no enrolled TU) ----
   The 0x50-byte downward probe. Callers own the reach word at +0x4c
   (Player_InitResources writes td*2, walk_window writes 0x100000) and read
   results at +0x3c/+0x44/+0x48, so C1 is pure zero-init. SetObjAndPos's
   target offsets are unevidenced, so the origin rides in a side table
   keyed by probe pointer instead of inside the struct: zero layout risk.
   DetectClsn runs the hosted harness ray (clsn_vtable.cpp) straight down
   from the stored origin by the caller's reach and reports ground_y at
   +0x3c, the word both readers check. */
struct RgOrigin {
    const void *probe;
    int x, y, z;
    void *obj;
};
static RgOrigin g_rg[8];
static int g_rg_n;

void _ZN13RaycastGroundC1Ev(void *p)
{
    std::memset(p, 0, 0x50);
    for (int i = 0; i < 8; ++i)
        if (g_rg[i].probe == p)
            g_rg[i].probe = 0;
}

void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(void *p,
                                                        const void *v,
                                                        void *a)
{
    const int *pos = (const int *)v;
    int slot = -1;
    for (int i = 0; i < 8; ++i)
        if (g_rg[i].probe == p || g_rg[i].probe == 0) {
            slot = i;
            break;
        }
    if (slot < 0)
        slot = (g_rg_n = (g_rg_n + 1) & 7);
    g_rg[slot].probe = p;
    g_rg[slot].x = pos[0];
    g_rg[slot].y = pos[1];
    g_rg[slot].z = pos[2];
    g_rg[slot].obj = a;
}

extern "C" int hal_ground_ray(void *mc, int x, int y, int z, int reach,
                              int *out_y);
extern "C" void *port_stage_object(void);

int _ZN13RaycastGround10DetectClsnEv(void *p)
{
    const int *reach = (const int *)((const char *)p + 0x4c);
    int *ground_y = (int *)((char *)p + 0x3c);
    int x = 0, y = 0, z = 0;
    for (int i = 0; i < 8; ++i)
        if (g_rg[i].probe == p) {
            x = g_rg[i].x;
            y = g_rg[i].y;
            z = g_rg[i].z;
            break;
        }
    void *stage = port_stage_object();
    void *mc = stage ? (char *)stage + 0x91c : 0;
    if (!mc)
        return 0;
    int gy = y;
    int hit = hal_ground_ray(mc, x, y, z, *reach, &gy);
    if (hit)
        *ground_y = gy;
    return hit;
}

/* Kept for the main-line fill call: the table is the compiler's own, so
   there is nothing to fill. */
void hal_fill_dbase_vtable(void)
{
}

}  /* extern "C" */
