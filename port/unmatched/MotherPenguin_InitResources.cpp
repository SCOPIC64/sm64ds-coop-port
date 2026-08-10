/* HOST COPY of src/_ZN7SkiLift13InitResourcesEv.cpp -- the SAME
 * TextureSequence::Prepare SHORT-1 calling-convention seam
 * port/unmatched/OneUpLogo_InitResources.cpp fixes for gate 190, applied to
 * a second, independently-broken caller.
 *
 * CLASS IDENTITY NOTE: despite the mangled name _ZN7SkiLift13InitResourcesEv,
 * this method belongs to MOTHER_PENGUIN (257), not the real SkiLift (63).
 * MotherPenguin_Spawn.c installs _ZTV7SkiLift == _ZTV10daPgMthr_c at ov018
 * 0x021139bc (MotherPenguin_SpawnInfo 0x02113998 +0x24); the eight
 * src/_ZN7SkiLift* files are MotherPenguin's own bodies under a dsd-era
 * class-identity mislabel (see port/slice_gate191.txt). The include/SkiLift.h
 * layout matches MotherPenguin's real member set (ModelAnim, TextureSequence,
 * ShadowModel, MovingCylinderClsn, WithMeshClsn at the offsets
 * MotherPenguin_Spawn.c constructs), not the real SkiLift's.
 *
 * THE PREPARE BUG, AND THE REGISTER-LEVEL CORRECTION.
 *
 * TextureSequence::Prepare(BMD_File &model, BTP_File &animFile) tail-calls
 * (a 0xc-byte veneer, ldr r12,[pc]; bx r12, register-preserving) into
 * func_02046d50(void *arg, struct Tbl *t) -- func_02046d50.c's own matched
 * body only reads r0 and r1 in its prologue (mov r6,r1; ldrh r1,[r6,#2];
 * mov r7,r0), so on the ARM side `arg`=r0, `t`=r1, and the C++ call site's
 * third value (&animFile, r2) is dead on entry -- never read.
 *
 * A NAIVE reading says r0=this (Prepare's own this), r1=&model (the first
 * declared parameter) -- and that IS what a straight `this,&model,&animFile`
 * forwarding would produce. It does NOT work: passing this+0x138 (the
 * TextureSequence) as `arg` and &model (the BMD_File) as `t` makes
 * func_02046d50 read struct Tbl {cnt1@2,p1@4,cnt2@8,p2@0xc} fields out of a
 * BMD_File's OWN memory (numBones@4, numTextures@0x14, ...) -- garbage at
 * those offsets (confirmed empirically: a real, valid, well-formed loaded
 * BMD_File -- 8 bones, 5 textures, 2 materials -- reads cnt2 as 0x63bc and
 * p2 as 0x2, and func_02046d50's second loop faults dereferencing p2+4).
 *
 * func_02046d50's job (per its body: walk t->p1[]/p2[]/p3[], strcmp each
 * entry's name against arg's own name table, write the found index back
 * into entry->res) is a NAME RESOLUTION pass: an animation file's texture/
 * palette/sequence records carry NAMES (portable across models), and Prepare
 * resolves each name against the ACTUAL LOADED MODEL's own name table,
 * caching the resolved index in the BTP_File's own records. That means the
 * table being WALKED (`t`) is the ANIMATION file (BTP_File, whose layout --
 * numTexRecords@2, unk_04@4, numPalRecords@8, unk_0c@0xc, see
 * include/TextureSequence.h -- matches struct Tbl exactly), and the table
 * being SEARCHED (`arg`) is the MODEL (BMD_File). Confirmed empirically:
 * this ordering resolves both of MotherPenguin's own TextureSequence loop
 * iterations cleanly (no fault, in either loop, in either iteration) where
 * the naive this/model ordering faults on the very first call.
 *
 * THE HOST SHAPE. The real bridge (hal/player_bridges.cpp) is
 * `_ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void *self, void *bmd,
 * void *btp) { ((TextureSequence*)self)->TextureSequence::Prepare(*bmd,
 * *btp); }`, and the matched TextureSequence::Prepare body itself is
 * `func_02046d50(this, &model, &animFile)` -- so on the HOST, `this` (the
 * bridge's `self`) is what becomes func_02046d50's `arg`, and the bridge's
 * `bmd` argument (Prepare's `model` parameter) is what becomes `t`. To get
 * arg=model and t=animFile, this host copy calls the bridge with the MODEL
 * pointer riding the `self` slot and the ANIMATION FILE pointer riding the
 * `bmd` slot -- an unusual-looking call, but it is what makes the two
 * registers land where func_02046d50 actually reads them. The bridge's
 * third argument (`btp`, Prepare's animFile parameter, becomes r2 at the
 * ARM ABI level) is provably dead inside func_02046d50 (never read past its
 * own prologue), so any valid pointer satisfies the bridge's own cast; the
 * animation file pointer is passed there too for a value that is at least
 * meaningful to a debugger, not because func_02046d50 uses it.
 *
 * Matched source line for line otherwise, INCLUDING the cross-overlay fix
 * already carried from main (c19c90882, #1301: data_ov036_02113c00 ->
 * data_ov018_02113c00, data_ov056_02112c04 -> data_ov018_02112c04,
 * func_ov022_021123d0 -> func_ov018_021123d0 -- dsd-era same-load-window
 * mislabels, ov018/ov022/ov036/ov056 all share base 0x021111a0 and are never
 * co-resident, confirmed via tools/ovsweep.py E2 + delinks.txt ownership).
 * src/_ZN7SkiLift13InitResourcesEv.cpp stays byte-locked and untouched,
 * dropped from slice_gate191.txt in favour of this file.
 */
