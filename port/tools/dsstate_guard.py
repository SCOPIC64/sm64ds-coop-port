"""Build-time guard: every hosted DS global must live in the .dsstate section.

The save state (hal/lk6_savestate.cpp) captures the hosted DS main-RAM arena
plus the whole .dsstate section, where every file that hosts DS globals routes
them with the DSSTATE_BEGIN/END markers (hal/dsstate_seg.h). The 0.2.0 restore
crash was a hosted global that was NOT captured -- the ANIM_PTRS SharedFilePtr
table in ov002. This guard is the ratchet that keeps that from recurring: it
reads the linked map and fails the build if a hosted DS symbol landed OUTSIDE
the bracket the snapshot copies.

WHAT COUNTS AS A HOSTED DS SYMBOL
--------------------------------
A public symbol in the read/write data segment whose name is a DS symbol the
port hosts:

    data_0XXXXXXX            a DS main-RAM BSS/data symbol (0x02......)
    data_ovNNN_0XXXXXXX      an overlay data symbol
    LCG_STATE_0XXXXXXX       the particle RNG
    port_ovNNN_image         a whole-overlay host image
    _ZNK...E / _ZN...E       a C++-mangled DS data symbol (rare; the heap
                             registry root iterator is one)

Host-only symbols (walk_window's playlog path, ntr_2x window state, the CRT)
never match these patterns, so they are correctly ignored: they must NOT be in
.dsstate and the guard does not ask them to be.

HOW THE BOUND IS READ
---------------------
hal/dsstate_seg.cpp puts dsstate_lo in .dsstate$aaa and dsstate_hi in
.dsstate$zzz, so [addr(dsstate_lo), addr(dsstate_hi)) is the captured span. A
hosted symbol passes iff its address is inside that half-open range. The two
sentinels themselves are excluded (they are the fence, not payload).

USAGE
    python tools/dsstate_guard.py <path-to-walk_window.map>

Exit 0 if every hosted DS symbol is inside .dsstate; non-zero (with the offending
symbols listed) otherwise.
"""

import re
import sys


# Publics-by-value rows look like:
#   0003:000002e8       _data_ov002_020ff480       005852e8     ov002_data.c.obj
# and, for a symbol the linker flagged as code, with an extra one-letter column:
#   0001:000936f0       _data_ov000_020ab3c4       004946f0 f   func_ov001_020ab3c4.c.obj
# groups: seg:off, name, rva+base, flag ('f' = function, 'i' = import, or none),
# object. The flag matters: dsd names some FUNCTIONS `data_*` (a symbol it saw
# referenced as data that turned out to be code), and those live in .text. They
# are not save state and must not be asked to move into .dsstate.
ROW = re.compile(
    r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+"
    r"(?:([fi])\s+)?(\S+)")

