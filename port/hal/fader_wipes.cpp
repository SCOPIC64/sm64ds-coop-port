// The fader-wipe array (WIPES / data_0209f324), staged for the host.
//
// Stage::InitResources builds the wipes at boot:
//
//     data_0209f324 = func_02073470(7, 0x60, 8, FaderWipe::FaderWipe,
//                                               FaderWipe::~FaderWipe);
//
// seven 0x60-byte FaderWipe objects on the stage heap, each carrying the
// ROM's _ZTV9FaderWipe. The port never boots a Stage, so that pointer was
// plain zero, and the first death took the null through
// KillPlayer -> StartExitFaderWipe -> &WIPES[4] -> a virtual call on
// address 0x180. That fault is why gate 1 excluded the wipe and why the
// tier-2 wave held the four death states out. The same null sat under
// St_Respawn_Main, which calls FUN_02029980 -> WIPES + 0x240.
//
// What is staged here is the array and its vtable, not the wipe itself.
// FaderWipe's fade is a ModelComponents::Render of a mesh that
// FaderWipe::LoadAndSetFile pulls from the stage filesystem, which the
// port does not mount, so nothing draws. The interpolator the rest of the
// engine polls IS real, and since wave 3 it is not merely mirrored: the five
// matched bodies that own it are CALLED from the slots below -- see the
// delegation note after the slot table. AdvanceFade keeps one deliberate
// divergence: an UNDRIVEN advance snaps currInterp to its target rather than
// stepping, because a fade nobody is driving must not hold a scene transition
// open for 30 frames, while port_fader_advance's driven advance steps for real
// so the fade renders.
//
// ---- SLOT ORDER: the ROM's, not MSVC's ------------------------------------
//
// The wipe is reached two different ways in src, and the two disagree:
//
//   raw-offset view   Scene::SetFaders, FUN_02029980 and FUN_020299f4 hand-
//                     write a struct of function pointers over the vtable
//                     word and index it at ROM byte offsets.
//   class view        StartExitFaderWipe.c / StartEntranceFaderWipe.c declare
//                     a `struct FaderWipe : Fader` with nine virtuals and let
//                     the compiler pick the slot.
//
// mwcc emits TWO destructor slots (D1 at 0x00, D0 at 0x04); MSVC folds them
// into one. So a host-generated class vtable runs one slot ahead of the ROM
// from AdvanceFade onward. The table below is laid out in ROM order, which
// is the order the raw-offset callers need -- and they are the ones that
// matter, because two of them pass arguments (FUN_02029980 calls the 0x0c
// slot as w3(0x1e, 0)). Under a class-ordered table that call would land on
// a no-argument predicate and unbalance the __thiscall stack. The 0x0c and
// 0x10 stubs therefore take the two ints those call sites push.
//
//     host/ROM  name                   reached by
//     0x00      ~FaderWipe   (ROM D1)  nobody on host
//     0x04      (ROM D0)               nobody on host
//     0x08      AdvanceFade            the scene loop, once hosted
//     0x0c      SetBackwardTime        FUN_02029980  (respawn)
//     0x10      SetForwardTime         FUN_020299f4  (dead-pit, battle levels)
//     0x14      IsAtStart              Scene::SetFaders
//     0x18      IsAtEnd                Scene::SetFaders
//     0x1c      IsBetweenStartAndEnd   FUN_02029980, FUN_020299f4
//     0x20      SetToEnd               FUN_02029980, Scene::SetFaders,
//                                      AND StartExitFaderWipe's SetToStart
//     0x24      SetToStart             FUN_020299f4, Scene::SetFaders
//     0x28/2c   tail                   nobody; they exist so a raw-offset
//                                      read past 0x24 stays inside a table
//
// The one skew left is on the last line of that 0x20 row: StartExitFaderWipe
// compiles `f->SetToStart()` into class slot 8 = byte 0x20, which here is
// SetToEnd. The host result is that the exit wipe lands snapped to its END
// (currInterp = 0x1000) rather than its start, which is the outcome the port
// wants anyway -- IsAtEnd, the predicate a scene transition waits on, reads
// true immediately instead of waiting on a fade that can never render.
//
// ---- WHAT IS HOST HERE AND WHAT IS THE DECOMP -----------------------------
//
// The table below is a host CLASS, because the ROM slot order above is the
// whole point of it and MSVC will not emit that order for a class derived from
// the ROM's Fader. But the ARITHMETIC inside the slots is no longer host: the
// five interpolator bodies that carry it are matched TUs, and this file now
// CALLS them instead of restating them (slice_w1l3.txt seats the five):
//
//     Fader::AdvanceInterp            the one-frame 20.12 step
//     FaderBrightness::SetToStart     currInterp = 0
//     FaderBrightness::SetToEnd       currInterp = 0x1000
//     FaderBrightness::IsAtStart      currInterp == 0
//     FaderBrightness::IsAtEnd        currInterp == 0x1000
//
// The cast is a reinterpret, not a base conversion: Fader is vptr at 0x0 then
// currInterp 0x4 and speed 0x8 (include/Fader.h, pinned by the bytes), which is
// exactly HalFaderWipe's prefix, and every one of the five is reached QUALIFIED
// so no call re-dispatches through the host table.
//
// THREE SLOTS ARE DELIBERATELY NOT DELEGATED, and the reason is the MSVC dtor
// fold. FaderBrightness's matched SetForwardTime/SetBackwardTime end in a
// VIRTUAL IsAtEnd()/IsAtStart(), and IsBetweenStartAndEnd is two more. Under
// mwcc those resolve at ROM slots 5 and 6; MSVC folds D1/D0 into one slot, so
// on a HalFaderWipe -- which spends two slots keeping ROM byte order -- a
// FaderBrightness-slot-4 call lands on SetForwardTime and a slot-5 call on
// IsAtStart. Slot 4 is the dangerous one: it takes two ints under __thiscall,
// so a no-argument virtual landing there unbalances the stack. Those three stay
// host, spelled to match the matched bodies they cannot safely call.
#include <cstdio>
#include <cstdlib>
#include <new>
#include "dsstate_seg.h"
/* The ROM classes whose interpolator this file mirrors. Included for the
   LAYOUT and the method declarations, never to derive from. Pulls types.h,
   whose `typedef s32 Fix12i` is the same `int` this file used to typedef
   locally, so the local typedef is gone rather than shadowed. */
