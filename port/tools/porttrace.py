#!/usr/bin/env python
"""Turn a PC-port function-entry trace into the same named listing the
cartridge-side trace produces, so the two can be compared.

THE OTHER HALF OF THIS IS AN EMULATOR. Lane ROMTRACE built an instrumented
melonDS that records every call the real cartridge's ARM9 makes and resolves the
addresses through config/arm9/**/symbols.txt. This reads the port's own trace,
written by port/hal/fn_trace.cpp in a build configured -DPORT_FN_TRACE=ON, and
resolves it through that build's walk_window.map. The output columns are
ROMTRACE's on purpose: frame, depth, caller -> callee.

WHAT THE TWO HALVES RECORD IS NOT THE SAME THING, and the difference is handled
here rather than papered over. The emulator hooks the ARM's BL, so it sees the
CALL: caller, callee and the arguments in r0-r3. /Gh hooks the function's own
entry, so the port sees only the CALLEE, plus the caller's stack pointer. The
caller is reconstructed from that stack pointer: a stack of (esp, name) is kept,
an entry deeper than the top pushes and an entry at or above it pops, and the
name left on top is the caller. That gives the same tree, and so the same
columns, without a return hook on either side. The offset inside the caller and
the argument values are NOT recoverable and are printed as +0x0 and omitted.

THREE CLASSES OF NAME COME BACK, and the caller decides what to do with each:
  ROM    a function the cartridge also has. These are the only ones that can be
         compared, and the name is canonicalised to the form the cartridge's
         symbol table uses, so `_func_ov007_020c4684` and
         `?Behavior@Stump@@UAEHXZ` come back as `func_ov007_020c4684` and
         `Stump::Behavior`.
  HOST   port/hal, port/unmatched, port/ntr and the C runtime: scaffolding the
         cartridge never had. ntr::io_write is the interesting one, because that
         is where a routed hardware store ends up, and counting those per ROM
         caller is what finds a translation unit whose stores are going nowhere.
  ???    resolved to no map symbol at all. Always reported, never hidden.
"""

import argparse
import bisect
import collections
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import linkage  # noqa: E402  (same directory, deliberately)

MAGIC = 0x50544632          # "PTF2"
FRAME_TAG = 0xFFFFFFFF

# The map row: section:offset, name, virtual address, flags, object. Spelled the
# same way linkage.map_symbols spells it, including the reason: a bare \S*
# placeholder for the flag column backtracks into the object name on flagless
# rows and truncates `ov010_syms.c.obj` to `c.obj`.
MAP_ROW = re.compile(
    r"\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+"
    r"((?:[fi]\s+)*)(\S+\.obj)\s*$")
PREFERRED = re.compile(r"Preferred load address is ([0-9a-fA-F]+)")

FUNC_ADDR = re.compile(r"^func_(?:ov\d{3}_)?0[12][0-9a-fA-F]{6}$")
DATA_ADDR = re.compile(r"^data_(?:ov\d{3}_)?0[12][0-9a-fA-F]{6}$")


def canonical(sym, matched_stems, qual_index):
    """(canonical name, class) for one raw map symbol.

    The canonical form is the one the cartridge's own symbol table uses, minus
    the overlay prefix and minus the argument list, because those are the two
    things the two halves spell differently and neither carries information the
    comparison needs.
    """
    stem = linkage.undecorate(sym)

    # A matched translation unit is named after its own symbol, so a stem that
    # IS a src/ file name is a ROM function by construction.
    if stem in matched_stems:
        q = linkage.itanium_qual(stem)
        return (q or stem), "ROM"

    # An address-named ROM function that has no src/ file of its own (it lives
    # inside a consolidated translation unit) is still a ROM function.
    if FUNC_ADDR.match(stem) or DATA_ADDR.match(stem):
        return stem, "ROM"

    # A C-linkage Itanium name that reached the port without a file of its own.
    q = linkage.itanium_qual(stem)
    if q:
        return q, ("ROM" if q in qual_index else "HOST")

    # MSVC's own mangling. ?Behavior@Stump@@UAEHXZ -> Stump::Behavior.
    q = linkage.msvc_qual(sym)
    if q:
        return q, ("ROM" if q in qual_index else "HOST")

    # ??1Door@@QAE@XZ and friends: constructors, destructors, operators and
    # vftables. linkage.msvc_qual refuses those on purpose; keep the raw name.
    return stem, "HOST"


