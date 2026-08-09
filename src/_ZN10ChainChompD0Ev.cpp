//cpp
// @symbol _ZN10ChainChompD0Ev
/* recovered: named members + shared header, declarations from a shared header */
#include "decl_Model.h"
#include "decl_ModelAnim.h"
#include "decl_ShadowModel.h"
#include "decl_common.h"
/* recovered: named members + shared header */
#include "ChainChomp.h"
extern "C" {
extern int __destroy_arr(void *p, int a, int b, void *fn);
extern void _ZN25MovingCylinderClsnWithPosD1Ev(void *p);
extern int func_ov002_020aed18(int *x);
/* The teardown vtable pointer is ChainChomp's OWN vtable, _ZTV10ChainChomp at
   0x021147ec in ov014 (config/arm9/overlays/ov014/symbols.txt:189) -- the same
   symbol ChainChomp_Spawn.cpp installs at construction. The decompiler had
   spelled the address under ov034, an overlay that is never co-resident with
   ov014, so at teardown the object's vtable slot took an ov034 address instead
   of the class's real one. Byte-identical on the ROM: the address is unchanged,
   only the overlay tag on the name. */
extern int _ZTV10ChainChomp[];
extern int func_020072c0(void);
extern int data_020a0eac;

void *_ZN10ChainChompD0Ev(struct ChainChomp *self) {
    *(int**)((char *)self) = _ZTV10ChainChomp;
    __destroy_arr(((char *)self) + 0x578, 7, 0xc, (void*)func_020072c0);
    __destroy_arr(((char *)self) + 0x524, 7, 0xc, (void*)func_020072c0);
    __destroy_arr(((char *)self) + 0x40c, 7, 0x28, (void*)_ZN11ShadowModelD1Ev);
    __destroy_arr(((char *)self) + 0x1dc, 7, 0x50, (void*)_ZN5ModelD1Ev);
    _ZN11ShadowModelD1Ev((char *)&self->mShadowModel);
    _ZN9ModelAnimD1Ev((char *)&self->mModelAnim);
    _ZN25MovingCylinderClsnWithPosD1Ev((char *)&self->mMovingCylinderClsnWithPos);
    func_ov002_020aed18((int*)((char *)self));
    _ZN6Memory10DeallocateEPvP4Heap(((char *)self), *(void**)&data_020a0eac);
    return ((char *)self);
}
}
