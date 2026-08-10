/* AUTO-GENERATED from matched-function evidence by tools/gen_header.py
 * class BabyPenguin: 5 matched functions, 19 evidenced fields.
 * Offsets/widths are observed, not guessed. Gaps are explicit padding.
 * Field NAMES are placeholders - renaming cannot change codegen. */
#ifndef BABYPENGUIN_H
#define BABYPENGUIN_H
#include "types.h"
#include "ModelAnim.h"
#include "ShadowModel.h"
#include "MovingCylinderClsn.h"
#include "WithMeshClsn.h"

struct BabyPenguin {
    u8  pad_000[0x5c];
    s32 mPosX;            /* 0x05c */
    s32 mPosY;            /* 0x060 */
    s32 mPosZ;            /* 0x064 */
    u8  pad_068[0x18];
    s32 mScaleX;            /* 0x080 */
    s32 mScaleY;            /* 0x084 */
    s32 mScaleZ;            /* 0x088 */
    u8  pad_08c[0x10];
    s32 unk_09c;            /* 0x09c */
    s32 unk_0a0;            /* 0x0a0 */
    u8  pad_0a4[0x2c];
    s32 mEatingPlayer;            /* 0x0d0 */
    /* ModelAnim member, named by _ZN9ModelAnimD1Ev at +0xd4 -- a relocation the ROM build checks.
       D1 and not D2, so it is this type and not an inlined base. Was a u8 marker. */
    ModelAnim mModelAnim;            /* 0x0d4 */
    /* ShadowModel member, named by the class's own destructor calling
       ShadowModel's D1 at +0x138 -- a relocation the ROM build
       checks. Was a u8 marker. [_ZN11BabyPenguinD0Ev.c] */
    ShadowModel mShadowModel;            /* 0x138 */
    /* MovingCylinderClsn member, named by the class's own destructor calling
       MovingCylinderClsn's D1 at +0x160 -- a relocation the ROM build
       checks. Was a u8 marker. [_ZN11BabyPenguinD0Ev.c] */
    MovingCylinderClsn mMovingCylinderClsn;            /* 0x160 */
    /* WithMeshClsn member, named by the class's own destructor calling
       WithMeshClsn's D1 at +0x194 -- a relocation the ROM build
       checks. Was a u8 marker. [_ZN11BabyPenguinD0Ev.c] */
    WithMeshClsn mWithMeshClsn;            /* 0x194 */
    s32 unk_350;            /* 0x350 */
    s32 unk_354;            /* 0x354 */
    s32 unk_358;            /* 0x358 */
    u8  pad_35c[0x4];
    s32 unk_360;            /* 0x360 */
    s32 unk_364;            /* 0x364 */
    u8  pad_368[0x4];
    s16 unk_36c;            /* 0x36c */
#ifdef __cplusplus
    /* methods */
    int Behavior();
    int InitResources();
    int Render();
#endif
};

#endif
