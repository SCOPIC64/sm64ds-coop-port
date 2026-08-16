#!/usr/bin/env python3
"""Decompose the difference between two linker maps, so a lane's claimed
delta is read off the linker's own two statements instead of by hand.

WHY THIS EXISTS. Every review in run link60 Stage 4 built base AND tip and
closed the delta by hand off the two maps: SD0's reviewer counted +19 with
20 objects added and none removed, MG2's counted +217 with 225 objects,
PC1's decomposed +18 into 3 new bodies plus 15 /OPT:REF returns, NFS's
proved 8-in-1-out. Ten-plus hand decompositions in one stage, and the
error class is on the record twice: MG2's "14 objects" was a parse
artifact that caught one of seven LNK2001 rows, and MG2's before-column
went unmeasured because a failed link had destroyed the baseline map.
This tool is that hand step, mechanized: two maps in, the symbol and
object delta out, with the moved-definition list that the UnloadArchive
LNK2005 collision made a review question.

WHAT IT REPORTS:

  ADDED / REMOVED   defined externals present in one map and not the
                    other, each with its defining object.
  MOVED             same symbol, DIFFERENT defining object. This is the
                    collision-adjacent signal: two lanes deriving the
                    same face independently reads as a move at their
                    merge, and a symbol quietly migrating between objects
                    is a thing a review wants shown, not inferred.
  OBJECTS           object basenames added and removed.
  NAME-SHAPE CENSUS a per-module grouping of the added and removed sets
                    by SYMBOL NAME (func_ovNNN_/data_ovNNN_ -> ovNNN,
                    func_02xxxxxxx -> arm9). ORIENTATION ONLY: name
                    shapes are a convention, not a link fact, and the
                    authoritative per-module linked count is
                    linkage.py --by-module's, not this census.

TWO NUMBERS THAT READ ALIKE AND ARE NOT THE SAME, closure.py's law in
this tool's terms: linkage.py's headline counts MATCHED TUs LINKED, and
this tool counts MAP SYMBOL ROWS. A lane that gained +217 linked TUs
gains more than 217 map symbols (vtables, literals, sinit thunks ride
along). The two travel together and neither substitutes for the other;
quote each as itself. That is why there is no --expect N flag: an
expected LINKED delta is linkage.py's to check, and pretending this
tool checks it would manufacture the exact confusion closure.py's
docstring warns about.

--expect-zero IS here, because delta-0 is a claim about the MAP: a
tool-only or doc-only commit must add nothing, remove nothing and move
nothing, and object sets must be identical. That is this tool's own
currency, it is the gate statement half the lanes in Stage 4 made by
eyeball, and a nonzero anything exits 2 loudly.

GROUND TRUTH AND ITS EDGES. Both inputs are the linker's own output;
this tool computes set differences and invents nothing. Both maps pass
closure.map_defined's hardened guard (zero-byte, row-less and
trailer-less maps refuse -- the failed-link truncation that destroyed
MG2's baseline cannot silently feed this tool). Two edges a reader
must know: map_defined keeps the FIRST definition row per symbol, so a
symbol the map lists twice shows one object here; and gate.py's build
relinks on every run (port_gittip.c), so a base map must be COPIED
ASIDE before the tip is built -- the same advice finding 3 gave probe
links. A base map that was overwritten is not recoverable from this
tool or any other.

    python port/tools/mapdiff.py --base saved/walk_window.map \\
                                 --tip build/port/walk_window.map
    python port/tools/mapdiff.py --base A.map --tip B.map --expect-zero
    python port/tools/mapdiff.py --base A.map --tip B.map --full
    python port/tools/mapdiff.py --selftest
"""

import argparse
import os
import pathlib
import re
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import closure  # noqa: E402  (map_defined and its truncation guard)

# Name-shape attribution, ORIENTATION ONLY (see the docstring). One
# leading ? or up to two leading underscores are decoration, not name.
NAME_OV = re.compile(r"^[?_]{0,2}(?:func|data|pk\d+_gap|__sinit)_ov(\d{3})_")
NAME_ARM9 = re.compile(r"^[?_]{0,2}(?:func|data|pk\d+_gap|__sinit)_0[0-9a-f]{7}\b")

LIST_CAP = 40  # per section, unless --full; the COUNT line always tells all


def module_of(sym):
    m = NAME_OV.match(sym)
    if m:
        return "ov" + m.group(1)
    if NAME_ARM9.match(sym):
        return "arm9"
    return "other"


