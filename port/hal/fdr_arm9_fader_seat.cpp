// The arm9 wipe fader's own static initialiser, RUN rather than reimplemented.
//
// Run link60 Stage 4, lane FDR. This retires the blocker lane MG2 named and
// port/tools/battery.py's SCENE_BLOCKED row for scene 374 records:
//
//     FAULT c0000005 at _ZN5Scene9SetFadersEP15FaderBrightness+0x24
//     accessing 0x00000024, reached through
//     mb_binit -> func_ov004_020b0930 (dScMgBase_c slot 1) -> Scene::SetFaders
//
// reproduced on this lane's own binary before a line of it was written
// (SM64DS_SCENE=374 SM64DS_SCENE_FRAMES=300 SM64DS_SCENE_NO_RENDER=1, exit
// 139, ecx = esi = the address walk_window.map gives for _data_0209f61c).
//
// ---- WHAT THE OBJECT ACTUALLY IS, BECAUSE THE HANDOVER GOT THE CLASS WRONG --
//
// MG2's report, port/mg_fanout_costs.txt section 10 and the SCENE_BLOCKED row
// all call data_0209f61c "an arm9 FaderBrightness". IT IS A dWipe_c, and
// FaderBrightness is its GRAND-grandbase. The ROM says so in its own RTTI:
// reading the typeinfo chain out of the image with config/arm9/relocs.txt
// applied,
//
//     0x020926dc  __si_class_type_info  "7dWipe_c"          base 0x0208ea00
//     0x0208ea00  __si_class_type_info  "10dFdColor_c"      base 0x0208ea24
//     0x0208ea24  __si_class_type_info  "15dFdBrightness_c" base 0x0208e9e0
//     0x0208e9e0  __class_type_info     "8dFader_c"
//
//     dFader_c <- dFdBrightness_c <- dFdColor_c <- dWipe_c
//
// which is exactly the four vptr stores func_0202fc40 makes, in base-to-derived
// construction order: data_0208eafc (Fader), data_0208eacc (FaderBrightness),
// data_0208eb2c (FaderColor), and data_020926f0 last, the most-derived table
// this file owns. Scene::SetFaders' `FaderBrightness*` parameter is legitimate
// by inheritance, so nothing in the ROM's C is wrong -- but the CLASS NAME is,
// and a successor reading "FaderBrightness" would go looking for the fix in
// the 0x02017xxx fader engine, where none of these bodies live.
//
// The correction does not change MG2's conclusion. MG2 named the table by
// ADDRESS ("a host fill for data_020926f0, which is NOT _ZTV9FaderWipe") and
// that address is right.
//
// ---- WHY A STATIC INITIALISER AND NOT A HAND-ROLLED CONSTRUCTOR -------------
//
// hal/fader_wipes.cpp's gate-31 block solved the SAME SHAPE for the sibling
// object data_0209f5e8 by placement-newing a host HalFaderWipe over the
// storage. That works and it ships. This lane is explicitly NOT allowed to
// copy it, and the reason is the port's north star: the port must BE the
// decomp. So what runs here is src/__sinit_02074f80.c -- the ROM's own
// initialiser, byte-for-byte the two calls the image makes:
//
//     0x02074f80  bl func_0202fc40        r0 = 0x0209f61c
//     0x02074f9c  bl func_020731dc        r0/r1/r2 = 0x0209f61c, the dtor
//                                         0x0202fc08, the node 0x0209f610
//
// and func_0202fc40, also the ROM's, installs the vptr. Nothing here writes a
// vptr, invents a constructor, or picks a plausible table. The one thing this
// file supplies that the image cannot is the CONTENTS of data_020926f0, because
// the ROM's ten words are DS code addresses and a host cannot call them.
//
// IT RUNS AT C++ STATIC-INIT TIME, which is where the DS runs it: before main,
// exactly once, with no seam to remember to call. That is the same mechanism
// and the same reasoning hal/fader_wipes.cpp already relies on for
// data_0209f5bc and data_0209f324 ("C++ static init is ordered before main, so
// the order is guaranteed"). The fill and the sinit sit in ONE initializer so
// their order is intra-TU and cannot be reordered by the linker.
//
// It depends on no other dynamic initialiser. func_0202fc40 writes four vptr
// words and calls func_0202ed14, which is nine plain field stores;
// func_020731dc pushes a node onto data_020aa3f0, which is zero-initialised
// storage in hal/cxx_aliases.cpp. The three base tables the ctor writes on its
// way past are already hosted, by hal/scene_boot.cpp:688-704, and nothing
// dispatches through them inside the window.
//
// ---- THE TABLE: TEN SLOTS, THE ROM'S ORDER ---------------------------------
//
// TEN, and the count is measured rather than assumed. config/arm9/relocs.txt
// carries `kind:load module:main` rows at 0x020926f0 through 0x02092714 and
// stops: 0x02092718 is a literal zero and 0x0209271c relocates to 0x02086ecc,
// which is 7dView_c's typeinfo, so a DIFFERENT class's vtable begins at
// 0x02092720. The slot NAMES are the ROM fader order pinned in
// hal/fader_wipes.cpp, and they are confirmed here independently: the ROM
// bodies at 0x20 and 0x24 are three-instruction veneers onto
// FaderBrightness::SetToEnd (0x0201761c) and FaderBrightness::SetToStart
// (0x02017610), and slots 0x14/0x18 tail into IsAtStart (0x02017684) and
// IsAtEnd (0x02017670).
//
//     off   ROM body        what              this tranche
//     0x00  func_0202fc08   ~dWipe_c   (D1)   SEATED (also the sinit's atexit)
//     0x04  func_0202fbc8   (D0)              SEATED
//     0x08  func_0202f428   AdvanceFade       TRAP -- next chunk
//     0x0c  func_0202f928   SetBackwardTime   TRAP -- next chunk
//     0x10  func_0202f708   SetForwardTime    TRAP -- next chunk
//     0x14  func_0202ee38   IsAtStart         SEATED
//     0x18  func_0202eddc   IsAtEnd           SEATED
//     0x1c  func_0202ed7c   IsBetweenStart..  SEATED
//     0x20  func_0202ed08   SetToEnd          VENEER, see below
//     0x24  func_0202ecfc   SetToStart        SEATED
//     0x28  --              overhang          TRAP
//     0x2c  --              overhang          TRAP
//
// 0x28 and 0x2c are NOT the ROM's. They are the same two-slot overhang
// hal/fader_wipes.cpp spends on HalTail28/HalTail2c, for the same reason: a
// raw-offset caller that reads past 0x24 lands on something callable and named
// instead of on the next class's vtable header. They are the documented
// overhang, not shared ground with the ROM.
//
// WHY THE THREE TRAPS ARE TRAPS AND NOT AN OMISSION. Each of the three bodies
// drags in its own closure -- func_0202f290, func_0202f2c4, func_0202f58c,
// FaderColor::AdvanceFade, and two more arm9 data symbols (data_020926c8,
// data_020926cc, data_0209f600, data_0209f604) that nothing hosts yet. Seating
// them here would put this tranche over the TU budget it was given, and the
// three of them are a coherent second chunk: they are the fade MOTION, where
// the five seated here are the fade STATE. port/fader_boot_map.txt carries the
// closure. Each trap is its own named function so a run says which slot fired
// rather than that some slot did.
//
// WHY 0x20 IS A VENEER AND NOT src/func_0202ed08.c. THE src TU IS HOST-BROKEN,
// and it is broken in exactly the way lane LK4 wrote up for Heap::_Destroy.
// The ROM body is
//
//     0202ed08  ldr ip, [pc] ; bx ip ; .word 0x0201761c
//
// a bare tail-call veneer, and on ARM r0 falls through it untouched. The src
// spells that as
//
//     void func_0202ed08(void) { _ZN15FaderBrightness8SetToEndEv(); }
//
// with NO parameter and NO argument, which is byte-correct under mwccarm and
// receiver-destroying on the host: the callee would read whatever the caller
// left. So the slot forwards to the same target the ROM veneer jumps to,
// reached with the receiver, and the src TU stays out of the link. It is not a
// hand-written body: the ROM's body has no content beyond that one edge.
// Whoever rewrites the veneer TU to take a receiver owns this line too.
//
// ---- THE CALLING CONVENTION, AND ONE HAZARD THIS FILE DOES NOT OWN ---------
//
// Every slot is a __fastcall thunk that hands the receiver to the flat-C ROM
// body as its first argument. That is the port's standing vtable-fill shape
// (hal/scene_mg.cpp's mb_* thunks, hal/lk4_solidheap_seat.cpp's sh_*), and it
// is what a __thiscall dispatch needs: MSVC puts `this` in ECX, __fastcall
// reads its first parameter from ECX.
//
// THE HAZARD, recorded because it is real, pre-existing, and now reachable.
// Scene::SetFaders touches a fader TWO different ways in one function:
//
//     data_0209f5bc->vt->f14(data_0209f5bc)   a struct of function pointers,
//                                             CDECL, receiver on the stack
//     thiz->v24()                             a local class, THISCALL,
//                                             receiver in ECX
//
// The second is what reaches this table today and it is correct. The first
// reaches whatever data_0209f5bc points at -- and after the first
// SetFaders(data_0209f61c) that IS this object, so a LATER SetFaders call would
// enter slot 0x14 cdecl-style and the thunk would read ECX instead. The stack
// stays balanced (a __fastcall with two register parameters cleans nothing, and
// the cdecl caller cleans its own push), so the failure mode is a garbage
// receiver rather than a smashed frame. This is NOT new and it is not this
// file's: hal/fader_wipes.cpp's HalFaderWipe has carried the identical mismatch
// on hal_wipes[0] since it was written, and the reproduction run above went
// THROUGH that path (f14 was called on hal_wipes[0] and returned true, which is
// how v24 was reached at all). It is measured, not assumed: the 300-frame
// scene-374 run after this seat reaches slot 0x14 only from func_0202ed7c,
// which is a thiscall virtual call. The fix belongs to whoever owns
// Scene::SetFaders' host shape, and port/fader_boot_map.txt names it.