#include "FaderBrightness.h"
/* For the C-name face at the bottom of this file. FaderWipe.h's FaderWipe is
   the auto-generated NON-polymorphic view -- pad_000[4] where the vptr sits,
   no virtuals -- so including it cannot collide with HalFaderWipe below. */
#include "FaderWipe.h"

/* Fader::AdvanceInterp calls the 20.12 approach helper by its ROM spelling,
   func_0203ae58. That name is DEFINED only in hal/shims.cpp, the gate-1 smoke's
   target, which walk_window does not link -- so seating AdvanceInterp leaves it
   unresolved unless this file supplies it. The matched ApproachLinear(int&,
   int, int) IS linked here (src/_Z14ApproachLinearRiii.cpp, reached through the
   C face in hal/player_bridges.cpp), and the two agree on the cdecl signature;
   func_0203ae58 is declared void and ApproachLinear returns int, which a cdecl
   caller discards. alternatename rather than a definition, so that a target
   which DOES link shims.cpp keeps its own strong one and nothing collides. */
#pragma comment(linker, "/alternatename:_func_0203ae58=__Z14ApproachLinearRiii")

/* SetBlendBrightness, the matched G2x routine, is what both FaderColor and
   FaderWipe write the 2D master-blend register with. It is in slice_gate1, so
   reuse it -- the host mapping at 0x4000050/0x4000054 latches the write and the
   framebuffer compositor (walk_window.cpp) reads it back. */
