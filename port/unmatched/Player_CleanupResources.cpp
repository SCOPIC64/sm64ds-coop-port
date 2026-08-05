/* HOST COPY of src/_ZN6Player16CleanupResourcesEv.cpp -- the Player's own
 * teardown, with the five declarations that collide with include/decl_common.h
 * taken out.
 *
 * WHY IT IS HERE AND NOT IN A SLICE. The matched TU opens an `extern "C"`
 * block that re-declares five functions decl_common.h has already declared:
 *
 *     TU              void func_ov002_020bdd2c(char *c);
 *     decl_common.h   extern void func_ov002_020bdd2c(void*);
 *
 * On mwcc that is one declaration seen twice with a compatible pointer type.
 * MSVC calls it an overload of an extern "C" function and refuses the TU
 * (C2733 x5, plus one C2664 where func_02073244's fourth parameter is
 * `void (*)(void*)` in the header and `void (*)(void)` in the TU). Neither
 * spelling is wrong about the ROM; they simply disagree, and src/ and
 * include/ are the byte-matched tree that the port does not edit.
 *
 * NOTHING ELSE CHANGES. The body below is the matched source's, statement for
 * statement: the four sub-object cleanups, the four-slot model/collider walk,
 * the ninety-odd SharedFilePtr::Release calls in their original order, and
 * the mLoadedResourceFlags tail. The only edits are the removed declarations
 * and the two casts that reconcile the header's prototypes.
 *
 *
 * SECOND EDIT, and it is the one that matters: the five `p->v1()` calls.
 * The matched TU deletes the body models, the head models and two more
 * model-family objects through a two-virtual SHADOW CLASS, so v1 is ROM slot
 * 1 -- the deleting destructor mwcc puts there. MSVC folds D1 and D0 into one
 * slot, so hal/cxxname_bridge.cpp's _ZTV5Model has DoSetFile at 1, and every
 * one of those five calls landed on Model::DoSetFile and faulted inside
 * Model::AddToCommonModelDataArr on the first level teardown. They are spelled
 * as port_model_family_delete now, which picks the destructor by the object's
 * own vtable pointer (same file) and then frees it, which is what the ROM's D0
 * is. Nothing dispatches through the ambiguous slot.
 *
 * WHAT REACHES IT (gate 31): a level change destroys every actor the level
 * spawned, and the Player is one of them. This is what gives his files back --
 * without it every transition would hold another copy of the character's
 * models and animations loaded.
 */
#include "types.h"
// @symbol _ZN6Player16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "Player.h"
#include "SharedFilePtr.h"
enum { false_, true_ };

extern "C" void port_model_family_delete(void *obj);

#pragma opt_common_subs off
#pragma opt_strength_reduction off

extern "C" {
void func_0203cbc0(int p);
void func_020072c0(void);
void func_ov002_020bebd4(char *c);
void UnloadSilverStarAndNumber(void);
void UnloadKeyModels(int n);
extern void *data_ov002_020ff480[];
extern u8 data_0209f2d8;
extern char data_ov002_0210d9a0[];
extern char data_ov002_0210d9a8[];
extern char data_ov002_0210d9c0[];
extern char data_ov002_0210da38[];
extern char data_ov002_0210da40[];
extern char data_ov002_0210e3d8[];
extern char data_ov002_0210e750[];
extern char data_ov002_0210e8d0[];
extern char data_ov002_0210eb20[];
extern char data_ov002_0210ebb8[];
extern char data_ov002_0210ebd8[];
extern char data_ov002_02110aa4[];
}

