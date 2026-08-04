// The bottom screen's own actors, and the vtables their constructors install.
//
// HUD (334) and Minimap (335) are the two ids the castle-grounds census has
// been printing as "not registered, skipped" since the level boot started
// spawning. Both are arm9 classes whose every method is matched; both are
// spawned by Stage::LoadClsnAndObjects like everything else; and both draw
// exclusively onto the sub screen, which is why they waited for gate 25.
//
// ---- the vtable -------------------------------------------------------------
//
// EIGHTEEN SLOTS, not the twenty the Player and the ov002 actor classes carry.
// The ROM settles it: _ZTV8dMeter_c is ov002 0x0210c2c0 (found by the word
// that points at its typeinfo, 0x0210c224, which sits right after the class
// name string "8dMeter_c"), and reading forward from the vptr at +8 gives
//
//     0  020fda04  HUD::InitResources             9  020fd5e0  HUD::Render
//     1  02043c78  ActorBase::BeforeInitResources 10 02043ac8  ActorBase::BeforeRender
//     2  02013ef4  ActorDerived::AfterInitRes.    11 02043ac4  ActorBase::AfterRender
//     3  020fd5d4  HUD::CleanupResources          12 020fd5dc  HUD::OnPendingDestroy
//     4  02043bac  ActorBase::BeforeCleanupRes.   13 0204357c  ActorBase::Virtual34
//     5  02043b2c  ActorBase::AfterCleanupRes.    14 0204349c  ActorBase::Virtual38
//     6  020fd7a4  HUD::Behavior                  15 02043494  ActorBase::OnHeapCreated
//     7  02043afc  ActorBase::BeforeBehavior      16 020fb8f8  ~HUD (D1)
//     8  02043af8  ActorBase::AfterBehavior       17 020fb928  ~HUD (D0)
//
// and slot 18 is not a function pointer at all. Every one of those addresses
// resolves to the name above in config/arm9/symbols.txt, so this is read out of
// the ROM rather than inferred from the header -- which matters twice over.
//
// FIRST: filling slots 18 and 19 the way the twenty-slot classes do would write
// past the table.
//
// SECOND, and this one crashed the port: EVERY shared slot here is ActorBase's
// own, not Actor's. The twenty-slot classes in hal/actor_classes.cpp share ten
// slots that resolve to Actor::, and copying that fill to an eighteen-slot
// class is a heap corruption with a fifty-frame fuse. Actor::AfterInitResources
// ends on
//
//     *(u32 *)(long long)(int)&mFlags |= 0x38;      /* Actor + 0xb0 */
//
// and dMeter_c is 124 bytes -- ActorBase::operator new(124) in the HUD's own
// constructor. 0xb0 is 0x34 past the end of it, so that read-modify-write lands
// in whatever the game heap handed out next. On the castle grounds that is the
// BUTTERFLY spawned immediately before, and 0xb0 falls exactly on its
// SceneNode's owner back-pointer: the OR turned a valid actor pointer into
// pointer|0x38, and the next scene-tree walk dereferenced it.
//
// Slot 2 is the one genuine override of the three: ActorDerived::, not
// ActorBase:: and not Actor::. The other nine are ActorBase's throughout.
//
// Otherwise the law is hal/actor_classes.cpp's: MSVC slot order, every entry a
// __fastcall thunk so ecx carries `this`, every thunk calls QUALIFIED, and
// slots the port cannot service trap by name.
//
// ---- _ZTV7dBase_c -----------------------------------------------------------
//
// Both constructors write the base vtable first and their own over it
// (`p[0] = _ZTV7dBase_c; p[0] = _ZTV8dMeter_c;` -- the ROM's own two stores,
// which is what the decomp recovered). Nothing dispatches through the base
// one, so it is storage and no more.
#include <cstdio>
#include <cstdlib>

#include "Actor.h"
#include "ActorBase.h"
#include "ActorDerived.h"

