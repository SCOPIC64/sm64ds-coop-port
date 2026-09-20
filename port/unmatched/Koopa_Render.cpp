/* HOST COPY of src/_ZN5Koopa6RenderEv.cpp (KOOPA, 203, ov062, gate 182) --
 * the ModelAnim slot-5 collision, the Whomp/Butterfly/Fish/BobOmb/Goomba/
 * KoopaTheQuick case, for the same overlay KoopaTheQuick already wrote down
 * (port/unmatched/BobEnemy_Renders.cpp) at the same +0x300 offset.
 *
 * The matched TU dispatches its ModelAnim at +0x300 through a LOCAL six-virtual
 * shadow (`struct Mdl { v0..v4; slot5(void*); }`, so `slot5` is the ROM's slot
 * 5) -- ROM disasm 0x021190b4: `ldr r2,[r0]; ldr r2,[r2,#0x14]; blx r2` with
 * r0 = this+0x300, r1 = &mScaleX. The host _ZTV9ModelAnim is MSVC-ordered (one
 * dtor slot where Itanium spends two), so ROM slot 5 lands on Virtual18 --
 * which takes two arguments where the shadow passes one -- and cannot be
 * dual-filled (the ModelAnim_Renders.cpp measurement). The fix is the
 * qualified ModelAnim::Render, exactly as BobEnemy_Renders.cpp spells it for
 * KoopaTheQuick.
 *
 * The matched source's control flow line for line:
 *   - the engine hide at flag 0x40000;
 *   - the shell material swap: mKoopaVariant (+0x390) == 1 shows material
 *     list 1 and hides list 2, anything else hides 1 and shows 2 (in-shell vs
 *     out-of-shell) -- the matched TU spells these _ZN5Model12ShowMaterialEii /
 *     _ZN5Model12HideMaterialEii; here they are the qualified Model methods,
 *     whose bodies are the matched src TUs (HideMaterial was already sliced;
 *     ShowMaterial joins in slice_gate182.txt);
 *   - the SCALE SWAP: while unk_10c == 1 and mKoopaVariant == 2 (the shell
 *     sliding under a rider) the three scale words are halved with rounding
 *     ((x * 0x800 + 0x800) >> 12) for the duration of one Render and put
 *     straight back. `saved` is volatile in the matched source and stays
 *     volatile here: it is what stops the compiler from folding the restore
 *     into the multiply (the Goomba reading);
 *   - the slot-5 draw at the actor's own scale Vector3 at +0x80.
 *
 * PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Fish case.
 */
#include "Model.h"
#include "ModelAnim.h"

/* PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Fish
 * case. */
extern "C" int _ZN5Koopa6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    volatile int savedX, savedY, savedZ;

    if ((*(int *)(c + 0xb0) & 0x40000) != 0)
        return 1;

    if (*(int *)(c + 0x390) == 1) {
        ((Model *)(c + 0x300))->Model::ShowMaterial(0, 1);
        ((Model *)(c + 0x300))->Model::HideMaterial(0, 2);
    } else {
        ((Model *)(c + 0x300))->Model::HideMaterial(0, 1);
        ((Model *)(c + 0x300))->Model::ShowMaterial(0, 2);
    }

    savedX = *(int *)(c + 0x80);
    savedY = *(int *)(c + 0x84);
    savedZ = *(int *)(c + 0x88);

    if (*(int *)(c + 0x10c) == 1 && *(int *)(c + 0x390) == 2) {
        *(int *)(c + 0x80) = (int)(((long long)*(volatile int *)(c + 0x80)
                                    * 0x800 + 0x800) >> 12);
        *(int *)(c + 0x84) = (int)(((long long)*(volatile int *)(c + 0x84)
                                    * 0x800 + 0x800) >> 12);
        *(int *)(c + 0x88) = (int)(((long long)*(volatile int *)(c + 0x88)
                                    * 0x800 + 0x800) >> 12);
    }

    /* ((Mdl *)&mModelAnim)->slot5(&mScaleX) -- ROM slot 5, ModelAnim::Render */
    ((ModelAnim *)(c + 0x300))->ModelAnim::Render((const Vector3 *)(c + 0x80));

    *(int *)(c + 0x80) = savedX;
    *(int *)(c + 0x84) = savedY;
    *(int *)(c + 0x88) = savedZ;
    return 1;
}
