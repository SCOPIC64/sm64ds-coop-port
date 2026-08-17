// HOST TRANSCRIPTION of func_ov006_020e1854 -- dScMgCurling_c's aim-and-charge
// state, slot 1 of data_ov006_021418b0. Run link60, lane CT1.
//
// WHAT IT IS, PLAINLY. This is the part of curling where you hold the stylus
// down: the cursor follows your finger inside a fixed box on the touch screen,
// the game measures how hard and how fast you are scrubbing to build up shot
// power, works out the aim angle from the direction you are dragging, and
// chirps once each time you reverse direction. Lift the stylus and it hands
// control back to the idle state with the "shot is ready" flag raised.
//
// WHY IT EXISTS. Every other state in this class has a matched src TU. This one
// has no delink block in config/arm9/overlays/ov006/delinks.txt -- the blocks
// stop dead at its first byte, `.text start:0x020e17f8 end:0x020e1854`
// (delinks.txt:2311) -- so no src file defines it and the port had to refuse
// it at the dispatch site. port/mg_fanout_costs.txt section 5 calls it "THE ONE
// MISSING STATE ... A HARD FLOOR FOR THIS CLASS" and port/touch_map.txt records
// the production refusal it caused: a consumed tap advances the sub-state to
// this address and every later tap is then ignored, because the state was never
// entered. This file is what that refusal was waiting for.
//
// PROVENANCE. Not a banked near-miss; there is no draft for this symbol in
// nearmiss/db.jsonl. Read straight off the ROM from a capstone listing of
//   extracted/overlays/overlay_0006.bin   at base 0x020BFEC0
// (ov006 base_address 34340544, code_size 0x7EB80, from
// extracted/dsd/arm9_overlays/overlays.yaml id:6).
//
// THE BASE IS PROVED, NOT ASSUMED. The word at 0x0213c2bc reads 0x020e1854 at
// that base, which is exactly what config/arm9/overlays/ov006/relocs.txt:11726
// says is there (`from:0x0213c2bc kind:load to:0x020e1854`). The dsd export
// extracted/dsd/arm9_overlays/ov006.bin does NOT agree at that offset (it reads
// 0x021379b0) and was not used for anything.
//
// EXTENT. config/arm9/overlays/ov006/symbols.txt:605
//     func_ov006_020e1854 kind:function(arm,size=0x300) addr:0x020e1854
//   0x020e1854..0x020e1b3b   code, 186 ARM instructions
//   0x020e1b3c..0x020e1b53   literal pool, 6 words (resolved in full below)
//   0x020e1b54               next symbol begins. The extent is tight.
//
// SEMANTICS, NOT BYTES. This is a host copy shaped for MSVC, not a byte match.
// When a byte match lands in src/ this file retires per the port rule, the same
// way MeshCollider_DetectClsn_RaycastLine.cpp retired.
//
// NO GAME LOGIC WAS INVENTED. Every constant, every branch, every field offset
// and every call below is carried over from the disassembly, and each block
// carries the ROM address it came from. Where the ROM's own form is odd it is
// transcribed odd and flagged, not tidied.
//
// ---- THE LITERAL POOL, RESOLVED ------------------------------------------
//
//   0x020e1b3c  0x020a0e40   data_020a0e40   active stylus slot index   (main)
//   0x020e1b40  0x020a0de8   data_020a0de8   touched flag, stride 4     (main)
//   0x020e1b44  0x020a0dea   data_020a0dea   stylus X,     stride 4     (main)
//   0x020e1b48  0x020a0deb   data_020a0deb   stylus Y,     stride 4     (main)
//   0x020e1b4c  0x000001d6   the scrub sound id
//   0x020e1b50  0x00004ec8   the shot-power field offset, re-materialised
//
// The first four are cited by config/arm9/overlays/ov006/relocs.txt as
// `kind:load` from 0x020e185c, 0x020e1860, 0x020e187c and 0x020e1880. The last
// two are plain immediates with no reloc, which is itself the evidence that
// they are values and not addresses.
//
// ---- CALLEES: THE HOLE DOES NOT CASCADE ----------------------------------
//
// Three distinct callees, four call sites, all in the main module, ALL of them
// already have decompiled bodies. Nothing had to be invented to host this.
//
//   0x0203d744  _ZN4cstd4sqrtEy              x2   integer sqrt of a u64. BODY is
//                                                 src/_ZN4cstd4sqrtEy.c (it is
//                                                 the hardware divider: writes
//                                                 0x40002b8, spins on 0x40002b0).
//                                                 src/func_0203d5dc.c only
//                                                 DECLARES it and is where the
//                                                 (u64)(s64) call idiom is taken
//                                                 from.
//   0x0203b4dc  _ZN4cstd5atan2E5Fix12IiES1_  x1   src/_ZN4cstd5atan2E5Fix12IiES1_.c
//                                                 declares (s32 y, s32 x)
//   0x02012718  func_02012718                x1   src/func_02012718.c, (void *id, int pos)
//
// ---- HOW THE ATTRIBUTION WAS CHECKED -------------------------------------
//
// Two things in this neighbourhood look like evidence that 0x020e1854 is Player
// code, and both are false. They are written down because the next reader will
// hit them too.
//
// 1. ov002 (0x020ad660..0x0210ff40) and ov006 (0x020bfec0..0x02141800) OVERLAP
//    in address space. Name an in-range branch target without filtering by
//    module and this function appears to call `_ZN6Player12St_Fall_MainEv` and
//    `_ZN6Player24St_SlideKickRecover_InitEv`. It calls neither. All four are
//    plain internal branches; the names are ov002 symbols aliasing ov006 code.
//
// 2. The immediate neighbour, `_ZN6Player16St_WallJump_InitEv` at 0x020e17f8,
//    is not the counter-evidence it looks like -- but the problem is NOT that
//    the config name is wrong. 0x020e17f8 in ov002 really is
//    Player::St_WallJump_Init, and ov002's symbol table is right about its own
//    overlay. The defect is downstream of that: because ov002 and ov006 overlap
//    in address space (trap 1), the src BODY carrying that name was decompiled
//    from the bytes at 0x020e17f8 in ov006, which are curling's. It shows:
//    src/_ZN6Player16St_WallJump_InitEv.cpp reads this+0x4eb0, +0x4eb4 and
//    +0x4ee5, dScMgCurling_c fields far past sizeof(Player) = 0x768, as that
//    file's own banner notes.
//
//    So a Player-named TU holds curling behaviour, and its adjacency says
//    nothing about who owns 0x020e1854. Flagged rather than fixed here: which
//    overlay that body should have been read from is a decomp question, not a
//    port one, and it is not this lane's to answer.
//
// What the attribution actually rests on is four already-decompiled siblings
// that interlock with this body at every seam:
//
//   src/func_ov006_020e3078.cpp:32
//       (((C*)c)->*data_ov006_021418b0[*(u8*)(c + 0x4ee4)])();
//     +0x4ee4 is the sub-state index. The release path below writes 0 to it.
//
//   src/func_ov006_020e1b54.c  -- the touch state, the state that hands off here
//       *(u8  *)(c + 0x4ee4) = 1;       selects slot 1, which is this function
//       *(u16 *)(c + 0x4ede) = 0xc000;  seeds the aim angle
//       *(int *)(c + 0x4ebc) = ...;     seeds the scrub anchor this body reads
//       *(int *)(c + 0x4ed4) = 0xff;    seeds the last-delta this body reads
//       *(u8  *)(c + 0x4eea) = 0;       seeds the scrub phase to 0, which is
//                                       the first branch this body takes
//     and it computes +0x4ec0/+0x4ec4 with the SAME grab-offset expression this
//     body re-anchors with at the tail. 0xc000 sits dead centre of the
//     [0x8000,0xffff] window the angle clamp below enforces.
//
//   src/func_ov006_020e2c08.c:24-36  -- the shot
//     reads +0x4ede as the angle and +0x4ec8 as the power and fires on
//     `if (*(int*)(self + 0x4ec8) >= 0x3800)`. Those two values are the whole
//     product of this state.
//
//   src/func_ov006_020e2dbc.c:33-34
//       *(u8*)(c + 0x4ee4) = 0;  *(u8*)(c + 0x4ee5) = 1;
//     the exact pair the release path below writes, already decompiled next
//     door.
//
// A sanity check on the arithmetic, since it is cheap and it passed: the power
// scale saturates at about 21 pixels of travel in one frame, and the sibling's
// firing threshold of 0x3800 needs about 6 pixels per frame sustained. Both are
// sane for a 192x36 drag box, which they would not be if a shift were wrong.
//
// ---- THE FIELDS THIS STATE TOUCHES ---------------------------------------
//
//   +0x4eb0  Fix12  cursor X, clamped to [0x20000, 0xe0000] = 32..224
//   +0x4eb4  Fix12  cursor Y, clamped to [0x94000, 0xb8000] = 148..184
//   +0x4ebc  Fix12  scrub anchor Y
//   +0x4ec0  Fix12  grab offset X = cursor - stylus, re-anchored every frame
//   +0x4ec4  Fix12  grab offset Y
//   +0x4ec8  Fix12  SHOT POWER; peak-hold, halving decay, cap 0xc000
//   +0x4ed4  s32    last vertical delta, whole pixels
//   +0x4ede  u16    AIM ANGLE; atan2, clamped, then averaged with the previous
//   +0x4ee4  u8     sub-state index; set to 0 on release
//   +0x4ee5  u8     "shot is ready"; set to 1 on release
//   +0x4eea  u8     scrub phase: 0 = arm, 1 = reversed, 2 = tracking
//
// The X axis is deliberately weighted half against Y: the aim angle and the
// power both use (dx >> 1) while the dead-zone test uses the full dx. That is
// the ROM's own anisotropy for a box that is wide and short, and it is
// transcribed as-is.
//
// ARGUMENT COUNT. r1 is never read before it is written, and the slot's table
// (data_ov006_021418b0) is arity 0 per MgCurling_StateDispatch.cpp's header, so
// the body takes self alone.
//
// ---- WHY THIS IS NOT CALLED func_ov006_020e1854 --------------------------
//
// Deliberate, and load-bearing. port/tools/stategen.py's reconstruct check
// refuses a hand switch that calls a `func_*` or `_Z*` symbol for an address
// with no delink block:
//
//     if d is None:
//         # the hole: the hand file must NOT call a ROM symbol for it
//         if h.startswith("func_") or h.startswith("_Z"): ... DIVERGE
//
// That guard is right and it stays armed. Its job is to stop somebody
// inventing a definition for a body the decomp does not have, which is the
// same guess port/tools/inferred_stub_guard refuses. Taking the ROM's own
// symbol here would silence a check that should keep firing, and would also
// claim, falsely, that the decomp now has this function.
//
// IT DOES NOT. The decomp still has no matched body and no delink block for
// 0x020e1854; stategen.py will keep reporting it as a hole and that report
// stays TRUE. What changed is only that the PORT can now run the state. The
// `port_` prefix is the repo's established way to say exactly that -- see
// port_player_st_climb_init / port_player_st_climb_main in
// port/unmatched/Player_St_Climb.cpp, dispatched by address from
// port/hal/player_states.inc. The address is kept in the name so the symbol is
// greppable from the dispatch case and from this file's name.
//
// This file retires the moment a byte match lands in src/.