def diff(base_map, tip_map):
    """(added, removed, moved, obj_added, obj_removed) between two maps.

    added/removed: sorted [(sym, obj)]. moved: sorted [(sym, was, now)].
    Objects are the maps' own object-column basenames.
    """
    base = closure.map_defined(base_map)
    tip = closure.map_defined(tip_map)
    added = sorted((s, tip[s]) for s in tip.keys() - base.keys())
    removed = sorted((s, base[s]) for s in base.keys() - tip.keys())
    moved = sorted((s, base[s], tip[s]) for s in base.keys() & tip.keys()
                   if base[s] != tip[s])
    base_objs = set(base.values())
    tip_objs = set(tip.values())
    return (added, removed, moved,
            sorted(tip_objs - base_objs), sorted(base_objs - tip_objs),
            len(base), len(tip), len(base_objs), len(tip_objs))


def census(rows):
    """{module: count} over [(sym, obj)] rows, by name shape."""
    out = {}
    for sym, _ in rows:
        out[module_of(sym)] = out.get(module_of(sym), 0) + 1
    return out


def print_capped(rows, fmt, full):
    shown = rows if full else rows[:LIST_CAP]
    for r in shown:
        print(fmt % r)
    if len(rows) > len(shown):
        print("    ... %d more (--full lists them)"
              % (len(rows) - len(shown)))


def report(base_map, tip_map, full=False, expect_zero=False):
    (added, removed, moved, obj_added, obj_removed,
     nbase, ntip, nbobj, ntobj) = diff(base_map, tip_map)
    print("base: %d symbols in %d objects  (%s)" % (nbase, nbobj, base_map))
    print("tip : %d symbols in %d objects  (%s)" % (ntip, ntobj, tip_map))
    print()
    print("=== ADDED symbols: %d ===" % len(added))
    print_capped(added, "    %s  --  %s", full)
    print("=== REMOVED symbols: %d ===" % len(removed))
    print_capped(removed, "    %s  --  %s", full)
    print("=== MOVED symbols (same name, different object): %d ==="
          % len(moved))
    print_capped(moved, "    %s  --  %s -> %s", full)
    print("=== objects added: %d / removed: %d ==="
          % (len(obj_added), len(obj_removed)))
    print_capped([(o,) for o in obj_added], "    + %s", full)
    print_capped([(o,) for o in obj_removed], "    - %s", full)
    for label, rows in (("added", added), ("removed", removed)):
        c = census(rows)
        if c:
            print("=== name-shape census of the %s set "
                  "(ORIENTATION ONLY, see docstring) ===" % label)
            for mod in sorted(c):
                print("    %-6s %d" % (mod, c[mod]))
    if expect_zero:
        bad = []
        if added:
            bad.append("%d added" % len(added))
        if removed:
            bad.append("%d removed" % len(removed))
        if moved:
            bad.append("%d moved" % len(moved))
        if obj_added or obj_removed:
            bad.append("object set changed (+%d/-%d)"
                       % (len(obj_added), len(obj_removed)))
        if bad:
            print()
            print("mapdiff: EXPECT-ZERO FAILED -- %s. A delta-0 claim is "
                  "about the MAP, and this map moved." % ", ".join(bad))
            return 2
        print()
        print("mapdiff: expect-zero holds (nothing added, removed or moved; "
              "object sets identical)")
    return 0


# ---------------------------------------------------------------------------

TRAILER = "\n entry point at        0001:00000000\n"


def _fabmap(path, rows):
    """A fabricated real-shaped map: rows + the trailer the guard requires."""
    text = "".join(
        " 0001:%08x       %-26s %08x f   %s\n" % (i * 16, sym, 0x10000000 + i,
                                                  obj)
        for i, (sym, obj) in enumerate(rows))
    pathlib.Path(path).write_text(text + TRAILER)
    return path


