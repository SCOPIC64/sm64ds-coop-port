// NAMED TRAP for ov002 0x020cfea4 (0x2d4 bytes), the one member of the
// ledge-hang family with no matched C in the tree. include/decl_common.h
// line 1412 declares it, src/ has no file for it, and the only other record
// is the single reloc that reaches it, from:0x020d0b48 -- an instruction
// inside St_LedgeHang_Main.
//
// WHAT IT IS. St_LedgeHang_Main's pull-up branch reads
//     if ((!func_ov002_020cfaf0(this) && (pressed & 2)) ||
//          func_ov002_020cfea4(this))
//         -> mStateStep = 1; ChangeState(ST_LEDGE_GRAB)
// so this is the SECOND way out of the hang and into the climb, in
// parallel with "the wall in front of me has ended and the player pressed
// A".
//
// nearmiss/db.jsonl DOES carry an attempt at it -- 76 divergences over 181
// instructions, so nowhere near matching, but close enough to read the
// shape off. It is an OBSTRUCTION probe: it answers 1 when the hang cannot
// continue (an actor floor or wall under him whose heading disagrees with
// his, or a RaycastGround ahead that comes back high, or either of two
// sideways func_ov002_020cfd84 line probes hitting), and 0 by fall-through
// when nothing is in the way. That reading is a lead for whoever finishes
// it, NOT a body: an unverified near-miss is not a decompilation, and this
// file does not pretend otherwise.
//
// Returning 0 is therefore both the conservative choice and the one the
// near-miss agrees with -- 0 is its default exit, "nothing blocking, keep
// hanging". It leaves the OTHER branch of the same `if` fully in charge, so
// Mario still pulls up by pressing A at the top of the hang, still drops,
// still shimmies, still falls. A 1 would force a pull-up on every frame of
// every hang, which nothing supports.
//
// The trap prints once, under the same rule as St_Climb_Main in
// Player_St_Climb.cpp: a state hole that a player would read as the game
// breaking gets a name, not the mapper's anonymous line.
#include <cstdio>

extern "C" int func_ov002_020cfea4(void *)
{
    static int said;
    if (!said) {
        said = 1;
        std::fprintf(stderr,
                     "UNMATCHED: func_ov002_020cfea4 (ov002 0x020cfea4, "
                     "0x2d4 bytes) has no host body -- returning 0. This is "
                     "the second of St_LedgeHang_Main's two paths into "
                     "ST_LEDGE_GRAB; the A-button path still works, so the "
                     "ledge hang is playable but the obstruction-forced "
                     "pull-up is missing. Decompile 0x020cfea4 (a 76-"
                     "divergence near-miss is already in nearmiss/db.jsonl)."
                     "\n");
    }
    return 0;
}
