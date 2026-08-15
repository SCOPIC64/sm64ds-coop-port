/* RUN LINKW WAVE 19 (lane w19): THE ModelAnim SLOT-5 SWEEP.
 *
 * HOST COPIES of src/_ZN6Snufit6RenderEv.cpp, src/_ZN5Swoop6RenderEv.cpp,
 * src/_ZN6Dorrie6RenderEv.cpp and src/_ZN9MontyMole6RenderEv.cpp -- the four
 * matched Render bodies that were LINKED AND LATENT in the shipping binary
 * with the ModelAnim slot-5 collision in them.
 *
 * ============================================================================
 * THE BUG, and why a green battery could not see it
 * ============================================================================
 *
 * hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in MSVC order -- dtor 0,
 * DoSetFile 1, UpdateVerts 2, Virtual10 3, Render 4, Virtual18 5 -- because
 * MSVC spends ONE slot on the destructor where Itanium spends two. The ROM's
 * own numbering therefore sits one slot higher from Virtual10 on, and
 * include/ModelAnim.h says so in its header block: "slot 5 0x020167f8
 * Render(Vector3 const *)".
 *
 * Each of these four TUs declares a LOCAL SIX-VIRTUAL SHADOW over a ModelAnim
 * member and dispatches the sixth virtual. That is the ROM's slot 5, which is
 * Render; the host array's slot 5 is Virtual18, a TWO-argument method that
 * reads its scale off the stack. _ZTV5Model can be dual-filled and is;
 * _ZTV9ModelAnim cannot, because Virtual18 really is there.
 *
 * This is the same bug that shipped as half of a real player crash: Amp and
 * FlameChomp faulted in exactly this way the moment their Behavior was fixed
 * (port/unmatched/Ov070_PmfDispatch.cpp, "BUG TWO"), and it had been hiding
 * behind a jump-to-zero.
 *
 * MEASURED, per class, against THIS binary before the change. Level 13 (Hazy
 * Maze Cave) places all four, and its 300-frame selftest is green -- so the
 * fault was reached by spawning each class at the player rather than by
 * waiting for the walk to find one:
 *
 *   SM64DS_LEVEL=13 SM64DS_FAULTS_FATAL=1 SM64DS_SPAWN_ACTOR=<id>
 *
 *   FAULT c0000005 accessing 00000000, all four identical above the leaf:
 *     Model::Virtual10      +0xc
 *       <- ModelAnim::Virtual10 +0x25
 *       <- ModelAnim::Virtual18 +0xe        <- THE COLLISION
 *       <- Snufit::Render     +0x1d   (id 236)
 *          Swoop::Render      +0x3b   (id 237)
 *          Dorrie::Render     +0x11   (id 168)
 *          MontyMole::Render  +0x11   (id 310)
 *
 * signature for signature the Amp/FlameChomp/Butterfly fault. A control spawn
 * of MR_I (262) on the same level and the same frame budget exits 0, so the
 * fault is the class's own and not the debug-spawn path's.
 *
 * WHY THE BATTERY IS GREEN ANYWAY, and why that is not reassurance. With
 * SM64DS_ACTOR_PROBE=1 only SEVEN of level 13's 42 registered classes reach
 * their Render in 300 frames (DOOR, QUESTION_BLOCK, CRATE, SCUTTLEBUG,
 * SIGN_POST and COIN twice). None of these four is among them: the render
 * pass is camera-gated and the selftest never brings the camera near one.
 * Every one of them is a crash waiting for a player to walk somewhere the
 * selftest does not go. Level 13's census is 188/0 before and after.
 *
 * ============================================================================
 * THE COST, stated plainly because it is a trade and not a free fix
 * ============================================================================
 *
 * Each host copy costs the matched Render TU. src/_ZN6Snufit6RenderEv.cpp and
 * its three siblings are C++ METHODS whose only reference was the face in the
 * class's vtable fill, so routing the fill past that face leaves the method
 * unreferenced and /OPT:REF strips it: linkage -4. That trade has been made
 * three times already -- Butterfly/Fish/QuestionBlock in
 * unmatched/ModelAnim_Renders.cpp, Whomp in the same file, Amp and FlameChomp
 * in unmatched/Ov070_PmfDispatch.cpp -- and it is correct: there is no way to
 * keep a body linked and also not call it, and a linked body that faults is
 * worth less than an unlinked one that does not.
 *
 * The real fix is a ModelAnim vtable the shadow TUs can dispatch through in
 * ROM order. That is a bigger change than a lane, and until it exists this is
 * the remedy.
 *
 * Each body below is the matched source's control flow line for line; only
 * the dispatch is spelled as the qualified method the ROM means.
 */
