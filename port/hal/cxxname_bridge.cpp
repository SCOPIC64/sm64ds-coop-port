// Bridges for TUs that declare C-NAMED symbols at C++ linkage (an extern
// declaration outside the file's extern "C" block): the reference mangles
// as ?_ZN...@@YA..., so a same-shape C++ definition here forwards to the
// real implementation. The forward hop goes through a differently-named
// extern "C" helper because one TU cannot name both linkages of the same
// identifier.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "MeshColliderBase.h"

extern "C" {
void hal_fileptr_release(void *self);
}

void _ZN13SharedFilePtr7ReleaseEv(void *self)
{
    hal_fileptr_release(self);
}
int _ZN16MeshColliderBase9IsEnabledEv(void *self)
{
    return ((MeshColliderBase *)self)->MeshColliderBase::IsEnabled();
}
void _ZN16MeshColliderBase7DisableEv(void *self)
{
    ((MeshColliderBase *)self)->MeshColliderBase::Disable();
}

// The ov098 file table's second column is addressed through its own symbol
// (base+4 on the DS -- an offset alias hosts cannot express). Column b gets
// its own 0xc-stride view; the smoke seeds both views with the same
// SharedFilePtr pointers.
extern "C" {
char data_ov098_0213c384[0x18];
}

// ---- gate-9 method bridges (C name -> MSVC method), the gx_upload pattern -
#include "Platform.h"
#include "ShadowModel.h"
#include "Model.h"
extern "C" {
void _ZN8Platform19UpdateClsnPosAndRotEv(void *self)
{ ((Platform *)self)->Platform::UpdateClsnPosAndRot(); }
void _ZN8Platform21UpdateModelPosAndRotYEv(void *self)
{ ((Platform *)self)->Platform::UpdateModelPosAndRotY(); }
/* SHADOW SYSTEM DEFERRED (cosmetic): the cuboid template BMD at
   data_020ad524 is BUILT AT RUNTIME by boot code not yet hosted (the ov000
   static image holds path strings there). InitCuboid and the per-frame
   drop-shadow install are no-ops until that boot path lands. */
void _ZN11ShadowModel10InitCuboidEv(void *) {}
void _ZN5Actor18DropShadowScaleXYZER11ShadowModelR9Matrix4x35Fix12IiES5_S5_j(
    void *, void *, void *, int, int, int, unsigned) {}
void _ZN13SharedFilePtr8LoadFileEv(void *fp);
void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp)
{
    int trace = getenv("PORT_TRACE_SETFILE") != 0;
    if (trace)
        fprintf(stderr, "  model_loadfile fp=%p id=%u refs=%u\n", fp,
                *(unsigned short *)fp, ((unsigned char *)fp)[2]);
    /* expanded body of the matched Model::LoadFile so the load stages are
       individually traceable on host (Reallocate is a DS heap shrink, no-op
       here -- see gx_upload_bridge.cpp) */
    _ZN13SharedFilePtr8LoadFileEv(fp);
    void *filePtr = *(void **)((char *)fp + 4);
    if (((unsigned char *)fp)[2] == 1 && filePtr != 0) {
        if (trace) fprintf(stderr, "    fixups buf=%p\n", filePtr);
        Model::UpdateFileOffsets(*(BMD_File *)filePtr);
        if (trace) fprintf(stderr, "    offsets ok\n");
        Model::AddToCommonModelDataArr(*(BMD_File *)filePtr);
        if (trace) fprintf(stderr, "    common-arr ok\n");
    }
    return filePtr;
}

// BSS the shadow/collider systems use
char data_020ad524[0x40];       /* ShadowModel's template BMD stub */
void *data_020a0c80[24];        /* the collision actor registry (gate 8) */
}

#include "MeshCollider.h"
#include "ModelBase.h"
extern "C" {
void *_ZN12MeshCollider8LoadFileER13SharedFilePtr(void *fp)
{ return MeshCollider::LoadFile(*(SharedFilePtr *)fp); }
void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *bmd, int a, int b)
{
    if (getenv("PORT_TRACE_SETFILE")) {
        extern void *_ZTV5Model[8];
        extern void *_ZTV10ModelAnim2[12];
        fprintf(stderr, "  setfile self=%p vt=%p slot1=%p bmd=%p b=%d"
                " (model_vt=%p ma2_vt=%p)\n",
                self, *(void **)self,
                self ? ((void ***)self)[0][1] : 0, bmd, b,
                (void *)_ZTV5Model, (void *)_ZTV10ModelAnim2);
    }
    ((ModelBase *)self)->ModelBase::SetFile((BMD_File *)bmd, a, b);
}
}
#pragma comment(linker, "/alternatename:?data_ov098_0213c380@@3PADA=_data_ov098_0213c380")
#pragma comment(linker, "/alternatename:?data_ov098_0213c384@@3PADA=_data_ov098_0213c384")

