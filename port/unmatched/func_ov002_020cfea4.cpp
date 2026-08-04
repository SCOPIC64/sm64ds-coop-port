// NAMED TRAP for ov002 0x020cfea4 (0x2d4 bytes), the one member of the
// ledge-hang family with no C anywhere in the tree. include/decl_common.h
// line 1412 declares it, src/ has no file for it, and no near-miss carries
// it either; the only record of it is the symbol row and the single reloc
// that reaches it, from:0x020d0b48 -- an instruction inside
// St_LedgeHang_Main.
//
// WHAT IT IS. St_LedgeHang_Main's climb-up branch reads
//     if ((!func_ov002_020cfaf0(this) && (pressed & 2)) ||
//          func_ov002_020cfea4(this))
//         -> mStateStep = 1; ChangeState(ST_LEDGE_GRAB)
// so this is the SECOND way to start the pull-up, in parallel with "the
// wall in front of me has ended and the player pressed A". Its two
// neighbours 0x020cfaf0 and 0x020cfbdc are both RaycastLine probes around
// the hang point, so by position and by size this is very likely the third
// such probe (the auto-climb / ledge-is-low-enough test). That is a shape
// argument, not a decompilation, so this file does NOT guess a body.
//
// Returning 0 is the honest choice rather than the convenient one: it is
// what the ROM's own code returns on every path where its raycast finds
// nothing, so a 0 leaves the OTHER branch of the same `if` fully in charge.
// Mario still pulls up by pressing A at the top of the hang, still drops
// with B, still shimmies, still falls -- only the automatic promotion this
// function alone can order is missing. A 1 would force a pull-up every
// single frame of every hang, which is a behavior nobody has evidence for.
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
                     "ledge hang is playable but one automatic pull-up "
                     "trigger is missing. Decompile 0x020cfea4.\n");
    }
    return 0;
}
