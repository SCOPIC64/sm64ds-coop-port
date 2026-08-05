/* HOST COPIES of the gate-32 classes' Render methods -- the ModelAnim slot-5
 * collision, for the fourth gate running.
 *
 * hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in MSVC order (dtor 0, DoSetFile
 * 1, UpdateVerts 2, Virtual10 3, Render 4, Virtual18 5), because MSVC spends
 * ONE slot on the destructor where Itanium spends two. _ZTV5Model CAN be
 * dual-filled and is; ModelAnim cannot, because Virtual18 -- which takes two
 * arguments where the shadow passes one -- already occupies the slot Render
 * needs. See port/unmatched/ModelAnim_Renders.cpp for the long version and for
 * the measurement that pinned it (a c0000005 in Model::Virtual10 through a
 * null matrix).
 *
 * Every one of these src bodies dispatches through a LOCAL SHADOW CLASS with
 * six virtuals, which counts in the ROM's numbering, so their "m5" is Render.
 * Each body below is the matched source's control flow line for line; only the
 * dispatches are spelled as the qualified methods the ROM means.
 *
 * KingBobOmb::Render is NOT here. It is the one of the six that calls
 * Model::Render by its Itanium C name rather than through a shadow, so
 * src/_ZN10KingBobOmb6RenderEv.cpp serves the host build unchanged.
 */
#include "Model.h"
#include "ModelAnim.h"

extern "C" {

/* ---- BOB_OMB (actor 206, ov102) ------------------------------------------
   +0x3f3 is the "I have a model to draw" flag InitResources sets last, and
   bit 0x40000 of the actor flags is the engine's own hide. The ModelAnim is at
   +0x300 and takes the actor's own scale Vector3 at +0x80. */
int _ZN6BobOmb6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(unsigned char *)(c + 0x3f3) != 0 &&
        (*(int *)(c + 0xb0) & 0x40000) == 0)
        /* ((VBase *)&mModelAnim)->method5(&unk_080) */
        ((ModelAnim *)(c + 0x300))->ModelAnim::Render((const Vector3 *)(c + 0x80));
    return 1;
}

}  /* extern "C" */
