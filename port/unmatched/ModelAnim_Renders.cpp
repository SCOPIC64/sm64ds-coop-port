/* HOST COPIES of src/_ZN9Butterfly6RenderEv.cpp, src/_ZN4Fish6RenderEv.cpp
 * and src/_ZN13QuestionBlock6RenderEv.cpp -- the ModelAnim slot-5 collision,
 * which gates 17 and 18 already wrote down and gates 21 and 23 walked into
 * again.
 *
 * hal/cxxname_bridge.cpp fills _ZTV9ModelAnim in MSVC order (dtor 0,
 * DoSetFile 1, UpdateVerts 2, Virtual10 3, Render 4, Virtual18 5), because
 * MSVC spends ONE slot on the destructor where Itanium spends two, and its
 * own comment says the rest out loud: "No dual-fill here: Render's ROM slot
 * (5) is Virtual18's MSVC slot, so shadow-TU Render dispatch cannot be served
 * by the same array." _ZTV5Model CAN be dual-filled and is (Render sits in
 * both slot 4 and slot 5); ModelAnim cannot, because something real already
 * occupies the slot.
 *
 * All three of these TUs dispatch through a LOCAL SHADOW CLASS with six
 * virtuals, which counts in the ROM's numbering, so their "slot 5" is Render
 * and the host array's slot 5 is Virtual18 -- which takes TWO arguments where
 * the shadow passes one, so it read its scale off the stack and then handed
 * Model::Virtual10 whatever that was.
 *
 * MEASURED, not reasoned: with Butterfly::Render compiled from src/ the walk
 * faulted c0000005 in Model::Virtual10 the first time a butterfly was close
 * enough to draw, through ModelAnim::Virtual18 -> ModelAnim::Virtual10 ->
 * Model::Virtual10 with a null matrix. It only showed up under a spawn
 * override because the default walk never brings the camera near one.
 *
 * Each body below is the matched source's control flow line for line; only
 * the dispatches are spelled as the qualified methods the ROM means.
 */
#include "Model.h"
#include "ModelAnim.h"

extern "C" {

/* ---- BUTTERFLY (actor 336, ov100) ----------------------------------------
   State 4 is the dormant one and does not draw. Past that it is the animated
   body while +0x3f1 is set and the plain wing Model at +0x138 otherwise, and
   the second one carries the actor's own scale Vector3 at +0x80. */
int _ZN9Butterfly6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(int *)(c + 0x3e4) == 4)
        return 1;
    if (*(unsigned char *)(c + 0x3f1) != 0)
        /* ((Base *)&mModelAnim)->m(0) */
        ((ModelAnim *)(c + 0xd4))->ModelAnim::Render(0);
    else
        /* ((Base2 *)&mModel)->m(&mScale) -- Model's slot 5, the dual-filled
           one, spelled qualified anyway so the file reads the same way twice */
        ((Model *)(c + 0x138))->Model::Render((const Vector3 *)(c + 0x80));
    return 1;
}

/* ---- FISH (actor 344, ov100) ---------------------------------------------
   One early out -- +0x159 is the flag its own spawner state sets while the
   shoal has not hatched -- and the draw. */
int _ZN4Fish6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(unsigned char *)(c + 0x159) == 0)
        /* ((Base *)&mModelAnim)->m(0) */
        ((ModelAnim *)(c + 0xd4))->ModelAnim::Render(0);
    return 1;
}

/* ---- QUESTION_BLOCK (actor 20, ov102) ------------------------------------
   State 2 is the used-up block and does not draw. The animated ModelAnim at
   +0x320 is the question-mark block proper (actor id 0x14) while the save's
   second word does not carry the top bit; every other block in the class --
   the exclamation and cap blocks -- is the plain Model at +0xd4, drawn at the
   actor's own scale. */
extern int data_0209caa0;

int _ZN13QuestionBlock6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    if (*(int *)(c + 0x3e8) == 2)
        return 1;
    if ((*(int *)((char *)&data_0209caa0 + 4) & 0x80000000) == 0) {
        if (*(unsigned short *)(c + 0xc) == 0x14) {
            /* ((Sub *)&mModelAnim)->m(0) */
            ((ModelAnim *)(c + 0x320))->ModelAnim::Render(0);
            return 1;
        }
    }
    /* ((Sub *)&mModel)->m(&mScaleX) */
    ((Model *)(c + 0xd4))->Model::Render((const Vector3 *)(c + 0x80));
    return 1;
}

}  /* extern "C" */
