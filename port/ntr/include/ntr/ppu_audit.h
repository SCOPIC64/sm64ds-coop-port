// The 2D register audit: which unmodelled hardware a scene ACTUALLY touches.
//
// Inert unless SM64DS_PPU_AUDIT names an output file. See ntr/ppu_audit.cpp for
// what it samples and, more importantly, for what sampling can and cannot see.

#ifndef NTR_PPU_AUDIT_H
#define NTR_PPU_AUDIT_H

#include <stdint.h>

namespace ntr {

// True when SM64DS_PPU_AUDIT is set. Cheap after the first call.
bool ppu_audit_on();

// Sample both engines' 2D register files, the OAM object-mode census and the
// stylus record. `tag` names the seam that called it, so the table can say
// which scan-out a value was live at. Call once per scan-out.
void ppu_audit_sample(const char *tag);

// Record a 2D-surface access that went through the io.cpp proxy. Exact rather
// than sampled, and a strict subset: most of the game's 2D stores reach mapped
// memory directly and never appear here.
void ppu_audit_proxy(uint32_t addr, uint64_t value, unsigned width, bool is_write);

// Record a block copy's DESTINATION, bucketed by DS memory region. The BG
// tilemap upload path ends in a halfword copy primitive, so this separates
// "the tilemap is empty because nothing wrote it" from "something wrote it
// somewhere else". Cheap and inert when the audit is off.
void ppu_audit_note_copy(uint32_t src, uint32_t dst, uint32_t bytes);

// The same for the 32-byte-block primitives, which move a tilemap into its
// upload buffer one step EARLIER than the halfword copy above. Distinct
// (src, dst) pairs with the source's nonzero count taken before the move, so
// a buffer that arrives at VRAM empty can be walked back to the step that
// should have filled it.
void ppu_audit_note_copy32(uint32_t src, uint32_t dst, uint32_t bytes);

// Write the table. Registered with atexit on first sample; also called every
// flush interval so a run that faults still leaves evidence.
void ppu_audit_dump();

}  // namespace ntr

#endif
