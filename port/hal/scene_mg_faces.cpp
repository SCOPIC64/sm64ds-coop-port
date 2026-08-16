// The minigame seat's SECOND WAVE: the storage, the two name-spelling faces and
// the named traps the first link asked for. Run link60, lane MG1.
//
// port/slice_mg1.txt is a relocation closure and the linker is the referee. The
// first link named 95 unresolved externals; 33 were matched TUs the closure
// reaches but the slice did not list, and they went into the slice's second
// block. The other 62 are here, in four groups, and each group is a different
// kind of statement.
//
// ---- 1. THREE arm9 BSS WORDS, SIZED BY ROM SPAN ---------------------------
//
// The ordinary treatment hal/scene_boot.cpp already gives seven of them for the
// star select. config/arm9/symbols.txt has all three as kind:bss and the next
// symbol in each case is four bytes on, so four bytes is the ROM's own span and
// not the width the one caller happens to touch -- the undersized-global rule.
// The DS clears bss at boot, so a zero-initialised host global reads exactly
// what the real machine reads.
//
// ---- 2. TWO NAME-SPELLING FACES, NEITHER A STAND-IN FOR A BODY -------------
//
// _ZTV14dScMgCurling_c. src/MgShuffleShell_Spawn.c writes the class vtable
// through this spelling, and no config holds that name. IT IS NOT A GUESS
// THOUGH, and that is worth separating from the ov007 lane's VT0/VT1/VT2 case:
// the ROM's own RTTI string at 0x0213c2d0 reads "14dScMgCurling_c", so the
// decomp recovered the right class name and the port simply has the table under
// its dsd address. The address is settled by the ROM twice: the RTTI, and
// config/arm9/overlays/ov006/relocs.txt's
//     from:0x020e3850 kind:load to:0x0213c304 module:overlay(6)
// where 0x020e3850 is inside MgShuffleShell_Spawn (0x020e3820, 0x34 bytes).
//
// func_020adc74. src/func_ov006_020e3578.c (InitResources) spells its callee
// with no overlay in the name, and no arm9 symbol exists at 0x020adc74. The
// reloc reads module:overlays(3,4), and ov003 can never be co-resident with
// ov006 while ov004 always is -- func_0201a798 loads the pair together -- so
// ov004's body is the only answer residency allows. Same shape as the twelve
// ov001 sprite-template faces the star-select seat carries.
//
// ---- 3. SIX NAMED TRAPS, AND WHY A PLAUSIBLE BODY WOULD BE WORSE -----------
//
// Five ov004/ov006 addresses in this closure have NO delink block in their
// overlay's delinks.txt and no src file anywhere, and one arm9 address has no
// config symbol at all. They are not decompiled and this lane does not guess
// them; port/tools/inferred_stub_guard exists to refuse exactly the plausible
// body that would go here. Each trap is named, counted, and reports itself, so
// "none of them fired" is a measurement rather than an absence.
//
// THREE OF THE SIX ARE ON dScMgCurling_c's OWN PATH and one of those three is
// a live STATE TARGET: func_ov006_020e1854 is the code word of the
// pointer-to-member pair at 0x0213c2bc, so this class cannot reach every one of
// its own states even after section 4 of port/mg_fanout_costs.txt is solved.
// (It has no trap here because nothing in the slice references it by name --
// the pair word is mounted DATA holding a DS address -- and a trap with no
// caller is a definition the linker drops. The omission cannot go quiet: a
// future slice that reaches it fails the link with an unresolved external,
// which is the same reasoning the ov007 lane applied to func_ov007_020c49bc.)
//
// ---- 4. THE mwcc POINTER-TO-MEMBER TU, WHICH IS THE WALL -------------------
//
// src/func_ov004_020b87e0.cpp is EXCLUDED from the slice and trapped here
// instead, and it is the single most important thing in this file because it is
// where the minigame seat stops.
//
// Twenty-one of the first link's ninety-five unresolved externals came from
// that one TU, all of the form
//
//   unresolved external symbol "void (__thiscall C::* data_ov004_020bc97c)(void)"
//     (?data_ov004_020bc97c@@3P8C@@AEXXZQ1@)
//
// and the mangling is the whole story: MSVC encodes the member-pointer TYPE into
// the symbol name, so a global the ov004 mount emits as the C symbol
// _data_ov004_020bc97c can never satisfy it. That is only the visible half.
// THE TU IS UNCOMPILABLE BY MSVC IN THREE INDEPENDENT WAYS, any one of which is
// fatal on its own:
//
//   the SIZE. mwcc's pointer-to-member is eight bytes, {code, adjustment}.
//   MSVC's single-inheritance one is four. The TU's own `struct C` places
//   field_8 at +8 after a leading PMF, which is only true at eight bytes, so
//   MSVC lays the object out wrong before any dispatch happens.
//   the CONTENT. Those twenty-one globals live in the ov004 mount and hold raw
//   DS code addresses. There is no host code at any of them.
//   the DISPATCH. `(self->*self->onUpdate)()` compiles to the ARM Itanium
//   sequence in the ROM -- virtual bit in the adjustment's LSB, `this` advanced
//   by the adjustment arithmetic-shifted right one -- and MSVC emits its own
//   incompatible shape.
//
// THE PORT ALREADY KNOWS THE ANSWER AND IT IS A LANE, NOT A LINE.
// port/unmatched/Player_ChangeState.cpp is the precedent: a host copy of the
// dispatching TU with the member-pointer site replaced by hal_call_state_fn, an
// address switch from DS code address to a real __thiscall call, generated into
// hal/player_states.inc with 197 cases. func_ov004_020b87e0 needs the same for
// its twenty, and dScMgCurling_c needs it for its own twenty-five across five
// more TUs. That is costed in port/mg_fanout_costs.txt section 4 and it is the
// next lane.
//
// SO THE TRAP IS THE HONEST SEAT. It returns without dispatching and says so.
// A minigame whose framework reaches this function does not run past it, and
// the run reports that rather than jumping to a DS address as a host one.

