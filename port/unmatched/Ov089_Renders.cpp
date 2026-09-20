/* HOST COPY of src/_ZN3Key6RenderEv.cpp -- the ModelAnim slot-5 collision on
 * the boss-reward class. run linkw wave 6, lane w6-A.
 *
 * The reading is port/unmatched/ModelAnim_Renders.cpp's: a matched TU that
 * dispatches through a LOCAL six-virtual shadow class counts in the ROM's
 * (Itanium) numbering, where the destructor takes two slots, so its "slot 5"
 * is Render. hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in MSVC order, where
 * slot 5 is Virtual18 -- and unlike _ZTV5Model, ModelAnim cannot be
 * dual-filled, because Virtual18 is really there.
 *
 * Key::Render dispatches slot 5 through the shadow TWICE, and only ONE of the
 * two is the collision:
 *
 *     ((Sub *)&mModelAnim)->m(...)   +0x114  ModelAnim  <- collision
 *     ((Sub *)&mModel)->m(0)         +0x178  Model      <- dual-filled, fine
 *
 * Hosting the whole body is still the right shape (one TU, one dispatch
 * convention) and it is what every earlier collision did. Offsets are Key.h's:
 * mScaleX 0x80, unk_0b0 0xb0, mModelAnim 0x114, mModel 0x178, mState 0x444,
 * unk_448 0x448. Both KEY (282) and LAST_STAR (283) run this body -- they
 * share _ZTV3Key, which is the only vtable ov089 defines.
 *
 * The body below is the matched source's control flow line for line; only the
 * shadow dispatches are respelled as the qualified methods the ROM means.
 */
#include "Model.h"
#include "ModelAnim.h"

extern "C" {
extern char data_ov089_021328b4[];

/* PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Fish case. */
int _ZN3Key6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if ((*(unsigned *)(c + 0xb0) & 0x40000) != 0)
        return 1;
    if (*(int *)(c + 0x448) != 0) {
        ((ModelAnim *)(c + 0x114))->ModelAnim::Render(0);
    } else {
        ((ModelAnim *)(c + 0x114))->ModelAnim::Render((const Vector3 *)(c + 0x80));
        if (*(int *)(data_ov089_021328b4 + (*(int *)(c + 0x444) << 2)) != 0 &&
            *(int *)(c + 0x448) == 0)
            ((Model *)(c + 0x178))->Model::Render(0);
    }
    return 1;
}
}