#include <cstdio>
#include <cstdlib>

typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned long long u64;
typedef long long          s64;

extern "C" {

/* the stylus record, read the same way src/func_ov006_020e1b54.c reads it */
extern u8 data_020a0e40;
extern u8 data_020a0de8[];
extern u8 data_020a0dea[];
extern u8 data_020a0deb[];

int  _ZN4cstd4sqrtEy(u64 val);
short _ZN4cstd5atan2E5Fix12IiES1_(int y, int x);
void func_02012718(void *a, int b);

void port_mg_curling_st_020e1854(char *c);
unsigned port_mg_curling_st_020e1854_entries(void);

}  /* extern "C" */

/* (len2 * 9) << 12 is a plain LSL on the DS (0x020e1a84). Doing it on a signed
   int is UB in C++, so the shift is done unsigned and converted back; the
   following >> 4 is then the ROM's ASR. Unreachable in practice -- len2 cannot
   exceed about 102 inside a 192x36 box -- but a transcription should not carry
   UB it can spell away. */
static inline int rom_lsl12(int v)
{
    return (int)((unsigned)v << 12);
}

static inline int rom_lsl12_asr4(int v)
{
    return rom_lsl12(v) >> 4;
}

/* The transcription proper. Kept static and kept PURE: the entry counter and
   the trace live in the wrapper below so that nothing observational sits inside
   the body being transcribed. */
