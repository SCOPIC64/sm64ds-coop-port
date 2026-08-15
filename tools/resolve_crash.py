#!/usr/bin/env python3
"""Resolve a port crash dump's module offsets to symbol+offset.

The port's crash path (port/tests/fault_probe.h) writes module-RELATIVE
addresses -- "+0x0002xxxx" tokens -- because the in-crash writer has no symbol
table and cannot afford one. This tool does the symbol resolution OFFLINE, so
the in-crash path stays a raw-Win32 buffer write with no dependencies.

Feed it a dump file (crash.txt, exit.txt, or one of the rolling
%TEMP%\\sm64ds-crashes\\crash-*.txt files) and it rewrites every "+0xNNNNNNNN"
offset in place with the nearest preceding symbol from the linker /MAP file,
e.g.

    offset    +0x00024f10   ->   func_02043fdc +0x90   (func_02043fdc_hostcopy.cpp.obj)

The map is build/port/walk_window.map by default (the build writes it via /MAP).

Usage:
    python tools/resolve_crash.py <dumpfile> [--map build/port/walk_window.map]
    python tools/resolve_crash.py --latest        # newest dump in %TEMP%
    python tools/resolve_crash.py <dumpfile> --quarantine   # also a quarantine.log

Resolution model: the MAP header names "Preferred load address is 00400000".
Each symbol row carries a preferred VA in its 4th column; the module-relative
RVA is (preferred_VA - preferred_load_address), which is exactly the "+offset"
the dump prints (ExceptionAddress - GetModuleHandle(0)). So a dump offset maps
to the symbol with the greatest RVA <= offset.
"""
import argparse
import glob
import os
import re
import sys

# the dump writes "+NNNNNNNN" (8 hex, no 0x); crash.txt/exit.txt also print a
# "+0xNNNNNNNN" style in a couple of spots, so accept both.
OFF_RE = re.compile(r"\+(?:0x)?([0-9a-fA-F]{8})\b")
# a MAP symbol row:  0001:00003c20       _main            00404c20 f   walk_window.cpp.obj
SYM_RE = re.compile(
    r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+"
    r"(?P<name>\S+)\s+"
    r"(?P<va>[0-9a-fA-F]{8})\s+"
    r"(?P<kind>[fi])\s+"
    r"(?P<obj>\S+)"
)
LOAD_RE = re.compile(r"Preferred load address is\s+([0-9a-fA-F]+)")


def load_map(map_path):
    """Return (sorted [(rva, name, obj)], load_base). Only code symbols (f)."""
    with open(map_path, "r", errors="replace") as f:
        text = f.read()
    m = LOAD_RE.search(text)
    load_base = int(m.group(1), 16) if m else 0x400000
    syms = []
    for line in text.splitlines():
        mm = SYM_RE.match(line)
        if not mm:
            continue
        if mm.group("kind") != "f":
            continue  # code symbols only; data offsets are not return addresses
        va = int(mm.group("va"), 16)
        rva = va - load_base
        if rva < 0:
            continue
        name = mm.group("name")
        # strip the leading underscore MSVC decoration on cdecl C names
        if name.startswith("_") and not name.startswith("__"):
            name = name[1:]
        syms.append((rva, name, mm.group("obj")))
    syms.sort()
    return syms, load_base


def resolve_one(offset, syms):
    """Nearest preceding code symbol for a module RVA. Binary search."""
    lo, hi, best = 0, len(syms) - 1, None
    while lo <= hi:
        mid = (lo + hi) // 2
        if syms[mid][0] <= offset:
            best = mid
            lo = mid + 1
        else:
            hi = mid - 1
    if best is None:
        return None
    rva, name, obj = syms[best]
    return name, offset - rva, obj


def annotate(line, syms):
    """Append resolved symbol names to every +offset token on a line."""
    hits = []
    for m in OFF_RE.finditer(line):
        off = int(m.group(1), 16)
        r = resolve_one(off, syms)
        if r:
            name, delta, obj = r
            hits.append("%s +0x%x (%s)" % (name, delta, obj))
    if not hits:
        return line.rstrip("\n")
    return "%s   ->   %s" % (line.rstrip("\n"), " | ".join(hits))


def newest_dump():
    tmp = os.environ.get("TEMP") or os.environ.get("TMP") or "."
    d = os.path.join(tmp, "sm64ds-crashes")
    cands = glob.glob(os.path.join(d, "crash-*.txt"))
    if not cands:
        return None
    return max(cands, key=os.path.getmtime)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("dump", nargs="?", help="dump file to resolve")
    ap.add_argument("--map", default=None,
                    help="linker /MAP (default build/port/walk_window.map)")
    ap.add_argument("--latest", action="store_true",
                    help="resolve the newest dump under %%TEMP%%\\sm64ds-crashes")
    ap.add_argument("--quarantine", action="store_true",
                    help="also annotate a quarantine.log (same +offset tokens)")
    args = ap.parse_args()

    dump = args.dump
    if args.latest:
        dump = newest_dump()
        if not dump:
            print("no dumps found under %TEMP%\\sm64ds-crashes", file=sys.stderr)
            return 2
        print("# latest dump: %s" % dump, file=sys.stderr)
    if not dump:
        ap.print_help()
        return 2

    map_path = args.map
    if not map_path:
        here = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        map_path = os.path.join(here, "build", "port", "walk_window.map")
    if not os.path.exists(map_path):
        print("map not found: %s (build the port first)" % map_path,
              file=sys.stderr)
        return 2

    syms, base = load_map(map_path)
    print("# resolved against %s (%d code symbols, base 0x%x)"
          % (os.path.basename(map_path), len(syms), base), file=sys.stderr)

    with open(dump, "r", errors="replace") as f:
        for line in f:
            print(annotate(line, syms))
    return 0


if __name__ == "__main__":
    sys.exit(main())
