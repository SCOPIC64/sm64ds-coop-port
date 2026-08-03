// Gate-10 method bridges: C-named references -> MSVC method definitions.
//
// The Player closure's callers reference these at C linkage while the
// defining TUs compile them as real methods against the shared headers.
// Same hop as gate 9 (cxxname_bridge.cpp), split into its own TU because
// Player.h drags a wider include surface than the gate-9 file wants.
#include <cstdio>

#include "Animation.h"
#include "BgCh.h"
#include "NestedHeapIterator.h"
#include "Player.h"
#include "ShadowModel.h"
#include "TextureSequence.h"
#include "Heap.h"
#include "ModelAnim.h"

extern "C" unsigned int _ZNK6Player14GetBodyModelIDEjb(char *, unsigned int, char);
extern "C" unsigned func_ov002_020becf4(char *self, unsigned j, int b);
extern "C" int _ZN6Player13InitResourcesEv(void *);

/* C++-linkage globals some slice TUs call under Itanium-style names */
int _ZNK9Animation12WillHitFrameEi(void *self, int f)
{ return ((Animation *)self)->Animation::WillHitFrame(f) ? 1 : 0; }
int _ZN9Animation8GetFlagsEv(void *self)
{ return ((Animation *)self)->Animation::GetFlags(); }
void _ZN6Player4HealEi(Player *p, int amt)
{ p->Player::Heal(amt); }

unsigned int Player::GetBodyModelID(unsigned int a, bool b_) const
{ return _ZNK6Player14GetBodyModelIDEjb((char *)this, a, b_ ? 1 : 0); }