def load_map(path, root):
    """(addresses, names, classes, objects, preferred_base), sorted by address."""
    if not os.path.isfile(path):
        sys.exit("porttrace: no map at {}".format(path))
    if os.path.getsize(path) < 4096:
        sys.exit("porttrace: map at {} is {} bytes, which is a failed or "
                 "truncated link, not a build to read"
                 .format(path, os.path.getsize(path)))

    matched = linkage.matched_index(root)
    quals = linkage.qual_index(matched)
    matched_stems = set(matched)

    preferred = 0x400000
    rows = {}
    with open(path, errors="replace") as f:
        for line in f:
            m = PREFERRED.search(line)
            if m:
                preferred = int(m.group(1), 16)
                continue
            m = MAP_ROW.match(line)
            if not m:
                continue
            if "f" not in m.group(3):
                continue                      # data rows cannot be entered
            va = int(m.group(2), 16)
            name, cls = canonical(m.group(1), matched_stems, quals)
            # Several names share one address (a folded destructor, an alias).
            # Prefer the ROM name: that is the one the cartridge also has and
            # therefore the only one that can take part in the comparison.
            old = rows.get(va)
            if old is None or (old[1] != "ROM" and cls == "ROM"):
                rows[va] = (name, cls, m.group(4))

    addrs = sorted(rows)
    return (addrs,
            [rows[a][0] for a in addrs],
            [rows[a][1] for a in addrs],
            [rows[a][2] for a in addrs],
            preferred)


