/* dScTitle_c's SEVEN VTABLE FACES -- run link100 wave 14, lane SEAT14D.
 *
 * ov003's third and last scene class, scene id 2. dScStarSel_c (id 4) was
 * seated on port/slice_scene1.txt and dScGameOver_c (id 8) on
 * port/slice_mpg2.txt; this file is the third and closes the overlay's set.
 * The derivation, the reference chain and both of port/slice_scene1.txt's
 * blockers are in port/slice_title.txt; the eighteen-word host vtable, the
 * fill and the registry row are in hal/scene_boot.cpp beside the other two.
 *
 * WHAT THIS CLASS IS. The DEBUG LEVEL SELECT, not the title screen.
 * out/LINK14/BATCHES.md has that backwards. hal/scene_boot.cpp:67 and
 * port/ov003_syms.txt:3 both name id 2 "the debug level select", the title
 * screen and file select are dScDSMT_c (id 1, ov007, seated by run link60
 * lane L2), and Behavior's own table at data_ov003_020b1180/0x020b1181 is a
 * level column and an entrance column. The opening route never enters scene 2.
 *
 * WHY THE FACES ARE HERE AND NOT IN hal/scene_boot.cpp, which is where the
 * other two classes' are. Those two dispatch through the ROM's own flat
 * Itanium names (`_ZN13dScGameOver_c8BehaviorEv`), which exist in this build
 * because hal/faces_sync_gen.cpp generates a forwarder for each from a row in
 * port/faces_sync.txt. dScTitle_c has no rows in that file, and
 * port/faces_sync.txt belongs to another batch tonight (out/LINK14/BATCHES.md's
 * collision table gives it to BATCH 1), so this seat reaches the matched
 * bodies the OTHER way the port already uses in hal/actor_classes.cpp: a
 * qualified non-virtual call through the class header. That needs
 * include/dScTitle_c.h, and hal/scene_boot.cpp is a 7000-line file with its own
 * flat declarations of names dScene_c.h also declares, so the include goes in
 * a file of its own rather than into the middle of that one. Same call
 * hal/dtor_forwarders_gen.cpp makes for the same reason.
 *
 * NOTHING HERE IS A LINK ROOT. Every face is reached only from the eighteen
 * words hal/scene_boot.cpp's scene_fill_title() writes into
 * data_ov003_020b1650, which is the table dScTitle_c_classInit installs and
 * the table the ROM's own vtable at 0x020b1650 is. No /include:, no alias, no
 * keep-alive reference.
 *
 * THE ARITIES ARE THE SRC TUs' OWN, checked one at a time against each file
 * rather than copied from the game-over block: a raw cast at the wrong arity
 * is what smashes the stack on the first dispatch and it stays invisible until
 * the slot is entered (memory note sm64ds-port-fastcall-face-arity.md).
 *
 *   slot 0   s32  dScTitle_c::InitResources()      src/_ZN10dScTitle_c13InitResourcesEv.cpp
 *   slot 3   s32  dScTitle_c::CleanupResources()   src/_ZN10dScTitle_c16CleanupResourcesEv.cpp
 *   slot 6   s32  dScTitle_c::Behavior()           src/_ZN10dScTitle_c8BehaviorEv.cpp
 *   slot 9   s32  dScTitle_c::Render()             src/_ZN10dScTitle_c6RenderEv.cpp
 *   slot 12  void dScTitle_c::OnPendingDestroy()   src/_ZN10dScTitle_c16OnPendingDestroyEv.cpp
 *   slot 16  D2                                    src/_ZN10dScTitle_cD1Ev.cpp
 *   slot 17  D0                                    the D2 body plus the one
 *            deallocation the cartridge's D0 makes
 *
 * SLOT 17 HAS NO TU OF ITS OWN AND THAT IS NOT A GAP. MSVC folds the ROM's
 * destructor variants into one symbol, so only one of a class's two
 * per-function destructor TUs can be compiled -- port/slice_scene1.txt made
 * the same call for src/_ZN12dScStarSel_cD0Ev.cpp. The D0 face is spelled the
 * way hal/dtor_forwarders_gen.cpp spells every other class's: the destructor
 * plus Memory::Deallocate(this, GAME_HEAP_PTR), which is what 0x020ad69c does
 * (include/dScTitle_c.h's VTABLE ORDER note reads it off the body).
 */

#include "types.h"
#include "dScTitle_c.h"
#include "port_d16.h"
#include "dsstate_seg.h"