#include <cstdio>
#include "types.h"
#include "FaderBrightness.h"
#include "dsstate_seg.h"

extern "C" {

/* The ROM bodies this table seats, declared flat. Each src TU spells its own
   struct for the receiver; these are the ABI-compatible views. */
void *func_0202fc08(void *self);   /* 0x00  D1, and the sinit's atexit dtor */
void *func_0202fbc8(void *self);   /* 0x04  D0 */
int   func_0202ee38(void *self);   /* 0x14  IsAtStart */
int   func_0202eddc(void *self);   /* 0x18  IsAtEnd */
int   func_0202ed7c(void *self);   /* 0x1c  IsBetweenStartAndEnd */
void  func_0202ecfc(void *self);   /* 0x24  SetToStart (veneer, receiver-correct) */

/* The ROM's static initialiser and the storage it constructs. */
void __sinit_02074f80(void);
extern int data_0209f61c[];        /* hal/auto_bss.cpp, 0x2c by ROM span */

}  /* extern "C" */

/* ---- the flat-C faces the seated bodies call ------------------------------
   The matched fader TUs are already in the link, but as MSVC __thiscall
   METHODS (?IsAtStart@FaderBrightness@@UAEHXZ and friends). The bodies above
   reach them by their flat Itanium names. An /alternatename between the two
   would drop the receiver -- hal/method_faces.cpp failure class 3, the one
   lane LK4 measured a fault on -- so these are real receiver-bridging faces,
   the same shape method_faces.cpp already uses for the two of this family it
   owns (_ZN15FaderBrightness14SetForwardTimeEj and
   _ZN15FaderBrightness7IsAtEndEv, which is why neither is repeated here).
   Each call is QUALIFIED so nothing re-dispatches through the host table. */
