// TEMPORARY PROBE: unlock the current level's cannon at boot, through the
// game's own matched save-write path.
//
// This is a test rig, not game behaviour. On the DS the cannon-unlock save bit
// is written by Bob-omb Buddy after his dialogue completes: buddy state 2
// (func_ov084_0212c1a0, matched src, sliced in slice_gate32) calls
// OpenCannonInCurLevel() once GetTalkState reports the box has closed. The
// dialogue that drives that talk is being built in a PARALLEL lane, so on a
// headless BoB boot nobody walks up to the buddy and the bit stays clear --
// which means CannonHatch::InitResources reads IsCannonOpenInCurLevel()==0, the
// grate collider stays disabled, the lid never opens, and the cannon-entry
// chain never gets a chance to fire.
//
// This probe writes the SAME bit the buddy would, through the SAME matched
// setter, so the cannon-entry chain can be exercised end to end before the
// dialogue lands. It calls nothing the game does not: OpenCannonInCurLevel ->
// OpenCannonInLevel(data_0209f2f8) -> SublevelToLevel -> OpenCannon(course),
// which is data_0209caa0[4] |= 1<<course -- the staged save block
// (.dsstate$savblk0000, hal/level_boot.cpp). It is idempotent (OR of a bit already
// set) and reads the current level from data_0209f2f8, so it must run AFTER
// level_boot sets data_0209f2f8 = port_level_id() and on the spawn boot path
// only. Its one call site is described in the wiring report; delete this file
// and that call when the buddy dialogue drives the open.
//
// SM64DS_CANNONS_OPEN=1  -- unlock the current level's cannon at boot.
#include <cstdio>
#include <cstdlib>

extern "C" {

/* OpenCannonInCurLevel at arm9 0x02013828 -- matched src src/OpenCannonInCurLevel.c,
   linked by slice_gate32. Plain C linkage, no alternatename needed. */
void OpenCannonInCurLevel(void);

/* the current level, set by level_boot to port_level_id(); the setter reads it.
   Declared here only for the trace line. Matched-side symbol is the staged bss
   in hal/level_boot.cpp. */
extern signed char data_0209f2f8;

/* Returns nonzero if the probe is on. */
int port_cannons_open_probe_on(void)
{
    static int on = -1;
    if (on < 0)
        on = std::getenv("SM64DS_CANNONS_OPEN") != 0;
    return on;
}

/* Set the current level's cannon-unlock save bit through the matched setter.
   Runs on every spawn boot when the switch is on; idempotent. */
void port_cannons_open_probe(void)
{
    if (!port_cannons_open_probe_on())
        return;
    OpenCannonInCurLevel();
    std::fprintf(stderr, "[probe] SM64DS_CANNONS_OPEN: OpenCannonInCurLevel() "
                 "for level %d -- cannon-unlock bit set through the matched "
                 "save path\n", (int)data_0209f2f8);
}

} /* extern "C" */