#include <cstdio>

extern "C" {

/* ---- 1. the arm9 bss words, sized by ROM SPAN and not by field width ---- */
unsigned int data_0209d458;      /* next symbol +4  */
unsigned int data_0209d460;      /* next symbol +4  */
unsigned int data_0209d4b8;      /* next symbol +4  */
unsigned int data_0209d474;      /* next symbol +4  */
unsigned int data_0209d488;      /* next symbol +4  */
/* SEVEN HUNDRED AND TWENTY-EIGHT BYTES, not four. config/arm9/symbols.txt's
   next symbol is data_0209cdcc, and the undersized-global rule says size by
   the ROM's span rather than by the width the one caller touches. */
unsigned char data_0209caf4[728];

/* ---- 1b. two NON-bss words, carrying the ROM's own bytes ---------------- */
/* arm9 .data, four bytes by span, read at (0x0208e424 - 0x02004000) out of
   extracted/arm9_dec.bin. It is ff 00 00 00: a -1 sentinel in the low byte,
   which is what a caller testing it for "not set" needs to see. Zeroing it
   would be a different value, so the bytes are transcribed rather than
   defaulted. */
unsigned char data_0208e424[4] = { 0xff, 0x00, 0x00, 0x00 };

/* ov000 .data, twenty-four bytes by span, read at (0x020ad494 - 0x020aa420)
   out of extracted/overlays/overlay_0000.bin. IT IS A FILENAME STRING,
   "thum_shinkei_ncg.bin" followed by four NULs -- shinkei is the memory-match
   minigame -- and a minigame's InitResources loads it by name, so an empty
   host global would be a load of "" rather than a load of nothing.
   WHY IT IS HOSTED HERE RATHER THAN MOUNTED. ov000 has no mount and must not
   get one from this lane: its footprint 0x020aa420..0x020bf4e0 COVERS ov004's
   whole window, and port/ov004_ov006_binding_diff.txt section 1 measured what
   an ov000 mount would do -- 20 of this build's rebased cross-mount pointers
   are withdrawn, all twenty of them ov006 -> ov004 rows this same pair
   created. A single 24-byte string is not worth that, and the binding diff
   asks the next lane not to do it by accident. */
unsigned char data_ov000_020ad494[24] =
    "thum_shinkei_ncg.bin\0\0\0";

/* arm9 .data, twenty-eight bytes by span -- the next symbol is data_0208ecf4,
   which is the 13-entry archive-mount table hal/scene_boot.cpp's LoadArchive
   face describes. Read at (0x0208ecd8 - 0x02004000) out of
   extracted/arm9_dec.bin: the string "myFS_ConvertPathToFileID" plus four
   NULs, the NitroFS entry-point name the card loader hands its own resolver.
   The port's fs seam (hal/fs.cpp) resolves file ids lazily and never reaches
   that resolver, so nothing consumes the string -- but it is a named symbol
   the closure references, and transcribing the ROM's bytes is cheaper than
   reasoning about whether a zeroed one would ever be read. */
unsigned char data_0208ecd8[28] = "myFS_ConvertPathToFileID\0\0\0";

}  /* extern "C" */