extern "C" {

int _ZN15FaderBrightness9IsAtStartEv(void *self)
{ return ((FaderBrightness *)self)->FaderBrightness::IsAtStart(); }

/* THIS ONE CARRIES A SKEW AND THE FACE IS WHERE IT GETS SAID OUT LOUD.
   The matched FaderBrightness::IsBetweenStartAndEnd ends in two UNQUALIFIED
   IsAtStart()/IsAtEnd() calls, so it re-dispatches through the RECEIVER's
   table. On mwcc those are ROM slots 5 and 6, bytes 0x14 and 0x18. MSVC folds
   the two destructor slots into one, so the same two calls compile to bytes
   0x10 and 0x14 -- one slot early against a table kept in ROM byte order,
   which is what hal/fader_wipes.cpp's dtor-fold note describes and why that
   file declines to delegate this method at all.

   IT IS STILL THE RIGHT BODY TO CALL, because the only path that reaches it
   here is func_0202ed7c's `type == 1` branch, and type is the object's +0x14.
   func_0202ed14 constructs data_0209f61c with +0x14 = 0. It can be MADE 1:
   func_ov004_020aeb24 calls func_0202ec9c(data_0209f61c, 1), which writes 1
   into either +0x14 or +0x18 depending on the 0x1c predicate. So this is
   reachable in principle and it is NOT reached on the scene-374 path, which
   is a measurement rather than a guarantee -- the check below is what turns
   the difference into a line of output instead of a wrong answer nobody sees.
   With the traps above cleaning no stack, the skewed dispatch lands on the
   0x10 trap and the 0x14 IsAtStart thunk and comes back wrong-but-alive, and
   both of them say so. Fixing it belongs with the three unseated motion slots,
   because it is the same class of work: port/fader_boot_map.txt. */
int _ZN15FaderBrightness20IsBetweenStartAndEndEv(void *self)
{
    if (((const int *)self)[0x14 / 4] == 1) {
        std::fprintf(stderr, "  [fdr] FaderBrightness::IsBetweenStartAndEnd on "
                     "%p with type 1: its two unqualified predicate calls are "
                     "ONE SLOT EARLY against a ROM-ordered table (MSVC folds "
                     "the dtor pair). The answer below is not to be trusted. "
                     "See port/fader_boot_map.txt.\n", self);
        std::fflush(stderr);
    }
    return ((FaderBrightness *)self)->FaderBrightness::IsBetweenStartAndEnd();
}

/* func_0202ecfc's target, spelled by address in its src because the veneer has
   no symbol name of its own in that TU. 0x02017610 IS
   _ZN15FaderBrightness10SetToStartEv (config/arm9/symbols.txt), whose matched
   TU rides slice_gate1. */
void func_02017610(void *self)
{ ((FaderBrightness *)self)->FaderBrightness::SetToStart(); }

}  /* extern "C" */

