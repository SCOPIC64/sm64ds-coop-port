// Stage::InitResources' closure: the host bodies and faces slot 0 needs.
//
// Run link60 Stage 4, lane SD0. port/stage_lifecycle_map.txt section 7 sizes
// the slot-0 swap at SIXTEEN named symbols measured by a real link; this file
// is the code half of closing them, port/tools/romdata.py and
// hal/ptr_tables.cpp are the data half, and port/slice_s4_slot0.txt is the
// wiring. Everything here is one of three shapes and each shape is separated
// below so a reviewer can check them by class rather than one at a time:
//
//   PORT_HOST_ABI bodies   a DS subsystem the port has decided not to have,
//                          whose observable is nothing
//   C-name faces           the TU is compiled and publishes only the MSVC
//                          mangling; the caller spells the Itanium C name
//   storage                two 4-byte BSS words and one alias
//
// ===========================================================================
// PART 1 -- THE THREE PORT_HOST_ABI BODIES
// ===========================================================================
//
// Each is the one-line companion to a decision the port made in another
// lane's file and wrote down there. Neither the decision nor the wording is
// new here; what is new is that Stage::InitResources now calls them by name.
//
// PORT_HOST_ABI: UnloadArchive releases a NARC the DS card loader mounted, and
// the host has no mount to release. hal/scene_boot.cpp's LoadArchive face
// records the whole trade in full: hal/fs.cpp resolves archive-interior file
// ids (>= 0x8000) LAZILY out of port_archive_map, so on the host an archive is
// never "mounted" and never "not mounted" -- the id resolves either way.
// LoadArchive's host body therefore answers "is archive N available" with 1
// for every archive in the table, and the symmetric answer for UnloadArchive
// is to do nothing. It returns void in the ROM and has no out-parameter, so
// there is no observable to reproduce.
//   Stage::InitResources calls it in two places: the 2..5 sweep that drops
// every level-specific archive except the one this level wants, and the tail
// release when func_0203da3c() != 2. Both are heap hygiene on a heap the port
// does not fill this way.
//   The alternative is src/UnloadArchive.c, which wants func_02018908 and the
// DS card overlay chain under it plus data_0208ecf4 hosted as raw ROM bytes
// with its DS string pointers. Eleven TUs to undo a mount that never happened.
//
// PORT_HOST_ABI: LoadLevelOverlays / UnloadLevelOverlays load and unload the
// per-level DS overlay, and the port has no overlay loader at all. Every
// overlay it hosts is a static host array mounted at build time
// (port/scene_boot_map.txt section 2 is the ruling and states it in these
// words: THE OVERLAY LOAD IS NOT SKIPPED, IT DOES NOT EXIST ON THE HOST).
// port_level_mount_register / port_level_mounts_install in hal/level_boot.cpp
// is what stands where the loader would be, and it runs before the boot rather
// than from inside it.
//   src/_Z17LoadLevelOverlaysi.cpp would compile: its four callees
// (LoadOverlay, LoadOrUnloadObjectOverlays, and the two id tables) are already
// hosted or stubbed. It is left out anyway, because what it would do is drive
// hal/scene_boot.cpp's empty LoadOverlay with an id read out of
// data_020758c8 and then latch that id into data_02092130 as "the resident
// level overlay" -- a residency claim about a loader that does not exist,
// which the next UnloadLevelOverlays would then act on. An empty body makes no
// claim. Both return void and neither has an out-parameter.
//
// THE THIRD BODY IS NOT DEMANDED BY THIS SLICE AND IS SUPPLIED ANYWAY.
// Measured (port/tools/closure.py over port/slice_s4_slot0.txt against
// walk_window.map): the slice's link wants _UnloadArchive and
// __Z17LoadLevelOverlaysi and does NOT want __Z19UnloadLevelOverlaysi, because
// nothing on it calls that one -- its caller is Stage::CleanupResources, which
// is slot 3 and is not in this lane. port/stage_lifecycle_map.txt section 7
// names three bodies and section 5 says why the third belongs with them, so it
// is here, unreferenced, one line, pre-paying a symbol slot 3 will want. Said
// out loud because "the handoff asked for three and the link asks for two" is
// exactly the kind of quiet discrepancy this repo has been bitten by.
#include "dsstate_seg.h"

extern "C" {

void UnloadArchive(int)             {}
void _Z17LoadLevelOverlaysi(int)    {}
void _Z19UnloadLevelOverlaysi(int)  {}

}  /* extern "C" */