extern "C" void _ZN3G2x18SetBlendBrightnessEPVtts(volatile unsigned short *p,
                                                  unsigned short val, short amt);

namespace {

/* Snap vs step: the historical stub snapped the interpolator to its target so
   an invisible fade could not hold a transition open. Now the fade renders, so
   it has to STEP -- but only when the frame loop is actually driving the
   advance (port_fader_advance). A direct SetToStart/SetToEnd still snaps. The
   flag is set for the duration of one driven advance. */
int g_hal_fader_stepping;

int hal_wipe_index(const void *self);

/* Loud, but not per-frame: the first few calls say what the host is
   skipping, then it goes quiet. */
void hal_wipe_note(const char *what, const void *self)
{
    static int said;
    if (said >= 8) return;
    ++said;
    std::fprintf(stderr, "  [wipe] %s on wipe %d (host stub: the "
                 "interpolator is real, no fade renders)\n",
                 what, hal_wipe_index(self));
    if (said == 8)
        std::fprintf(stderr, "  [wipe] (further wipe calls stay quiet)\n");
}

}  /* anonymous namespace */

/* ---- THE CLASS IS NAMED, AND ITS VTABLE CARRIES THE ROM'S SYMBOL ----------
   run link60 lane SL0. HalFaderWipe used to sit in the anonymous namespace,
   which made its vtable a local symbol nothing could refer to by name. It is
   at file scope now for exactly one reason: MSVC emits its vtable as
   ??_7HalFaderWipe@@6B@, and the ROM's _ZTV9FaderWipe is aliased onto it at
   the bottom of this file.

   THAT ALIAS IS ONLY LEGITIMATE BECAUSE THE TWO TABLES AGREE, and the
   agreement was MEASURED rather than assumed. _ZTV9FaderWipe is arm9
   0x0208ea9c; read out of extracted/arm9_dec.bin at (addr - 0x02004000) and
   every word resolved against config/arm9/symbols.txt, the ROM's twelve words
   are:

     0x00 FaderWipe::~FaderWipe (D1)        0x18 FaderBrightness::IsAtEnd
     0x04 FaderWipe D0                      0x1c FaderBrightness::
     0x08 FaderWipe::AdvanceFade                 IsBetweenStartAndEnd
     0x0c FaderBrightness::SetBackwardTime  0x20 FaderBrightness::SetToEnd
     0x10 FaderBrightness::SetForwardTime   0x24 FaderBrightness::SetToStart
     0x14 FaderBrightness::IsAtStart        0x28 0x00000000
                                            0x2c data_0208ea24

   which is the slot table at the head of this file, byte for byte, 0x00
   through 0x24. The ROM order this file adopted from the raw-offset callers
   is now confirmed against the ROM record itself, and the two dtor slots the
   class spends to keep that order are what make the alias sound: MSVC folds
   D1/D0 into one slot, so a class with a single virtual destructor would run
   one slot ahead of the ROM from AdvanceFade onward.

   TWO THINGS THE ROM RECORD CORRECTS, both recorded rather than acted on:

   1. 0x28 AND 0x2c ARE NOT SPARE FUNCTION SLOTS. In the ROM 0x28 is zero and
      0x2c points at data_0208ea24, and the same pair-shape ends all four
      fader records. They are the table's trailer, not virtuals. HalTail28 and
      HalTail2c put real function pointers there. That is still the safe
      choice for a raw-offset read that runs past 0x24 -- it lands on a
      callable stub instead of on 0x00000000 -- but it is a DIVERGENCE from
      the ROM and not, as the old comment had it, a faithful tail.

   2. include/Fader.h AND include/FaderBrightness.h ARE WRONG. Both declare
      IsBetweenStartAndEnd, SetToEnd and SetToStart as NON-virtual members.
      The ROM has all three in the vtable, at slots 7, 8 and 9 of all four
      fader records. That error is the source of the one skew this file
      documents at its 0x20 row. NOT FIXED HERE ON PURPOSE: include/ is byte
      gate input, mwccarm already emits two destructor slots, and adding a
      virtual to Fader shifts every slot and breaks the matched fader TUs.
      It is a decomp-side correction and it needs the byte gate, not this
      file. */