extern "C" {
/* HUD's own C-named halves: the two destructors */
void *_ZN3HUDD1Ev(void *self);
void *_ZN3HUDD0Ev(void *self);

/* Minimap's own C-named halves: the two destructors and Behavior */
void *_ZN7MinimapD1Ev(void *self);
void *_ZN7MinimapD0Ev(void *self);
int _ZN7Minimap8BehaviorEv(void *self);

/* vtable storage the three constructors install by name (declared in
   include/decl_common.h, defined nowhere until now). dMeter_c is the HUD's and
   dMap_c is the Minimap's -- the ROM class names, which is what the matched
   constructors write, rather than the config names _ZTV8dMeter_c sits under. */
void *_ZTV7dBase_c[18];
void *_ZTV8dMeter_c[18];
void *_ZTV6dMap_c[18];

const char *port_actor_class_name(unsigned id);
void port_scene_canary(const char *where);
extern int data_0209caa0[];
extern unsigned char data_0209f2d8, data_0209f2c4, data_0209f20c, data_0209f294;
static int port_sub_oam_nonzero(void)
{
    const unsigned char *p = (const unsigned char *)0x07000400;
    int n = 0;
    for (int i = 0; i < 1024; ++i) n += p[i] != 0;
    return n;
}
extern int data_0209f334[]; extern unsigned char data_0209f2e8[];
extern unsigned short *data_0209f340;
extern void *_ZN3OAM15MM_PLAYER_ICONSE[];
extern char _ZN3OAM8MM_ARROWE[];
extern signed char data_ov002_02111148; extern unsigned char data_0209d454;
extern void *data_0209f394[];   /* per-player Actor* */
extern unsigned char data_0209f250;   /* local player index */

/* the ov001 sprite-template mount (see port/ov001_syms.txt) */
void port_ov001_syms_patch(void);
void port_ov001_pack_check(void);
}

/* HUD's other five slots are real MSVC members in their own TUs, each of
   which declares its own `struct HUD` with just the methods it defines. The
   mangled name of a member does not depend on the class layout, so one
   declaration here reaches all of them. include/HUD.h is deliberately NOT
   included: it declares the same class without InitResources or Behavior. */
struct HUD {
    int InitResources();
    int CleanupResources();
    int Behavior();
    int Render();
    void OnPendingDestroy();
    void CalculateDigits(unsigned short n);
    void RenderCoinCount();
    void RenderLifeCount();
    void RenderTimeTimer();
    void RenderHealthMeter();
    void UpdateHealthMeter();
    static void RenderCameraButtons();
};

/* Same story for the Minimap, and the same reason include/Minimap.h is not
   included: it declares three of the four members and Render's own TU declares
   a fourth against a struct of its own. Behavior is the odd one -- its TU
   defines the C name taking the object as an argument, so it needs no face. */
struct Minimap {
    int InitResources();
    int CleanupResources();
    int Render();
    void OnPendingDestroy();
    /* STATIC on the ROM -- it takes no `this` at all, and InitResources calls
       it with no argument. Declaring it a member would ask the linker for
       ?UpdateLevelSpecific@Minimap@@QAEXXZ and get the S-form's nothing. */
    static void UpdateLevelSpecific();
};

/* UpdateLevelSpecific is the reverse shape: its own TU defines a real MSVC
   member and InitResources calls it by the Itanium C name, so it needs a face
   the way HUD::RenderCoinCount and friends do. */
extern "C" void _ZN7Minimap19UpdateLevelSpecificEv(void)
{ Minimap::UpdateLevelSpecific(); }

// ---- the arm9 bss the Minimap reads ----------------------------------------
//
// Four of these are ONE STRUCT the delink split at the boundaries code happened
// to touch, and separate host arrays would put the pieces on whatever the
// linker chose. UpdateMinimap settles it: it writes a 16-byte descriptor to
// data_0209f3c8 and four words to data_0209f3c4 + 0x14..0x20 -- addresses that
// land INSIDE data_0209f3c8's own 0x20 bytes. So f3c4 and f3c8 are one object
// and the run has to be contiguous, the mechanism hal/level_boot.cpp uses for
// the save block. Sizes are each symbol's own delta in config/arm9/symbols.txt:
// f3a4 +0x20, f3c4 +4, f3c8 +0x20, f3e8 +0x24.
#define MMBLK(sec, name, size)                                    \
    __pragma(section(sec, read, write))                           \
    extern "C" __declspec(allocate(sec)) __declspec(align(4))     \
    unsigned char name[size] = {0}

