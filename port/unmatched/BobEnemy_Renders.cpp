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

/* ---- CHAIN_CHOMP (actor 219, ov014) --------------------------------------
   Its ModelAnim is at +0x150 and it draws at the actor's own scale, and then
   it draws SEVEN MORE: the chain links live at +0x1dc on a 0x50 stride and
   each is a plain Model rendered at 1.0. The ROM's loop is unconditional --
   all seven every frame -- which is why the chomp's Render is longer than
   every other one in this gate put together. */
int _ZN10ChainChomp6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* ((B *)&mModelAnim)->m5(&mScaleX) */
    ((ModelAnim *)(c + 0x150))->ModelAnim::Render((const Vector3 *)(c + 0x80));
    for (int j = 0; j < 7; ++j)
        ((Model *)(c + 0x1dc + j * 0x50))->Model::Render(0);
    return 1;
}

/* ---- CHAIN_CHOMP_FENCE (actor 41, ov014) ---------------------------------
   The wooden post the chomp is chained to. One early out on its own +0x31e
   and one draw of the plain Model at +0xd4, at 1.0.

   Its shadow declares SIX virtuals and takes the sixth, which is ROM slot 5 --
   Model's Render. _ZTV5Model IS dual-filled (hal/cxxname_bridge.cpp puts
   Render in slot 4 and slot 5 for exactly this), so this one would have worked
   from src/. It is here anyway because the shadow's methods all return int
   where Model::Render returns void, which MSVC decorates differently. */
int _ZN15ChainChompFence6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(unsigned char *)(c + 0x31e) != 0)
        return 1;
    ((Model *)(c + 0xd4))->Model::Render(0);
    return 1;
}

/* ---- KOOPA_THE_QUICK (actor 188, ov062) ----------------------------------
   Hides material list 1 on bone 0 -- the shell he is not wearing while he
   races -- and then draws the ModelAnim at +0x300 at the actor's own scale. */
extern "C" void _ZN5Model12HideMaterialEii(void *self, int boneID, int listIdx);

int _ZN13KoopaTheQuick6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    _ZN5Model12HideMaterialEii(c + 0x300, 0, 1);
    /* ((Model *)&mModelAnim)->v5(&mScaleX) -- the shadow's sixth virtual, so
       ROM slot 5, which on a ModelAnim is Render */
    ((ModelAnim *)(c + 0x300))->ModelAnim::Render((const Vector3 *)(c + 0x80));
    return 1;
}

/* ---- KOOPA_FLAG (actor 205, ov062) ---------------------------------------
   The other half of the race, and the shortest Render in the gate: one
   dispatch on the ModelAnim at +0x108, drawn at 1.0. */
int _ZN9KoopaFlag6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* ((Base *)&mModelAnim)->m(0) */
    ((ModelAnim *)(c + 0x108))->ModelAnim::Render(0);
    return 1;
}

}  /* extern "C" */