struct HalFaderWipe {
    Fix12i currInterp;
    Fix12i speed;
    unsigned short color;
    unsigned short unk0e;
    unsigned char model[0x50];

    HalFaderWipe() : currInterp(0), speed(0), color(0), unk0e(0)
    {
        for (int i = 0; i < 0x50; ++i) model[i] = 0;
    }

    virtual ~HalFaderWipe() {}                       /* 0x00  ROM D1 */
    virtual void DtorDeleting() {}                   /* 0x04  ROM D0 */
    virtual int AdvanceFade()                        /* 0x08 */
    {
        /* Driven advance (the frame loop's port_fader_advance) STEPS the
           interpolator one frame and writes the 2D master-blend register the
           way FaderColor::AdvanceFade does, so the fade renders. Any other
           caller keeps the old snap: a fade nobody is driving must not stall a
           transition. */
        if (!g_hal_fader_stepping) {
            hal_wipe_note("AdvanceFade", this);
            currInterp = speed >= 0 ? 0x1000 : 0;
            return 1;
        }
        Fix12i old = currInterp;
        Fix12i target = speed >= 0 ? 0x1000 : 0;
        /* the step itself is the matched body: it picks target and |step| from
           the sign of speed exactly as the three lines here used to */
        ((Fader *)(void *)this)->Fader::AdvanceInterp();
        if (currInterp == old)
            return currInterp == target;
        /* FaderColor::AdvanceFade, verbatim on the numbers: color != 0 means a
           WHITE fade (brightness increase), color == 0 a BLACK fade (decrease).
           r is the 5-bit EVY coefficient, 0..0x10. Both 2D engines
           (0x4000050 main, 0x4001050 sub) get it so the whole panel fades. */
        int m = color ? 0x10 : -0x10;
        int r = (currInterp * m) >> 12;
        if (r != 0) {
            _ZN3G2x18SetBlendBrightnessEPVtts(
                (volatile unsigned short *)0x4000050, 0x3f, (short)r);
            _ZN3G2x18SetBlendBrightnessEPVtts(
                (volatile unsigned short *)0x4001050, 0x3f, (short)r);
        } else {
            *(volatile unsigned short *)0x4000050 = 0;
            *(volatile unsigned short *)0x4001050 = 0;
        }
        return currInterp == target;
    }
    /* 0x0c / 0x10 -- NOT delegated: the matched bodies end in a virtual
       IsAtStart()/IsAtEnd() that would land one slot off here. See the dtor-fold
       note in the header. The arithmetic below is the matched arithmetic:
       cstd::fdiv(+-0x1000, frames << 12) is +-0x1000 / frames. */
    virtual int SetBackwardTime(int frames, int)     /* 0x0c */
    {
        speed = frames ? -(Fix12i)(0x1000 / frames) : -0x1000;
        return HalFaderWipe::IsAtStart();
    }
    virtual int SetForwardTime(int frames, int)      /* 0x10 */
    {
        speed = frames ? (Fix12i)(0x1000 / frames) : 0x1000;
        return HalFaderWipe::IsAtEnd();
    }
    /* 0x14 / 0x18 -- the matched predicates, reached qualified */
    virtual int IsAtStart()
    { return ((FaderBrightness *)(void *)this)->FaderBrightness::IsAtStart(); }
    virtual int IsAtEnd()
    { return ((FaderBrightness *)(void *)this)->FaderBrightness::IsAtEnd(); }
    virtual int IsBetweenStartAndEnd()                         /* 0x1c */
    {
        return HalFaderWipe::IsAtStart() == 0 &&
               HalFaderWipe::IsAtEnd() == 0;
    }
    virtual void SetToEnd()                                    /* 0x20 */
    {
        hal_wipe_note("SetToEnd", this);
        ((FaderBrightness *)(void *)this)->FaderBrightness::SetToEnd();
    }
    virtual void SetToStart()                                  /* 0x24 */
    {
        hal_wipe_note("SetToStart", this);
        ((FaderBrightness *)(void *)this)->FaderBrightness::SetToStart();
    }
    virtual void HalTail28() { hal_wipe_note("tail slot 0x28", this); }
    virtual void HalTail2c() { hal_wipe_note("tail slot 0x2c", this); }
};

