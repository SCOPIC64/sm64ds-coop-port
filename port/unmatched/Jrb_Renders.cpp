/* HOST COPIES of Jolly Roger Bay's (ov016, gate 188) three slot-5 Render
 * collisions -- SHIP_UP (57), FLOAT_ON_WATER_PLATFORM_JRB (60) and ROCK_PILLAR
 * (58, the daObjKi_Hasira_c func_ov016_02112b28) -- the ModelAnim/Model slot-5
 * seam that gates 17/18/21/23/33/64/83/173/176/177 already walked into.
 *
 * hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in MSVC order (dtor 0, DoSetFile
 * 1, UpdateVerts 2, Virtual10 3, Render 4, Virtual18 5), because MSVC spends ONE
 * slot on the destructor where Itanium spends two, so ROM slot 5 (Render) lands
 * on the host array's Virtual18 -- a two-arg method called with the shadow's one
 * arg, which reads a scale off the stack and hands Model::Virtual10 a null
 * matrix (a c0000005 the first frame the actor draws). See
 * port/unmatched/ModelAnim_Renders.cpp for the long version and the measurement.
 *
 * All three JRB Renders are Platforms whose slot-5 dispatch is off a PLAIN Model
 * at +0xd4 (their D-tors destroy _ZN5ModelD1Ev at +0xd4, not a ModelAnim), the
 * RotatingFirebar shape exactly. _ZTV5Model IS dual-filled (Render sits in both
 * slot 4 and slot 5), so a Model draw would not itself fault -- but the matched
 * src dispatches through a LOCAL six-virtual shadow class whose methods return
 * int where Model::Render returns void (MSVC decorates that differently), so the
 * shadow-TU vtable call cannot be served from src/ and these are host copies out
 * of slice_gate188.txt (the ChainChompFence reading). Each body is the matched
 * source's control flow line for line; only the slot-5 dispatch is spelled as
 * the qualified Model::Render the ROM means.
 *
 * ShipDown (56) reuses ShipUp's slot (same _ZTV6ShipUp, same body); SlidingBox
 * (313) reuses FloatOnWater's slot (same _ZTV23FloatOnWaterPlatformJrb). Neither
 * has a separate Render.
 *
 * NEVER in ModelAnim_Renders.cpp -- A owns that file. These live here.
 */
#include "Model.h"
#include "ModelAnim.h"

extern "C" {

/* ---- SHIP_UP (actor 57, ov016) / shared by SHIP_DOWN (56) -----------------
   src/_ZN6ShipUp6RenderEv.cpp is the bare slot-5 draw through a six-virtual
   ROM-order local shadow (`Base *b = &((Derived *)this)->base; b->m(0)`) over a
   `Derived { char pad[0xd4]; Base base; }`, so its "slot 5" is the Model at
   +0xd4 (ShipUpD1 destroys _ZN5ModelD1Ev at +0xd4). No early outs.
   PORT_HOST_ABI: ROM-order model slot-5 dispatch, the RotatingFirebar case. */
int _ZN6ShipUp6RenderEv(void *selfv)
{ ((Model *)((char *)selfv + 0xd4))->Model::Render(0); return 1; }

/* ---- SLIDING_BOX (actor 313, ov016) via _ZTV23FloatOnWaterPlatformJrb -----
   The config class name is a decoy (the gate-178 Amilift pattern): the named
   _ZN23FloatOnWaterPlatformJrb* methods and this 37-word derived table belong to
   SLIDING_BOX (id 313), which SlidingBox_Spawn installs directly. The BASE class
   FLOAT_ON_WATER_PLATFORM_JRB (id 60) is the daObjKi_Ita_c base table
   data_ov016_02114bcc, whose slot 9 is the ARM9 Platform::Render default -- it
   has NO slot-5 collision and no host render here.
   src/_ZN23FloatOnWaterPlatformJrb6RenderEv.cpp is the same bare slot-5 draw
   through the six-virtual ROM-order shadow off the Model at +0xd4
   (FloatOnWaterPlatformJrbD1 destroys _ZN5ModelD1Ev at +0xd4). No early outs.
   PORT_HOST_ABI: ROM-order model slot-5 dispatch, the RotatingFirebar case. */
int _ZN23FloatOnWaterPlatformJrb6RenderEv(void *selfv)
{ ((Model *)((char *)selfv + 0xd4))->Model::Render(0); return 1; }

/* ---- ROCK_PILLAR (actor 58, ov016) / daObjKi_Hasira_c::Render ------------
   src/func_ov016_02112b28.cpp is the same bare slot-5 draw through the
   six-virtual ROM-order shadow off the Model at +0xd4 (func_ov016_02112a00, the
   RockPillar D1, destroys _ZN5ModelD1Ev at +0xd4). No early outs.
   PORT_HOST_ABI: ROM-order model slot-5 dispatch, the RotatingFirebar case. */
int func_ov016_02112b28(void *selfv)
{ ((Model *)((char *)selfv + 0xd4))->Model::Render(0); return 1; }

/* ---- daObjKi_Ita_c BASE Render (func_ov002_020b5c24, ov002) --------------
   id 60 (FLOAT_ON_WATER_PLATFORM_JRB / daObjKi_Ita_c base) does not override
   Render -- its slot 9 is the Platform base method func_ov002_020b5c24, which is
   the SAME bare slot-5 draw through a six-virtual ROM-order shadow off the Model
   at +0xd4. Host copy for the same reason. Shared Platform base method, but no
   other hosted class has needed it yet, so it lives here with the gate.
   PORT_HOST_ABI: ROM-order model slot-5 dispatch, the RotatingFirebar case. */
int func_ov002_020b5c24(void *selfv)
{ ((Model *)((char *)selfv + 0xd4))->Model::Render(0); return 1; }

}  /* extern "C" */
