/* Clam::Render -- the MODELANIM SLOT-5 COLLISION, hosted.
 *
 * run linkw wave 12, lane w12. Companion to hal/actor_classes_ov064_w12.cpp.
 *
 * Clam_Spawn constructs a ModelAnim at +0xd4 (_ZN9ModelAnimC1Ev) and
 * src/_ZN4Clam6RenderEv.cpp dispatches through a LOCAL six-virtual shadow
 * class:
 *
 *     struct Base { virtual void v0..v4(); virtual void m(int); };
 *     struct Derived { char pad[0xd4]; Base base; };
 *     Base *b = &((Derived *)this)->base; b->m(0);
 *
 * A local shadow counts in ROM/Itanium numbering (two destructor slots), so its
 * slot 5 is ModelAnim::Render. hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in
 * MSVC numbering, where slot 5 is Virtual18 -- and its own comment says the
 * consequence out loud: "No dual-fill here: Render's ROM slot (5) is
 * Virtual18's MSVC slot, so shadow-TU Render dispatch cannot be served by the
 * same array." Virtual18 takes TWO arguments where the shadow passes one, so
 * the callee reads its scale off the stack and hands Model::Virtual10 whatever
 * that was. That is the MEASURED Butterfly / Fish / QuestionBlock / Whomp
 * fault (a c0000005 the first frame the object is drawn) and the measured
 * BabyPenguin one (a DEP execute-violation in the actor Process frame, whose
 * forensics are in hal/actor_classes_ov072.cpp).
 *
 * The fix is theirs, in port/unmatched/ModelAnim_Renders.cpp: the dispatch is
 * spelled as the qualified ModelAnim::Render, and the matched TU is dropped
 * from the slice (port/slice_w12c.txt) and left byte-locked in src/ as the
 * record of what it was.
 *
 * The shadow passes literal 0 as the scale, so the qualified call passes a null
 * Vector3 * -- the ROM's own argument, and the no-scale path every Model draw
 * without a scale vector takes.
 *
 * NOT here: _ZN13TreasureChest6RenderEv, which is the identical shape one class
 * over. Its class is blocked for a different reason (an unmatched state body,
 * see hal/actor_classes_ov064_w12.cpp's header), so hosting its Render now
 * would be dead code; when that class lands, this file is where its Render
 * belongs.
 */
#include "ModelAnim.h"

extern "C" {

/* PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Fish case. */
int _ZN4Clam6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* ((Base *)&mModelAnim)->m(0) -- the ROM slot-5 Render, spelled qualified */
    ((ModelAnim *)(c + 0xd4))->ModelAnim::Render((const Vector3 *)0);
    return 1;
}

}  /* extern "C" */