/* TWO HOSTED ARM9 .bss WORDS, and they are this class's own private state.
 *
 * The port had no storage for either: `grep data_0209b2f4 build/port/walk_window.map`
 * was empty at 8ddff3187, and seating Render turned that into two LNK2019s.
 * They are hosted HERE rather than appended to hal/auto_bss.cpp, which is the
 * usual accumulator, for the reason data_0209b2ec moved out of that file into
 * hal/scene_vs_menu.cpp (run rel0215 lane prop15): a word only one class reads
 * belongs beside that class. Every reader in the whole tree is a dScTitle_c TU --
 *
 *   data_0209b2f4  src/_ZN10dScTitle_c8BehaviorEv.cpp (the row cursor: += 1,
 *                  += 0x35, % 0x36), src/_ZN10dScTitle_c6RenderEv.cpp,
 *                  src/func_ov003_020ad6ec.c
 *   data_0209b2f8  src/_ZN10dScTitle_c6RenderEv.cpp (the scroll top),
 *                  src/func_ov003_020ad6ec.c
 *
 * -- so nothing else in the build can want them, and a later reader that does
 * will find them by name the way it finds auto_bss's.
 *
 * SIZED BY ROM SPAN, NOT BY FIELD WIDTH (memory note
 * sm64ds-port-undersized-globals.md, and the opposite trap too). config/arm9/
 * symbols.txt:4537-4541 lists data_0209b2f0, f4, f8, fc, 0209b300 consecutively,
 * so each of these two is exactly one 4-byte word. hal/auto_bss.cpp's generous
 * `int x[8]` default is deliberately NOT copied: these two are read as
 * `data_0209b2f8[0]` in one TU and as a plain int in the others, and four bytes
 * is what the cartridge gives them.
 *
 * They are MUTABLE DS STATE, so they go inside the .dsstate bracket the save
 * state captures wholesale (hal/dsstate_seg.h), which is where auto_bss.cpp
 * puts every word it hosts. */
DSSTATE_BEGIN
extern "C" {
int data_0209b2f4;
int data_0209b2f8;
}
DSSTATE_END

// The deallocation the ROM's D0 body makes and the heap pointer word it reads,
// both under the ROM's own flat names -- what the cartridge's relocations name
// and what this port already resolves. Same block hal/dtor_forwarders_gen.cpp
// carries.
extern "C" {
void _ZN6Memory10DeallocateEPvP4Heap(void *ptr, void *heap);
extern void *GAME_HEAP_PTR;
}

/* THE WITNESS, dScStarSel_c's and dScGameOver_c's, one counter per dispatched
   slot. A scene that BOOTS is not a scene that RUNS, and from outside the two
   look identical. hal/scene_boot.cpp's census prints these. */
/* HOST BOOKKEEPING, NOT A HOSTED DS GLOBAL: both stay OUT of the .dsstate
   bracket, the way hal/scene_boot.cpp's g_go_hits and g_go_vptr_after_d2 do.
   What the ROM's own D2 leaves in the object's +0 word is read back AFTER the
   body returns -- the runtime half of the table-word proof dScGameOver_c's
   seat established. D2 ONLY: D0 deallocates the object, so reading it back
   after that one would be a use-after-free. */
extern "C" {
unsigned g_ti_hits[18];
void *g_ti_vptr_after_d2;
}

/* SLOT 0 AND SLOT 6 ARE STAGED, and these two lines are the staging.
 * out/LINK14/BATCHES.md calls this the highest-risk batch of the night, so the
 * seat lands in three commits and each one is a RUNNABLE state: a registry row
 * puts scene 2 into port/tools/bootab.py's scenes=all set, and a null slot 0
 * would fault on the first thing the scene does. Sub-batch 1 seats slots
 * 3/9/12/16/17 from the matched bodies and gives 0 and 6 a stub that does
 * nothing and says it did; sub-batch 2 replaces the slot-0 stub with
 * dScTitle_c::InitResources; sub-batch 3 replaces the slot-6 stub with
 * dScTitle_c::Behavior. NEITHER STUB SURVIVES TO THE TIP and neither ever
 * raises the linkage count: they are here so an intermediate commit is not a
 * crash, and the final commit has none. */
/* SUB-BATCH 1 STUB -- src/_ZN10dScTitle_c13InitResourcesEv.cpp is not on
   port/slice_title.txt yet. Returns the ROM body's own return value, 1. */
extern "C" int __fastcall port_title_init(void *, void *)
{ ++g_ti_hits[0];  return 1; }

extern "C" int __fastcall port_title_clean(void *s, void *)
{ ++g_ti_hits[3];  return ((dScTitle_c *)s)->dScTitle_c::CleanupResources(); }

/* SUB-BATCH 1 STUB -- src/_ZN10dScTitle_c8BehaviorEv.cpp is not on
   port/slice_title.txt yet. Returns the ROM body's own return value, 1. */
extern "C" int __fastcall port_title_beh(void *, void *)
{ ++g_ti_hits[6];  return 1; }

extern "C" int __fastcall port_title_render(void *s, void *)
{ ++g_ti_hits[9];  return ((dScTitle_c *)s)->dScTitle_c::Render(); }

extern "C" int __fastcall port_title_pdes(void *s, void *)
{ ++g_ti_hits[12]; ((dScTitle_c *)s)->dScTitle_c::OnPendingDestroy(); return 0; }

extern "C" void *__fastcall port_title_d2(void *s, void *)
{
    ++g_ti_hits[16];
    ((dScTitle_c *)s)->dScTitle_c::~dScTitle_c();
    g_ti_vptr_after_d2 = *(void **)s;
    return s;
}

extern "C" void *__fastcall port_title_d0(void *s, void *)
{
    ++g_ti_hits[17];
    ((dScTitle_c *)s)->dScTitle_c::~dScTitle_c();
    _ZN6Memory10DeallocateEPvP4Heap(s, GAME_HEAP_PTR);
    return s;
}