#include "Model.h"
#include "ModelAnim.h"

extern "C" {

/* ---- SNUFIT (actor 236, ov065, daYurei_Mucho_c) ---------------------------
   src/_ZN6Snufit6RenderEv.cpp:
       int b = ((unk_0b0 & 0x40000) != 0);
       if (b) return 1;
       ((Obj *)&mModelAnim)->Target(0);     <- the ROM's slot-5 Render
       return 1;
   unk_0b0 is the actor flag word at 0x0b0 and mModelAnim is at 0x300
   (include/Snufit.h). 0x40000 is the "do not draw" bit.

   PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Butterfly
   ruling applied to SNUFIT. */
int port_w19_snufit_render(void *selfv)
{
    char *c = (char *)selfv;
    if ((*(unsigned int *)(c + 0x0b0) & 0x40000) != 0)
        return 1;
    ((ModelAnim *)(c + 0x300))->ModelAnim::Render(0);
    return 1;
}

/* ---- SWOOP (actor 237, ov065, daBasabasa_c) -------------------------------
   src/_ZN5Swoop6RenderEv.cpp, the same draw guard and then a two-way pick
   between TWO ModelAnims -- 0x300 and 0x364 (include/Swoop.h names them
   mModelAnim1/mModelAnim2 and both carry the same 0x5c span, which is what
   says the second one is a ModelAnim and not the shadow Model its neighbour
   class keeps at that offset). unk_43c is the u8 selector at 0x43c.

   BOTH branches dispatch slot 5, so both are the collision; the fault
   captured above came through the 0x300 branch because unk_43c is 1 at spawn.

   PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch. */
int port_w19_swoop_render(void *selfv)
{
    char *c = (char *)selfv;
    if ((*(int *)(c + 0x0b0) & 0x40000) != 0)
        return 1;
    if (*(unsigned char *)(c + 0x43c) == 1)
        ((ModelAnim *)(c + 0x300))->ModelAnim::Render(0);
    else
        ((ModelAnim *)(c + 0x364))->ModelAnim::Render(0);
    return 1;
}

/* ---- DORRIE (actor 168, ov065, daDossy_c) ---------------------------------
   src/_ZN6Dorrie6RenderEv.cpp is one dispatch and a return:
       struct Derived { char pad[0xec]; Base base; };
       Base *b = &((Derived *)this)->base; b->m(0); return 1;
   The pad pins the offset at 0xec, which include/Dorrie.h names mModelAnim.
   No draw guard: Dorrie's own Behavior owns its visibility.

   PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch. */
int port_w19_dorrie_render(void *selfv)
{
    ((ModelAnim *)((char *)selfv + 0xec))->ModelAnim::Render(0);
    return 1;
}

/* ---- MONTY_MOLE (actor 310, ov080) ----------------------------------------
   src/_ZN9MontyMole6RenderEv.cpp, the identical one-dispatch shape at 0xd4.

   NOTE FOR WHOEVER READS THE MAP: MontyMole::Render and Trap::Render are
   byte-identical bodies and MSVC FOLDS THEM (both names sit at 0x0043ec80 in
   walk_window.map). The fault stack therefore prints whichever name the map
   resolves first, which is Trap's -- the code that ran was MontyMole's. Trap
   is NOT hosted here: its 0xd4 is a real Model (hal/actor_classes.cpp's own
   slot-16 note says so, and TRAP's render probe reads 0x320, a different
   object), and _ZTV5Model IS dual-filled, so Trap::Render is correct as
   written. Reading the folded name as evidence about Trap would have cost a
   linked TU for nothing.

   PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch. */
int port_w19_montymole_render(void *selfv)
{
    ((ModelAnim *)((char *)selfv + 0xd4))->ModelAnim::Render(0);
    return 1;
}

}
