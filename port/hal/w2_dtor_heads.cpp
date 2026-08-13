// LANE w2-b SEAT (run linkw, wave 2) -- destructor heads on the ROM vtable
// storage the port hosts but never fills.
//
// This is wave 1's meshcollider_dtor_seat mechanism applied to what a full
// sweep of the port's hosted vtable storage turned up. The sweep read every
// two-word head in config/arm9 and every overlay config against the port's
// host definitions, and asked one question per table: do the ROM's own
// relocations at base+0 and base+4 name a matched TU that the link does not
// carry? Thirty tables answered yes. Twenty-seven of them are blocked for
// reasons recorded at the bottom of this file. What is left is seated here.
//
// WHY A SEAT AND NOT JUST A SLICE LINE, one more time. Release links with
// /OPT:REF, so a matched TU added to a slice with nothing referencing it is
// dropped before the map is written and the headline does not move. The
// reference edge IS the work; a ROM vtable slot the port never filled is the
// honest place to put one, because the ROM says what belongs there.
//
// ===========================================================================
// SEATED: _ZTV12CylinderClsn, arm9 0x0208e6ec, slots 0 and 1.
//
// The ROM's own two words, config/arm9/relocs.txt:
//
//     from:0x0208e6ec kind:load to:0x020150a8 module:main   D1 (complete)
//     from:0x0208e6f0 kind:load to:0x0201507c module:main   D0 (deleting)
//
// and config/arm9/symbols.txt puts a class destructor on both addresses:
//
//     _ZN12CylinderClsnD1Ev kind:func addr:0x020150a8
//     _ZN12CylinderClsnD0Ev kind:func addr:0x0201507c
//
// THE INDEX CONVENTION IS THE CLASS'S OWN, not an inference. src/
// _ZN12CylinderClsnD1Ev.c -- byte-matched, and the body this seat names at
// index 0 -- opens by writing the table base as the object's vptr:
//
//     *(int*)self = (int)data_0208e6ec; // set vptr
//
// so 0x0208e6ec is the vptr address, index 0 is the first virtual slot, and
// the Itanium pair sits at 0 and 1. src/_ZN12CylinderClsnD0Ev.c writes the
// same base and adds the Memory::operator_delete2 call, which is the D1/D0
// split every class in this band shows.
//
// TWO WORDS, NO MORE. dsd's next data symbol after 0x0208e6ec is 0x0208e704,
// so this head has room for six words, but there is relocation evidence for
// exactly two and this file does not own the storage. Index 2 and up stay
// zero.
//
// NOTHING FILLS IT AND NOTHING DISPATCHES IT. The storage is
// `int data_0208e6ec[4]` in hal/cxx_aliases.cpp, in the BSS ring the gate-10
// aliases land on; the only other mention of the name in the whole port is
// the /alternatename pragma four lines above it. No fill in the tree ever
// wrote a slot and no call site reads one, so this changes no behaviour: it
// makes the port's copy of the table carry the two words the ROM puts there,
// and that is the reference edge that pulls the two matched TUs into the link.
// The callee closure was already linked before this seat (func_02014fa4 and
// Memory::operator_delete2 are both in the map).
//
// ABI. The flat-C bodies are `void *f(void *this)` with `this` on the stack.
// A ROM-shadow dispatch through an MSVC vtable would arrive __thiscall with
// `this` in ecx, so each slot takes the ecx->arg adapter rather than the body
// directly -- the same convention hal/meshcollider_dtor_seat.cpp and
// hal/model_dtor_seat.cpp use for every slot they seat.
// ===========================================================================

extern "C" {

/* _ZTV12CylinderClsn -- storage `int data_0208e6ec[4]` in hal/cxx_aliases.cpp */
extern int data_0208e6ec[];

void *_ZN12CylinderClsnD1Ev(void *self);   /* arm9 0x020150a8 */
void *_ZN12CylinderClsnD0Ev(void *self);   /* arm9 0x0201507c */

}

static void __fastcall cyl_d1(void *self, void *) { _ZN12CylinderClsnD1Ev(self); }
static void __fastcall cyl_d0(void *self, void *) { _ZN12CylinderClsnD0Ev(self); }

extern "C" void hal_seat_w2_dtor_heads(void)
{
    data_0208e6ec[0] = (int)(size_t)cyl_d1;
    data_0208e6ec[1] = (int)(size_t)cyl_d0;
}

// ---- how this fill gets called ---------------------------------------------
//
// Every seat wave 1 landed is called by name from hal/level_boot.cpp, at the
// tail of the boot that installs the collision and model tables. This lane
// does not own that file, and the campaign's rule is that a lane never edits
// another lane's file, so the call edge is a namespace-scope object whose
// constructor runs the fill -- ordinary C++ static initialisation, entirely
// inside the one file this lane owns.
//
// THAT IS ONLY SAFE BECAUSE OF WHAT IS SEATED. A pre-main write would be
// wrong for any table a runtime fill also writes (hal/stage_bridges.cpp's
// trap fill would simply overwrite it, and the seat would silently do
// nothing). The table above is written by nothing else in the port, so
// "before main" and "at level boot" put the same two words in the same two
// slots, and the storage is zero-initialised before any dynamic initialiser
// runs, so there is no ordering question with its own definition either.
//
// The fill keeps external C linkage so a later change that does own
// level_boot.cpp can call it explicitly and drop the object below.
namespace {
struct W2SeatDtorHeads {
    W2SeatDtorHeads() { hal_seat_w2_dtor_heads(); }
};
W2SeatDtorHeads g_w2_seat_dtor_heads;
}