extern "C" void _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(
    void *self, void *kcl, void *clps)
{
    ((MeshCollider *)self)->MeshCollider::SetFile((KCL_File *)kcl,
                                                  *(CLPS_Block *)clps);
}

// operator new support: the game heap pointer for actors (the smoke seeds
// it with the root heap), and the zero-fill veneer -- its DS chain rides
// arguments through registers, so the host supplies the semantics direct.
extern "C" {
void *data_020a0eac_c;
}
#pragma comment(linker, "/alternatename:?data_020a0eac@@3PAUHeap@@A=_data_020a0eac_c")
#pragma comment(linker, "/alternatename:_data_020a0eac=_data_020a0eac_c")
void func_0206e2f8(void *p, int v, unsigned n)
{
    unsigned char *b = (unsigned char *)p;
    for (unsigned i = 0; i < n; ++i) b[i] = (unsigned char)v;
}
extern "C" void hal_m43_roty(void *m, int a);
void Matrix4x3_FromRotationY(void *m, int a) { hal_m43_roty(m, a); }

#include "MeshColliderBase.h"
extern "C" int _ZN16MeshColliderBase6EnableEP5Actor(void *self, void *actor)
{
    return ((MeshColliderBase *)self)->MeshColliderBase::Enable((Actor *)actor);
}
extern "C" {
/* player-list globals ClosestPlayer scans: empty world -> null result */
int data_0208e37c[2];
int data_0208e380[2];
int data_0209b450[2];
int data_0209b458[2];
int data_0209f21c[8];
int data_0209f394[8];
}

extern "C" void _ZN6Memory10DeallocateEPv(void *p);
extern "C" void Memory_Deallocate(void *p) { _ZN6Memory10DeallocateEPv(p); }
extern "C" int hal_f0204424c(char *c);
int func_0204424c(int c) { return hal_f0204424c((char *)c); }

// gate 9: the Model vtable gains real slots at RUNTIME (targets without the
// Model methods keep the zero storage; nothing dispatches there). MSVC
// order per Model.h: dtor 0, DoSetFile 1, UpdateVerts 2, Virtual10 3,
// Render 4. ModelBase::SetFile dispatches DoSetFile -- the first virtual
// the actor lifecycle exercises.
static int __fastcall mv_dosetfile(void *self, void *, char *f, int a, int b)
{
    if (getenv("PORT_TRACE_SETFILE"))
        fprintf(stderr, "  dosetfile self=%p f=%p a=%d b=%d\n", self, f, a, b);
    return ((Model *)self)->Model::DoSetFile(f, a, b);
}
/* ---- the actor bucket's unit conversion ---------------------------------
   THE SCENE/WORLD SEAM, and processing list 5 is where the two conventions
   meet. The ROM renders in SCENE units (world >> 3): an actor's Render fills
   its model matrix with rotation rows at 1.0 and a scene-unit translation,
   and Camera::Render's LookAt_ leaves the view matrix's translation row in
   scene units to match. The harness renders in WORLD units and gets there by
   scaling that translation row back up by 8 (walk_window's R6 shim).
   x8 IS THE WHOLE CONVERSION EITHER WAY, and Mario is the proof: his body
   matrix in hal_render_player_world is the ROM's matrix with the rotation
   rows scaled by 8 and the translation left in world units, and he comes out
   the right size in the right place against real-game footage.
   The bucket runs BEFORE the shim, because an actor's Render also clips
   through the Clipper against data_0209b3ec and its positions are scene
   units. So the conversion is per-DRAW rather than per-bucket: scale this
   model's matrix and the view's translation row together, draw, put both
   back. One model's worth of world units inside an otherwise scene-unit
   pass.
   Retires with the shim, when the port's own two draws move to the ROM's
   convention and the whole frame is scene units. */
extern "C" int port_actor_bucket_depth;
int port_actor_bucket_depth;
extern "C" int data_0209b3ec[12];       /* the view matrix Model::Render composes with */

