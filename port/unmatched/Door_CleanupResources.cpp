/* HOST COPY of src/func_ov100_0214542c.c (daDoor_c::CleanupResources) -- the
 * MSVC DESTRUCTOR SLOT SHIFT, caught by the first level teardown.
 *
 * The matched TU deletes the door's second Model through a LOCAL SHADOW CLASS:
 *
 *     struct V { virtual void v0(); virtual void v1(); };
 *     obj = (V *)*(void **)(c + 0x138);
 *     if (obj) obj->v1();
 *
 * v1 is ROM/Itanium slot 1, which on the DS is the DELETING destructor (mwcc
 * emits D1 at slot 0 and D0 at slot 1). MSVC folds the two into one slot, so
 * hal/cxxname_bridge.cpp's _ZTV5Model is one short from there on and slot 1 is
 * Model::DoSetFile. The call landed on DoSetFile with the argument registers
 * holding nothing, walked into Model::AddToCommonModelDataArr and faulted on a
 * null BMD -- the first thing a level change ever did.
 *
 * The array cannot serve both readings at once: it is MSVC-ordered throughout
 * and already double-fills slot 5 so that shadow-TU Render dispatch works
 * (see the note there). Every other slot a shadow TU reaches has to be spelled
 * by the consumer, which is what this file does. `delete obj` is written as
 * what the ROM's D0 is -- the D1 body, then operator delete -- so nothing
 * dispatches through the ambiguous slot at all.
 *
 * NOTHING ELSE CHANGES: the two SharedFilePtr releases before it, the third
 * behind the same null test, and the key-model tail are the matched source's,
 * in its order.
 */
#include "decl_common.h"
#include "SharedFilePtr.h"

struct PortDoorElem { void *sfp1; void *sfp2; char pad[8]; };

extern "C" {
/* Inside the extern "C" block, unlike the matched TU's own spelling: the
   overlay mount emits this as a plain C symbol and the door's other three
   TUs reach it that way. */
extern PortDoorElem data_ov100_02148204[];
extern void *data_ov100_02148744;
void port_model_family_delete(void *obj);

// PORT_HOST_ABI: mwcc virtual-shadow dispatch (MSVC thiscall vs the slice's cdecl).
int func_ov100_0214542c(char *c)
{
    int idx = *(int *)(c + 8);
    PortDoorElem *e = &data_ov100_02148204[idx];
    void *obj;

    ((SharedFilePtr *)(e->sfp1))->Release();
    ((SharedFilePtr *)(&data_ov100_02148744))->Release();
    obj = *(void **)(c + 0x138);
    if (obj != 0) {
        /* the ROM's `obj->v1()`: the model-family deleting destructor,
           picked by the object's own vtable (hal/cxxname_bridge.cpp) */
        port_model_family_delete(obj);
        *(void **)(c + 0x138) = 0;
        ((SharedFilePtr *)(e->sfp2))->Release();
    }
    if (*(void **)(c + 0x13c) != 0) {
        unsigned int v = *(unsigned int *)(c + 8);
        if (v >= 9 && v <= 0xd)
            UnloadKeyModels(v - 7);
        ((SharedFilePtr *)(*(void **)(c + 0x13c)))->Release();
    }
    return 1;
}
}