static void st_020e1854_body(char *c)
{
    int idx;
    int sx, sy;
    int oldX, oldY;
    int dx, dy, dySq;
    int len;
    int dAnchor;
    int phase;
    int ang, prevAng;
    int power, held;

    /* 0x020e1854..0x020e1878 -- who is touching, and are they still down */
    idx = data_020a0e40;
    if (data_020a0de8[idx * 4] == 0) {
        /* 0x020e1b1c -- STYLUS RELEASED. The state does not launch the stone
           itself: it drops back to sub-state 0 and raises the ready flag, and
           the parent advances to the throw. Same pair as
           src/func_ov006_020e2dbc.c:33-34. */
        *(u8 *)(c + 0x4ee4) = 0;
        *(u8 *)(c + 0x4ee5) = 1;
        return;
    }

    /* 0x020e187c..0x020e18ac -- cursor follows the stylus, keeping the offset
       it was grabbed at, so it does not snap on the first frame */
    sx = data_020a0dea[idx * 4];
    sy = data_020a0deb[idx * 4];
    oldX = *(int *)(c + 0x4eb0);
    oldY = *(int *)(c + 0x4eb4);
    *(int *)(c + 0x4eb0) = *(int *)(c + 0x4ec0) + (sx << 12);
    *(int *)(c + 0x4eb4) = *(int *)(c + 0x4ec4) + (sy << 12);

    /* 0x020e18b0..0x020e18f8 -- clamp into the drag box. The ROM does the four
       in this order, Y-low, X-low, X-high, Y-high, and re-forms the base
       pointer between each; the order is kept because the low clamp is `<=`
       and the high clamp is `>=` and a reordering is only equivalent while the
       box stays non-degenerate. */
    if (*(int *)(c + 0x4eb4) <= 0x94000) *(int *)(c + 0x4eb4) = 0x94000;
    if (*(int *)(c + 0x4eb0) <= 0x20000) *(int *)(c + 0x4eb0) = 0x20000;
    if (*(int *)(c + 0x4eb0) >= 0xe0000) *(int *)(c + 0x4eb0) = 0xe0000;
    if (*(int *)(c + 0x4eb4) >= 0xb8000) *(int *)(c + 0x4eb4) = 0xb8000;

    /* 0x020e18fc..0x020e1940 -- dead zone. Moved a pixel or less this frame?
       put the cursor back exactly where it was and do nothing else. */
    dy   = (*(int *)(c + 0x4eb4) - oldY) >> 12;
    dx   = (*(int *)(c + 0x4eb0) - oldX) >> 12;
    dySq = dy * dy;
    len  = _ZN4cstd4sqrtEy((u64)(s64)(dx * dx + dySq));
    if (len <= 1) {
        *(int *)(c + 0x4eb0) = oldX;
        *(int *)(c + 0x4eb4) = oldY;
        return;
    }

    /* 0x020e1944..0x020e1a0c -- the scrub detector. It watches the sign of the
       vertical travel away from the anchor and counts a reversal as one rub.
       Phase 0 arms it and chirps, 2 tracks, 1 means a reversal just happened. */
    phase   = *(u8 *)(c + 0x4eea);
    dAnchor = (*(int *)(c + 0x4eb4) - *(int *)(c + 0x4ebc)) >> 12;

    if (phase == 0) {
        /* 0x020e1964..0x020e1998. Note there is NO cooldown test here: the
           touch state next door gates its own 0x1d2 chirp on +0x4ee9, this one
           gates on nothing but the phase. Transcribed as found. */
        func_02012718((void *)0x1d6, *(int *)(c + 0x4eb0));
        *(u8 *)(c + 0x4eea) = 2;
        *(int *)(c + 0x4ed4) =
            (*(int *)(c + 0x4eb4) - *(int *)(c + 0x4ebc)) >> 12;
        *(int *)(c + 0x4ebc) = *(int *)(c + 0x4eb4);
    } else if (phase == 1) {
        /* 0x020e199c..0x020e19dc */
        if (*(int *)(c + 0x4ed4) * dAnchor > 0) {
            /* still travelling the same way; a big enough run re-arms */
            if (dAnchor < 0) dAnchor = -dAnchor;
            if (dAnchor >= 10) *(u8 *)(c + 0x4eea) = 0;
        } else {
            /* 0x020e19d0 -- reached on the `<= 0` branch, where dAnchor still
               carries its sign because the abs above is on the other path */
            *(int *)(c + 0x4ed4) = dAnchor;
            *(int *)(c + 0x4ebc) = *(int *)(c + 0x4eb4);
        }
    } else {
        /* 0x020e19e0..0x020e1a0c -- phase 2, tracking. A sign flip against the
           stored delta is the reversal. */
        if (*(int *)(c + 0x4ed4) * dAnchor < 0) *(u8 *)(c + 0x4eea) = 1;
        *(int *)(c + 0x4ed4) =
            (*(int *)(c + 0x4eb4) - *(int *)(c + 0x4ebc)) >> 12;
        *(int *)(c + 0x4ebc) = *(int *)(c + 0x4eb4);
    }

    /* 0x020e1a10..0x020e1a74 -- aim angle. atan2 of the travel with X halved,
       snapped out of the forward half of the circle, then averaged with the
       previous frame's angle so the arrow eases instead of jumping.

       The snap, exactly as the unsigned compares run it:
         ang <  0x4000            -> 0
         0x4000 <= ang <= 0x8000  -> 0x8000
         ang >  0x8000            -> left alone
       which pins the aim to the half the player shoots from. */
    prevAng = *(u16 *)(c + 0x4ede);
    *(u16 *)(c + 0x4ede) = (u16)_ZN4cstd5atan2E5Fix12IiES1_(dy, dx >> 1);
    ang = *(u16 *)(c + 0x4ede);
    if (ang > 0x8000) {
        /* 0x020e1a48 falls through with `ls` false; no store */
    } else if (ang >= 0x4000) {
        *(u16 *)(c + 0x4ede) = 0x8000;
    } else {
        *(u16 *)(c + 0x4ede) = 0;
    }
    *(u16 *)(c + 0x4ede) = (u16)((*(u16 *)(c + 0x4ede) + prevAng) >> 1);

    /* 0x020e1a58..0x020e1ac8 -- shot power. Peak-hold with a halving decay, so
       a hard scrub sets the bar and easing off slides it down rather than
       dropping it. The ROM stores the averaged angle above BEFORE this sqrt
       runs; the two are independent and the store order is kept. */
    power = _ZN4cstd4sqrtEy((u64)(s64)((dx >> 1) * (dx >> 1) + dySq));
    power = rom_lsl12_asr4(power * 9);
    if (power >= 0xc000) power = 0xc000;
    if (power > *(int *)(c + 0x4ec8)) *(int *)(c + 0x4ec8) = power;
    held = *(int *)(c + 0x4ec8);
    if (held > power) *(int *)(c + 0x4ec8) = held - ((held - power) >> 1);

    /* 0x020e1acc..0x020e1b18 -- re-anchor the grab offset against where the
       stylus actually is now, which is what keeps the cursor glued to it next
       frame. The ROM re-reads data_020a0e40 here rather than reusing idx; kept,
       since nothing in between writes it and the reload is what it does. */
    idx = data_020a0e40;
    sx  = data_020a0dea[idx * 4];
    sy  = data_020a0deb[idx * 4];
    /* Through the helper, not a bare `<< 12`. The subtraction can go negative
       (the cursor sits left of or above the stylus whenever the grab offset is
       negative), and a bare left shift of a negative int is UB even though MSVC
       emits the LSL the ROM wants. Same reason rom_lsl12_asr4 exists; this file
       should not use the helper in one place and the raw shift in another. */
    *(int *)(c + 0x4ec0) = rom_lsl12(((*(int *)(c + 0x4eb0)) >> 12) - sx);
    *(int *)(c + 0x4ec4) = rom_lsl12(((*(int *)(c + 0x4eb4)) >> 12) - sy);
}