/* THE CONVERSION IS ONE NUMBER, AND IT IS 8. world = scene << 3, in every
   row of every matrix in the pass.
   The earlier reading put the model's BMD scale into the ROTATION rows as
   `1 << (shift + 10)` -- the factor hal_render_model uses for the LEVEL --
   and left translation at x8. That was never exercised: the Tree is the
   first actor with geometry, and it disproves the factor twice over.
   * func_02044534, the BILLBOARD part walk, NORMALIZES the rotation it is
     handed (NormalizeVec3 divides every row back to 1.0) before scaling it
     by the render's scale vector. Anything folded into the rotation rows is
     divided straight back out, so a billboard came out in SCENE units inside
     a world-unit frame -- 8x too small -- no matter what the factor was.
   * The same walk squares two rotation entries (`mp->r1.x * mp->r1.x + ...`
     against 0x4000) in 32-bit ints. At 1<<(shift+10) those entries are
     ~0x4000000 and the square overflows to nonsense, so which billboard axis
     the part walk picks was decided by wraparound. At x8 the largest possible
     sum is exactly 2^30 and the test means what the ROM meant.
   So the model matrix keeps the rotation rows the actor's own Render wrote
   (1.0, the ROM's convention) and the x8 travels the way the ROM's own code
   already carries a scale: the Vector3 scale argument of Model::Render. The
   part walk spends it as MTX_SCALE on ordinary parts and as the billboard's
   axis length on billboards, which is the one place a billboard can be
   scaled at all. */
enum { MV_SCENE_TO_WORLD = 8 };
static void mv_convert_matrix(void *mm)
{
    int *p = (int *)mm;
    for (int i = 9; i < 12; ++i) p[i] *= MV_SCENE_TO_WORLD;
}

static void __fastcall mv_updateverts(void *self, void *)
{ ((Model *)self)->Model::UpdateVerts(); }
static void __fastcall mv_virtual10(void *self, void *, void *m)
{ ((Model *)self)->Model::Virtual10(*(Matrix4x3 *)m); }
static void __fastcall mv_render(void *self, void *, const void *s)
{
    Model *m = (Model *)self;
    if (port_actor_bucket_depth) {
        int save[12], vsave[3];
        memcpy(save, &m->mat4x3, sizeof save);
        memcpy(vsave, data_0209b3ec + 9, sizeof vsave);
        mv_convert_matrix(&m->mat4x3);
        for (int i = 9; i < 12; ++i) data_0209b3ec[i] *= 8;
        /* the scene->world factor, spent through the ROM's own scale
           argument; an actor that asked for a scale of its own gets that
           scale converted rather than replaced */
        Vector3 wscale = {0x1000 * MV_SCENE_TO_WORLD,
                          0x1000 * MV_SCENE_TO_WORLD,
                          0x1000 * MV_SCENE_TO_WORLD};
        if (s) {
            const int *sv = (const int *)s;
            int *wv = (int *)&wscale;
            for (int i = 0; i < 3; ++i) wv[i] = sv[i] * MV_SCENE_TO_WORLD;
        }
        /* SM64DS_ACTOR_SCALE_MUL=N: multiply the scale the bucket hands
           Model::Render. The two part walks spend it differently -- the
           billboard walk (func_02044534) as its axis LENGTH, the ordinary walk
           (func_0204488c) as an MTX_SCALE on top of ModelComponents::Render's
           own 1 << (shift + 12) -- and gate 16 measured that they do not agree
           on host: the Tree (all-billboard) is right at 1, and SIGN_POST
           (all-ordinary, BMD shift 1) comes out about 256 times too small,
           which is the ratio between this chain and hal_render_model's own
           0x1000 << (shift + 10) for the level. The lever is here so the next
           pass can bracket it; the fix belongs in the walk, not in a constant.
           */
        {
            static int mul = -1;
            if (mul < 0) {
                const char *e = getenv("SM64DS_ACTOR_SCALE_MUL");
                mul = e ? atoi(e) : 1;
                if (mul < 1) mul = 1;
            }
            if (mul > 1) {
                int *wv = (int *)&wscale;
                for (int i = 0; i < 3; ++i) wv[i] *= mul;
            }
        }
        /* SM64DS_TRACE_ACTOR_MAT=1: the converted model matrix, the BMD
           header behind it, and the bone transform the shape walk composes
           with. This is the trace that found the collapsed-geometry wall
           documented above the bucket. */
        if (getenv("SM64DS_TRACE_ACTOR_MAT")) {
            const unsigned char *bf = (const unsigned char *)m->data.modelFile;
            const int *q = (const int *)&m->mat4x3;
            const int *bt = (const int *)m->data.transforms;
            fprintf(stderr, "  [amat] file %p shift %u bones %u | %d %d %d | "
                    "%d %d %d | %d %d %d | %d %d %d\n", (const void *)bf,
                    bf ? bf[0] : 0u, bf ? *(const unsigned *)(bf + 4) : 0u,
                    q[0], q[1], q[2], q[3], q[4], q[5], q[6], q[7], q[8],
                    q[9], q[10], q[11]);
            if (bt)
                fprintf(stderr, "  [abone] %d %d %d | %d %d %d | %d %d %d | "
                        "%d %d %d\n", bt[0], bt[1], bt[2], bt[3], bt[4], bt[5],
                        bt[6], bt[7], bt[8], bt[9], bt[10], bt[11]);
        }
        m->Model::Render(&wscale);
        memcpy(data_0209b3ec + 9, vsave, sizeof vsave);
        memcpy(&m->mat4x3, save, sizeof save);
        return;
    }
    m->Model::Render((const Vector3 *)s);
}
extern "C" {
extern void *_ZTV5Model[8];
void hal_fill_model_vtable(void)
{
    _ZTV5Model[1] = (void *)mv_dosetfile;
    _ZTV5Model[2] = (void *)mv_updateverts;
    _ZTV5Model[3] = (void *)mv_virtual10;
    _ZTV5Model[4] = (void *)mv_render;
    /* Slot 5 too: TUs that dispatch through LOCAL shadow classes count in
       ROM/Itanium numbering (two dtor slots), which lands Render at 5.
       Model.h-compiled TUs land it at 4. The object serves both. */
    _ZTV5Model[5] = (void *)mv_render;
}
}