MMBLK(".mmblk$0000", data_0209f3a4, 0x20);   /* 8 Obj* -- the red-coin markers */
MMBLK(".mmblk$0001", data_0209f3c4, 0x04);
MMBLK(".mmblk$0002", data_0209f3c8, 0x20);
MMBLK(".mmblk$0003", data_0209f3e8, 0x24);   /* 9 Obj* -- the star markers */

#undef MMBLK

extern "C" {
/* Standalone, and zero on this boot -- nothing on the castle grounds writes
   them, so the branches they gate stay off and say nothing. */
unsigned char data_0209f288;          /* "draw the second marker set" flag */
unsigned char data_0209f370[0xc];     /* 9 marker tile indices, walked by Render */
unsigned data_020a60a4;               /* GXS ext-palette: the saved VRAM bank */

/* The stylus half of the Ctrl block. On the DS these are bytes 0x10 and 0x11
   of each 0x18-byte record and the readers index them [player * 0x18], which
   is why they are sized for the whole four-record block rather than one byte.
   The port keeps them as separate storage the way hal/actor_vtables.cpp
   already does for data_0209f4ac / data_0209f4ae. Nothing on the host writes
   them: the host stylus goes to data_020a0de8, so the recentre-the-minimap
   branch Minimap::Behavior gates on data_0209f4ac stays shut and these are
   never read for a value that matters. */
unsigned char data_0209f4a8[0x60];
unsigned char data_0209f4a9[0x60];
}

// ---- the faces --------------------------------------------------------------
//
// HUD::Render calls its own leaves by their ITANIUM C name with `this` spelled
// out as an argument -- `_ZN3HUD17RenderHealthMeterEv((void *)this)` -- while
// every one of those leaves is defined in its own TU as a real MSVC
// __thiscall member. A linker alias would hand a __thiscall body an ecx that
// never held `this`, so each needs a real face. Same mechanism as
// hal/method_faces.cpp.
#define HUD_FACE(sym, meth)                                                   \
    extern "C" void sym(void *s) { ((HUD *)s)->HUD::meth(); }

/* Only the leaves whose own TU defines a __thiscall MEMBER. The other four --
   RenderStarCount, RenderSilverStars, RenderRedCoins, RenderVsTimer -- define
   the C name themselves, so a face here would be a duplicate symbol. Which of
   the two shapes a leaf has is not a pattern to guess at: it is whichever the
   decomp recovered, and the linker names the ones that disagree. */
HUD_FACE(_ZN3HUD15RenderCoinCountEv, RenderCoinCount)
HUD_FACE(_ZN3HUD15RenderLifeCountEv, RenderLifeCount)
HUD_FACE(_ZN3HUD15RenderTimeTimerEv, RenderTimeTimer)
HUD_FACE(_ZN3HUD17RenderHealthMeterEv, RenderHealthMeter)
HUD_FACE(_ZN3HUD17UpdateHealthMeterEv, UpdateHealthMeter)

#undef HUD_FACE

/* RenderCameraButtons is a STATIC member (no `this` at all -- it draws the two
   arrows and the zoom button from OAM's own templates), so its face takes and
   passes nothing. */
extern "C" void _ZN3HUD19RenderCameraButtonsEv(void *) { HUD::RenderCameraButtons(); }

/* Player::IsInsideOfCannon is a real MSVC member, and HUD::RenderHealthMeter
   declares it as a C++ FREE function taking void* -- so this face has to be a
   C++ free function of exactly that name, not an extern "C" one. */
struct Player { int IsInsideOfCannon(); int Unk_020ca8f8(); };
int _ZN6Player16IsInsideOfCannonEv(void *s)
{ return ((Player *)s)->Player::IsInsideOfCannon(); }