def selftest():
    """The differ against fabricated maps with every section populated.

    base carries: _stay (kept), _gone (removed), _mover (moves object),
    _func_ov006_020e0000 (removed, for the census), old.obj's only symbol.
    tip carries: _stay, _mover (new object), _new_c, ?New@C@@QAEXXZ,
    _func_ov006_020e1111 and _func_02012345 (census rows), new.obj.
    Expected: 4 added / 2 removed / 1 moved, objects +2/-1, census
    added = {arm9:1, other:2, ov006:1}, removed = {other:1, ov006:1};
    expect-zero FAILS here and HOLDS on an identical pair; a zero-byte
    base refuses through closure's guard (finding 3 travels with the
    import, but the arm proves THIS tool's path hits it).
    """
    ok = True

    def expect(cond, what, got):
        nonlocal ok
        if not cond:
            print("FAIL: %s: %s" % (what, got))
            ok = False

    with tempfile.TemporaryDirectory() as td:
        tdp = pathlib.Path(td)
        base = _fabmap(tdp / "base.map", [
            ("_stay", "a.cpp.obj"),
            ("_gone", "a.cpp.obj"),
            ("_mover", "a.cpp.obj"),
            ("_func_ov006_020e0000", "func_ov006_020e0000.cpp.obj"),
            ("_only_old", "old.cpp.obj"),
        ])
        tip = _fabmap(tdp / "tip.map", [
            ("_stay", "a.cpp.obj"),
            ("_mover", "b.cpp.obj"),
            ("_new_c", "b.cpp.obj"),
            ("?New@C@@QAEXXZ", "b.cpp.obj"),
            ("_func_ov006_020e1111", "func_ov006_020e1111.cpp.obj"),
            ("_func_02012345", "func_02012345.c.obj"),
        ])
        (added, removed, moved, obj_added, obj_removed,
         nbase, ntip, nbobj, ntobj) = diff(base, tip)
        expect([s for s, _ in added] ==
               ["?New@C@@QAEXXZ", "_func_02012345",
                "_func_ov006_020e1111", "_new_c"],
               "added set", added)
        expect([s for s, _ in removed] ==
               ["_func_ov006_020e0000", "_gone", "_only_old"],
               "removed set", removed)
        expect(moved == [("_mover", "a.cpp.obj", "b.cpp.obj")],
               "moved set", moved)
        expect(obj_added == ["b.cpp.obj", "func_02012345.c.obj",
                             "func_ov006_020e1111.cpp.obj"] and
               obj_removed == ["func_ov006_020e0000.cpp.obj",
                               "old.cpp.obj"],
               "object delta", (obj_added, obj_removed))
        expect(census(added) == {"other": 2, "ov006": 1, "arm9": 1},
               "added census", census(added))
        expect(census(removed) == {"other": 2, "ov006": 1},
               "removed census", census(removed))

        # expect-zero must FAIL on this pair and HOLD on an identical one
        import contextlib
        import io
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = report(base, tip, expect_zero=True)
        expect(rc == 2 and "EXPECT-ZERO FAILED" in buf.getvalue(),
               "expect-zero fails on a moved map", rc)
        twin = _fabmap(tdp / "twin.map", [
            ("_stay", "a.cpp.obj"),
            ("_gone", "a.cpp.obj"),
            ("_mover", "a.cpp.obj"),
            ("_func_ov006_020e0000", "func_ov006_020e0000.cpp.obj"),
            ("_only_old", "old.cpp.obj"),
        ])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = report(base, twin, expect_zero=True)
        expect(rc == 0 and "expect-zero holds" in buf.getvalue(),
               "expect-zero holds on an identical pair", rc)

        # and on a pair whose ONLY difference is one moved definition:
        # a lane that gains and loses nothing but lets a symbol migrate
        # between objects has not made a delta-0 map, and an expect-zero
        # that only counts added/removed would bless it.
        onemove = _fabmap(tdp / "onemove.map", [
            ("_stay", "a.cpp.obj"),
            ("_gone", "a.cpp.obj"),
            ("_mover", "elsewhere.cpp.obj"),
            ("_func_ov006_020e0000", "func_ov006_020e0000.cpp.obj"),
            ("_only_old", "old.cpp.obj"),
        ])
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            rc = report(base, onemove, expect_zero=True)
        expect(rc == 2 and "1 moved" in buf.getvalue(),
               "expect-zero fails on a moved-only pair", rc)

        # finding 3 travels with the closure import; prove THIS tool's
        # input path hits the guard rather than diffing an empty universe.
        zmap = tdp / "zero.map"
        zmap.write_text("")
        try:
            diff(zmap, tip)
            expect(False, "zero-byte base map ACCEPTED", "no SystemExit")
        except SystemExit as e:
            expect("ZERO BYTES" in str(e), "zero-map refusal", str(e))

    print("selftest %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(
        description="decompose the delta between two linker maps; "
                    "see the docstring")
    ap.add_argument("--base", help="the BEFORE map (copy it aside before "
                                   "rebuilding; the build overwrites it)")
    ap.add_argument("--tip", help="the AFTER map")
    ap.add_argument("--full", action="store_true",
                    help="list every row (default caps each section at %d)"
                         % LIST_CAP)
    ap.add_argument("--expect-zero", action="store_true",
                    help="exit 2 unless nothing was added, removed or moved")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        sys.exit(selftest())
    if not args.base or not args.tip:
        sys.exit("pass --base and --tip (or --selftest)")
    sys.exit(report(args.base, args.tip, full=args.full,
                    expect_zero=args.expect_zero))


if __name__ == "__main__":
    main()