// gate 10: ModelAnim2's primary table (the Player's two body ModelAnims
// dispatch DoSetFile through it via ModelBase::SetFile). MSVC order: dtor 0,
// DoSetFile 1, UpdateVerts 2, Virtual10 3, Render 4, Virtual18 5. ROM slots
// carry the ModelAnim overrides for everything past DoSetFile. No dual-fill
// here: Render's ROM slot (5) is Virtual18's MSVC slot, so shadow-TU Render
// dispatch cannot be served by the same array -- trap-by-Virtual18 will name
// it if such a TU ever appears.
#include "ModelAnim.h"
static void __fastcall ma2_dtor(void *, void *) {}
static void __fastcall ma2_updateverts(void *self, void *)
{ ((ModelAnim *)self)->ModelAnim::UpdateVerts(); }
static void __fastcall ma2_virtual10(void *self, void *, void *m)
{ ((ModelAnim *)self)->ModelAnim::Virtual10(*(Matrix4x3 *)m); }
static void __fastcall ma2_render(void *self, void *, const void *s)
{ ((ModelAnim *)self)->ModelAnim::Render((const Vector3 *)s); }
static void __fastcall ma2_virtual18(void *self, void *, unsigned m, const void *s)
{ ((ModelAnim *)self)->ModelAnim::Virtual18(m, (const Vector3 *)s); }
extern "C" {
extern void *_ZTV10ModelAnim2[12];
extern void *VTable_Animation_ModelAnim2Thunk[12];
extern void *_ZTV9ModelAnim[10];
extern void *VTable_Animation_ModelAnimThunk[8];
void hal_fill_modelanim2_vtable(void)
{
    _ZTV10ModelAnim2[0] = (void *)ma2_dtor;
    _ZTV10ModelAnim2[1] = (void *)mv_dosetfile;
    _ZTV10ModelAnim2[2] = (void *)ma2_updateverts;
    _ZTV10ModelAnim2[3] = (void *)ma2_virtual10;
    _ZTV10ModelAnim2[4] = (void *)ma2_render;
    _ZTV10ModelAnim2[5] = (void *)ma2_virtual18;
    /* the Animation-base secondary table only ever destructs */
    VTable_Animation_ModelAnim2Thunk[0] = (void *)ma2_dtor;
    VTable_Animation_ModelAnim2Thunk[1] = (void *)ma2_dtor;
    /* plain ModelAnim (the Player's head models) shares every slot */
    _ZTV9ModelAnim[0] = (void *)ma2_dtor;
    _ZTV9ModelAnim[1] = (void *)mv_dosetfile;
    _ZTV9ModelAnim[2] = (void *)ma2_updateverts;
    _ZTV9ModelAnim[3] = (void *)ma2_virtual10;
    _ZTV9ModelAnim[4] = (void *)ma2_render;
    _ZTV9ModelAnim[5] = (void *)ma2_virtual18;
    VTable_Animation_ModelAnimThunk[0] = (void *)ma2_dtor;
    VTable_Animation_ModelAnimThunk[1] = (void *)ma2_dtor;
}
}

#include "ShadowModel.h"
extern "C" {
void hal_fill_shadow_vtable(void) {}   /* shadow system deferred */
}

extern "C" {
void *_ZN5Model23AddToCommonModelDataArrER8BMD_File(void *file)
{ return Model::AddToCommonModelDataArr(*(BMD_File *)file); }
void *func_0203cc0c(unsigned size);
void _ZN6Memory10DeallocateEPv(void *p);
/* the DS global operator new/delete route through the Memory layer */
void *_Znwj(unsigned size) { return func_0203cc0c(size); }
void _ZN6Memory16operator_delete2EPv(void *p) { _ZN6Memory10DeallocateEPv(p); }
}