/* Minimap::Behavior reaches the same class the same way: it calls
   Player::Unk_020ca8f8 -- "is he in a state that hides the minimap" -- as a
   free C function while its own TU defines a real MSVC member. */
extern "C" int _ZN6Player12Unk_020ca8f8Ev(void *s)
{ return ((Player *)s)->Player::Unk_020ca8f8(); }

/* Stage::RenderBouncingArrows draws from an OamAttr template at ov001
   0x020abd88 that config does not name, and nothing in emitted DATA points at
   it, so ovdata cannot pull it in either -- naming it is a config change.
   HUD::Render reaches this only when data_0209f284 is set, which is zeroed bss
   on the port's boot. Stubbed by name, and it says so if that ever changes. */
extern "C" void _ZN5Stage20RenderBouncingArrowsEv(void)
{
    static int said;
    if (!said++)
        std::printf("  [hud] Stage::RenderBouncingArrows reached: its sprite "
                    "template at ov001 0x020abd88 is unnamed in config\n");
}

/* ...and the reverse. CalculateDigits' TU is a .c file, so it defines the C
   name, while RenderCoinCount and RenderLifeCount call it as a member. */
extern "C" void _ZN3HUD15CalculateDigitsEt(void *self, unsigned short n);
void HUD::CalculateDigits(unsigned short n)
{
    _ZN3HUD15CalculateDigitsEt(this, n);
}