extern "C" {
/* gate-10 smoke drives the state machine directly (the ChangeState PMF
   dispatch reads DS-baked member-fn bytes MSVC cannot represent) */
int hal_player_init_resources(void *p)
{ return _ZN6Player13InitResourcesEv(p); }
int hal_player_st_wait_init(void *p)
{ return ((Player *)p)->Player::St_Wait_Init(); }
int hal_player_st_wait_main(void *p)
{ return ((Player *)p)->Player::St_Wait_Main(); }
int hal_player_st_walk_init(void *p)
{ return ((Player *)p)->Player::St_Walk_Init(); }
int hal_player_st_walk_main(void *p)
{ return ((Player *)p)->Player::St_Walk_Main(); }
int hal_player_behavior(void *p)
{ return ((Player *)p)->Player::Behavior(); }
/* the walk demo renders the Player's current body ModelAnim in place:
   identity model matrix, bones posed from the anim Behavior advanced */
/* level model render for the window: identity world matrix (stage models
   are authored in world space; the KCL shares it) */
void hal_render_model(void *model, int scaleShift)
{
    Model *m = (Model *)model;
    /* the components walk latches 1<<(shift+12) globally; the host path
       does not consume the latch yet, so the world scale rides the matrix.
       Empirically the stage lands at KCL scale with shift+4. */
    int d = 0x1000 << (scaleShift + 4);
    for (int i = 0; i < 12; ++i) ((int *)&m->mat4x3)[i] = 0;
    ((int *)&m->mat4x3)[0] = d;
    ((int *)&m->mat4x3)[4] = d;
    ((int *)&m->mat4x3)[8] = d;
    m->Model::Render(0);
}

void hal_render_player_body_ex(void *player, int with_head);
void hal_render_player_body(void *player)
{ hal_render_player_body_ex(player, 1); }
void hal_render_player_body_only(void *player)
{ hal_render_player_body_ex(player, 0); }
/* world-space variant: mat4x3 = Y-rotation(mAngleY) + fx translation, the
   head composed through the body's world matrix (neck is model-space) */
extern "C" short data_02082214[];   /* s16 trig pairs [sin, cos], 4096 = 1 */
static void m43_mul(const int *a, const int *b, int *out)
{
    for (int r = 0; r < 3; ++r)
        for (int c2 = 0; c2 < 3; ++c2)
            out[r * 3 + c2] = (int)(((long long)a[r * 3] * b[c2] +
                                     (long long)a[r * 3 + 1] * b[3 + c2] +
                                     (long long)a[r * 3 + 2] * b[6 + c2]) >>
                                    12);
    for (int c2 = 0; c2 < 3; ++c2)
        out[9 + c2] = (int)(((long long)a[9] * b[c2] +
                             (long long)a[10] * b[3 + c2] +
                             (long long)a[11] * b[6 + c2]) >>
                            12) +
                      b[9 + c2];
}
void hal_render_player_world(void *player)
{
    char *c = (char *)player;
    unsigned id = _ZNK6Player14GetBodyModelIDEjb(c, *(int *)(c + 8) & 0xff, 0);
    ModelAnim *ma = ((ModelAnim **)(c + 0xdc))[id];
    if (!ma) return;
    unsigned idx = ((unsigned short)*(short *)(c + 0x8e)) >> 4;
    int s = data_02082214[idx * 2], co = data_02082214[idx * 2 + 1];
    int world[12] = {co, 0, -s, 0, 0x1000, 0, s, 0, co,
                     *(int *)(c + 0x5c), *(int *)(c + 0x60),
                     *(int *)(c + 0x64)};
    for (int i = 0; i < 12; ++i) ((int *)&ma->mat4x3)[i] = world[i];
    ma->ModelAnim::UpdateVerts();
    ma->ModelAnim::Render(0);

    unsigned hid = func_ov002_020becf4(c, *(unsigned char *)(c + 0x6db), 1);
    if (hid != 8 && hid != 9) {
        char *head = ((char **)(c + 0x154))[hid];
        if (head) {
            char *neck = *(char **)((char *)ma + 0x14) + 0x2d0;
            if (neck)
                m43_mul((const int *)neck, world, (int *)(head + 0x1c));
            ((void(__fastcall *)(void *, void *, const void *))(
                ((void ***)head)[0][4]))(head, 0, 0);
        }
    }
}

void hal_render_player_body_ex(void *player, int with_head)
{
    char *c = (char *)player;
    unsigned id = _ZNK6Player14GetBodyModelIDEjb(c, *(int *)(c + 8) & 0xff, 0);
    ModelAnim *ma = ((ModelAnim **)(c + 0xdc))[id];
    if (!ma) return;
    for (int i = 0; i < 12; ++i) ((int *)&ma->mat4x3)[i] = 0;
    ((int *)&ma->mat4x3)[0] = 0x1000;
    ((int *)&ma->mat4x3)[4] = 0x1000;
    ((int *)&ma->mat4x3)[8] = 0x1000;
    ma->ModelAnim::UpdateVerts();
    ma->ModelAnim::Render(0);

    /* the head is its own model; Player::Render seats it by copying the
       body's neck-bone matrix (+0x2d0 in the bone array) into the head's
       matrix slot, then renders through the object's own vtable */
    unsigned hid = func_ov002_020becf4(c, *(unsigned char *)(c + 0x6db), 1);
    if (with_head && hid != 8 && hid != 9) {
        char *head = ((char **)(c + 0x154))[hid];
        if (head) {
            char *src = *(char **)((char *)ma + 0x14) + 0x2d0;
            /* host Model::Render consumes mat4x3; seat the neck transform
               there (the game writes the bone array, but writing both
               double-transforms on host) */
            if (src)
                for (int i = 0; i < 12; ++i)
                    ((int *)(head + 0x1c))[i] = ((const int *)src)[i];
            ((void(__fastcall *)(void *, void *, const void *))(
                ((void ***)head)[0][4]))(head, 0, 0);
        }
    }
}

/* State-machine dispatch: the State objects come from the overlay image
   with DS code addresses baked into their function slots (mwcc PMFs). The
   host ChangeState (port/unmatched/Player_ChangeState.cpp) routes every
   dispatch here; the table grows one line per state the slice hosts.
   Unknown state = loud no-op success so the boot path keeps moving. */
extern "C" int _ZN6Player18St_LevelEnter_MainEv(int *c);
extern "C" int hal_call_state_fn(void *self, unsigned ds_addr)
{
    switch (ds_addr) {
#include "player_states.inc"
    }
    std::fprintf(stderr, "  [state] unhosted state fn 0x%08x (no-op)\n",
                 ds_addr);
    return 1;
}

void _ZN6Player16InitWingFeathersEb(void *self, unsigned char b_)
{ ((Player *)self)->Player::InitWingFeathers(b_ != 0); }
extern "C++" int ApproachLinear(int &ref, int target, int step);
int _Z14ApproachLinearRiii(int *ref, int target, int step)
{ return ApproachLinear(*ref, target, step); }
int hal_anim_willhit(void *self, int f)
{ return ((Animation *)self)->Animation::WillHitFrame(f) ? 1 : 0; }
int hal_nhi_next(void *self, void *h)
{ return ((NestedHeapIterator *)self)->NestedHeapIterator::Next(
      (HeapAllocator *)h); }

void _ZN9Animation7AdvanceEv(void *self)
{ ((Animation *)self)->Animation::Advance(); }
int _ZN9Animation8FinishedEv(void *self)
{ return ((Animation *)self)->Animation::Finished(); }
char *_ZN9Animation8LoadFileER13SharedFilePtr(void *fp)
{
    if (!fp) {
        std::fprintf(stderr, "  [anim] LoadFile on NULL fileptr (table hole)\n");
        return 0;
    }
    return Animation::LoadFile(*(SharedFilePtr *)fp);
}

int _ZN6Player6IsAnimEj(void *self, unsigned a)
{ return ((Player *)self)->Player::IsAnim(a); }
int _ZN6Player12FinishedAnimEv(void *self)
{ return ((Player *)self)->Player::FinishedAnim(); }
int _ZN6Player17SetNoControlStateEhih(void *self, unsigned char a, int b,
                                      unsigned char c)
{ return ((Player *)self)->Player::SetNoControlState(a, b, c); }
int _ZN6Player8HasNoCapEv(void *self)
{ return ((Player *)self)->Player::HasNoCap(); }
int _ZN6Player9GetHealthEv(void *self)
{ return ((Player *)self)->Player::GetHealth(); }

void _ZN4BgCh19StartDetectingWaterEv(void *self)
{ ((BgCh *)self)->BgCh::StartDetectingWater(); }

/* SHADOW SYSTEM DEFERRED (matches InitCuboid, cxxname_bridge.cpp): the
   template BMD at data_020ad560 is runtime-built by un-hosted boot code;
   parsing the zero stub would walk garbage. */
void _ZN11ShadowModel12InitCylinderEv(void *) {}

void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void *self, void *bmd,
                                                       void *btp)
{ ((TextureSequence *)self)->TextureSequence::Prepare(*(BMD_File *)bmd,
                                                      *(BTP_File *)btp); }
