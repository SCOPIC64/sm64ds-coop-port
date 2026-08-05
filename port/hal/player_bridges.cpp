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
/* GetFlags is C linkage now: main's mangled-declaration sweep gave the Player
   state TUs an extern "C" declaration, so they call the plain name. The alias
   at the head of hal/cxx_aliases.cpp still serves the old C++ mangling. */
extern "C" int _ZN9Animation8GetFlagsEv(void *self)
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
extern "C" void func_ov002_020e1e70(char *c);
extern "C" void func_ov002_020e1c20(char *c);
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
/* ST_CLIMB's two host entries, port/unmatched/Player_St_Climb.cpp */
extern "C" int port_player_st_climb_init(void *self);
extern "C" int port_player_st_climb_main(void *self);

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

/* ---- LIVE CHARACTER SWAP (port mod) ------------------------------------
   The mod is the ENTRY POINT, not the swap. Player::SetRealCharacter is the
   game's own permanent change -- the one the character doors call -- and it is
   already instant: it starts the hat morph and then writes unk_73c = 0, which
   cancels the morph's state machine before a single stage of it runs. So there
   is nothing to reimplement here and nothing to animate away. What the port
   owes it is a caller with guards, because the ROM only ever reaches it from
   one place that has already checked everything.

   All four body and head models are resident from InitResources
   (func_ov002_020e5948), so no model loading happens. One thing does load: the
   BCA for (current animation, new character). mCharFileBase is animId * 4, not
   a per-character base, so ANIM_PTRS[base + chr] is a 2D lookup and only the
   current row's Mario column is resident at boot.

   Return codes exist so the caller can print WHY nothing happened rather than
   looking broken: 0 done, 1 no player, 2 out of range, 3 already that one. */
extern void *data_0209f394[];        /* per-player Actor*, cxxname_bridge's */
extern short data_02092144[];        /* per-player health, high byte = HP */
extern void *data_ov002_020ff480[];  /* ANIM_PTRS[anim*4 + char] */
extern signed char data_02092114;    /* queued in-level swap, -1 = none */
void _ZN10ModelAnim24CopyERKS_Pcj(void *self, const void *src, char *nf,
                                  unsigned nof);

