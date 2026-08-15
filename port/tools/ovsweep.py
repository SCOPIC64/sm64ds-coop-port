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
# pkNNN_gap_020d7db8 / port_ov022_gap_02114874 -- the DS address is the name.
GAP_NAME = re.compile(r"_gap_([0-9a-f]{8})$")


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

    # WHERE EACH ARRAY LIVES IN DS MEMORY, so a word can be tested against the
    # delink table below. Three sources, because not every emitted array is a
    # named symbol in a sidecar: the named symbols and the synthetic gap blocks
    # are, a whole-image mount is one array at its overlay's base, and --pack's
    # padding blocks carry their own address in their name (pkNNN_gap_ADDR).
    # Anything else is refused rather than skipped -- an array with no address
    # cannot be checked, and quietly not checking it is how this goes blind.
    _relocs = {}

    def relocs_for(ov):
        """The overlay's kind:load sites, or an empty map for an unowned array.

        Shared with ovdata rather than re-parsed, for the reason the docstring
        gives about two copies of a rule drifting apart.
        """
        if ov not in _relocs:
            _relocs[ov] = ovdata.load_relocs(root, ov) if ov else {}
        return _relocs[ov]

    ds_addr = {}
    for _, doc in docs:
        for a, _sz, n in doc["provides"]:
            ds_addr[n] = a
        if doc["mode"] == "whole":
            ds_addr[f"port_{doc['overlay']}_image"] = doc["window"][0]

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

    # A WORD IS A POINTER BECAUSE THE DELINK TABLE SAYS SO, not because its
    # value looks like an address. This check reads the same standard of
    # evidence the emitter it checks now uses (see the in-mount pass in
    # ovdata.py), and it has to, or the two disagree by construction: ovdata
    # declines to patch a word relocs.txt does not list, and a value-range
    # sweep then reports that same word as an unpatched pointer.
    #
    # ov007 is where the two came apart. data_ov007_020ef022 is 3830 bytes at a
    # DS address ending in 2, so a 4-byte stride from the array start walks a
    # grid the ROM never used, and at offset 1488 the byte run 08 0e 0f 02
    # reads as 0x020f0e08, which lands inside data_ov007_020f020e. It is four
    # entries of a byte table, not a pointer, and no relocation is recorded
    # there.
    #
    # WHAT THIS GIVES UP, stated rather than argued away. reloc_blob() starts
    # from the ROM image bytes and OVERWRITES only the recorded sites, so a
    # pointer baked into the image with no relocation recorded against it would
    # survive into the emitted array and is now invisible to the emitter and to
    # this check both, where the old value scan would have caught it. The claim
    # here is empirical, not structural: that set is empty today. Of the 2753
    # patches the per-symbol mounts emit, every site is 4-aligned and listed in
    # relocs.txt, and all 92 mounts regenerate byte-identical under the gate, so
    # nothing the emitter used to rebase is lost. Injected-defect coverage is
    # unchanged at 30 of 30 across 30 mounts. If a delink table ever under-reports
    # a real pointer, this goes quiet about it, and the place that would catch it
    # is a delink-table check rather than a value scan here.
    unaddressed = set()
    for name in arrays:
        if name in ds_addr:
            continue
        mg = GAP_NAME.search(name)
        if mg:
            ds_addr[name] = int(mg.group(1), 16)
        else:
            unaddressed.add(name)
    if unaddressed:
        sys.exit(f"ovsweep: {len(unaddressed)} emitted arrays have no DS "
                 f"address (not in a sidecar's provides, not a whole-image "
                 f"mount, no address in the name): "
                 f"{', '.join(sorted(unaddressed)[:5])}. Refusing to run: an "
                 f"array that cannot be located cannot be checked, and "
                 f"skipping it silently is how this check goes blind.")

    findings = []
    raw_elsewhere = 0
    unattributed = set()
    for name in sorted(arrays):
        blob = arrays[name]
        src_ov = array_ov.get(name)
        base = ds_addr[name]
        ov_relocs = relocs_for(src_ov)
        for off in range(0, len(blob) - 3, 4):
            if (base + off) not in ov_relocs:
                continue
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
