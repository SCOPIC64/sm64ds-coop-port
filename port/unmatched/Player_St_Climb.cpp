// HOST BRIDGE for ST_CLIMB (data_ov002_021106dc) -- the state Mario enters
// when he grabs a tree trunk or a pole. Landing on a castle-grounds bush or
// tree canopy runs the actor-286 Tree's CylinderClsn through
// CylinderClsn::Process, whose notify bit reaches func_ov002_020caf98, which
// sets Player+0x37c to the cylinder and changes to this state.
//
// ALL THREE HALVES WERE UNHOSTED, and that is Tango's freeze-then-slide:
//   St_Climb_Init    0x020cbd34  matched src, was in no slice manifest
//   St_Climb_Main    0x020cb5bc  NO SOURCE ANYWHERE -- not decompiled
//   St_Climb_Cleanup 0x020cb568  matched src, was in no slice manifest
// hal_call_state_fn no-ops what it cannot map, so Init never ran. Init is
// what starts the climb animation (SetAnim 0x26/0x27) and what ZEROES the
// horizontal speed at +0x98/+0x9c and the vertical at +0xa8. Without it the
// anim never changes (the freeze) and he keeps the speed he grabbed at while
// sitting in a state whose Main also does nothing (the slide) -- and the
// fields Init seats (+0x688 the climb height, +0x5c/+0x64 snapped to the
// trunk, +0x6e5 the step) stay whatever they were, which is what a later
// consumer then reads.
//
// St_Climb_Init is matched source but include/Player.h does not declare it,
// so it cannot be called through the port's Player class. The source
// declares its own one-method `class Player`; MSVC's mangling for that is
// the same symbol, so this TU re-declares the identical shadow class -- the
// technique the port already uses for Butterfly::Render's ModelAnim -- and
// hands player_states.inc a plain C entry point. Cleanup needs no bridge:
// include/Player.h line 377 declares it, so the .inc calls it directly.
#include <cstdio>

class Player {
public:
    int St_Climb_Init();
};

extern "C" int port_player_st_climb_init(void *self)
{
    return ((Player *)self)->Player::St_Climb_Init();
}

// ov002 0x020cb5bc, 0x778 bytes, has no C anywhere in src/ -- only a
// near-miss in nearmiss/db.jsonl. It gets a NAMED trap rather than the
// mapper's anonymous "unhosted state fn" line, the Rabbit 0x0212b8dc
// precedent, because this is the one state where a silent no-op reads to a
// player as the game breaking rather than as a missing actor. With Init now
// hosted he holds the trunk with the climb anim seated and his speed zeroed
// instead of sliding, but he cannot climb, drop or jump off until this body
// exists: St_Climb_Main is what promotes him to ST_HEADSTAND at the top,
// ST_POLE_JUMP on the jump button, and ST_FALL on release.
extern "C" int port_player_st_climb_main(void *)
{
    static int said;
    if (!said) {
        said = 1;
        std::fprintf(stderr,
                     "UNMATCHED: Player::St_Climb_Main (ov002 0x020cb5bc, "
                     "0x778 bytes) has no host body -- Mario will hold the "
                     "tree/pole and cannot climb, jump or drop off it. "
                     "Decompile 0x020cb5bc to finish ST_CLIMB.\n");
    }
    return 1;
}
