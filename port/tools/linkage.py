"""How much of the port is the real decomp, and what is still scaffolding?

The goal for the port is that it IS the decompiled game: every host file under
port/hal and port/unmatched replaced by the byte-matched translation unit it
stands in for, step by step, until there is nothing left standing in.

This measures that two ways.

  headline   how many matched TUs from src/ actually reach the binary
  queue      for every symbol a HOST file defines, whether src/ already has a
             matched TU defining the same symbol -- those are the direct
             replacement candidates, the work list for getting to 100%

The queue is the useful half. A host file whose symbols have no matched
counterpart is genuine host code (the ntr layer, the window, the SDAT host,
MSVC ABI shims for register ride-throughs and pointer-to-member dispatch) and
is not a defect. A host file whose symbols DO have matched counterparts is
scaffolding that outlived its reason, and port_stage_a_boot is the example
that motivated this file: a hand-written subset of Stage::InitResources, while
Stage::InitResources sat in src/ decompiled and unlinked.

    python port/tools/linkage.py [repo-root] [--queue]
"""
import os
import re
import sys

HOST_DIRS = ("hal", "unmatched")


def matched_index(root):
    """{symbol stem: repo-relative src path} for every matched TU."""
    out = {}
    for dirpath, _d, files in os.walk(os.path.join(root, "src")):
        for fn in files:
            if fn.endswith((".c", ".cpp")):
                stem = os.path.splitext(fn)[0]
                rel = os.path.relpath(os.path.join(dirpath, fn), root)
                out.setdefault(stem, rel.replace("\\", "/"))
    return out


def map_symbols(mapfile):
    """[(symbol, objfile)] out of an MSVC /MAP."""
    out = []
    with open(mapfile, errors="replace") as f:
        for line in f:
            m = re.match(
                r"\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+[0-9a-fA-F]{8}\s+\S*\s*(\S+\.obj)",
                line)
            if m:
                out.append((m.group(1).lstrip("_"), m.group(2)))
    return out


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    show_queue = "--queue" in sys.argv

    mapfile = os.path.join(root, "build", "port", "walk_window.map")
    if not os.path.exists(mapfile):
        print("no %s -- build walk_window first" % mapfile)
        return 2

    matched = matched_index(root)
    syms = map_symbols(mapfile)

    linked_stems = set()
    host_hits = {}
    for sym, obj in syms:
        stem = os.path.splitext(os.path.splitext(obj)[0])[0]
        if stem in matched:
            linked_stems.add(stem)
        # a host object is one whose name is not itself a matched TU
        if stem not in matched and sym in matched:
            host_hits.setdefault(obj, set()).add(sym)

    total = len(matched)
    linked = len(linked_stems)
    print("matched TUs in src/        : %d" % total)
    print("linked into walk_window    : %d  (%.1f%%)" % (linked, 100.0 * linked / total))
    print("still unlinked             : %d" % (total - linked))
    print()

    if not host_hits:
        print("no host object defines a symbol that src/ also has. Nothing to replace.")
        return 0

    ranked = sorted(host_hits.items(), key=lambda kv: -len(kv[1]))
    n = sum(len(v) for v in host_hits.values())
    print("REPLACEMENT QUEUE: %d symbols across %d host objects are defined by a"
          % (n, len(host_hits)))
    print("host file while src/ carries a matched TU of the same name.")
    print()
    for obj, s in ranked:
        print("  %-40s %d symbol(s)" % (obj, len(s)))
        if show_queue:
            for sym in sorted(s):
                print("        %-46s %s" % (sym, matched[sym]))
    if not show_queue:
        print()
        print("re-run with --queue to list the symbols and their matched sources")
    return 0


if __name__ == "__main__":
    sys.exit(main())
