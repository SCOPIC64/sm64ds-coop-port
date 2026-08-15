// ov004's font-destination BSS word, so LoadFont reaches the real body.
//
// src/LoadFont.cpp has three arms and the data_0209d698 == 2 one reads ov004:
//
//     LoadFileAt(0x980e, func_ov004_020adc4c());
//     data_0209d6f8 = func_ov004_020adc4c();
//
// and src/func_ov004_020adc4c.c is four instructions:
//
//     int func_ov004_020adc4c(void) { return data_ov004_020beb60[0]; }
//
// LoadFont is linked. The callee was not: hal/sub_screen.cpp carried a printf
// stand-in tagged `PORT_HOST_ABI: src reads data_ov004_020beb60, and ov004 is
// not mounted`, which is a blocker with a removable cause rather than a
// hardware or ABI ruling. Hosting the one word it reads retires the tag and
// the matched TU takes over, which is the whole point: this is the only edge
// in the build where a LINKED arm9 caller reaches ov004 code. See
// port/ov004_ov007_2d_map.txt section 5.
//
// WHAT ov004 IS. Not the VS mode the old sub_screen.cpp comment guessed. Its
// RTTI names it: 11dScMgBase_c and N10dMgPsOpt_c11TouchIcon_cE, the MINIGAME
// scene framework, loaded only as a passenger of ov006 (func_0201a798 calls
// LoadOverlay(ov004) when the scene's overlay is ov006). So the arm == 2 arm
// of LoadFont is the minigames' font, and walk_window never takes it: the
// window loads font 0. The body links, and does not run.
//
// SIZE BY ROM SPAN, not by field width. config/arm9/overlays/ov004/symbols.txt
// has data_ov004_020beb60 at 0x020beb60 and data_ov004_020beb64 next, so the
// span is exactly four bytes. src declares it `extern int
// data_ov004_020beb60[]` and indexes [0] only.
//
// ZERO IS THE FAITHFUL VALUE. It is BSS. With ov004 not resident the ROM reads
// this word out of whatever the slot's previous occupant left, and with ov004
// resident but its init not run it reads zero. Nothing in this build writes
// it, and nothing in this build calls the arm that reads it. The sibling word
// data_ov004_020beb68 is hosted the same way and for the same reason at the
// top of hal/actor_classes_ov081.cpp.
//
// DSSTATE because it is a DS global: a save state must roll it back with the
// rest of the hosted arena, not leave it behind as host-only bookkeeping.
#include "dsstate_seg.h"
DSSTATE_BEGIN
extern "C" int data_ov004_020beb60[1];
int data_ov004_020beb60[1] = {0};
DSSTATE_END
