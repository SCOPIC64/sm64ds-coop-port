// Run link100 wave 9c, lane LINK21: the rows the last wall needed that no
// generator emits. This is port/hal/int4_rows.cpp's shape one wave on, and the
// same rule applies: every alias here is a NAME bridge whose two sides were
// read out of the link's own objects with dumpbin, never derived from a
// filename, and every host body says what it stands in for and why.

// =========================================================================
// 1. Sound::PlayBank2_2D, the one name the two bannered drafts bring with them
// =========================================================================
//
// src/_ZN12dScMgSlot1_c8BehaviorEv.cpp, taken verbatim from origin/main, spells
// its own forward declaration as
//
//     namespace Sound { void PlayBank2_2D(unsigned int); }
//
// so it references ?PlayBank2_2D@Sound@@YAXI@Z. The body in this link is
// src/_ZN5Sound12PlayBank2_2DEj.cpp on port/slice_mg1.txt line 504, whose own
// spelling returns the handle:
//
//     ?PlayBank2_2D@Sound@@YAII@Z
//
// A NAME BRIDGE AND NOT AN ABI BRIDGE, on the standing test. Both sides are
// __cdecl (YA) with one unsigned int argument (I) and no receiver at all. The
// only difference is the return: void against unsigned int, and a cdecl caller
// that declares void simply never reads EAX. That is the same reading
// port/hal/fader_wipes.cpp:112 makes for func_0203ae58 against
// _Z14ApproachLinearRiii, and the same shape port/hal/cxx_aliases.cpp:3239
// already uses for the flat spelling of this very function:
//
//     /alternatename:__ZN5Sound12PlayBank2_2DEj=?PlayBank2_2D@Sound@@YAII@Z
//
// The alias rather than a host body, for that row's stated reason: a target
// which one day compiles a translation unit declaring the void form strongly
// keeps its own definition and nothing collides.
#pragma comment(linker, "/alternatename:?PlayBank2_2D@Sound@@YAXI@Z=?PlayBank2_2D@Sound@@YAII@Z")