def read_trace(path):
    """(words, header)."""
    with open(path, "rb") as f:
        blob = f.read()
    if len(blob) < 24:
        sys.exit("porttrace: {} is {} bytes, too short to be a trace"
                 .format(path, len(blob)))
    magic, recsize, count, filled, image_base, _ = struct.unpack("<6I", blob[:24])
    if magic != MAGIC:
        sys.exit("porttrace: {} does not start with PTF2 (got {:08x}). This "
                 "reader does not guess at a format it does not know."
                 .format(path, magic))
    if recsize != 8:
        sys.exit("porttrace: record size {} is not 8".format(recsize))
    body = blob[24:24 + count * 8]
    words = struct.unpack("<{}I".format(len(body) // 4), body)
    return words, dict(count=count, filled=filled, image_base=image_base)


def resolve(words, hdr, mapinfo):
    addrs, names, classes, _objs, preferred = mapinfo
    slide = (hdr["image_base"] - preferred) & 0xFFFFFFFF

    frames = []                      # [(frame_no, [(depth, caller, callee, cls)])]
    cur = None
    stack = []                       # [(esp, name)]
    unresolved = collections.Counter()

    for i in range(0, len(words) - 1, 2):
        w0, w1 = words[i], words[i + 1]
        if w0 == FRAME_TAG:
            cur = (w1, [])
            frames.append(cur)
            stack = []
            continue
        if cur is None:
            cur = (0, [])
            frames.append(cur)

        va = (w0 - slide) & 0xFFFFFFFF
        k = bisect.bisect_right(addrs, va) - 1
        if k < 0 or va - addrs[k] > 0x20000:
            unresolved[va] += 1
            name, cls = "???{:08x}".format(va), "???"
        else:
            name, cls = names[k], classes[k]

        esp = w1
        while stack and stack[-1][0] <= esp:
            stack.pop()
        caller = stack[-1][1] if stack else "(root)"
        depth = len(stack)
        cur[1].append((depth, caller, name, cls))
        stack.append((esp, name))

    return frames, unresolved


def write_named(frames, path, max_lines):
    n = 0
    with open(path, "w") as f:
        f.write("# named function-entry trace of the PC PORT, from MSVC /Gh\n")
        f.write("# columns: frame  depth  caller -> callee   [class]\n")
        f.write("# the caller is reconstructed from the stack pointer; the\n")
        f.write("# offset inside it and the arguments are not recorded.\n")
        for fno, calls in frames:
            f.write("\n=== frame {} ===\n".format(fno))
            for depth, caller, callee, cls in calls:
                if max_lines and n >= max_lines:
                    f.write("... truncated at --max-lines\n")
                    return
                f.write("{:5d} {:3d} {}{} -> {}   [{}]\n"
                        .format(fno, depth, "  " * min(depth, 24), caller,
                                callee, cls))
                n += 1


def write_frames(frames, path):
    seen = set()
    with open(path, "w") as f:
        f.write("# per frame: how many entries, and which functions are entered\n")
        f.write("# HERE FOR THE FIRST TIME in this trace.\n\n")
        for fno, calls in frames:
            new = []
            for _d, _c, callee, cls in calls:
                if callee not in seen:
                    seen.add(callee)
                    new.append((callee, cls))
            f.write("frame {:8d}   {} calls   {} new\n"
                    .format(fno, len(calls), len(new)))
            for callee, cls in new:
                f.write("        NEW  {}   [{}]\n".format(callee, cls))


def write_summary(frames, hdr, unresolved, path):
    counts = collections.Counter()
    edges = collections.Counter()
    byclass = collections.Counter()
    for _fno, calls in frames:
        for _d, caller, callee, cls in calls:
            counts[callee] += 1
            edges[(caller, callee)] += 1
            byclass[cls] += 1
    total = sum(counts.values())
    with open(path, "w") as f:
        f.write("records        : {}\n".format(hdr["count"]))
        f.write("buffer filled  : {}\n".format(
            "YES -- THE TRACE IS TRUNCATED, raise SM64DS_FN_TRACE_MB"
            if hdr["filled"] else "no"))
        f.write("image base     : 0x{:08x}\n".format(hdr["image_base"]))
        f.write("frames         : {}\n".format(len(frames)))
        f.write("calls          : {}\n".format(total))
        for cls in ("ROM", "HOST", "???"):
            f.write("  {:<12s} : {} ({:.1f}%)\n".format(
                cls, byclass[cls],
                100.0 * byclass[cls] / total if total else 0.0))
        f.write("distinct callees: {}\n".format(len(counts)))
        f.write("distinct edges  : {}\n".format(len(edges)))
        if unresolved:
            f.write("\nunresolved addresses: {} across {} distinct\n"
                    .format(sum(unresolved.values()), len(unresolved)))
            for va, n in unresolved.most_common(20):
                f.write("    {:8d}  0x{:08x}\n".format(n, va))
        f.write("\n--- most called ---\n")
        for name, n in counts.most_common(60):
            f.write("{:8d}  {}\n".format(n, name))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace", help="the .bin written by SM64DS_FN_TRACE")
    ap.add_argument("--map", required=True, help="walk_window.map of THAT build")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--root", default=os.path.dirname(os.path.dirname(HERE)))
    ap.add_argument("--max-lines", type=int, default=400000,
                    help="cap on trace_named.txt only; the summary and the "
                         "per-frame report are always complete")
    a = ap.parse_args()

    mapinfo = load_map(a.map, a.root)
    words, hdr = read_trace(a.trace)
    frames, unresolved = resolve(words, hdr, mapinfo)

    if not os.path.isdir(a.out_dir):
        os.makedirs(a.out_dir)
    write_named(frames, os.path.join(a.out_dir, "trace_named.txt"), a.max_lines)
    write_frames(frames, os.path.join(a.out_dir, "trace_frames.txt"))
    write_summary(frames, hdr, unresolved,
                  os.path.join(a.out_dir, "trace_summary.txt"))

    total = sum(len(c) for _f, c in frames)
    rom = sum(1 for _f, c in frames for r in c if r[3] == "ROM")
    bad = sum(1 for _f, c in frames for r in c if r[3] == "???")
    print("porttrace: {} frames, {} entries, {} ROM, {} unresolved -> {}"
          .format(len(frames), total, rom, bad, a.out_dir))
    if hdr["filled"]:
        print("porttrace: THE BUFFER FILLED. The trace is truncated; raise "
              "SM64DS_FN_TRACE_MB and re-run before believing a diff.")


if __name__ == "__main__":
    main()
