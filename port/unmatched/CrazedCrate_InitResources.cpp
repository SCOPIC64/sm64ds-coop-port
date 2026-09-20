/* HOST MIRROR of the matched src/_ZN11CrazedCrate13InitResourcesEv.cpp with
 * exactly ONE call fixed: the recovered source calls the state setter
 * func_ov080_0212513c with the second argument DROPPED -- the r1-passthrough
 * flavor of the ClosestPlayer receiver seam. The ROM's own call site
 * (0x021252dc: `mov r1, #0; mov r0, r4; str r1, [r4, #0x374]; bl 0x212513c`)
 * enters state 0; on the host the missing argument read stack garbage, the
 * setter indexed the three-state table at garbage*16, and the dispatcher
 * jumped through junk (the measured level-12 boot fault at eax=0x1400f407,
 * CrazedCrate::InitResources+0xff). The explicit 0 below is the ROM's value,
 * not a guess. Nothing else differs -- diff against the src TU.
 */
//cpp
// @symbol _ZN11CrazedCrate13InitResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "CrazedCrate.h"
struct RaycastGround { char buf0[0x14]; int floor[12]; char buf1[0x50-0x14-0x30]; };

extern "C" void* _ZN5Model8LoadFileER13SharedFilePtr(void* fp);
extern "C" void _ZN9ModelBase7SetFileEP8BMD_Fileii(void* self, void* file, int a, int b);
extern "C" void _ZN11ShadowModel10InitCuboidEv(void* self);
extern "C" void _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(void* self, void* actor, int a, int b, unsigned int c, unsigned int d);
extern "C" void _ZN13RaycastGroundC1Ev(RaycastGround* self);
extern "C" void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(RaycastGround* self, const Vector3& v, void* actor);
extern "C" int _ZN13RaycastGround10DetectClsnEv(RaycastGround* self);
extern "C" void _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(void* self, void* actor, int a, int b, void* v, int c);
extern "C" void func_ov080_02124c3c(void* self);
extern "C" void _ZN13RaycastGroundD1Ev(RaycastGround* self);


int CrazedCrate::InitResources()
{
    RaycastGround rc;
    Vector3 pos;
    void* file = _ZN5Model8LoadFileER13SharedFilePtr(&data_ov080_02128468);
    _ZN9ModelBase7SetFileEP8BMD_Fileii(((char*)this) + 0xd4, file, 1, 1);
    _ZN11ShadowModel10InitCuboidEv((char*)&mShadowModel);
    _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(((char*)this) + 0x14c, ((char*)this), 0x64000, 0x78000, 0x800004, 0x9010);
    unk_09c = -0x2000;
    unk_0a0 = -0x3c000;
    {
        int p60;
        pos.x = mPosX;
        p60 = mPosY;
        pos.y = p60;
        pos.z = mPosZ;
        pos.y = p60 + 0xc8000;
    }
    _ZN13RaycastGroundC1Ev(&rc);
    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(&rc, pos, 0);
    if (_ZN13RaycastGround10DetectClsnEv(&rc))
        mPosY = rc.floor[(0x44 - 0x14) / 4];
    else
        mPosY = pos.y;
    mScaleX = 0x1000;
    mScaleY = 0x1000;
    mScaleZ = 0x1000;
    _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(((char*)this) + 0x180, ((char*)this), 0x32000, 0x32000, 0, 0);
    unk_374 = 0;
    ((void (*)(char *, int))func_ov080_0212513c)((char *)this, 0); /* host edit:
        the ROM's r1=0 (see banner). Cast call: the shared decl_common.h
        declares the setter one-arg and cannot change without breaking the
        matched callers under mwcc; cdecl caller-clean makes the extra
        argument safe through the cast. */
    func_ov080_02124c3c(((char*)this));
    _ZN13RaycastGroundD1Ev(&rc);
    return 1;
}