# The map's leading section table. Rows look like:
#   0005:00000008 0002b8c8H .dsstate$mmm            DATA
# groups: seg, offset, length, name, class
SECTION_ROW = re.compile(
    r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+([0-9a-fA-F]+)H\s+(\.\S+)\s+(\S+)\s*$")

# A hosted DS symbol by name. Leading underscore is MSVC's cdecl decoration on
# C symbols; C++-mangled names (_ZN..) keep their own leading underscore-Z.
HOSTED = re.compile(
    r"^_?("
    r"data_0[0-9a-fA-F]{7}"          # DS main-RAM data/bss
    r"|data_ov\d+_0[0-9a-fA-F]{7}"   # overlay data
    r"|LCG_STATE_0[0-9a-fA-F]{7}"    # particle RNG
    r"|port_ov\d+_image"             # whole-overlay host image
    r")$"
    # plus the handful of C++-mangled DS data symbols the port hosts; matched
    # loosely by the trailing E on a data name we know lands in rw data.
    r"|^_ZN6Memory16rootHeapIteratorE$"
    r"|^_ZN6Memory25isRootHeapIterInitializedE$")

SENTINELS = {"_dsstate_lo", "dsstate_lo", "_dsstate_hi", "dsstate_hi"}

# Named boot-constant exclusions. These ARE hosted DS globals but are written
# exactly once at boot and never during play, so they cannot diverge between a
# save and a load and are deliberately outside .dsstate. Each entry names every
# map spelling of the symbol (the real storage AND its /alternatename aliases:
# an alias appears in the map as its own public at the same address, so the
# guard must know both or it flags the alias). Keep the rationale next to the
# definition too -- data_020a0eac's is in hal/cxxname_bridge.cpp.
BOOT_CONSTANT = {
    # the game heap pointer: Heap::InitializeGameHeap writes it once off the
    # main.c boot spine; the port seeds it once before the frame loop.
    "_data_020a0eac", "data_020a0eac", "_data_020a0eac_c", "data_020a0eac_c",
    "?data_020a0eac@@3PAUHeap@@A",
}

# Object files whose data_* symbols are READ-ONLY ROM constants, not mutable
# game state: the game never writes them, so they never diverge between a save
# and a load and do not need to roll back. romdata.py emits only such constants
# (its own header says so, and it explicitly excludes the symbols that ARE
# written at runtime, which live in model_host.cpp / auto_bss.cpp and so are in
# .dsstate). Capturing them would just bloat the snapshot by the ROM's constant
# tables. They are allowed to sit outside .dsstate.
CONST_OBJS = ("romdata.c.obj",)


def main():
    if len(sys.argv) != 2:
        sys.exit("usage: dsstate_guard.py <walk_window.map>")
    try:
        text = open(sys.argv[1], encoding="ascii", errors="replace").read()
    except OSError as e:
        sys.exit(f"dsstate_guard: cannot read map: {e}")

    lo = hi = None
    rows = []
    dsstate_segs = {}
    for line in text.splitlines():
        s = SECTION_ROW.match(line)
        if s:
            sseg, soff, slen, sname, sclass = s.groups()
            if sname.startswith(".dsstate"):
                dsstate_segs.setdefault(sseg, []).append(sname)
            continue
        m = ROW.match(line)
        if not m:
            continue
        seg, off, name, rva, flag, obj = m.groups()
        if flag == "f":       # code, not data -- see the ROW comment
            continue
        rva = int(rva, 16)
        if name in ("_dsstate_lo", "dsstate_lo"):
            lo = rva
        elif name in ("_dsstate_hi", "dsstate_hi"):
            hi = rva
        rows.append((name, rva, obj))

    # The split-section trap. MSVC will not merge two same-named sections whose
    # characteristics differ, so a contribution routed in with a bare bss_seg
    # (CNT_UNINITIALIZED_DATA) forms a SECOND output section also called
    # .dsstate, placed outside the sentinels, and every hosted global in it is
    # silently uncaptured. The compiler says only `LNK4078` about it, which is a
    # warning nobody reads in a 2000-line build log. Catch it here, by name,
    # because the symptom otherwise reads as "hundreds of files forgot their
    # markers" and sends the reader off fixing files that are already correct.
    if len(dsstate_segs) > 1:
        print(f"dsstate_guard: .dsstate was SPLIT across {len(dsstate_segs)} "
              f"segments -- the linker refused to merge them because their "
              f"section attributes differ (this is what LNK4078 means). Only "
              f"the segment holding the sentinels is captured; the rest is "
              f"silently lost on a restore.", file=sys.stderr)
        for sseg, names in sorted(dsstate_segs.items()):
            shown = ", ".join(sorted(set(names))[:4])
            more = "" if len(set(names)) <= 4 else f" (+{len(set(names)) - 4} more)"
            print(f"  segment {sseg}: {shown}{more}", file=sys.stderr)
        print("Fix: every file routing into .dsstate must declare the section's "
              "attributes BEFORE routing, so both bss- and data-flavoured "
              "contributions agree: `#pragma section(\".dsstate$mmm\", read, "
              "write)`. hal/dsstate_seg.h does this for anything that includes "
              "it; a generator emitting raw pragmas must emit it too.",
              file=sys.stderr)
        sys.exit(1)

    if lo is None or hi is None:
        sys.exit("dsstate_guard: .dsstate sentinels (dsstate_lo/dsstate_hi) not "
                 "found in the map -- hal/dsstate_seg.cpp is not linked into this "
                 "target, or the section collapsed. The save state cannot "
                 "capture the hosted DS globals without it.")
    if hi <= lo:
        sys.exit(f"dsstate_guard: .dsstate span is empty or inverted "
                 f"(lo={lo:#x} hi={hi:#x}); the section did not gather anything.")

    outside = []
    seen = set()
    for name, rva, obj in rows:
        if name in SENTINELS or name in BOOT_CONSTANT:
            continue
        if not HOSTED.match(name):
            continue
        if name in seen:      # a symbol can appear under several C++ aliases
            continue
        seen.add(name)
        if obj.endswith(CONST_OBJS):   # read-only ROM constants, never diverge
            continue
        if not (lo <= rva < hi):
            outside.append((name, rva, obj))

    if outside:
        print(f"dsstate_guard: {len(outside)} hosted DS symbol(s) are OUTSIDE "
              f".dsstate [{lo:#x}, {hi:#x}) and would NOT be captured by a save "
              f"state:", file=sys.stderr)
        for name, rva, obj in sorted(outside, key=lambda r: r[0])[:60]:
            print(f"  {name}  @ {rva:#010x}  ({obj})", file=sys.stderr)
        if len(outside) > 60:
            print(f"  ... and {len(outside) - 60} more", file=sys.stderr)
        print("Fix, in the file that DEFINES each symbol above:\n"
              "  - a plain global: put DSSTATE_BEGIN before and DSSTATE_END "
              "after it (hal/dsstate_seg.h).\n"
              "  - a global placed by hand into its own grouped section to keep "
              "ROM spacing (the COMM/MMBLK/SAVEBLK-style macros): the segment "
              "default does NOT reach it, because __declspec(allocate) wins. "
              "Rename its section into the captured family instead, "
              "`.dsstate$<family><NNNN>`, the way tools/ovdata.py names packed "
              "overlay mounts `.dsstate$pkNNN_SSSS`.\n"
              "  - a generated overlay file: have tools/ovdata.py emit the "
              "markers.", file=sys.stderr)
        sys.exit(1)

    print(f"dsstate_guard: OK -- {len(seen)} hosted DS symbols all inside "
          f".dsstate [{lo:#x}, {hi:#x}), {(hi - lo)} bytes captured.")


if __name__ == "__main__":
    main()