void *_ZN15TextureSequence8LoadFileER13SharedFilePtr(void *fp)
{ return TextureSequence::LoadFile(*(SharedFilePtr *)fp); }

void _ZN18NestedHeapIterator6RemoveEP13HeapAllocator(void *self, void *node)
{ ((NestedHeapIterator *)self)->NestedHeapIterator::Remove(
      (HeapAllocator *)node); }
int _ZN18NestedHeapIterator8PreviousEP13HeapAllocator(void *self, void *h)
{ return ((NestedHeapIterator *)self)->NestedHeapIterator::Previous(
      (HeapAllocator *)h); }

int _ZN4Heap6RescueEv(void *self)
{ return ((Heap *)self)->Heap::Rescue(); }
int _ZN4Heap21MaxAllocationUnitSizeEv(void *self)
{ return ((Heap *)self)->Heap::MaxAllocationUnitSize(); }
int _ZN4Heap6IntactEv(void *self)
{ return ((Heap *)self)->Heap::Intact() ? 1 : 0; }

/* gate-10 BSS, second ring */
int data_0208e428[8], data_0209b44c[4], data_0209b480[4];
int data_020a4d60[8], data_020a6438[8], data_020a6488[4], data_020a648c[4];
int data_020a6490[4], data_020a649c[4], data_020a64a0[4], data_020a64a4[4];
int data_020a64a8[4], data_020a6760[8];
int data_020a0f1c[4], data_020a4d54[4], data_020a6440[4], data_020a6444[4];
int data_020a6484[4], data_020a6494[4], data_020a6498[4];
int data_0209cdd0, data_0209cdd4, data_0209cdd8, data_0209cddc, data_0209cde0;
int data_0209f220[8], data_0209f264[8], data_020a0d90[8], data_020a0f38[8];
int data_020a4b58[4], data_020a4b68[4], data_020a60f4[4];
/* DTCM scratch the timer list walker anchors at */
__declspec(align(8)) unsigned char data_023c0000[64];
int data_02099e94[4], data_02099ebc[4], data_02099ec4[4], data_02099fcc[4];
int data_020a6084[4], data_020a6088[2], data_020a8114[4];

}  /* extern "C" */
