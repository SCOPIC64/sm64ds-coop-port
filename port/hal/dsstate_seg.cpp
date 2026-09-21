/* The two sentinels that bracket the hosted-DS-state section family. See
 * hal/dsstate_seg.h for the scheme. lk6_savestate.cpp captures the bytes
 * between them.
 *
 * dsstate_lo sits in .dsstate$aaa and dsstate_hi in .dsstate$zzz; every hosted
 * global a DSSTATE_BEGIN/END block emits lands in .dsstate$mmm, which the
 * linker orders after aaa and before zzz. The two sentinels are one byte each
 * and their addresses are the capture bounds: &dsstate_hi - &dsstate_lo is the
 * whole hosted-DS extent, allocator-independent and recomputed by the linker
 * every build, so a newly hosted global is captured the moment it is placed in
 * the section -- no list to grow.
 *
 * These live in their own translation unit so nothing accidentally emits a
 * global between the two markers here. */
#include "dsstate_seg.h"

#pragma section(".dsstate$aaa", read, write)
#pragma section(".dsstate$zzz", read, write)

extern "C" {
__declspec(allocate(".dsstate$aaa")) char dsstate_lo = 0;
__declspec(allocate(".dsstate$zzz")) char dsstate_hi = 0;
}
