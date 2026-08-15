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

"Storage the port actually hosts" is a RESIDENCY question and this asks it with
ovdata's own model, imported rather than re-derived. It used to decide with a
window-overlap test of its own: any address inside a window that overlapped any
other window was written off as unanswerable. That is much coarser than what
--cross actually does, and it fails in the direction that matters -- it goes
quiet. An ov006 mount alone would have put every level overlay and most of
ov002 inside one overlapping window and excused essentially the whole sweep,
exactly when a 519 KB mount makes the check load-bearing. Two copies of the
rules would drift apart the same way, so there is only one: ovdata.Residency.
"""

import collections
import json
import pathlib
import re
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import ovdata  # noqa: E402

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

    # What the port HOSTS, by DS address, kept per owning overlay so the
    # residency filter below can drop the ones a given mount could never be
    # looking at. Claims outside their own overlay's footprint are dropped for
    # the same reason ovdata drops them: the per-symbol gap hunter reaches past
    # the image end, and those blocks are not that overlay's storage.
    windows = {}
    docs = []
    for m in maps:
        doc = json.loads(m.read_text())
        docs.append((m, doc))
        windows[doc["overlay"]] = tuple(doc["window"])

    # Footprints come from the sidecars, not from a second read of the config.
    # cross_mode() already checks the two agree, over these same sidecars, in
    # the build step immediately before this one -- re-deriving them here would
    # add nothing but a duplicate copy of the yaml-vs-delinks base notice.
    # Residency fills in the scene occupants and ov075, which have no sidecar.
    res = ovdata.Residency(root, {int(ov[2:]): tuple(win)
                                  for ov, win in windows.items()})

    claims_by_ov = {}
    for _, doc in docs:
        s, e = windows[doc["overlay"]]
        for a, sz, n in doc["provides"]:
            if s <= a < e:
                claims_by_ov.setdefault(doc["overlay"], []).append(
                    (a, a + sz, n))

    _cov = {}

    def covering_for(src):
        key = tuple(sorted(ov for ov in claims_by_ov
                           if res.coresident(src, int(ov[2:]))))
        if key not in _cov:
            _cov[key] = ovdata.make_covering(
                [c for ov in key for c in claims_by_ov[ov]])
        return _cov[key]

    def window_count(src, v):
        return sum(1 for ov, (s, e) in windows.items()
                   if s <= v < e and res.coresident(src, int(ov[2:])))

    # Which mount emitted which array. A sidecar sits next to the .c it
    # describes, and the pointer's OWNER is what the residency test keys on --
    # the same address means different storage depending on who is reading it.
    ov_of_file = {m.parent / m.name[:-len(".map")]: doc["overlay"]
                  for m, doc in docs}

    arrays = {}          # name -> bytes
    array_ov = {}        # name -> owning overlay, or None
    patched = collections.defaultdict(set)   # name -> {byte offset}
    for c in sorted(where.glob("*.c")):
        text = c.read_text()
        for mm in ARRAY.finditer(text):
            name, size, body = mm.group(1), int(mm.group(2)), mm.group(3)
            vals = [int(x) for x in body.replace("\n", "").split(",") if x.strip()]
            if len(vals) == 1 and vals[0] == 0:
                vals = [0] * size            # `= { 0 }` pack padding
            arrays[name] = bytes(vals[:size])
            array_ov[name] = ov_of_file.get(c)
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
    unattributed = set()
    for name in sorted(arrays):
        blob = arrays[name]
        src_ov = array_ov.get(name)
        for off in range(0, len(blob) - 3, 4):
            v = int.from_bytes(blob[off:off + 4], "little")
            if not (0x02000000 <= v < 0x02400000):
                continue
            if off in patched[name]:
                continue
            raw_elsewhere += 1
            if src_ov is None:
                # No sidecar, so no owner, so no residency answer. Every array
                # in host-src comes from an ovdata mount today and this branch
                # is dead; it is here so a future emitter cannot be silently
                # judged against the wrong overlay's view of memory.
                unattributed.add(name)
                continue
            # Words a hand seat re-patches later, on purpose. ovdata leaves
            # them raw and hal/actor_overlays.cpp writes the same host address
            # afterwards, so reporting them here would be reporting the design.
            if (name, off) in ovdata.HAND_SEATED:
                continue
            src = int(src_ov[2:])
            # Exactly the cross pass's own test: one co-resident window owns
            # the address, and a co-resident mount provides storage there.
            if window_count(src, v) != 1:
                continue
            hit = covering_for(src)(v)
            if hit is None:
                continue
            raw_elsewhere -= 1
            findings.append((name, off, v, hit[2]))

    if unattributed:
        print(f"  NOTE {len(unattributed)} arrays have no ovdata sidecar and "
              f"were not checked: {', '.join(sorted(unattributed)[:5])}")

    for name, off, v, owner in findings:
        print(f"  RAW {name}+{off} = {v:#010x} -- hosted by {owner}, "
              f"no patch statement")
    print(f"ovsweep: {len(arrays)} arrays, {len(findings)} raw cross-mount "
          f"words, {raw_elsewhere} raw words with no host (arm9, an unmounted "
          f"overlay, or a contested level range)")
    return 1 if findings else 0


if __name__ == "__main__":
    sys.exit(main())