namespace {

int g_trap_slot;
const char *const kSlotName[18] = {
    "InitResources", "BeforeInitResources", "AfterInitResources",
    "CleanupResources", "BeforeCleanupResources", "AfterCleanupResources",
    "Behavior", "BeforeBehavior", "AfterBehavior",
    "Render", "BeforeRender", "AfterRender",
    "OnPendingDestroy", "Virtual34", "Virtual38", "OnHeapCreated",
    "~D1", "~D0"};

int __fastcall sa_trap(void *s, void *)
{
    std::fprintf(stderr, "FATAL: %s vtable slot %d (%s) is not hosted\n",
                 port_actor_class_name(
                     s ? *(unsigned short *)((char *)s + 0xc) : 0u),
                 g_trap_slot, kSlotName[g_trap_slot & 17]);
    std::abort();
    return 0;
}

/* The ten shared slots, every one of them reached QUALIFIED so the call cannot
   re-dispatch through the vtable it is filling. ActorBase throughout, except
   slot 2. */
int __fastcall sa_binit(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::BeforeInitResources(); }
void __fastcall sa_ainit(void *s, void *, unsigned a)
{ ((ActorDerived *)s)->ActorDerived::AfterInitResources(a); }
int __fastcall sa_bclean(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::BeforeCleanupResources(); }
void __fastcall sa_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
int __fastcall sa_bbeh(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::BeforeBehavior(); }
void __fastcall sa_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
int __fastcall sa_bren(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::BeforeRender(); }
void __fastcall sa_aren(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterRender(a); }
int __fastcall sa_heap(void *s, void *)
{ return ((ActorBase *)s)->ActorBase::OnHeapCreated(); }
int __fastcall sa_trap13(void *s, void *) { g_trap_slot = 13; return sa_trap(s, 0); }
int __fastcall sa_trap14(void *s, void *) { g_trap_slot = 14; return sa_trap(s, 0); }

void sa_fill_shared(void **vt)
{
    for (int i = 0; i < 18; ++i) vt[i] = (void *)sa_trap;
    vt[1] = (void *)sa_binit;
    vt[2] = (void *)sa_ainit;
    vt[4] = (void *)sa_bclean;
    vt[5] = (void *)sa_aclean;
    vt[7] = (void *)sa_bbeh;
    vt[8] = (void *)sa_abeh;
    vt[10] = (void *)sa_bren;
    vt[11] = (void *)sa_aren;
    vt[13] = (void *)sa_trap13;
    vt[14] = (void *)sa_trap14;
    vt[15] = (void *)sa_heap;
}

// ---- HUD -------------------------------------------------------------------
int __fastcall hud_init(void *s, void *) { return ((HUD *)s)->HUD::InitResources(); }
int __fastcall hud_clean(void *s, void *) { return ((HUD *)s)->HUD::CleanupResources(); }
int __fastcall hud_behavior(void *s, void *) { return ((HUD *)s)->HUD::Behavior(); }
int __fastcall hud_render(void *s, void *)
{
    /* SM64DS_HUD_TRACE=1: the two globals every HUD render leaf dereferences
       before it draws anything -- the per-player Actor table and the local
       player index. A null there faults inside a matched leaf at a small
       offset, which reads as a codegen bug and is not one. */
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_HUD_TRACE") != 0;
    if (on) {
        static int said;
        if (said < 3) {
            ++said;
            std::printf("  [hud] render: this=%p f394[0]=%p f250=%u "
                        "caa0[8]=%02x f2d8=%u f2c4|f20c|f294=%02x oam=%d\n", s,
                        (void *)data_0209f394[0], (unsigned)data_0209f250,
                        (unsigned)((unsigned char *)data_0209caa0)[8],
                        (unsigned)data_0209f2d8,
                        (unsigned)(data_0209f2c4 | data_0209f20c | data_0209f294),
                        port_sub_oam_nonzero());
        }
    }
    return ((HUD *)s)->HUD::Render();
}
void __fastcall hud_pdes(void *s, void *) { ((HUD *)s)->HUD::OnPendingDestroy(); }
void *__fastcall hud_d1(void *s, void *) { return _ZN3HUDD1Ev(s); }
void *__fastcall hud_d0(void *s, void *) { return _ZN3HUDD0Ev(s); }

// ---- Minimap ---------------------------------------------------------------
int __fastcall map_init(void *s, void *) { return ((Minimap *)s)->Minimap::InitResources(); }
int __fastcall map_clean(void *s, void *) { return ((Minimap *)s)->Minimap::CleanupResources(); }
int __fastcall map_behavior(void *s, void *) { return _ZN7Minimap8BehaviorEv(s); }
int __fastcall map_render(void *s, void *)
{
    /* SM64DS_MM_TRACE=1: the marker's own minimap coordinates and the live sub
       OAM count. A minimap that draws its map and no marker reads as a Render
       bug and is usually a sprite-template one -- see port_mm_icons_patch. */
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_MM_TRACE") != 0;
    if (on) {
        static int n;
        if (n < 2 || (n % 120) == 0) {
            char *b = (char *)s;
            std::printf("  [mm] render #%d: marker=(%d,%d) mode=%u hidden=%u "
                        "scale=%d oam=%d\n", n,
                        ((int *)(b + 0x70))[0], ((int *)(b + 0x80))[0],
                        (unsigned)*(unsigned char *)(b + 0x251),
                        (unsigned)*(unsigned char *)(b + 0x255),
                        ((int *)(b + 0x214))[0], port_sub_oam_nonzero());
            std::fflush(stdout);
        }
        ++n;
    }
    return ((Minimap *)s)->Minimap::Render();
}
void __fastcall map_pdes(void *s, void *) { ((Minimap *)s)->Minimap::OnPendingDestroy(); }
void *__fastcall map_d1(void *s, void *) { return _ZN7MinimapD1Ev(s); }
void *__fastcall map_d0(void *s, void *) { return _ZN7MinimapD0Ev(s); }

}  // namespace

extern "C" void hal_fill_hud_vtable(void)
{
    void **vt = _ZTV8dMeter_c;
    sa_fill_shared(vt);
    vt[0] = (void *)hud_init;
    vt[3] = (void *)hud_clean;
    vt[6] = (void *)hud_behavior;
    vt[9] = (void *)hud_render;
    vt[12] = (void *)hud_pdes;
    vt[16] = (void *)hud_d1;
    vt[17] = (void *)hud_d0;
    /* the base table is never dispatched through, but a null slot in it would
       be indistinguishable from a bug if one ever were */
    for (int i = 0; i < 18; ++i) _ZTV7dBase_c[i] = (void *)sa_trap;

    /* the sprite templates the render leaves index, and the pointer pass that
       puts OAM::NUMBERS' ten digit pointers on host addresses */
    port_ov001_syms_patch();
    port_ov001_pack_check();
}