/* ---- 2. the two name-spelling faces ------------------------------------ */
#pragma comment(linker, "/alternatename:__ZTV14dScMgCurling_c=_data_ov006_0213c304")
#pragma comment(linker, "/alternatename:_func_020adc74=_func_ov004_020adc74")

/* ---- 3 and 4. the traps ------------------------------------------------ */
static unsigned g_mg_trap_hits;

static void mg_trap(const char *name)
{
    static int said[8];
    ++g_mg_trap_hits;
    /* one line per distinct site, so a loop cannot flood the log and a single
       entry cannot hide in one */
    unsigned h = 0;
    for (const char *p = name; *p; ++p) h = h * 31u + (unsigned char)*p;
    h %= 8u;
    if (!said[h]) {
        said[h] = 1;
        std::fprintf(stderr, "  [scene] UNMATCHED ov004/ov006 body entered: "
                     "%s (returns 0; port/hal/scene_mg_faces.cpp)\n", name);
        std::fflush(stderr);
    }
}

extern "C" unsigned port_mg_trap_hits(void) { return g_mg_trap_hits; }

extern "C" {

/* no delink block and no src in their own overlay's config */
int func_ov004_020ae858(void *)             { mg_trap("func_ov004_020ae858"); return 0; }
int func_ov004_020b1710(void *)             { mg_trap("func_ov004_020b1710"); return 0; }
int func_ov004_020b2220(void *)             { mg_trap("func_ov004_020b2220"); return 0; }
int func_ov006_020e1dc8(void *)             { mg_trap("func_ov006_020e1dc8"); return 0; }
int func_ov006_020e20bc(void *)             { mg_trap("func_ov006_020e20bc"); return 0; }
/* an arm9 address with no config symbol at all, the func_02054c80 shape */
int func_0202e78c(void *)                   { mg_trap("func_0202e78c"); return 0; }

/* THE WALL. See section 4 of the header. Not a body, and deliberately not a
   plausible one: the ROM assigns self->onUpdate from a twenty-entry table of
   mwcc member pointers and then dispatches it, and there is no host form of
   that call until the address switch exists. */
void func_ov004_020b87e0(void *, int idx)
{
    static int said;
    ++g_mg_trap_hits;
    if (!said) {
        said = 1;
        std::fprintf(stderr, "  [scene] mwcc POINTER-TO-MEMBER WALL: "
                     "func_ov004_020b87e0(idx=%d) is the state-setter for "
                     "dScMgBase_c and MSVC cannot compile its TU (8-byte mwcc "
                     "member pointers, DS code addresses, ARM Itanium "
                     "dispatch). No state was set and nothing was dispatched. "
                     "port/mg_fanout_costs.txt section 4.\n", idx);
        std::fflush(stderr);
    }
}

}  /* extern "C" */
