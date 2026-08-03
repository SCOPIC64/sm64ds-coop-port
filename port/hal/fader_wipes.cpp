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
// engine polls IS real: the four functions that own it
// (FaderBrightness::SetToStart / SetToEnd / IsAtStart / IsAtEnd) are
// three-line matched bodies and mirroring them costs nothing. AdvanceFade
// is the one deliberate divergence -- it snaps currInterp to its target
// instead of stepping, because a fade nobody can see should not hold a
// scene transition open for 30 frames.
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
#include <cstdio>

typedef int Fix12i;

namespace {

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

/* Layout mirrors the callers' FaderWipe: vptr, currInterp, speed, color,
   unk0e, then the 0x50-byte Model -- 0x60 total, the stride &WIPES[i]
   uses. Virtuals are declared in ROM slot order (see the table above). */
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
        hal_wipe_note("AdvanceFade", this);
        currInterp = speed >= 0 ? 0x1000 : 0;
        return 1;
    }
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
    virtual int IsAtStart() { return currInterp == 0; }        /* 0x14 */
    virtual int IsAtEnd() { return currInterp == 0x1000; }     /* 0x18 */
    virtual int IsBetweenStartAndEnd()                         /* 0x1c */
    {
        return HalFaderWipe::IsAtStart() == 0 &&
               HalFaderWipe::IsAtEnd() == 0;
    }
    virtual void SetToEnd()                                    /* 0x20 */
    {
        hal_wipe_note("SetToEnd", this);
        currInterp = 0x1000;
    }
    virtual void SetToStart()                                  /* 0x24 */
    {
        hal_wipe_note("SetToStart", this);
        currInterp = 0;
    }
    virtual void HalTail28() { hal_wipe_note("tail slot 0x28", this); }
    virtual void HalTail2c() { hal_wipe_note("tail slot 0x2c", this); }
};

/* Seven, the count Stage::InitResources passes. */
HalFaderWipe hal_wipes[7];

int hal_wipe_index(const void *self)
{
    long long d = (const char *)self - (const char *)&hal_wipes[0];
    return (int)(d / (long long)sizeof(HalFaderWipe));
}

}  /* anonymous namespace */

extern "C" {
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
}
#pragma comment(linker, "/alternatename:_WIPES=_data_0209f324")