/* ---- entry point, plus the proof that it runs -----------------------------
   The counter is the same shape as g_curling_state_hits in
   MgCurling_StateDispatch.cpp. The trace is OFF unless SM64DS_MG_CURLING_TRACE
   is set, so an ordinary run -- and every selftest frame -- is byte-identical
   with it compiled in; it exists because "the state was entered" has to be a
   measurement and not an assumption, and because the refusal line this file
   removes was the only signal anyone had. */
static unsigned g_st_020e1854_entries;

extern "C" unsigned port_mg_curling_st_020e1854_entries(void)
{
    return g_st_020e1854_entries;
}

extern "C" void port_mg_curling_st_020e1854(char *c)
{
    /* Read once, not once per entry. This runs every frame the stylus is down
       and a getenv per state tick is a syscall inside the game loop for a value
       that cannot change; -1 means "not read yet". */
    static int on = -1;
    static int traced;
    if (on < 0) {
        const char *e = std::getenv("SM64DS_MG_CURLING_TRACE");
        on = (e && *e && *e != '0') ? 1 : 0;
    }

    ++g_st_020e1854_entries;

    if (on && !traced) {
        traced = 1;
        std::fprintf(stderr,
                     "  [scene] dScMgCurling_c STATE 0x020e1854 ENTERED "
                     "(port_mg_curling_st_020e1854, host transcription, "
                     "port/unmatched/MgCurling_State_020e1854.cpp)\n");
        std::fflush(stderr);
    }

    st_020e1854_body(c);

    if (on) {
        std::fprintf(stderr,
                     "  [curling] n=%u substate=%u ready=%u phase=%u "
                     "ang=0x%04x power=0x%x cur=(%d,%d)\n",
                     g_st_020e1854_entries,
                     (unsigned)*(u8 *)(c + 0x4ee4),
                     (unsigned)*(u8 *)(c + 0x4ee5),
                     (unsigned)*(u8 *)(c + 0x4eea),
                     (unsigned)*(u16 *)(c + 0x4ede),
                     (unsigned)*(int *)(c + 0x4ec8),
                     (*(int *)(c + 0x4eb0)) >> 12,
                     (*(int *)(c + 0x4eb4)) >> 12);
        std::fflush(stderr);
    }
}
