/* HOST COPY of src/_ZN11BabyPenguin13InitResourcesEv.cpp -- the
 * func_ov072_02121d50 calling-convention seam (a freshly-found SHORT-1
 * bug, not in Brain A's argsweep FINAL_TABLE.csv/FINAL_hits.csv).
 *
 * THE BUG. func_ov072_02121d50's real matched signature is
 * `func_ov072_02121d50(char *c, int i)` -- it installs a pointer to
 * state cell `i` (data_ov072_02122d6c + i*0x10) at self+0x35c. The
 * matched src here calls it with only ONE argument --
 * `func_ov072_02121d50(((char *)this))` -- decl=1/push=1, consumed=2. On
 * the DS this rides byte-identical because the ARM call site still had
 * `i` sitting in r1 from whatever the compiler last set it to; on the
 * host, cdecl passes only the declared argument and `i` reads stack
 * garbage.
 *
 * THE FIX: i=0. Immediately before this call, the matched src sets
 * `unk_360 = 0` -- the SAME "reset to idle" pattern every OTHER call
 * site that explicitly passes i=0 also carries right alongside it
 * (func_ov072_021212c0.cpp, func_ov072_021218dc.cpp -- both zero a
 * tracking field then transition to state 0). func_ov072_021217ac.cpp
 * (port/unmatched/BabyPenguin_LandTransition.cpp) has the identical
 * one-argument call shape with the identical `*(int*)(c+0x360) = 0`
 * pairing, reinforcing the same reading.
 *
 * This host copy is the matched src line for line otherwise; only the
 * func_ov072_02121d50 call gains its real second argument. src/
 * _ZN11BabyPenguin13InitResourcesEv.cpp is dropped from slice_gate193.txt
 * in favour of this file; the byte-locked source is unchanged.
 *
 * D1 NOTE (unrelated to this fix, carried from the matched src's own
 * situation): BabyPenguin.h's real method faces (InitResources/Behavior/
 * Render) are declared there; this file defines BabyPenguin::InitResources
 * against that header exactly as the matched src did, so hal/
 * actor_classes_ov072.cpp's own `_ZN11BabyPenguin13InitResourcesEv` face
 * thunk (`((BabyPenguin*)self)->BabyPenguin::InitResources()`) reaches
 * this definition unchanged.
 */
#include "decl_common.h"
#include "BabyPenguin.h"

extern "C" {
void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp);
void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *thiz, void *file, int a, int b);
void _ZN9Animation8LoadFileER13SharedFilePtr(void *fp);
int _ZN11ShadowModel12InitCylinderEv(void *thiz);
void _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(void *thiz, void *actor, int r, int h, unsigned int a, unsigned int b);
void _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(void *thiz, void *actor, int r, int h, void *v, int b);
void func_ov072_02121d50(void *c, int i);
void func_ov072_021210c4(void *c);
}

int BabyPenguin::InitResources()
{
    void *f = _ZN5Model8LoadFileER13SharedFilePtr(&data_ov072_02122cb4);
    _ZN9ModelBase7SetFileEP8BMD_Fileii(((char *)this) + 0xd4, f, 1, -1);
    int i;
    for (i = 0; i < 5; i++) {
        _ZN9Animation8LoadFileER13SharedFilePtr((void *)data_ov072_02122004[i]);
    }
    if (_ZN11ShadowModel12InitCylinderEv((char *)&mShadowModel) == 0) return 0;
    _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(((char *)this) + 0x160, ((char *)this), 0x28000, 0x50000, 0x800004, 0x9000);
    unk_09c = -0x2000;
    unk_0a0 = -0x3c000;
    unk_350 = mPosX;
    unk_354 = mPosY;
    unk_358 = mPosZ;
    mScaleX = 0x400;
    mScaleY = 0x400;
    mScaleZ = 0x400;
    _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(((char *)this) + 0x194, ((char *)this), 0x32000, 0x32000, 0, 0);
    mEatingPlayer = 0;
    unk_360 = 0;
    func_ov072_02121d50(((char *)this), 0);
    func_ov072_021210c4(((char *)this));
    return 1;
}
