/* ChiefChilly::Render -- the BLENDMODELANIM SLOT-5 COLLISION, hosted.
 *
 * run linkw wave 12, lane w12. Its own file rather than a block in
 * port/unmatched/Ov073_State.cpp for one mechanical reason: this body needs the
 * port's real BlendModelAnim.h (and so the port's Vector3), while that file
 * carries _ZN11ChiefChilly8BehaviorEv transplanted from matched src, which
 * declares its OWN `struct Vector3 { int x, y, z; };` at file scope. The two
 * cannot share a translation unit, and the transplanted body is kept byte for
 * byte with its source rather than reworded to fit.
 *
 * hal/blend_vtable.cpp's header predicted this landing in so many words: "if a
 * shadow-class TU ever dispatches BlendModelAnim::Render it will land on
 * blend_virtual18 and read its scale off the stack". src/_ZN11ChiefChilly6Render
 * Ev.cpp does exactly that -- it dispatches through a LOCAL six-virtual shadow,
 * which counts in ROM/Itanium numbering (two destructor slots), so its slot 5
 * is Render; the host _ZTV14BlendModelAnim is filled in MSVC numbering, where
 * slot 5 is Virtual18 and takes TWO arguments against the shadow's one. Same
 * fault and same fix as Butterfly / Fish / QuestionBlock / Whomp in
 * port/unmatched/ModelAnim_Renders.cpp: the dispatch is spelled as the
 * qualified BlendModelAnim::Render.
 *
 * mBlendModelAnim is at +0x30c and mScaleX at +0x80 (include/ChiefChilly.h,
 * both offsets evidenced by the class's matched functions; the factory
 * constructs the BlendModelAnim at +0x30c and InitResources hands
 * ModelBase::SetFile the same address).
 *
 * CccArena::Render is NOT here. It shadow-dispatches a plain Model at +0xd4,
 * and _ZTV5Model IS dual-filled -- slot 4 and slot 5 both mv_render,
 * hal/cxxname_bridge.cpp -- so the matched TU is correct and stays sliced.
 */
#include "BlendModelAnim.h"

extern "C" {

/* ---- ChiefChilly::Render, the BlendModelAnim slot-5 collision ------------- */
/* PORT_HOST_ABI: ROM-order BlendModelAnim slot-5 dispatch, the Whomp/Fish
   case one class up. */
int _ZN11ChiefChilly6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* ((Sub *)&mBlendModelAnim)->M(&mScaleX) -- the ROM slot-5 Render */
    ((BlendModelAnim *)(c + 0x30c))->BlendModelAnim::Render((const Vector3 *)(c + 0x80));
    return 1;
}

}  /* extern "C" */