namespace {

/* Seven, the count Stage::InitResources passes. */
HalFaderWipe hal_wipes[7];

int hal_wipe_index(const void *self)
{
    long long d = (const char *)self - (const char *)&hal_wipes[0];
    return (int)(d / (long long)sizeof(HalFaderWipe));
}

}  /* anonymous namespace */

/* ---- gate 31: the COLOR fader, 0x0209f5e8 --------------------------------
   The eighth object, and the one that is not in the WIPES array.
   Scene::SetAndStopColorFader hands &data_0209f5e8 to Scene::SetFaders, which
   installs it and, if the fader it is replacing is at one end of its travel,
   dispatches the NEW one's SetToEnd or SetToStart -- ROM bytes 0x20 and 0x24,
   the same two slots the wipes carry. The symbol was zeroed host storage in
   hal/auto_bss.cpp, so that dispatch went through a null vptr and the very
   first LoadLevel faulted there. That is why the port could LEAVE a level
   through ExitLevel (SetNextLevel writes the two words itself and never
   touches a fader) but not ENTER one through LoadLevel, which opens with
   SetAndStopColorFader.

   It is the same object as a wipe, deliberately. FaderColor and FaderWipe are
   different ROM classes, but everything the port dispatches on either of them
   is the Fader interpolator both inherit, and Scene::StartSceneFade writes the
   colour into +0xc, which is where HalFaderWipe's `color` sits. Placement-new
   rather than a plain C++ definition because the storage has to keep its C
   name and its address. */
DSSTATE_BEGIN
extern "C" __declspec(align(8)) unsigned char
    data_0209f5e8[sizeof(HalFaderWipe)] = {0};
DSSTATE_END

namespace {
struct HalColorFaderInit {
    HalColorFaderInit() { new (data_0209f5e8) HalFaderWipe(); }
};
HalColorFaderInit hal_color_fader_init;
}  /* anonymous namespace */

extern "C" {
DSSTATE_BEGIN
/* 0x0209f324: the pointer Stage::InitResources fills and every wipe caller
   derefs. WIPES is what the two fader-wipe entry points call the same
   word. */
void *data_0209f324 = &hal_wipes[0];

/* 0x0209f5bc: the fader currently installed. Scene::SetFaders reads it
   before overwriting it, and FUN_02029980 / FUN_020299f4 deref it with no
   null check at all -- the respawn path reaches the second one -- so it
   needs a real object from frame zero, not only after the first wipe
   starts. Wipe 0 is an arbitrary but valid choice: it sits at currInterp
   = 0, which is what "no fade in progress" looks like. */
void *data_0209f5bc = &hal_wipes[0];
DSSTATE_END
}
#pragma comment(linker, "/alternatename:_WIPES=_data_0209f324")

