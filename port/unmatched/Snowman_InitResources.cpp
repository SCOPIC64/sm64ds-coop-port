/* HOST COPY of src/func_ov072_02120a44.c -- the TextureSequence::Prepare
 * calling-convention seam (the SHORT-1 argsweep row
 * __ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File), applied to
 * daBgSnwmn_c's ("SNOWMAN", actor id 272 -- NOT SnowmanBody or
 * SnowmanHead, see port/ov072_syms.txt's own header for the class-
 * identity derivation) InitResources.
 *
 * THE BUG, SAME SHAPE AS MOTHERPENGUIN'S (gate 191, port/unmatched/
 * MotherPenguin_InitResources.cpp). TextureSequence::Prepare is a real
 * non-static C++ method, Prepare(BMD_File &model, BTP_File &animFile),
 * consuming THREE ARM registers at the ABI level: r0=this, r1=&model,
 * r2=&animFile. Its ROM body is a tail-call veneer that does not touch
 * r0-r2, so whatever the caller already has loaded rides straight
 * through. The matched src/func_ov072_02120a44.c calls it as a
 * TWO-argument free function --
 *
 *   _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(
 *       (void *)data_ov072_02122c48[1], (void *)data_ov072_02122c50[1]);
 *
 * -- decl=2/push=2, consumed=3, the SHORT-1 row. On the DS this is byte-
 * identical (the enclosing InitResources's own `this` already sits in r0
 * at the call site, model in r1, animFile in r2, regardless of the C
 * declaration's arity). On the host, cdecl passes only the two declared
 * arguments; the real three-argument bridge's `self` parameter then reads
 * whatever the model pointer happens to be, and its own `bmd`/`btp`
 * parameters shift one slot short.
 *
 * THE FIX (MotherPenguin's own empirically-derived shape, re-applied):
 * self=model, bmd=animFile, btp=animFile -- func_02046d50's own matched
 * body (src/func_02046d50.c) only reads r0(arg)/r1(t), and its `struct
 * Tbl` layout matches BTP_File's, not BMD_File's; the model's own name
 * table is what gets searched (arg=model) using names walked out of the
 * animation file's table (t=animFile). This host copy is the matched src
 * line for line otherwise; only the Prepare call gains its real
 * three-argument shape.
 *
 * src/func_ov072_02120a44.c is dropped from slice_gate193.txt in favour
 * of this file; the byte-locked source is unchanged.
 */
#include "decl_common.h"

extern "C" {
extern int IsStarCollectedInLevel(signed char levelID, int starID);
extern void _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16as(unsigned int id, unsigned int param, void *pos, void *ang, int a, int b);
extern void _ZN9ActorBase18MarkForDestructionEv(void *self);
extern void *_ZN5Model8LoadFileER13SharedFilePtr(void *f);
extern void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *f, int a, int b);
extern void _ZN15TextureSequence8LoadFileER13SharedFilePtr(void *f);
/* the real three-argument bridge (hal/player_bridges.cpp): self, model, animFile */
extern void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void *self, void *bmd, void *btp);
extern void _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(void *self, void *btp, int a, int fix, unsigned int u);
extern int _ZN11ShadowModel12InitCylinderEv(void *self);
extern void _ZN25MovingCylinderClsnWithPos4InitEP5ActorRK7Vector35Fix12IiES6_jj(void *self, void *act, void *pos, int f1, int f2, unsigned int u1, unsigned int u2);
extern void _ZN13RaycastGroundC1Ev(void *self);
extern void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(void *self, void *pos, void *act);
extern int _ZN13RaycastGround10DetectClsnEv(void *self);
extern void _ZN13RaycastGroundD1Ev(void *self);
extern void func_ov072_021208d8(void *c);

extern int data_ov072_02122c70[];
extern int data_ov072_02122c48[];
extern int data_ov072_02122c50[];
extern int data_ov072_02122c40[];

/* PORT_HOST_ABI: TextureSequence::Prepare calling-convention seam (the
   SHORT-1 argsweep row, MotherPenguin's own shape re-applied); cdecl needs
   the third argument spelled explicitly. */
int func_ov072_02120a44(char *c)
{
    char rg[0x50];
    int v[3];
    void *m;

    if (IsStarCollectedInLevel(0xa, 5) == 0) {
        _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16as(0x111, 0, c + 0x5c, c + 0x8c,
            *(signed char *)(c + 0xcc), -1);
        _ZN9ActorBase18MarkForDestructionEv(c);
    }

    m = _ZN5Model8LoadFileER13SharedFilePtr(data_ov072_02122c48);
    _ZN9ModelBase7SetFileEP8BMD_Fileii(c + 0xd4, m, 1, 1);
    m = _ZN5Model8LoadFileER13SharedFilePtr(data_ov072_02122c40);
    _ZN9ModelBase7SetFileEP8BMD_Fileii(c + 0x124, m, 1, 1);

    _ZN15TextureSequence8LoadFileER13SharedFilePtr(data_ov072_02122c50);
    _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File((void *)data_ov072_02122c48[1], (void *)data_ov072_02122c50[1], (void *)data_ov072_02122c50[1]);
    _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(c + 0x174, (void *)data_ov072_02122c50[1], 0, 0x1000, 0);

    if (_ZN11ShadowModel12InitCylinderEv(c + 0x188) == 0)
        return 0;

    _ZN25MovingCylinderClsnWithPos4InitEP5ActorRK7Vector35Fix12IiES6_jj(
        c + 0x1b0, c, data_ov072_02122c70, 0xc3000, 0x17c000, 0x800004, 0);

    v[0] = *(int *)(c + 0x5c);
    v[1] = *(int *)(c + 0x60);
    v[2] = *(int *)(c + 0x64);
    v[1] += 0x14000;
    _ZN13RaycastGroundC1Ev(rg);
    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(rg, v, 0);
    if (_ZN13RaycastGround10DetectClsnEv(rg))
        *(int *)(c + 0x60) = *(int *)(rg + 0x44);
    else
        *(int *)(c + 0x60) = v[1];
    (*(int *)(c + 0x60)) += 0xc3000;
    *(int *)(c + 0x9c) = 0;
    *(int *)(c + 0xa0) = 0;
    *(int *)(c + 0x80) = 0x1800;
    *(int *)(c + 0x84) = 0x1800;
    *(int *)(c + 0x88) = 0x1800;
    func_ov072_021208d8(c);
    _ZN13RaycastGroundD1Ev(rg);
    return 1;
}
}