// ---- the one cross-overlay pointer table -----------------------------------
//
// OAM::MM_PLAYER_ICONS is sixteen pointers in ov002 (0x0210c174) and every one
// of them points into ov001 -- the sixteen MM_*_ICON OamAttr records at
// 0x020ab800..0x020ab928, four per character times four cap states.
//
// ovdata's pointer pass rewrites pointers whose TARGET is inside the same
// mount, which is why the other three minimap tables need nothing here:
// MM_VS_PLAYER_ICONS, MM_VS_PLAYER_ICONS_S and the two star-marker tables at
// 0x0210c748 / 0x0210cac8 all point inside ov002 and come out already rebased.
// This one table crosses, so the pass leaves DS addresses in it and
// OAM::RenderSub is handed 0x020ab800 -- which reads as "the minimap draws its
// map and no player marker", because the OAM entry it builds out of whatever
// is at that host address is not a sprite.
//
// The order is the ROM's own, read out of overlay_0002.bin rather than guessed
// from the names: the table is indexed `character + capState * 4`, and the
// character order inside each group of four is not the order the symbol names
// are declared in.
extern "C" {
extern unsigned char _ZN3OAM13MM_MARIO_ICONE[];
extern unsigned char _ZN3OAM19MM_MARIO_W_CAP_ICONE[];
extern unsigned char _ZN3OAM19MM_MARIO_L_CAP_ICONE[];
extern unsigned char _ZN3OAM20MM_MARIO_NO_CAP_ICONE[];
extern unsigned char _ZN3OAM13MM_LUIGI_ICONE[];
extern unsigned char _ZN3OAM19MM_LUIGI_W_CAP_ICONE[];
extern unsigned char _ZN3OAM19MM_LUIGI_M_CAP_ICONE[];
extern unsigned char _ZN3OAM20MM_LUIGI_NO_CAP_ICONE[];
extern unsigned char _ZN3OAM13MM_WARIO_ICONE[];
extern unsigned char _ZN3OAM19MM_WARIO_L_CAP_ICONE[];
extern unsigned char _ZN3OAM19MM_WARIO_M_CAP_ICONE[];
extern unsigned char _ZN3OAM20MM_WARIO_NO_CAP_ICONE[];
extern unsigned char _ZN3OAM13MM_YOSHI_ICONE[];
extern unsigned char _ZN3OAM19MM_YOSHI_L_CAP_ICONE[];
extern unsigned char _ZN3OAM19MM_YOSHI_M_CAP_ICONE[];
extern unsigned char _ZN3OAM19MM_YOSHI_W_CAP_ICONE[];
}

static void port_mm_icons_patch(void)
{
    void *const src[16] = {
        _ZN3OAM13MM_MARIO_ICONE,        /* 020ab800 */
        _ZN3OAM19MM_MARIO_L_CAP_ICONE,  /* 020ab820 */
        _ZN3OAM19MM_MARIO_W_CAP_ICONE,  /* 020ab808 */
        _ZN3OAM20MM_MARIO_NO_CAP_ICONE, /* 020ab838 */
        _ZN3OAM19MM_LUIGI_M_CAP_ICONE,  /* 020ab870 */
        _ZN3OAM13MM_LUIGI_ICONE,        /* 020ab850 */
        _ZN3OAM19MM_LUIGI_W_CAP_ICONE,  /* 020ab858 */
        _ZN3OAM20MM_LUIGI_NO_CAP_ICONE, /* 020ab888 */
        _ZN3OAM19MM_WARIO_M_CAP_ICONE,  /* 020ab8d0 */
        _ZN3OAM19MM_WARIO_L_CAP_ICONE,  /* 020ab8b8 */
        _ZN3OAM13MM_WARIO_ICONE,        /* 020ab8a0 */
        _ZN3OAM20MM_WARIO_NO_CAP_ICONE, /* 020ab8a8 */
        _ZN3OAM19MM_YOSHI_M_CAP_ICONE,  /* 020ab908 */
        _ZN3OAM19MM_YOSHI_L_CAP_ICONE,  /* 020ab8f0 */
        _ZN3OAM19MM_YOSHI_W_CAP_ICONE,  /* 020ab920 */
        _ZN3OAM13MM_YOSHI_ICONE,        /* 020ab8e8 */
    };
    for (int i = 0; i < 16; ++i)
        _ZN3OAM15MM_PLAYER_ICONSE[i] = src[i];
}

