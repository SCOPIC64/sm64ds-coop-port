// Gate-10 method bridges: C-named references -> MSVC method definitions.
//
// The Player closure's callers reference these at C linkage while the
// defining TUs compile them as real methods against the shared headers.
// Same hop as gate 9 (cxxname_bridge.cpp), split into its own TU because
// Player.h drags a wider include surface than the gate-9 file wants.
#include <cstdio>
#include <cstdlib>

#include "Animation.h"
#include "BgCh.h"
#include "NestedHeapIterator.h"
#include "Player.h"
#include "ShadowModel.h"
#include "TextureSequence.h"
#include "Heap.h"
#include "ModelAnim.h"

/* how many times hal_call_state_fn fell off the end of its switch this run --
   read by the F3 overlay in port/tests/walk_window.cpp */
extern "C" unsigned g_port_unhosted_hits = 0;

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
/* SM64DS_TEX_LOG=1: the BMD's own texture/palette/material tables next to
   the runtime material records func_020462d0 built from them -- the
   ground truth a [texbind] line is checked against. One shot per file. */
extern "C" int hal_tex_log(void);
static void hal_dump_model_tables(Model *m)
{
    static BMD_File *done[16];
    static int ndone;
    /* ModelBase::modelFile is only filled by LoadAndSetFile; the harness
       drives SetFile directly, so read the file the components carry. */
    BMD_File *f = m->data.modelFile;
    if (!f) return;
    for (int i = 0; i < ndone; ++i)
        if (done[i] == f) return;
    if (ndone < 16) done[ndone++] = f;

    printf("[texmodel] file=%p tex=%u pal=%u mat=%u\n", (void *)f,
           f->numTextures, f->numPalettes, f->numMaterials);
    for (u32 i = 0; i < f->numTextures; ++i) {
        BMD_Texture *t = f->textures + i;
        printf("  tex %2u %-20s flags %08x fmt %u %3dx%-3d size %5u "
               "vramoff %05x\n",
               i, (const char *)t->unk_00, t->flags, (t->flags >> 26) & 7,
               8 << ((t->flags >> 20) & 7), 8 << ((t->flags >> 23) & 7),
               t->size, (t->flags & 0xffff) << 3);
    }
    for (u32 i = 0; i < f->numPalettes; ++i) {
        BMD_Palette *p = f->palettes + i;
        printf("  pal %2u %-20s size %5u vramoff %05x -> pltt %04x\n", i,
               (const char *)p->unk_00, p->size, p->vramOffset,
               p->vramOffset >> 4);
    }
    const unsigned char *mm = (const unsigned char *)m->data.materials;
    for (u32 i = 0; i < f->numMaterials && mm; ++i) {
        const u32 *e = (const u32 *)(mm + i * 0x30);
        printf("  mat %2u %-20s tex %3d pal %3d teximage %08x pltt %04x "
               "attr %08x difamb %08x\n",
               i, (const char *)f->materials[i].unk_00, (int)e[0], (int)e[1],
               e[7], e[8], e[9], e[10]);
    }
}

/* Stage::RenderModel's own scale argument, {125.0, 125.0, 125.0} Fix12i.
   Emitted by port/tools/romdata.py from the arm9 image. */
extern unsigned char data_020755d4[];

