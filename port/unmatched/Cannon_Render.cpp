/* HOST COPY of src/_ZN6Cannon6RenderEv.cpp -- the Model vtable's slot 3, the
 * one the dual fill cannot cover.
 *
 * hal/cxxname_bridge.cpp fills _ZTV5Model in MSVC order (dtor 0, DoSetFile 1,
 * UpdateVerts 2, Virtual10 3, Render 4) and ALSO writes Render into slot 5,
 * with the reason written next to it: a TU that dispatches through a LOCAL
 * SHADOW CLASS counts in ROM/Itanium numbering, where two slots go to the
 * destructor, so its slot 5 is Render. Cannon::Render is the first TU to
 * reach slot THREE the same way -- ROM 3 is UpdateVerts, MSVC 3 is
 * Virtual10 -- and that one cannot be dual-filled, because Virtual10 takes a
 * Matrix4x3 the shadow's `virtual void m3()` never passes.
 *
 * The body is the matched source's line for line; only the two dispatches are
 * spelled as the qualified Model methods the ROM means.
 */
#include "Model.h"

extern "C" int _ZN6Cannon6RenderEv(void *selfv)
{
    char *c = (char *)selfv;
    /* the lid's own animation state: past step 3 of type 3 it stops drawing */
    if (*(int *)(c + 0x180) == 3 && *(unsigned char *)(c + 0x185) >= 3)
        return 1;
    Model *m = (Model *)(c + 0xd4);
    m->Model::UpdateVerts();                  /* ((Sub *)&mModel)->m3()  */
    m->Model::Render(0);                      /* ((Sub *)&mModel)->m5(0) */
    return 1;
}