extern "C" void hal_fill_minimap_vtable(void)
{
    void **vt = _ZTV6dMap_c;
    sa_fill_shared(vt);
    vt[0] = (void *)map_init;
    vt[3] = (void *)map_clean;
    vt[6] = (void *)map_behavior;
    vt[9] = (void *)map_render;
    vt[12] = (void *)map_pdes;
    vt[16] = (void *)map_d1;
    vt[17] = (void *)map_d0;
    port_mm_icons_patch();
}

// ---- the ov001 name aliases -------------------------------------------------
//
// The HUD's render TUs name five sprite tables by address because dsd could
// not attribute them: ov000 and ov001 share the base 0x020aa420, and ov000
// holds filename strings where ov001 holds OamAttr records. config names all
// five in ov001, and both names are DATA at the same address, so each alias is
// exact -- no calling convention to get wrong.
#pragma comment(linker, "/alternatename:_func_020ab948=__ZN3OAM10LIFE_ICONSE")
#pragma comment(linker, "/alternatename:_func_020ab9c8=__ZN3OAM5TIMESE")
#pragma comment(linker, "/alternatename:_func_020aba70=__ZN3OAM7NUMBERSE")
#pragma comment(linker, "/alternatename:_func_020abad0=__ZN3OAM10POWER_STARE")
#pragma comment(linker, "/alternatename:_func_020abad8=__ZN3OAM4COINE")
/* the same table under its ov000-prefixed misattribution, which one TU uses */
#pragma comment(linker, "/alternatename:_data_ov000_020aba70=__ZN3OAM7NUMBERSE")
#pragma comment(linker, "/alternatename:_data_ov000_020ab9c8=__ZN3OAM5TIMESE")

// ---- C++-mangled references onto C-named definitions -----------------------
//
// The HUD's leaves declare their callees inside `namespace OAM { ... }` or as
// plain C++ free functions, so MSVC emits C++ manglings for symbols that are
// C-named everywhere else in the port. Every one of these is cdecl on both
// sides, so the alias is exact -- the mechanism hal/cxx_aliases.cpp documents.
#pragma comment(linker, "/alternatename:?Render@OAM@@YAX_NPAUOamAttr@@HHHHPAUMatrix2x2@@@Z=__ZN3OAM6RenderEbP7OamAttriiiiP9Matrix2x2")
#pragma comment(linker, "/alternatename:?RenderSub@OAM@@SAXPAUOamAttr@@HHHH@Z=__ZN3OAM9RenderSubEP7OamAttriiii")
#pragma comment(linker, "/alternatename:?GetOwnerLanguage@@YAHXZ=_GetOwnerLanguage")
#pragma comment(linker, "/alternatename:?_ZN5Timer7GetTimeEv@@YA_KPAX@Z=__ZN5Timer7GetTimeEv")
#pragma comment(linker, "/alternatename:?LoadOBJPltt@GX@@YAXPBXII@Z=__ZN2GX11LoadOBJPlttEPKvjj")
#pragma comment(linker, "/alternatename:?LoadOBJPltt@GXS@@YAXPBXII@Z=__ZN3GXS11LoadOBJPlttEPKvjj")
#pragma comment(linker, "/alternatename:?data_0209fc9c@@3EA=_data_0209fc9c")
#pragma comment(linker, "/alternatename:?data_ov002_0210c29c@@3PAHA=_data_ov002_0210c29c")
#pragma comment(linker, "/alternatename:?data_ov002_0210c310@@3PAFA=_data_ov002_0210c310")
#pragma comment(linker, "/alternatename:?data_ov002_02111178@@3EA=_data_ov002_02111178")
#pragma comment(linker, "/alternatename:?GiveLives@@YAXH@Z=_GiveLives")