void hal_render_model(void *model, int scaleShift)
{
    Model *m = (Model *)model;
    if (hal_tex_log()) hal_dump_model_tables(m);
    /* THE STAGE RENDERS IN SCENE UNITS, and this is Stage::RenderModel's own
       shape: the model matrix is the Model constructor's identity
       (data_02082128) and the entire scale travels through Model::Render's
       Vector3 argument -- data_020755d4 -- which the ordinary part walk
       spends as an MTX_SCALE on top of its own 1 << (shift + 12). Scene is
       what everything else in the frame is now in: the view matrix
       Camera::Render hands LookAt_ as (v + 4) >> 3, the model matrices the
       actors' own Render methods fill, and the positions Actor::BeforeBehavior
       clips with. The old world-unit matrix (0x1000 << (shift + 10), x40/41)
       and the R6 view shim that paid for it are both gone.

       NOTHING IS CORRECTED HERE ANY MORE. The old world matrix carried a
       measured x40/41, and that 2.5% was the residue of forcing a power of two
       (0x1000 << (shift + 10) = x2048) onto a scale the ROM writes as 125.0.
       With the part walk's own MTX_SCALE reaching the geometry engine again
       (the hostgen MMIO_PTR hole, port/tools/hostgen.py) the ROM's vector is
       exact: the stage comes out x[-1000..1000] y[-225..953.5] scene, which is
       x[-8000..8000] y[-1799.8..7627.9] world, and the castle grounds' KCL --
       the same terrain, in the world units the Player and the object table use
       -- puts its lowest vertex at -1800.0 and its outer wall at +-8500.
       SM64DS_LEVEL_SCALE=N overrides the whole vector for the A/B. */
    /* This runs per model per frame, and MSVC's getenv scans the whole
       environment block, so both overrides are latched on first use. */
    int s = *(const int *)data_020755d4;
    static int scale_override = 0, scale_value = 0;
    if (!scale_override) {
        const char *e = std::getenv("SM64DS_LEVEL_SCALE");
        scale_override = e ? 1 : -1;
        if (e) scale_value = std::atoi(e);
    }
    if (scale_override > 0) s = scale_value;
    static int probe = -1;
    if (probe < 0) probe = std::getenv("SM64DS_MODEL_PROBE") ? 1 : 0;
    if (probe) {
        const BMD_File *f2 = m->data.modelFile;
        const int *t = (const int *)m->data.transforms;
        int lo[3] = {1 << 30, 1 << 30, 1 << 30},
            hi[3] = {-(1 << 30), -(1 << 30), -(1 << 30)};
        unsigned nb = f2 ? f2->numBones : 0;
        for (unsigned i = 0; i < nb && t; ++i)
            for (int k = 0; k < 3; ++k) {
                int v = t[i * 12 + 9 + k];
                if (v < lo[k]) lo[k] = v;
                if (v > hi[k]) hi[k] = v;
            }
        std::printf("[mprobe] shift %u (arg %d) bones %u scale %d (%.3f) "
                    "part.t x[%.3f..%.3f] y[%.3f..%.3f] z[%.3f..%.3f]\n",
                    f2 ? f2->scaleShift : 0u, scaleShift, nb, s, s / 4096.0,
                    lo[0] / 4096.0, hi[0] / 4096.0, lo[1] / 4096.0,
                    hi[1] / 4096.0, lo[2] / 4096.0, hi[2] / 4096.0);
    }
    Vector3 scale = {s, s, s};
    for (int i = 0; i < 12; ++i) ((int *)&m->mat4x3)[i] = 0;
    ((int *)&m->mat4x3)[0] = 0x1000;
    ((int *)&m->mat4x3)[4] = 0x1000;
    ((int *)&m->mat4x3)[8] = 0x1000;
    m->Model::Render(&scale);
}

