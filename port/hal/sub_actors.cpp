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
//     0  020fda04  HUD::InitResources          9  020fd5e0  HUD::Render
//     1  02043c78  Actor::BeforeInitResources 10  02043ac8  Actor::BeforeRender
//     2  02013ef4  Actor::AfterInitResources  11  02043ac4  AfterRender
//     3  020fd5d4  HUD::CleanupResources      12  020fd5dc  HUD::OnPendingDestroy
//     4  02043bac  BeforeCleanupResources     13  0204357c  ActorBase::Virtual34
//     5  02043b2c  AfterCleanupResources      14  0204349c  ActorBase::Virtual38
//     6  020fd7a4  HUD::Behavior              15  02043494  OnHeapCreated
//     7  02043afc  Actor::BeforeBehavior      16  020fb8f8  ~HUD (D1)
//     8  02043af8  AfterBehavior              17  020fb928  ~HUD (D0)
//
// and slot 18 is not a function pointer at all. Every one of those addresses
// resolves to the name above in config/arm9/overlays/ov002/symbols.txt, so
// this is read out of the ROM rather than inferred from the header -- which
// matters here: filling slots 18 and 19 the way the twenty-slot classes do
// would write past the table.
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

extern "C" {
/* the shared half, exactly as hal/actor_classes.cpp reaches them */
int _ZN5Actor19BeforeInitResourcesEv(void *self);
void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a);
int _ZN5Actor14BeforeBehaviorEv(void *self);
int _ZN5Actor12BeforeRenderEv(void *self);

/* HUD's own C-named halves: the two destructors */
void *_ZN3HUDD1Ev(void *self);
void *_ZN3HUDD0Ev(void *self);

/* vtable storage both constructors install by name (declared in
   include/decl_common.h, defined nowhere until now) */
void *_ZTV7dBase_c[18];
void *_ZTV8dMeter_c[18];

const char *port_actor_class_name(unsigned id);
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
struct Player { int IsInsideOfCannon(); };
int _ZN6Player16IsInsideOfCannonEv(void *s)
{ return ((Player *)s)->Player::IsInsideOfCannon(); }

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

/* the ten slots that are the same functions in both classes */
int __fastcall sa_binit(void *s, void *)
{ return _ZN5Actor19BeforeInitResourcesEv(s); }
void __fastcall sa_ainit(void *s, void *, unsigned a)
{ _ZN5Actor18AfterInitResourcesEj(s, a); }
int __fastcall sa_bclean(void *s, void *)
{ return ((Actor *)s)->Actor::BeforeCleanupResources(); }
void __fastcall sa_aclean(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterCleanupResources(a); }
int __fastcall sa_bbeh(void *s, void *)
{ return _ZN5Actor14BeforeBehaviorEv(s); }
void __fastcall sa_abeh(void *s, void *, unsigned a)
{ ((ActorBase *)s)->ActorBase::AfterBehavior(a); }
int __fastcall sa_bren(void *s, void *)
{ return _ZN5Actor12BeforeRenderEv(s); }
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
            std::printf("  [hud] render: this=%p f394[0]=%p f250=%u\n", s,
                        (void *)data_0209f394[0], (unsigned)data_0209f250);
        }
    }
    return ((HUD *)s)->HUD::Render();
}
void __fastcall hud_pdes(void *s, void *) { ((HUD *)s)->HUD::OnPendingDestroy(); }
void *__fastcall hud_d1(void *s, void *) { return _ZN3HUDD1Ev(s); }
void *__fastcall hud_d0(void *s, void *) { return _ZN3HUDD0Ev(s); }

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