int port_set_character(int chr)
{
    char *c = (char *)data_0209f394[0];
    if (!c) return 1;
    /* SetRealCharacter masks with & 3 in two places but uses chr RAW in the
       other two (the ANIM_PTRS index and the chr + 0xc4 default-anim index),
       so an out-of-range value would read past the table into whatever ov002
       packed next. Reject it -- masking would silently hand back a different
       character than the caller asked for. */
    if (chr < 0 || chr > 3) return 2;
    /* the body ModelAnim2 slots: null until InitResources has run, and
       SetRealCharacter dereferences one with no check of its own */
    if (!*(void **)(c + 0xdc)) return 1;
    if ((int)(*(unsigned *)(c + 8) & 0xff) == chr) return 3;

    /* A cap collect in flight is CANCELLED, not refused. Cancelling is what an
       instant swap means, and refusing would be a trap: the debug menu pauses
       the tick, so unk_73c can never advance while someone is holding the menu
       open. Say it happened so an odd-looking result is attributable. */
    if (((Player *)c)->Player::IsCollectingCap())
        std::fprintf(stderr, "[chr] swap during a cap collect (unk_73c=%04x);"
                             " the morph is cancelled\n",
                     *(unsigned short *)(c + 0x73c));

    /* SetRealCharacter ends in Heal(0x880) unconditionally. Save the whole
       short, not just the HP byte -- the low byte is a fraction GiveHealth
       works in. Without this every swap visibly refills the HUD hearts, which
       makes the row useless for anything you were testing damage against. */
    const unsigned pno = *(unsigned char *)(c + 0x6d8) & 3;
    const short saved_hp = data_02092144[pno];
    /* which body model is animating RIGHT NOW, before param1 moves */
    const unsigned old_id =
        _ZNK6Player14GetBodyModelIDEjb(c, *(unsigned *)(c + 8) & 0xff, 0);

    ((Player *)c)->Player::SetRealCharacter((unsigned)chr);

    /* KNOWN, LEFT ALONE DELIBERATELY: Luigi's head draws with the toon/flash
       shading the power flower uses. The swap does not turn it on -- unk_73c
       = 0 inside SetRealCharacter cancels func_ov002_020be3b0 before stage 1's
       SetPolygonMode(m, 2) can run, which is what makes the swap instant. It
       means stage 7, the only thing that turns the flash OFF, never runs
       either, so a model already in that polygon mode keeps it. A
       Player::TurnOffToonShading(chr) here clears it, but that is a rendering
       question about Luigi's head model rather than part of the swap, and it
       has not been eyeballed. Chip it separately. */

    /* THE OTHER THING THE ROM'S CALLER ALREADY HAD RIGHT. data_02092114 is the
       QUEUED in-level swap -- the file-select picker writes the character it
       wants and func_ov002_020be008 performs it on the next landing, spawning
       the cap actor and freezing the player in a no-control state while the
       poof plays. -1 means "nothing queued", and the port never seats it: it
       is plain zero-initialised storage, which reads as "a swap to Mario is
       pending". Inert for as long as you ARE Mario, which is why nothing has
       ever noticed. The moment param1 becomes anything else the poll fires and
       drags you straight back, ~28 frames after the swap, through a state the
       port does not host -- so you end up frozen as the character you asked
       for. SetRealCharacter is a direct swap, so the honest value afterwards is
       the one 020be008 itself writes when it has consumed the request. */
    data_02092114 = -1;

    data_02092144[pno] = saved_hp;
    /* The body renders off param1, which the call above wrote, but the HEAD
       renders off unk_6db, and the only thing that re-syncs unk_6db from
       param1 is the tail of func_ov002_020e4bb8 -- inside Player::Behavior.
       With the menu open the tick is skipped, so without this the new body
       wears the old head for exactly as long as somebody is looking at it.
       This is that same assignment, one tick early, and it is idempotent once
       the tick resumes. */
    *(unsigned char *)(c + 0x6db) = (unsigned char)chr;

    /* THE ONE THING SetRealCharacter LEAVES TO ITS CALLER, and the reason a
       swap that looked fine faulted on the next tick.

       Its tail is ModelAnim2::Copy(body[m1], body[m2], newFile, 0) -- but it
       assigns param1 = chr BEFORE computing both ids, so m1 == m2 and the copy
       is a model onto itself. That cannot carry an animation across, and
       body[chr] has no base Animation to begin with: InitResources
       (func_ov002_020e5948) seats only otherAnim for all four models and the
       base Animation for whoever you actually are. So the new model comes out
       with numFramesAndFlags == 0, and Animation::Advance's `% len` on the
       very next Player_AdvanceAnims is an integer divide by zero.

       On the ROM this is invisible, because SetRealCharacter has exactly one
       caller -- the character door -- and the door's state immediately calls
       Player::SetAnim with a DIFFERENT animation id, which takes SetAnim's
       slow path and seats the frame count off the new file. A swap that does
       not change animation never gets that. Calling SetAnim here would not
       help either: ModelAnim::SetAnim fast-paths when the file pointer already
       matches, and SetRealCharacter's own Copy already stored it.

       So do what the hat morph does. This is func_ov002_020be3b0's stage-3
       call with the operands it has: dst != src, which carries the live
       cursor, speed and frame count off the character who was walking a moment
       ago onto the one who is now. */
    {
        const unsigned base = *(unsigned *)(c + 0x63c);
        const unsigned new_id =
            _ZNK6Player14GetBodyModelIDEjb(c, (unsigned)chr, 0);
        void *dst = *(void **)(c + 0xdc + new_id * 4);
        void *src = *(void **)(c + 0xdc + old_id * 4);
        /* SharedFilePtr's layout is not recovered; +4 is the file pointer every
           caller in src/ indexes, SetRealCharacter's own Copy argument included.
           It is a real load off the file seam, so it can come back empty. */
        char *bca = *(char **)((char *)data_ov002_020ff480[base + chr] + 4);
        if (dst && src && dst != src && bca)
            _ZN10ModelAnim24CopyERKS_Pcj(dst, src, bca, 0);
        /* and say so if it still came out unplayable, rather than leaving the
           divide to announce it */
        if (!bca || !dst ||
            (*(unsigned *)((char *)dst + 0x54) & ~0xc0000000u) == 0)
            std::fprintf(stderr, "[chr] character %d came out with no playable "
                                 "body animation (anim base %u, file %p) -- "
                                 "Advance will divide by zero\n",
                         chr, base, (void *)bca);
    }
    return 0;
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
/* data_0209b44c is NOT here: it is the one-byte area id, hosted in
   hal/actor_vtables.cpp so the smoke targets that link no player bridge
   still get it, and so writer and reader share one object. */
int data_0208e428[8], data_0209b480[4];
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