void hal_render_player_body_ex(void *player, int with_head);
void hal_render_player_body(void *player)
{ hal_render_player_body_ex(player, 1); }
void hal_render_player_body_only(void *player)
{ hal_render_player_body_ex(player, 0); }
/* scene-space variant: mat4x3 = Y-rotation(mAngleY) at 1.0 + the scene
   translation, the head composed through the body's own matrix (the neck
   bone is model-space) */
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
    /* THE ROM'S OWN MATRIX: rotation rows at 1.0 and the translation row in
       SCENE units, which is what an actor's Render writes and what
       Camera::Render's view matrix expects. `(v + 4) >> 3` is Camera::Render's
       own rounding for the same conversion.
       The x8 rotation rows this replaces were that same matrix carried into a
       world-unit frame; they came out the same size on screen and left Mario
       ~145 world units tall, which is what the migration has to preserve. */
    int s = data_02082214[idx * 2], co = data_02082214[idx * 2 + 1];
    int scene[12] = {co, 0, -s, 0, 0x1000, 0, s, 0, co,
                     (*(int *)(c + 0x5c) + 4) >> 3,
                     (*(int *)(c + 0x60) + 4) >> 3,
                     (*(int *)(c + 0x64) + 4) >> 3};
    for (int i = 0; i < 12; ++i) ((int *)&ma->mat4x3)[i] = scene[i];
    ma->ModelAnim::UpdateVerts();
    ma->ModelAnim::Render(0);

    unsigned hid = func_ov002_020becf4(c, *(unsigned char *)(c + 0x6db), 1);
    if (hid != 8 && hid != 9) {
        char *head = ((char **)(c + 0x154))[hid];
        if (head) {
            char *neck = *(char **)((char *)ma + 0x14) + 0x2d0;
            if (neck)
                m43_mul((const int *)neck, scene, (int *)(head + 0x1c));
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
extern "C" int _ZN6Player12St_Jump_InitEv(char *c);
extern "C" void func_ov002_020e200c(char *c);
/* Three state slots the ROM fills with plain ov002 functions rather than
   Player methods: Null's Init, WallJump's Init, InYoshiMouth's Cleanup.
   The community St_ names at those addresses belong to ov006, not ov002. */
extern "C" int func_ov002_020cac30(void);
extern "C" int func_ov002_020d6084(char *c);
extern "C" int func_ov002_020e17f8(void *c);
extern "C" int _ZN6Player16St_BurnLava_MainEv(char *c);
/* gate 14: the level-boot state and the seven entrance-step handlers */
extern "C" int func_ov002_020c6f3c(void *c);
extern "C" void func_ov002_020c75f0(char *c);
extern "C" void func_ov002_020c7350(char *c);
extern "C" void func_ov002_020c71e0(char *c);
extern "C" void func_ov002_020c7194(char *c);
extern "C" void func_ov002_020c72a4(void *c);
extern "C" void func_ov002_020c70ac(char *c);
extern "C" void func_ov002_020c6fe4(char *c);
/* SM64DS_TRACE_STATE: every state function the dispatcher runs. The state
   machine is otherwise opaque from outside -- the Player carries a State* and
   the port switches on the DS address inside it -- and this is how the water
   test reads: walking into the moat has to show 0x020ce550 (St_Swim_Init) and
   then 0x020cd94c (St_Swim_Main), and climbing out has to come back through
   0x020cd1e4 (St_Swim_Cleanup) to St_Walk.
     =1  once per distinct DS address (the quiet census)
     =2  on every CHANGE, with the Player's height, which is what a
         walk-in/swim-across/climb-out run has to be read from */
extern "C" int hal_call_state_fn(void *self, unsigned ds_addr)
{
    {
        static int on = -1;
        if (on < 0) {
            const char *e = std::getenv("SM64DS_TRACE_STATE");
            on = e ? std::atoi(e) : 0;
            if (e && !on) on = 1;
        }
        if (on >= 2) {
            static unsigned prev;
            if (ds_addr != prev) {
                prev = ds_addr;
                std::printf("  [state] 0x%08x y=%.1f\n", ds_addr,
                            *(int *)((char *)self + 0x60) / 4096.0);
            }
        } else if (on) {
            static unsigned said[64];
            int seen = 0;
            for (int i = 0; i < 64 && said[i]; ++i)
                if (said[i] == ds_addr) seen = 1;
            if (!seen) {
                for (int i = 0; i < 64; ++i)
                    if (!said[i]) { said[i] = ds_addr; break; }
                std::printf("  [state] 0x%08x\n", ds_addr);
            }
        }
    }
    switch (ds_addr) {
#include "player_states.inc"
    }
    /* Every miss here is a state the port silently does not run, and the only
       record of it used to be a line in the flight recorder that nobody reads
       until after the glitch. The counter is what the F3 overlay shows live,
       so a state hole announces itself while it is happening. */
    ++g_port_unhosted_hits;
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

/* SHADOW SYSTEM DEFERRED (matches InitCuboid, cxxname_bridge.cpp, which now
   carries the full writeup). The template BMD at data_020ad560 is NOT
   runtime-built: it is static .data in overlay 1, a complete BMD_File with one
   bone at 0x020ad5dc and one material at 0x020ad4c4. Parsing the stub does
   walk garbage, but because the port never mounts ov001, not because a
   builder is missing.

   Note the stub is also UNDERSIZED where it is declared (actor_vtables.cpp):
   the ROM record is 0x3c bytes, 0x020ad560 to the bone at 0x020ad59c. */
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
/* The sound command queue's two SIZED objects. Placeholders while sound was
   stubbed; the hosted ARM7 in hal/sdat/ needs their real extents.
   data_020a6760 is the node pool func_0205b070 cache-flushes as 0x1800
   bytes = 256 nodes x 0x18, and the seeding chains all 256 of them.
   data_020a64a8 is the batch ring that func_0205b070 and func_0205b274 index
   with "idx++; if (idx > 8) idx = 0", so it needs nine slots. At the old
   sizes, seeding the pool walked off the end of a 32-byte object. */
void *data_020a64a8[16];
int data_020a6760[256 * 0x18 / sizeof(int)];
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