/* ---- _ZTV9FaderWipe: the ROM's vtable SYMBOL, onto the host's table -------
   run link60 lane SL0. This is the one symbol the whole fader half of
   Stage::InitResources' closure was blocked on, and it is the last one:
   measured by compiling _ZN9FaderWipeC1Ev.c, _ZN9FaderWipeD1Ev.c,
   _ZN5ColorD1Ev.c and _ZN9FaderWipe14LoadAndSetFileEt.cpp with walk_window's
   own flags and checking every undefined external of the four against
   walk_window.map, the set wants exactly ONE name that the image does not
   already have, and it is this one.

   port/stage_lifecycle_map.txt section 4 sizes the fader wall at six symbols.
   THREE OF THOSE SIX HAVE SINCE CLOSED and the map is corrected here with the
   measurement that corrects it:

     _ZN5ColorD1Ev    ALREADY LINKED. The map lists it as "a matched TU that
                      exists in src/ and is on no active slice". It is on
                      port/slice_ov007.txt:678 and it is in the image at
                      walk_window.map:9986 (_ZN5ColorD1Ev.c.obj). Adding it to
                      a slice here would be an LNK2005, not a gain.
     data_0208eafc    ALL THREE ALREADY HOSTED, by hal/scene_boot.cpp:683-697,
     data_0208eacc    which stages them for func_02017278. The FaderWipe ctor
     data_0208eb2c    writes all three into the vptr on its way to this table,
                      so the ctor's closure is already paid.

   WHAT THE ALIAS CLAIMS, precisely. ??_7HalFaderWipe@@6B@ is a REAL vtable
   MSVC emits for a real class, and the class was laid out in the ROM's slot
   order on purpose -- two destructor slots so that AdvanceFade lands at byte
   0x08 rather than 0x04. That order is no longer inferred from the callers:
   the ROM record at 0x0208ea9c was read out of extracted/arm9_dec.bin and
   resolved word by word (the table is in the block above HalFaderWipe), and
   it agrees with this class byte for byte from 0x00 through 0x24.

   WHAT IT DOES NOT CLAIM. The two trailing words differ: the ROM has
   0x00000000 at 0x28 and a pointer to data_0208ea24 at 0x2c, where this table
   has two callable stubs. Nothing in the port or in src reads past 0x24, so
   the difference is unobservable today; it is a divergence and it is written
   down rather than papered over.

   The alias, not a definition, so a target that one day compiles a real
   FaderWipe vtable keeps its own strong symbol and nothing collides. */
#pragma comment(linker, "/alternatename:__ZTV9FaderWipe=??_7HalFaderWipe@@6B@")

/* ---- FaderWipe::LoadAndSetFile, the C-name face --------------------------
   run link60 lane SL0, and the fifth of slot 0's five faces.
   src/engine/fader/_ZN9FaderWipe14LoadAndSetFileEt.cpp compiles the real C++
   method and publishes ?LoadAndSetFile@FaderWipe@@QAEXG@Z; Stage::InitResources
   declares and calls the ROM's C name.

   A FACE AND NOT AN /alternatename, and the difference is an ABI bug rather
   than a style preference. The MSVC method is __thiscall -- receiver in ECX --
   while the caller declares it
       void _ZN9FaderWipe14LoadAndSetFileEt(int thiz, u16 fileID);
   (src/_ZN5Stage13InitResourcesEv.cpp:127), a plain cdecl function with the
   receiver as its first STACK argument. Aliasing one onto the other would run
   the method with a garbage `this` out of ECX. hal/method_faces.cpp's
   Model::UpdateFileOffsets row is the same call and the same resolution. */
extern "C" void _ZN9FaderWipe14LoadAndSetFileEt(void *thiz, unsigned short fileID)
{ ((FaderWipe *)thiz)->FaderWipe::LoadAndSetFile(fileID); }

