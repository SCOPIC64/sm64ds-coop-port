#!/usr/bin/env python3
"""Audit the synthetic gap blocks the overlay-data generator emits.

tools/ovdata.py emits two different things it calls a "gap", and only one of
them is save state:

  pkNNN_gap_ADDR     PACK PADDING. Inert filler between two named symbols in a
                     packed mount, there so the named symbols keep their ROM
                     relative spacing. Nothing points into it and nothing reads
                     it. Already routed into .dsstate by an explicit
                     __declspec(allocate) per slot.

  port_ovNNN_gap_ADDR
                     A SYNTHETIC GAP BLOCK. Real overlay bytes for an
                     un-symbolized static that some emitted pointer targets.
                     The patch pass rebases pointers ONTO it and the game's
                     walks step through it, so it is mutable DS state. A plain
                     mount emits it while the .dsstate default data_seg is
                     open; a packed mount has no such default.

Reads the generated host sources (build/port/host-src/*.c) and reports, per
mount, the synthetic gap blocks and whether the mount is packed.

    python port/tools/gapaudit.py <host-src-dir> [--map <walk_window.map>]

With --map, each block is also looked up in the linked map and reported as
INSIDE or OUTSIDE the captured [dsstate_lo, dsstate_hi) span, which is the
question that matters: a block outside it does not roll back on a load.
"""

import re
import sys
from pathlib import Path

GAP = re.compile(r"^u8 (port_ov\d+_gap_[0-9a-f]{8})\[(\d+)\]")
PACKED = re.compile(r'__declspec\(allocate\("\.dsstate\$pk')

ROW = re.compile(
    r"^\s*([0-9a-fA-F]{4}):([0-9a-fA-F]{8})\s+(\S+)\s+([0-9a-fA-F]{8})\s+"
    r"(?:([fi])\s+)?(\S+)")


def read_map(path):
    lo = hi = None
    at = {}
    for line in Path(path).read_text(encoding="ascii", errors="replace").splitlines():
        m = ROW.match(line)
        if not m:
            continue
        _, _, name, rva, flag, obj = m.groups()
        if flag == "f":
            continue
        rva = int(rva, 16)
        if name in ("_dsstate_lo", "dsstate_lo"):
            lo = rva
        elif name in ("_dsstate_hi", "dsstate_hi"):
            hi = rva
        at.setdefault(name.lstrip("_"), rva)
    return lo, hi, at


def main():
    argv = sys.argv[1:]
    if not argv:
        sys.exit("usage: gapaudit.py <host-src-dir> [--map <walk_window.map>]")
    src = Path(argv[0])
    mp = argv[argv.index("--map") + 1] if "--map" in argv else None
    lo = hi = None
    at = {}
    if mp:
        lo, hi, at = read_map(mp)
        if lo is None or hi is None:
            sys.exit("gapaudit: no .dsstate sentinels in the map")
        print(f"captured span [{lo:#x}, {hi:#x})  {hi - lo} bytes")

    tot_n = tot_b = 0
    out_n = out_b = 0
    mounts = []
    for f in sorted(src.glob("*.c")):
        text = f.read_text(encoding="ascii", errors="replace")
        packed = bool(PACKED.search(text))
        blocks = [(m.group(1), int(m.group(2)))
                  for m in (GAP.match(l) for l in text.splitlines()) if m]
        if not blocks:
            continue
        rows = []
        for name, size in blocks:
            where = ""
            if mp:
                rva = at.get(name)
                if rva is None:
                    where = "ABSENT"
                elif lo <= rva < hi:
                    where = "inside"
                else:
                    where = "OUTSIDE"
                    out_n += 1
                    out_b += size
            rows.append((name, size, where))
        tot_n += len(blocks)
        tot_b += sum(s for _, s in blocks)
        mounts.append((f.name, packed, rows))

    for fname, packed, rows in mounts:
        kind = "PACKED" if packed else "plain "
        print(f"{kind} {fname}: {len(rows)} synthetic gap block(s), "
              f"{sum(r[1] for r in rows)} bytes")
        for name, size, where in rows:
            print(f"    {name:<32} {size:>6}  {where}")

    print(f"\nTOTAL synthetic gap blocks: {tot_n}, {tot_b} bytes, "
          f"over {len(mounts)} mount(s)")
    packed_rows = [(f, r) for f, p, r in mounts if p]
    pn = sum(len(r) for _, r in packed_rows)
    pb = sum(s for _, r in packed_rows for _, s, _ in r)
    print(f"  in PACKED mounts: {pn} blocks, {pb} bytes, "
          f"over {len(packed_rows)} mount(s)")
    if mp:
        print(f"  OUTSIDE the captured span: {out_n} blocks, {out_b} bytes")
    return 1 if out_n else 0


if __name__ == "__main__":
    sys.exit(main())