#include "decl_common.h"
#include "SkiLift.h"

extern "C" {
extern void *_ZN5Model8LoadFileER13SharedFilePtr(void *f);
extern void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *f, int a, int b);
extern void *_ZN9Animation8LoadFileER13SharedFilePtr(void *f);
extern void *_ZN15TextureSequence8LoadFileER13SharedFilePtr(void *f);
/* the real bridge (hal/player_bridges.cpp): self, model(->bmd), animFile(->btp) */
extern void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void *self, void *bmd, void *btp);
extern int _ZN11ShadowModel12InitCylinderEv(void *self);
extern void _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(void *self, void *act, int a, int b, unsigned int c2, unsigned int d);
extern void _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(void *self, void *act, int a, int b, void *c2, void *d);
extern void _ZN13RaycastGroundC1Ev(void *self);
extern void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(void *self, void *pos, void *act);
extern int _ZN13RaycastGround10DetectClsnEv(void *self);
extern void func_ov018_02111d28(char *c, int r1);
extern void _ZN13RaycastGroundD1Ev(void *self);
extern int data_ov018_02113c00[];
extern int data_ov018_02112c04[];
/* data_ov018_02112c0c[] comes from decl_common.h's shared umbrella, the
   same reading the matched src file (which never declares it locally
   either) relies on. */
extern void func_ov018_021123d0(char *self, int a);
}

int SkiLift::InitResources()
{
    void *m = _ZN5Model8LoadFileER13SharedFilePtr(data_ov018_02113c00);
    _ZN9ModelBase7SetFileEP8BMD_Fileii(((char *)this)+0xd4, m, 1, 1);
    for (int i = 0; i < 2; i++)
        _ZN9Animation8LoadFileER13SharedFilePtr((void*)data_ov018_02112c0c[i]);
    for (int i = 0; i < 2; i++) {
        void *t = (void*)data_ov018_02112c04[i];
        _ZN15TextureSequence8LoadFileER13SharedFilePtr(t);
        {
            void *bmdModel = (void*)data_ov018_02113c00[1];
            void *btpAnim = (void*)((int*)t)[1];
            /* self=model (-> func_02046d50's arg), bmd=animFile (-> func_02046d50's
               t); see this file's header for the register-level derivation. */
            _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(bmdModel, btpAnim, btpAnim);
        }
    }
    if (_ZN11ShadowModel12InitCylinderEv((char *)&mShadowModel) == 0) return 0;
    _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(((char *)this)+0x174, ((char *)this), 0x104000, 0x12c000, 0x4800004, 0x900000);
    func_ov018_021123d0((char *)this, 0);
    unk_09c = -0x2000;
    unk_0a0 = -0x3c000;
    mScaleX = 0x1000;
    mScaleY = 0x1000;
    mScaleZ = 0x1000;
    _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(((char *)this)+0x1a8, ((char *)this), 0x32000, 0x32000, 0, 0);
    char rg[0x54];
    int v[3];
    v[0] = mPosX;
    v[1] = mPosY;
    v[2] = mPosZ;
    v[1] += 0x14000;
    _ZN13RaycastGroundC1Ev(rg);
    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(rg, v, 0);
    if (_ZN13RaycastGround10DetectClsnEv(rg))
        mPosY = *(int*)(rg+0x44);
    else
        mPosY = v[1];
    unk_364 = mPosX;
    unk_368 = mPosY;
    unk_36c = mPosZ;
    unk_374 = 0;
    func_ov018_02111d28(((char *)this), 0);
    _ZN13RaycastGroundD1Ev(rg);
    return 1;
}