/* ---- the per-frame fade driver -------------------------------------------
   func_02018ec0 is the ROM's own per-frame fade advance: it reads
   data_0209d4b0 -- the fader CURRENTLY IN MOTION, distinct from data_0209f5bc
   the installed one -- and calls its AdvanceFade (vtable slot 0x08). The port
   does not run func_02018ec0 (the port has its own frame loop, not
   func_020197b8), so nothing advanced a fade. This is that driver, called from
   walk_window.cpp's frame loop right where phase 2 (func_02019390) sits.

   data_0209d4b0 is an int[8] in hal/cxx_aliases.cpp; its first word holds the
   animating-fader pointer. When a fade reaches its resting end the driver
   clears it, exactly as FUN_02029934 / FUN_02029a68 do, so the Stage::Behavior
   gates that test `data_0209d4b0 == 0` come back true. */
extern "C" {
extern int data_0209d4b0[8];

/* The animating fader, spelled through the same int[8] Stage::Behavior reads.
   Nonzero while a fade is stepping. */
static HalFaderWipe *port_fader_animating(void)
{
    return (HalFaderWipe *)(size_t)data_0209d4b0[0];
}

void port_fader_advance(void)
{
    HalFaderWipe *f = port_fader_animating();
    if (!f)
        return;
    g_hal_fader_stepping = 1;
    int at_target = f->AdvanceFade();
    g_hal_fader_stepping = 0;
    /* Settled: drop it out of motion so the next transition's gates open, and
       leave the blend register at 0 when the fade landed fully OPEN (interp 0),
       the way FUN_02029934 clears 0x4000050 at the end of a fade-in. */
    if (at_target && f->currInterp == 0) {
        data_0209d4b0[0] = 0;
        *(volatile unsigned short *)0x4000050 = 0;
        *(volatile unsigned short *)0x4001050 = 0;
    } else if (at_target) {
        data_0209d4b0[0] = 0;
    }
}

/* Start a COLOR fade on the installed color fader (data_0209f5e8) and put it in
   motion. frames is the fade length; toEnd != 0 fades toward interp 1.0 (screen
   fully covered -- a fade-OUT to color), toEnd == 0 fades toward 0.0 (fade-IN,
   screen clears). color is the fade colour word: 0 = black, nonzero = white.
   This is the port's explicit stand-in for the Stage/Scene actor machinery that
   would otherwise arm data_0209d4b0. */
void port_fader_start_color(int frames, int toEnd, unsigned short color)
{
    HalFaderWipe *f = (HalFaderWipe *)(void *)data_0209f5e8;
    f->color = color;
    if (toEnd) {
        f->currInterp = 0;
        f->speed = frames > 0 ? (Fix12i)(0x1000 / frames) : 0x1000;
    } else {
        f->currInterp = 0x1000;
        f->speed = frames > 0 ? -(Fix12i)(0x1000 / frames) : -0x1000;
    }
    /* install it as the scene's fader and arm it as the animating one */
    data_0209f5bc = f;
    data_0209d4b0[0] = (int)(size_t)f;
    std::fprintf(stderr, "[fade] color start: frames=%d dir=%s color=%s "
                 "interp=%d speed=%d\n", frames, toEnd ? "out" : "in",
                 color ? "white" : "black", f->currInterp, f->speed);
}

/* Is a fade in motion right now, and how far along is it? Returns 1 and fills
   *evy (0..16, the EVY coefficient the compositor darkens/brightens by) and
   *toWhite (nonzero for a white fade, zero for black) when a fade is being
   driven; 0 otherwise. Reads the blend registers the advance wrote, so the
   compositor sees exactly what the ROM's own hardware path produced. */
int port_fader_blend_state(int *evy, int *toWhite)
{
    unsigned short bldcnt = *(volatile unsigned short *)0x4000050;
    unsigned short bldy = *(volatile unsigned short *)0x4000054;
    int mode = (bldcnt >> 6) & 3;            /* 2 = brighten, 3 = darken */
    if (mode != 2 && mode != 3)
        return 0;
    int e = bldy & 0x1f;
    if (e > 16) e = 16;
    if (e == 0)
        return 0;
    if (evy) *evy = e;
    if (toWhite) *toWhite = (mode == 2);
    return 1;
}
}  /* extern "C" */