int Player::CleanupResources()
{
    int i;
    u32 b;

    func_ov002_020bdd2c((void *)this);
    func_ov002_020bdef0((void *)this);
    func_ov002_020bdd9c((void *)this);
    func_ov002_020e032c((void *)this);
    for (i = 0; i < 4; i++) {
        int j;
        port_model_family_delete(*(void **)(((char *)this) + i * 4 + 0xdc));
        port_model_family_delete(*(void **)(((char *)this) + i * 4 + 0x154));
        j = i + 4;
        port_model_family_delete(*(void **)(((char *)this) + j * 4 + 0x154));
        {
            int q = *(int *)(((char *)this) + i * 4 + 0x27c);
            if (q != 0)
                func_0203cbc0(q);
            q = *(int *)(((char *)this) + i * 4 + 0x28c);
            if (q != 0)
                func_0203cbc0(q);
            q = *(int *)(((char *)this) + j * 4 + 0x28c);
            if (q != 0)
                func_0203cbc0(q);
        }
    }
    port_model_family_delete(*(void **)((char *)&unk_0ec));
    port_model_family_delete(*(void **)((char *)&unk_1d8));
    {
        int q = unk_578;
        if (q != 0)
            func_02073244((void *)q, 0xc, 8,
                          (void (*)(void *))func_020072c0);
        q = unk_57c;
        if (q != 0)
            func_0203cbc0(q);
        q = mHeldObjQueue;
        if (q != 0)
            func_0203cbc0(q);
    }
    ((SharedFilePtr *)(data_ov002_020ff480[mCharFileBase + (param1 & 3)]))->Release();
    b = data_0209f2d8;
    b = b == 1;
    if (b != false_)
        func_ov002_020bebd4(((char *)this));
    ((SharedFilePtr *)(data_ov002_0210ec50))->Release();
    ((SharedFilePtr *)(data_ov002_0210ebb8))->Release();
    ((SharedFilePtr *)(data_ov002_0210ec60))->Release();
    ((SharedFilePtr *)(data_ov002_0210e818))->Release();
    ((SharedFilePtr *)(data_ov002_0210e690))->Release();
    ((SharedFilePtr *)(data_ov002_0210eb20))->Release();
    ((SharedFilePtr *)(data_ov002_0210e758))->Release();
    ((SharedFilePtr *)(data_ov002_0210ec38))->Release();
    ((SharedFilePtr *)(data_ov002_0210eb00))->Release();
    ((SharedFilePtr *)(data_ov002_0210e790))->Release();
    ((SharedFilePtr *)(data_ov002_0210e7a8))->Release();
    ((SharedFilePtr *)(data_ov002_0210e7f0))->Release();
    ((SharedFilePtr *)(data_ov002_0210e798))->Release();
    ((SharedFilePtr *)(data_ov002_0210e4f0))->Release();
    ((SharedFilePtr *)(data_ov002_0210e3d8))->Release();
    ((SharedFilePtr *)(data_ov002_0210eb88))->Release();
    ((SharedFilePtr *)(data_ov002_0210e6c0))->Release();
    ((SharedFilePtr *)(data_ov002_0210eb98))->Release();
    ((SharedFilePtr *)(data_ov002_0210e4c0))->Release();
    ((SharedFilePtr *)(data_ov002_0210e500))->Release();
    ((SharedFilePtr *)(data_ov002_0210e1c8))->Release();
    ((SharedFilePtr *)(data_ov002_0210e770))->Release();
    ((SharedFilePtr *)(data_ov002_0210e230))->Release();
    ((SharedFilePtr *)(data_ov002_0210e288))->Release();
    ((SharedFilePtr *)(data_ov002_0210e478))->Release();
    ((SharedFilePtr *)(data_ov002_0210e9d0))->Release();
    ((SharedFilePtr *)(data_ov002_0210ea10))->Release();
    ((SharedFilePtr *)(data_ov002_0210e538))->Release();
    ((SharedFilePtr *)(data_ov002_0210eac8))->Release();
    ((SharedFilePtr *)(data_ov002_0210e898))->Release();
    ((SharedFilePtr *)(data_ov002_0210e9e8))->Release();
    ((SharedFilePtr *)(data_ov002_0210eba8))->Release();
    ((SharedFilePtr *)(data_ov002_0210eb70))->Release();
    ((SharedFilePtr *)(data_ov002_0210eaf0))->Release();
    ((SharedFilePtr *)(data_ov002_0210e640))->Release();
    ((SharedFilePtr *)(data_ov002_0210e670))->Release();
    ((SharedFilePtr *)(data_ov002_0210e680))->Release();
    ((SharedFilePtr *)(data_ov002_0210e438))->Release();
    ((SharedFilePtr *)(data_ov002_0210e3b0))->Release();
    ((SharedFilePtr *)(data_ov002_0210e3c8))->Release();
    ((SharedFilePtr *)(data_ov002_0210e408))->Release();
    ((SharedFilePtr *)(data_ov002_0210e400))->Release();
    ((SharedFilePtr *)(data_ov002_0210ecb8))->Release();
    ((SharedFilePtr *)(data_ov002_0210e8d0))->Release();
    ((SharedFilePtr *)(data_ov002_0210ebd8))->Release();
    b = data_0209f2d8;
    b = b == 1;
    if (b == false_) {
        ((SharedFilePtr *)(data_ov002_0210d9a8))->Release();
        ((SharedFilePtr *)(data_ov002_02110aa4))->Release();
        ((SharedFilePtr *)(data_ov002_0210da38))->Release();
        ((SharedFilePtr *)(data_ov002_0210e750))->Release();
        ((SharedFilePtr *)(data_ov002_0210e588))->Release();
        ((SharedFilePtr *)(data_ov002_0210e7e8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e450))->Release();
        ((SharedFilePtr *)(data_ov002_0210e788))->Release();
        ((SharedFilePtr *)(data_ov002_0210e1e8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e1e0))->Release();
        ((SharedFilePtr *)(data_ov002_0210e4d0))->Release();
        ((SharedFilePtr *)(data_ov002_0210e4e8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e1d0))->Release();
        ((SharedFilePtr *)(data_ov002_0210e800))->Release();
        ((SharedFilePtr *)(data_ov002_0210e4b8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e270))->Release();
        ((SharedFilePtr *)(data_ov002_0210e458))->Release();
        ((SharedFilePtr *)(data_ov002_0210ea70))->Release();
        ((SharedFilePtr *)(data_ov002_0210e540))->Release();
        ((SharedFilePtr *)(data_ov002_0210e8e8))->Release();
        ((SharedFilePtr *)(data_ov002_0210eca8))->Release();
        ((SharedFilePtr *)(data_ov002_0210ec98))->Release();
        ((SharedFilePtr *)(data_ov002_0210e600))->Release();
        ((SharedFilePtr *)(data_ov002_0210ea88))->Release();
        ((SharedFilePtr *)(data_ov002_0210e910))->Release();
        ((SharedFilePtr *)(data_ov002_0210e460))->Release();
        ((SharedFilePtr *)(data_ov002_0210e2f0))->Release();
        ((SharedFilePtr *)(data_ov002_0210e958))->Release();
        ((SharedFilePtr *)(data_ov002_0210eb38))->Release();
        ((SharedFilePtr *)(data_ov002_0210e660))->Release();
        ((SharedFilePtr *)(data_ov002_0210ea30))->Release();
        ((SharedFilePtr *)(data_ov002_0210e3e8))->Release();
        ((SharedFilePtr *)(data_ov002_0210eb90))->Release();
        ((SharedFilePtr *)(data_ov002_0210e6b8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e620))->Release();
        ((SharedFilePtr *)(data_ov002_0210e6a0))->Release();
        ((SharedFilePtr *)(data_ov002_0210eb58))->Release();
        ((SharedFilePtr *)(data_ov002_0210e3e0))->Release();
        ((SharedFilePtr *)(data_ov002_0210eb10))->Release();
        ((SharedFilePtr *)(data_ov002_0210e708))->Release();
        ((SharedFilePtr *)(data_ov002_0210ebc8))->Release();
        ((SharedFilePtr *)(data_ov002_0210e6e0))->Release();
        ((SharedFilePtr *)(data_ov002_0210e430))->Release();
        ((SharedFilePtr *)(data_ov002_0210ec40))->Release();
        ((SharedFilePtr *)(data_ov002_0210e738))->Release();
        ((SharedFilePtr *)(data_ov002_0210e250))->Release();
        ((SharedFilePtr *)(data_ov002_0210ec10))->Release();
        ((SharedFilePtr *)(data_ov002_0210ec00))->Release();
        ((SharedFilePtr *)(data_ov002_0210e728))->Release();
    }
    if (mLoadedResourceFlags & 1)
        UnloadSilverStarAndNumber();
    if (mLoadedResourceFlags & 2)
        ((SharedFilePtr *)(data_ov002_0210da40))->Release();
    if (mLoadedResourceFlags & 4)
        ((SharedFilePtr *)(data_ov002_0210d9a0))->Release();
    if (mLoadedResourceFlags & 8)
        ((SharedFilePtr *)(data_ov002_0210d9c0))->Release();
    if (mLoadedResourceFlags & 0x10)
        UnloadKeyModels(unk_719);
    b = data_0209f2d8;
    b = b == 1;
    if (b == false_) {
        if (mLoadedResourceFlags & 0x20) {
            ((SharedFilePtr *)(data_ov002_0210e6f8))->Release();
            ((SharedFilePtr *)(data_ov002_0210e7b0))->Release();
            ((SharedFilePtr *)(data_ov002_0210e780))->Release();
        }
    } else {
        ((SharedFilePtr *)(data_ov002_0210e7b0))->Release();
    }
    if (mLoadedResourceFlags & 0x40) {
        u32 idx = mHatCharacter;
        if (idx == (u32)param1)
            idx = unk_6dc;
        ((SharedFilePtr *)(data_ov002_020ff480[mCharFileBase + idx]))->Release();
    }
    return 1;
}