/* ---- the storage ---------------------------------------------------------
   Both are hosted DS globals, so both go inside the .dsstate bracket and roll
   back with a save state.

   data_020926f0 is arm9 .data in the ROM and its ten words are relocated code
   addresses; hosted raw it would hand a host dispatch ten DS addresses. Twelve
   words here: the ROM's ten plus the two-slot overhang described in the header.

   data_0209f610 is arm9 bss and its ROM span is exactly 0xc -- the next symbol
   in config/arm9/symbols.txt is data_0209f61c. Three words, which is what
   func_020731dc writes into it: {next, dtor, object}. */
DSSTATE_BEGIN
extern "C" {
void *data_020926f0[12];
void *data_0209f610[3];
}
DSSTATE_END

namespace {

int fdr_trap_hits;

void fdr_trap(const char *slot, const char *rom_body)
{
    ++fdr_trap_hits;
    std::fprintf(stderr, "  [fdr] dWipe_c vtable %s FIRED -- the ROM body is "
                 "%s and this tranche did not seat it. See "
                 "port/fader_boot_map.txt.\n", slot, rom_body);
    std::fflush(stderr);
}

/* The three unseated ROM slots.
   NO STACK PARAMETERS, AND THAT IS A DELIBERATE DIFFERENCE FROM
   hal/fader_wipes.cpp, whose 0x0c and 0x10 stubs take the two ints their call
   sites push. Work the two caller shapes through:
     a CDECL raw-offset caller pushes its arguments and cleans them itself, so
       a callee that cleans nothing is balanced whatever the arity;
     a THISCALL no-argument caller pushes nothing and expects the callee to
       clean nothing, so the same callee is balanced there too.
   A stub with two int parameters is balanced for the first shape only, and the
   second shape is the one that reaches THIS table -- see the header's note on
   the MSVC dtor fold, which sends a FaderBrightness::IsAtStart() virtual call
   to byte 0x10 with no arguments at all. Cleaning eight bytes off that caller's
   frame is a smashed stack; leaking four off a thiscall caller that DID push
   one is the worst this direction can do. */
int __fastcall fdr_s08(void *, void *)
{ fdr_trap("slot 0x08 AdvanceFade", "func_0202f428"); return 0; }
int __fastcall fdr_s0c(void *, void *)
{ fdr_trap("slot 0x0c SetBackwardTime", "func_0202f928"); return 0; }
int __fastcall fdr_s10(void *, void *)
{ fdr_trap("slot 0x10 SetForwardTime", "func_0202f708"); return 0; }
int __fastcall fdr_s28(void *, void *)
{ fdr_trap("overhang slot 0x28", "nothing, this slot is not the ROM's"); return 0; }
int __fastcall fdr_s2c(void *, void *)
{ fdr_trap("overhang slot 0x2c", "nothing, this slot is not the ROM's"); return 0; }

/* The seven seated slots. Receiver out of ECX, into the ROM body's first
   argument. */
void *__fastcall fdr_s00(void *s, void *) { return func_0202fc08(s); }
void *__fastcall fdr_s04(void *s, void *) { return func_0202fbc8(s); }
int   __fastcall fdr_s14(void *s, void *) { return func_0202ee38(s); }
int   __fastcall fdr_s18(void *s, void *) { return func_0202eddc(s); }
int   __fastcall fdr_s1c(void *s, void *) { return func_0202ed7c(s); }
/* 0x20: the target the ROM's veneer at 0x0202ed08 tail-calls. See the header
   for why src/func_0202ed08.c is not in the link. */
void  __fastcall fdr_s20(void *s, void *)
{ ((FaderBrightness *)s)->FaderBrightness::SetToEnd(); }
void  __fastcall fdr_s24(void *s, void *) { func_0202ecfc(s); }

void fdr_fill(void)
{
    data_020926f0[0] = (void *)fdr_s00;
    data_020926f0[1] = (void *)fdr_s04;
    data_020926f0[2] = (void *)fdr_s08;
    data_020926f0[3] = (void *)fdr_s0c;
    data_020926f0[4] = (void *)fdr_s10;
    data_020926f0[5] = (void *)fdr_s14;
    data_020926f0[6] = (void *)fdr_s18;
    data_020926f0[7] = (void *)fdr_s1c;
    data_020926f0[8] = (void *)fdr_s20;
    data_020926f0[9] = (void *)fdr_s24;
    data_020926f0[10] = (void *)fdr_s28;
    data_020926f0[11] = (void *)fdr_s2c;
}

/* Fill, then run the ROM's initialiser. One object so the order is the
   compiler's, not the linker's. */
struct FdrArm9FaderBoot {
    FdrArm9FaderBoot()
    {
        fdr_fill();
        __sinit_02074f80();
    }
};
FdrArm9FaderBoot fdr_arm9_fader_boot;

}  /* anonymous namespace */

/* The proof line, callable from a harness that wants it in its own output.
   Nothing in the build depends on it being called: the seat is complete
   without it, and the object above is what makes that true. */
extern "C" void port_fdr_fader_report(void)
{
    std::printf("[fdr] dWipe_c at data_0209f61c: vptr %p (table %p), "
                "%d unseated-slot hit(s)\n",
                (void *)(size_t)data_0209f61c[0], (void *)data_020926f0,
                fdr_trap_hits);
    std::fflush(stdout);
}
