#!/usr/bin/env python
"""Line up two function-entry traces and name what they disagree about.

THE POINT OF THIS TOOL IS THE DROPPED-STORE FAULT, so it is worth saying what
that fault looks like before saying what the tool prints. A translation unit the
port builds PLAIN writes its hardware registers straight into mapped memory. The
words land, nothing faults, no call is skipped, and the call trace is IDENTICAL
to a working build's. What changes is one thing only: a routed store goes
through ntr::io_write and a plain one does not. So the signal is not a missing
call. It is a ROM function whose count of host hardware-write calls fell to zero
while everything else about it stayed the same.

That is why this compares EDGES (caller -> callee) rather than call counts
alone, and why the per-caller hardware-write table is the first thing it prints.

TWO WAYS TO USE IT:

  port vs port       two builds of the port, one of them deliberately broken.
                     This is the self-check: the answer is known in advance, so
                     a tool that cannot find it is not to be trusted on
                     anything else.

  port vs cartridge  the real job. Lane ROMTRACE's instrumented melonDS writes
                     the cartridge half and its resolver reports the same
                     columns on purpose.

WHAT IT WILL NOT TELL YOU. The port's caller is reconstructed from the stack
pointer, so the offset inside the caller and the argument values are not
recoverable on the port side. Anything that needs argument values has to come
off the cartridge half alone. Frame numbers line up only if both halves were
asked for the same window; the tool prints the window each one covers and says
so when they differ, rather than sliding one trace over the other until
something appears to match.
"""

import argparse
import collections
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import porttrace  # noqa: E402  (same directory, deliberately)

# The host functions that mean a hardware register was really written.
# ntr::io_write is the general seam; the geometry ports have their own.
WRITE_SEAMS = ("ntr::io_write", "ntr::gx_write_port", "ntr::gx_write_fifo",
               "ntr::ipc_reg_write")


def load_side(trace_path, map_path, root, label):
    mapinfo = porttrace.load_map(map_path, root)
    words, hdr = porttrace.read_trace(trace_path)
    frames, unresolved = porttrace.resolve(words, hdr, mapinfo)
    calls = sum(len(c) for _f, c in frames)
    print("{:<10s} {:>3d} frames  {:>12,d} calls  buffer {}".format(
        label, len(frames), calls,
        "FILLED -- TRUNCATED" if hdr["filled"] else "ok"))
    if hdr["filled"]:
        print("           raise SM64DS_FN_TRACE_MB or narrow the frame window")
    if unresolved:
        print("           {:,} unresolved addresses across {} distinct".format(
            sum(unresolved.values()), len(unresolved)))
    return {"frames": frames, "hdr": hdr, "label": label,
            "truncated": bool(hdr["filled"])}


def tally(side):
    """Per ROM caller: every edge out of it, and its hardware-write calls."""
    edges = collections.Counter()
    writes = collections.Counter()
    romset = set()
    for _fno, calls in side["frames"]:
        for _d, caller, callee, cls in calls:
            edges[(caller, callee)] += 1
            if cls == "ROM":
                romset.add(callee)
            if callee in WRITE_SEAMS:
                writes[caller] += 1
    return {"edges": edges, "writes": writes, "rom": romset}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--a", required=True, help="reference trace (.bin)")
    ap.add_argument("--a-map", required=True)
    ap.add_argument("--b", required=True, help="candidate trace (.bin)")
    ap.add_argument("--b-map", required=True)
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(HERE)))
    ap.add_argument("--top", type=int, default=25,
                    help="how many rows of each table to print")
    ap.add_argument("--expect", default=None,
                    help="a ROM function this diff MUST name. With it the tool "
                         "exits non-zero unless that function appears in the "
                         "hardware-write table, which is what makes this "
                         "usable as a self-check with a known answer.")
    a = ap.parse_args()

    print("=== the two traces ===")
    A = load_side(a.a, a.a_map, a.root, "A (ref)")
    B = load_side(a.b, a.b_map, a.root, "B (cand)")

    fa = [f for f, _c in A["frames"]]
    fb = [f for f, _c in B["frames"]]
    print("\nframe windows: A {}..{}   B {}..{}".format(
        min(fa) if fa else "-", max(fa) if fa else "-",
        min(fb) if fb else "-", max(fb) if fb else "-"))
    if fa and fb and (min(fa), max(fa)) != (min(fb), max(fb)):
        print("NOTE: the windows differ, so raw per-frame counts are not "
              "comparable. The per-caller tables below still are.")

    ta = tally(A)
    tb = tally(B)

    print("\n=== ROM functions entered by one side only ===")
    only_a = sorted(ta["rom"] - tb["rom"])
    only_b = sorted(tb["rom"] - ta["rom"])
    if not only_a and not only_b:
        print("none: both sides enter exactly the same {} ROM functions"
              .format(len(ta["rom"])))
    for n in only_a[:a.top]:
        print("  A only   {}".format(n))
    for n in only_b[:a.top]:
        print("  B only   {}".format(n))

    print("\n=== hardware writes per ROM caller: the dropped-store signal ===")
    print("A caller whose count falls to zero in B is latching its registers")
    print("into mapped memory instead of routing them. That is the fault shape.")
    print("")
    rows = []
    for c in set(ta["writes"]) | set(tb["writes"]):
        na, nb = ta["writes"][c], tb["writes"][c]
        if na != nb:
            rows.append((abs(na - nb), c, na, nb))
    rows.sort(reverse=True)
    if not rows:
        print("  no caller changed its hardware-write count")
    else:
        print("  {:>9s} {:>9s}  {}".format("A", "B", "ROM caller"))
        for _d, c, na, nb in rows[:a.top]:
            flag = ""
            if nb == 0 and na > 0:
                flag = "   <-- WRITES VANISHED"
            elif na == 0 and nb > 0:
                flag = "   <-- WRITES APPEARED"
            print("  {:>9,d} {:>9,d}  {}{}".format(na, nb, c, flag))
        if len(rows) > a.top:
            print("  ... and {} more".format(len(rows) - a.top))

    print("\n=== call edges that changed most ===")
    erows = []
    for k in set(ta["edges"]) | set(tb["edges"]):
        na, nb = ta["edges"][k], tb["edges"][k]
        if na != nb:
            erows.append((abs(na - nb), k, na, nb))
    erows.sort(reverse=True)
    if not erows:
        print("  no edge changed")
    for _d, (caller, callee), na, nb in erows[:a.top]:
        print("  {:>9,d} {:>9,d}  {} -> {}".format(na, nb, caller, callee))
    if len(erows) > a.top:
        print("  ... and {} more".format(len(erows) - a.top))

    rc = 0
    if a.expect:
        named = [c for _d, c, _na, _nb in rows]
        print("\n=== self-check ===")
        if a.expect in named:
            print("PASS: {} is named, at rank {} of {} in the hardware-write "
                  "table".format(a.expect, named.index(a.expect) + 1,
                                 len(named)))
        else:
            print("FAIL: {} is NOT named in the hardware-write table. The tool "
                  "did not find a fault whose answer was already known, so it "
                  "is not to be trusted on one that is not.".format(a.expect))
            rc = 1

    if A["truncated"] or B["truncated"]:
        print("\nWARNING: a trace was truncated, so nothing above is safe to "
              "believe. Re-capture before using this.")
        rc = rc or 2
    return rc


if __name__ == "__main__":
    sys.exit(main())
