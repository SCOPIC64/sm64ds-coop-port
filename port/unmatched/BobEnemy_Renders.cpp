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

/* ---- GOOMBA (actor 200, ov084) -------------------------------------------
   Two early outs -- the engine hide at flag 0x40000 and the class's own
   +0x111 -- then the draw, and the draw is bracketed by a SCALE SWAP: while
   the goomba is in death type 1 (the squash) its three scale words are
   multiplied by the per-size factor in data_ov084_02130258 for the duration of
   one Render and put straight back. The ModelAnim is at +0x370 and takes the
   actor's scale Vector3 at +0x80.

   `backup` is volatile in the matched source and stays volatile here: it is
   what stops the compiler from folding the restore into the multiply. */
extern "C" int data_ov084_02130258[];
extern "C" void _ZN15MaterialChanger6UpdateER15ModelComponents(char *self,
                                                               void *model);
extern "C" void _ZN8CapEnemy14RenderCapModelEPK7Vector3(void *self,
                                                        const void *v);

int _ZN6Goomba6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    int *scale = (int *)(c + 0x80);
    volatile int backup[3];
    if ((*(int *)(c + 0xb0) & 0x40000) != 0 || *(unsigned char *)(c + 0x111))
        return 1;
    backup[0] = scale[0];
    backup[1] = scale[1];
    backup[2] = scale[2];
    if (*(int *)(c + 0x10c) == 1) {
        int k = data_ov084_02130258[*(int *)(c + 0x460)];
        for (int i = 0; i < 3; ++i)
            scale[i] = (int)((((long long)scale[i] * k) + 0x800) >> 12);
    }
    /* ((Sub *)&mModelAnim)->m5(&mScaleX) */
    ((ModelAnim *)(c + 0x370))->ModelAnim::Render((const Vector3 *)scale);
    scale[0] = backup[0];
    scale[1] = backup[1];
    scale[2] = backup[2];
    _ZN15MaterialChanger6UpdateER15ModelComponents(c + 0x3fc, c + 0x378);
    _ZN8CapEnemy14RenderCapModelEPK7Vector3(c, 0);
    return 1;
}

/* ---- BOB_OMB_BUDDY (actor 181, ov084) ------------------------------------
   The whole body is one dispatch. Its ModelAnim is at +0x108 and it draws at
   1.0 rather than at the actor's scale. */
int _ZN11BobOmbBuddy6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* ((Base *)&mModelAnim)->m(0) */
    ((ModelAnim *)(c + 0x108))->ModelAnim::Render(0);
    return 1;
}

}  /* extern "C" */