/* The same for the Minimap's: its InitResources declares these at file scope
   OUTSIDE the extern "C" block, so MSVC asks for a C++ mangling of storage the
   rest of the port owns under the C name. */
#pragma comment(linker, "/alternatename:?data_0209f2e8@@3EA=_data_0209f2e8")
#pragma comment(linker, "/alternatename:?data_0209f334@@3PAGA=_data_0209f334")
#pragma comment(linker, "/alternatename:?data_0209f394@@3PAPAXA=_data_0209f394")
#pragma comment(linker, "/alternatename:?data_0209d454@@3EA=_data_0209d454")
#pragma comment(linker, "/alternatename:?data_ov002_02111148@@3CA=_data_ov002_02111148")
#pragma comment(linker, "/alternatename:?data_ov002_02111150@@3EA=_data_ov002_02111150")
#pragma comment(linker, "/alternatename:?data_ov002_0211064c@@3UState@@A=_data_ov002_0211064c")
#pragma comment(linker, "/alternatename:?data_ov002_02110664@@3UState@@A=_data_ov002_02110664")
#pragma comment(linker, "/alternatename:?GetBG3CharPtr@G2S@@YAPAXXZ=__ZN3G2S13GetBG3CharPtrEv")
#pragma comment(linker, "/alternatename:?GetBit@Event@@SAHI@Z=__ZN5Event6GetBitEj")
#pragma comment(linker, "/alternatename:?SublevelToLevel@@YAHH@Z=_SublevelToLevel")

/* OAM's camera-button templates are static DATA members of class OAM in the
   TU that draws them, and ov002 data named _ZN3OAM..E in the mount. */
#pragma comment(linker, "/alternatename:?CAM_BUTTON_L@OAM@@2UOamAttr@@A=__ZN3OAM12CAM_BUTTON_LE")
#pragma comment(linker, "/alternatename:?CAM_BUTTON_R@OAM@@2UOamAttr@@A=__ZN3OAM12CAM_BUTTON_RE")
#pragma comment(linker, "/alternatename:?CAM_BUTTON_L_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM20CAM_BUTTON_L_PRESSEDE")
#pragma comment(linker, "/alternatename:?CAM_BUTTON_R_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM20CAM_BUTTON_R_PRESSEDE")
#pragma comment(linker, "/alternatename:?S_CAM_BUTTON_L@OAM@@2UOamAttr@@A=__ZN3OAM14S_CAM_BUTTON_LE")
#pragma comment(linker, "/alternatename:?S_CAM_BUTTON_R@OAM@@2UOamAttr@@A=__ZN3OAM14S_CAM_BUTTON_RE")
#pragma comment(linker, "/alternatename:?S_CAM_BUTTON_L_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM22S_CAM_BUTTON_L_PRESSEDE")
#pragma comment(linker, "/alternatename:?S_CAM_BUTTON_R_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM22S_CAM_BUTTON_R_PRESSEDE")
#pragma comment(linker, "/alternatename:?CAM_ZOOM_BUTTON@OAM@@2UOamAttr@@A=__ZN3OAM15CAM_ZOOM_BUTTONE")
#pragma comment(linker, "/alternatename:?CAM_ZOOM_BUTTON_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM23CAM_ZOOM_BUTTON_PRESSEDE")
#pragma comment(linker, "/alternatename:?S_CAM_ZOOM_BUTTON@OAM@@2UOamAttr@@A=__ZN3OAM17S_CAM_ZOOM_BUTTONE")
#pragma comment(linker, "/alternatename:?S_CAM_ZOOM_BUTTON_PRESSED@OAM@@2UOamAttr@@A=__ZN3OAM25S_CAM_ZOOM_BUTTON_PRESSEDE")
