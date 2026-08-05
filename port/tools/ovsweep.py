#!/usr/bin/env python3
"""Flag raw cross-overlay pointer words left in the generated overlay data.

    python port/tools/ovsweep.py [build/port/host-src]

ovdata.py mounts each overlay in its own invocation, so its per-mount pointer
pass can only rebase a word whose target is inside that mount, and the --cross
pass afterwards handles the rest. This checks the RESULT rather than either
pass's own bookkeeping: it reads the emitted C, reconstructs the bytes, and
asks whether any aligned word still holds the DS address of storage the port
actually hosts, with no patch statement anywhere that fixes it.

Why it is worth keeping: a miss is silent. ntr/io.cpp reserves DS main RAM at
0x02000000 as committed zeroed pages, so a raw DS pointer reads back zeros
instead of trapping, and for an OamAttr list that means no 0xffff terminator
and an unbounded walk in OAM::Render the moment the sprite is culled. The
fault lands megabytes from the cause. See hal/oam_lists.cpp.

The emitted C legitimately CONTAINS raw DS addresses -- the arrays hold ROM
bytes and the patch functions fix them at runtime -- so a word is only a
finding if no patch statement covers it. Exit status is 1 if any is found.
"""

import collections
import json
import pathlib
import re
import sys

# u8 NAME[123] = { 1,2,3 };  -- with or without the --pack section pragmas.
ARRAY = re.compile(r"(?:^|\s)(?:static\s+)?u8\s+(\w+)\[(\d+)\]\s*=\s*\{([^}]*)\}")
# *(unsigned int *)(NAME + 12) = ...;
PATCH = re.compile(r"\*\(unsigned(?: int)? \*\)\((\w+) \+ (\d+)\)")
# whole-image mounts patch through a table instead: { 4348u, 3204u },
REBASE_ROW = re.compile(r"\{\s*(\d+)u,\s*\d+u\s*\}")
REBASE_HEAD = re.compile(r"static const unsigned (\w+)_rebase\[\]\[2\]")


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    where = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 \
        else root / "build/port/host-src"
    if not where.is_dir():
        sys.exit(f"no such directory: {where}  (build the port first)")

    maps = sorted(where.glob("*.c.map"))
    if not maps:
        sys.exit(f"no ovdata sidecars in {where}  (build the port first)")

    # What the port HOSTS, by DS address. A word holding an address in here is
    # a word that could have been rebased, which is what makes it a finding
    # rather than an ordinary unmounted pointer.
    hosted = []
    windows = {}
    for m in maps:
        doc = json.loads(m.read_text())
        windows[doc["overlay"]] = tuple(doc["window"])
        for a, sz, n in doc["provides"]:
            hosted.append((a, a + sz, n))
    hosted.sort()

    # Level overlays share a DS window, so an address in that range names
    # whichever level is loaded and no static answer is right. ovdata --cross
    # refuses those on purpose; this must not report them as misses.
    contested = set()
    for ov, (s, e) in windows.items():
        for ov2, (s2, e2) in windows.items():
            if ov2 != ov and s2 < e and s < e2:
                contested.add((s, e))

    def is_contested(v):
        return any(s <= v < e for s, e in contested)

    def hosts(v):
        for s, e, n in hosted:
            if s <= v < e:
                return n
        return None

    arrays = {}          # name -> bytes
    patched = collections.defaultdict(set)   # name -> {byte offset}
    for c in sorted(where.glob("*.c")):
        text = c.read_text()
        for mm in ARRAY.finditer(text):
            name, size, body = mm.group(1), int(mm.group(2)), mm.group(3)
            vals = [int(x) for x in body.replace("\n", "").split(",") if x.strip()]
            if len(vals) == 1 and vals[0] == 0:
                vals = [0] * size            # `= { 0 }` pack padding
            arrays[name] = bytes(vals[:size])
        for mm in PATCH.finditer(text):
            patched[mm.group(1)].add(int(mm.group(2)))
        mh = REBASE_HEAD.search(text)
        if mh:
            tag = mh.group(1) + "_image"
            tail = text[mh.end():text.index("};", mh.end())]
            for row in REBASE_ROW.finditer(tail):
                patched[tag].add(int(row.group(1)))

    findings = []
    raw_elsewhere = 0
    for name in sorted(arrays):
        blob = arrays[name]
        for off in range(0, len(blob) - 3, 4):
            v = int.from_bytes(blob[off:off + 4], "little")
            if not (0x02000000 <= v < 0x02400000):
                continue
            if off in patched[name]:
                continue
            owner = hosts(v)
            if owner is None or is_contested(v):
                raw_elsewhere += 1       # arm9, an unmounted overlay, or a
                continue                 # level range with no static answer
            findings.append((name, off, v, owner))

    for name, off, v, owner in findings:
        print(f"  RAW {name}+{off} = {v:#010x} -- hosted by {owner}, "
              f"no patch statement")
    print(f"ovsweep: {len(arrays)} arrays, {len(findings)} raw cross-mount "
          f"words, {raw_elsewhere} raw words with no host (arm9, an unmounted "
          f"overlay, or a contested level range)")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
