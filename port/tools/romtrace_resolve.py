#!/usr/bin/env python
"""Turn an instrumented-melonDS cartridge call trace into named functions.

THE OTHER HALF OF THIS IS THE PC PORT. port/tools/porttrace.py reads the port's
own function-entry trace and resolves it through that build's walk_window.map.
This reads the trace an instrumented melonDS writes while the REAL cartridge
runs, and resolves it through the decomp's own symbol tables. The two print the
same columns on purpose: frame, depth, caller -> callee. port/tools/tracediff.py
consumes both.

CANONICAL NAMES MUST AGREE BETWEEN THE TWO HALVES or the comparison is
worthless, so this spells a name the way porttrace.canonical spells it: no
overlay prefix on the name, and no argument list. `_ZN8SaveData19IsCharacter
UnlockedEj` comes back as `SaveData::IsCharacterUnlocked`, and an address-named
function keeps the address name it already has, `func_ov007_020c0b78`, which
carries its overlay inside it anyway. Where a function lives is reported in its
own column instead, and in the summary.

THE RECORD FORMAT, from C:/tmp/melontrace/src/ROMTrace.h. Eight 32-bit words per
record, raw, NO FILE HEADER. A file whose size is not a multiple of 32 is not
one of these and is refused rather than guessed at.

    [0] tag   1 = CALL, 2 = FRAME, 3 = FINGERPRINT
    CALL         [1] address of the BL/BLX, bit 0 set if the caller was Thumb
                 [2] target address, bit 0 set if the target is Thumb
                 [3..6] r0, r1, r2, r3 at the call
                 [7] r13, the stack pointer, which is where nesting comes from
    FRAME        [1] frame number, rest zero
    FINGERPRINT  [1] address, [2..5] the four words read there

THE THREE TRAPS, and what this does about each.

1. 103 OVERLAYS LOAD AT 22 DISTINCT BASE ADDRESSES, and 52 of them share
   0x021111A0, so an address alone does not name a function. The tracer writes
   one FINGERPRINT record per overlay base per frame holding the first four
   words actually sitting there. This compares those against the head of every
   overlay body in extracted/overlays/ and so knows which overlay is resident.
   Four all-zero words mean nothing is loaded there and the base is skipped. A
   base whose contents match no overlay of ours is recorded as unknown and
   skipped, never guessed at.

   RESOLUTION GRANULARITY IS ONE FRAME. An overlay swapped in and out inside a
   single frame is missed, and calls into it in that frame resolve against
   whichever overlay the fingerprint caught. This is stated, not hidden, and the
   summary counts how often a base was ambiguous or unknown.

2. ITCM FUNCTIONS LIVE AT 0x01FF8000, BELOW THE ARM9 BASE. Every division in
   the game goes through the ITCM aeabi helpers, so a resolver that only knows
   about 0x02000000 and up is blind to all of them. config/arm9/itcm/symbols.txt
   is loaded alongside the main table, and the summary reports the ITCM share
   separately so its absence is visible rather than silent.

3. arm9_dec.bin IS NOT AT THE ARM9 RAM BASE. Not an issue here and NOT corrected
   for: the trace records live addresses as the processor sees them and every
   symbol table in config/ is already absolute. No file offsets are involved.
"""

import argparse
import bisect
import collections
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))

RECORD_WORDS = 8
RECORD_BYTES = RECORD_WORDS * 4

TAG_CALL = 1
TAG_FRAME = 2
TAG_FINGERPRINT = 3

# name kind:function(arm,size=0x1c) addr:0x020137e0
SYM_ROW = re.compile(
    r"^(\S+)\s+kind:(\w+)\(([^)]*)\)\s+addr:(0x[0-9a-fA-F]+)")
SIZE_IN = re.compile(r"size=(0x[0-9a-fA-F]+|\d+)")

ADDR_NAME = re.compile(r"^(?:func|data)_(?:ov\d{3}_)?0[0-9a-fA-F]{7}$")


# ------------------------------------------------------------------ naming

def itanium_qual(stem):
    """`_ZN6Player8BehaviorEv` -> `Player::Behavior`, or None.

    Deliberately the same partial parse port/tools/linkage.py does, character
    for character, because the two halves have to agree on the spelling or the
    diff between them is meaningless. Reads the length-prefixed components
    between `_ZN` and its `E`, after any cv-qualifier letters. Needs at least
    two components: a one-component `_ZN...E` is a namespace-scope free function
    with no class to pair against the port's MSVC `?name@Scope@@` row on equal
    terms. Constructors, destructors and operators are NOT handled, on purpose:
    the two manglings do not spell those the same way at all and a wrong pair
    would invent a function that does not exist.
    """
    if not stem.startswith("_ZN"):
        return None
    i = 3
    while i < len(stem) and stem[i] in "KVr":
        i += 1
    parts = []
    while i < len(stem) and stem[i].isdigit():
        j = i
        while j < len(stem) and stem[j].isdigit():
            j += 1
        n = int(stem[i:j])
        if j + n > len(stem):
            return None
        parts.append(stem[j:j + n])
        i = j + n
    if i >= len(stem) or stem[i] != "E" or len(parts) < 2:
        return None
    return "::".join(parts)


def canonical(sym):
    """The name as the port half spells it: no overlay prefix, no arguments."""
    q = itanium_qual(sym)
    return q if q else sym


# ------------------------------------------------------------------ symbols

class Table(object):
    """One sorted [address, address+size) -> name table."""

    def __init__(self, region):
        self.region = region
        self.rows = {}          # addr -> (size, raw name)

    def add(self, addr, size, name):
        old = self.rows.get(addr)
        if old is None:
            self.rows[addr] = (size, name)
            return
        osize, oname = old
        # Several names share one address: an alias, or a zero-sized helper
        # name laid over a func_ address name. Keep the widest span, and prefer
        # the real name over the address-shaped placeholder, because the real
        # name is the one the port's own map will also carry.
        best = oname
        if ADDR_NAME.match(oname) and not ADDR_NAME.match(name):
            best = name
        self.rows[addr] = (max(osize, size), best)

    def finish(self):
        self.addrs = sorted(self.rows)
        self.names = [canonical(self.rows[a][1]) for a in self.addrs]
        self.sizes = [self.rows[a][0] for a in self.addrs]
        self.lo = self.addrs[0] if self.addrs else 0
        self.hi = ((self.addrs[-1] + max(self.sizes[-1], 4))
                   if self.addrs else 0)
        return self

    def lookup(self, addr):
        """(name, offset) or None. A size of 0 is an alias, not a span."""
        k = bisect.bisect_right(self.addrs, addr) - 1
        if k < 0:
            return None
        base = self.addrs[k]
        size = self.sizes[k]
        off = addr - base
        if size and off >= size:
            # Past the end of the last function that starts at or below addr.
            # Symbol tables are not always complete, so allow a landing inside
            # a modest gap rather than claiming the address is unknown, but
            # refuse anything absurd.
            if off > 0x400:
                return None
        return self.names[k], off


def load_table(path, region):
    """Function symbols from one config/**/symbols.txt."""
    t = Table(region)
    if not os.path.isfile(path):
        return None
    with open(path, errors="replace") as f:
        for line in f:
            m = SYM_ROW.match(line)
            if not m:
                continue
            name, kind, attrs, addr = m.groups()
            if kind != "function":
                continue
            s = SIZE_IN.search(attrs)
            size = int(s.group(1), 0) if s else 0
            t.add(int(addr, 16), size, name)
    if not t.rows:
        return None
    return t.finish()


def load_overlay_yaml(path):
    """[{id, base, code_size, bss_size}], parsed without a YAML dependency.

    The file is a flat list of scalar keys under `overlays:` and has been for
    every dsd version this repo has used. Anything else is refused rather than
    half-read.
    """
    out = []
    cur = None
    started = False
    with open(path, errors="replace") as f:
        for line in f:
            if line.startswith("overlays:"):
                started = True
                continue
            if not started:
                continue
            st = line.strip()
            if st.startswith("- "):
                if cur is not None:
                    out.append(cur)
                cur = {}
                st = st[2:].strip()
            if cur is None or ":" not in st:
                continue
            k, v = st.split(":", 1)
            v = v.strip()
            try:
                cur[k.strip()] = int(v, 0)
            except ValueError:
                cur[k.strip()] = v
    if cur is not None:
        out.append(cur)
    rows = []
    for o in out:
        if "id" not in o or "base_address" not in o:
            sys.exit("romtrace_resolve: {} has an overlay row with no id or "
                     "base_address; this reader will not guess".format(path))
        rows.append(dict(id=o["id"], base=o["base_address"],
                         code=o.get("code_size", 0),
                         bss=o.get("bss_size", 0)))
    return rows


def load_overlays(repo):
    """Overlay metadata, head fingerprints and per-overlay symbol tables."""
    ypath = os.path.join(repo, "extracted", "dsd", "arm9_overlays",
                         "overlays.yaml")
    if not os.path.isfile(ypath):
        sys.exit("romtrace_resolve: no {}. The extracted ROM is a gitignored "
                 "input; wire it in before running this.".format(ypath))
    rows = load_overlay_yaml(ypath)

    bodies = os.path.join(repo, "extracted", "overlays")
    missing = []
    for o in rows:
        # extracted/overlays/, never extracted/dsd/arm9_overlays/: the dsd
        # copies are shifted.
        p = os.path.join(bodies, "overlay_{:04d}.bin".format(o["id"]))
        if not os.path.isfile(p):
            missing.append(o["id"])
            o["head"] = None
        else:
            with open(p, "rb") as f:
                head = f.read(16)
            o["head"] = (struct.unpack("<4I", head)
                         if len(head) == 16 else None)
        o["table"] = load_table(
            os.path.join(repo, "config", "arm9", "overlays",
                         "ov{:03d}".format(o["id"]), "symbols.txt"),
            "ov{:03d}".format(o["id"]))
    if missing:
        sys.exit("romtrace_resolve: no overlay body for {}. Overlay residency "
                 "cannot be decided without them and this will not guess."
                 .format(", ".join("ov{:03d}".format(i) for i in missing)))

    by_base = collections.defaultdict(list)
    for o in rows:
        by_base[o["base"]].append(o)
    return rows, by_base


# ------------------------------------------------------------------ resolver

class Resolver(object):
    def __init__(self, repo):
        self.arm9 = load_table(
            os.path.join(repo, "config", "arm9", "symbols.txt"), "arm9")
        if self.arm9 is None:
            sys.exit("romtrace_resolve: no config/arm9/symbols.txt under {}"
                     .format(repo))
        # TRAP 2. Without this every division in the game is nameless.
        self.itcm = load_table(
            os.path.join(repo, "config", "arm9", "itcm", "symbols.txt"), "itcm")
        if self.itcm is None:
            sys.exit("romtrace_resolve: no config/arm9/itcm/symbols.txt. Every "
                     "division in the game goes through ITCM; resolving "
                     "without it would silently lose them.")
        self.overlays, self.by_base = load_overlays(repo)

        self.resident = ()          # ids, set per frame from FINGERPRINTs
        self._chain_cache = {}
        self.fp_empty = collections.Counter()
        self.fp_unknown = collections.Counter()
        self.fp_ambiguous = collections.Counter()
        self.residency_frames = collections.Counter()   # id -> frames resident

    # -- overlay residency ------------------------------------------------

    def set_residency(self, fingerprints, frame):
        """fingerprints: {base: (w0,w1,w2,w3)} as read this frame."""
        ids = []
        for base, words in sorted(fingerprints.items()):
            cands = self.by_base.get(base)
            if not cands:
                self.fp_unknown[base] += 1
                continue
            if words == (0, 0, 0, 0):
                self.fp_empty[base] += 1
                continue
            hits = [o for o in cands if o["head"] == words]
            if not hits:
                # Loaded, but it is not one of ours. Say so, do not guess.
                self.fp_unknown[base] += 1
                continue
            if len(hits) > 1:
                self.fp_ambiguous[base] += 1
            for o in hits:
                ids.append(o["id"])
        self.resident = tuple(sorted(set(ids)))
        for i in self.resident:
            self.residency_frames[i] += 1

    def _chain(self):
        key = self.resident
        c = self._chain_cache.get(key)
        if c is None:
            c = [self.itcm, self.arm9]
            for i in key:
                o = self.overlays[i] if (
                    i < len(self.overlays) and self.overlays[i]["id"] == i
                ) else next(x for x in self.overlays if x["id"] == i)
                if o["table"] is not None:
                    c.append(o["table"])
            self._chain_cache[key] = c
        return c

    # -- address -> name --------------------------------------------------

    def resolve(self, addr):
        """(name, offset, region). region is '???' when nothing owns it."""
        for t in self._chain():
            if addr < t.lo or addr >= t.hi:
                continue
            hit = t.lookup(addr)
            if hit:
                return hit[0], hit[1], t.region
        return "unk_{:08x}".format(addr), 0, "???"


# ------------------------------------------------------------------ reading

def read_records(path):
    """Every record in the file, as a flat tuple of words.

    There is NO file header. The only structural check available is that the
    size divides by 32, and a file that fails it is refused: a reader that
    silently truncates a misaligned file would produce a listing that looks
    exactly like a good one.
    """
    if not os.path.isfile(path):
        sys.exit("romtrace_resolve: no trace at {}".format(path))
    size = os.path.getsize(path)
    if size == 0:
        sys.exit("romtrace_resolve: {} is empty".format(path))
    if size % RECORD_BYTES:
        sys.exit("romtrace_resolve: {} is {} bytes, which is not a whole "
                 "number of {}-byte records. This is not a romtrace .bin, or "
                 "it was truncated mid-write. Refusing to read it."
                 .format(path, size, RECORD_BYTES))
    with open(path, "rb") as f:
        blob = f.read()
    return struct.unpack("<{}I".format(size // 4), blob)


def walk(words, res, args):
    """[(frame_no, [call, ...])], plus counters. A call is a tuple:
    (depth, caller, caller_off, callee, region, thumb_caller, thumb_callee,
     r0, r1, r2, r3)
    """
    frames = []
    cur = None
    stack = []
    fps = {}
    unresolved = collections.Counter()
    tag_bad = collections.Counter()
    ncalls = 0

    def open_frame(n):
        f = (n, [])
        frames.append(f)
        return f

    for i in range(0, len(words), RECORD_WORDS):
        r = words[i:i + RECORD_WORDS]
        tag = r[0]

        if tag == TAG_FRAME:
            if fps and cur is not None:
                res.set_residency(fps, cur[0])
            fps = {}
            cur = open_frame(r[1])
            stack = []
            continue

        if tag == TAG_FINGERPRINT:
            fps[r[1]] = (r[2], r[3], r[4], r[5])
            continue

        if tag != TAG_CALL:
            tag_bad[tag] += 1
            if args.strict:
                sys.exit("romtrace_resolve: record {} has tag {} (0x{:08x}), "
                         "which is not CALL, FRAME or FINGERPRINT. The file is "
                         "not the format this reads, or it is corrupt. Rerun "
                         "with --no-strict to count them and continue."
                         .format(i // RECORD_WORDS, tag, tag))
            continue

        if cur is None:
            cur = open_frame(0)
        if fps:
            res.set_residency(fps, cur[0])
            fps = {}

        frm, to = r[1], r[2]
        sp = r[7]
        caller, coff, _creg = res.resolve(frm & ~1)
        callee, eoff, ereg = res.resolve(to & ~1)
        if ereg == "???":
            unresolved[to & ~1] += 1
        # The callee address is a call TARGET, so a non-zero offset into a
        # function means the symbol table does not have that entry point.
        if eoff:
            callee = "{}+0x{:x}".format(callee, eoff)

        while stack and stack[-1] <= sp:
            stack.pop()
        depth = len(stack)
        stack.append(sp)

        cur[1].append((depth, caller, coff, callee, ereg,
                       frm & 1, to & 1, r[3], r[4], r[5], r[6]))
        ncalls += 1

    if fps and cur is not None:
        res.set_residency(fps, cur[0])

    return frames, unresolved, tag_bad, ncalls


# ------------------------------------------------------------------ output

def write_named(frames, path, max_lines, show_args):
    n = 0
    with open(path, "w") as f:
        f.write("# named call trace of the REAL CARTRIDGE, from an "
                "instrumented melonDS\n")
        f.write("# columns: frame  depth  caller+off -> callee   [region]\n")
        f.write("# depth comes from the stack pointer at the call, the same "
                "way\n# port/tools/porttrace.py derives it, so the two "
                "indentations line up.\n")
        f.write("# names carry no overlay prefix and no argument list, so they "
                "are\n# spelled exactly as the port half spells them.\n")
        for fno, calls in frames:
            f.write("\n=== frame {} ===\n".format(fno))
            for (depth, caller, coff, callee, reg, tc, te,
                 r0, r1, r2, r3) in calls:
                if max_lines and n >= max_lines:
                    f.write("... truncated at --max-lines. The summary and the "
                            "per-frame report are complete.\n")
                    return
                extra = ""
                if show_args:
                    extra = ("   r0={:08x} r1={:08x} r2={:08x} r3={:08x}"
                             .format(r0, r1, r2, r3))
                f.write("{:5d} {:3d} {}{}+0x{:x} -> {}   [{}]{}\n"
                        .format(fno, depth, "  " * min(depth, 24), caller,
                                coff, callee, reg, extra))
                n += 1


def write_frames(frames, path):
    seen = set()
    with open(path, "w") as f:
        f.write("# per frame: how many calls, and which functions are called\n")
        f.write("# HERE FOR THE FIRST TIME in this capture.\n")
        f.write("#\n")
        f.write("# This is the useful one. A function appearing for the first "
                "time is\n# the game starting to do something new, which is "
                "where a screen gets built.\n\n")
        for fno, calls in frames:
            new = []
            for c in calls:
                callee, reg = c[3], c[4]
                if callee not in seen:
                    seen.add(callee)
                    new.append((callee, reg, c[7], c[8], c[9], c[10]))
            f.write("frame {:8d}   {} calls   {} new\n"
                    .format(fno, len(calls), len(new)))
            for callee, reg, r0, r1, r2, r3 in new:
                f.write("        NEW  {:<46s} [{}]  r0={:08x} r1={:08x} "
                        "r2={:08x} r3={:08x}\n"
                        .format(callee, reg, r0, r1, r2, r3))


def write_summary(frames, res, unresolved, tag_bad, path, trace_path, size):
    counts = collections.Counter()
    edges = collections.Counter()
    byregion = collections.Counter()
    thumb = 0
    for _fno, calls in frames:
        for (_d, caller, coff, callee, reg, tc, te, _a, _b, _c, _e) in calls:
            counts[callee] += 1
            edges[("{}+0x{:x}".format(caller, coff), callee)] += 1
            byregion[reg] += 1
            thumb += te
    total = sum(counts.values())
    named = total - byregion["???"]

    with open(path, "w") as f:
        f.write("trace             : {}\n".format(trace_path))
        f.write("bytes             : {} ({} records)\n"
                .format(size, size // RECORD_BYTES))
        f.write("frames            : {}\n".format(len(frames)))
        f.write("calls             : {}\n".format(total))
        f.write("NAMED             : {} of {} ({:.4f}%)\n"
                .format(named, total,
                        100.0 * named / total if total else 0.0))
        f.write("thumb targets     : {}\n".format(thumb))
        f.write("distinct callees  : {}\n".format(len(counts)))
        f.write("distinct edges    : {}\n".format(len(edges)))
        f.write("\n--- where the callees live ---\n")
        for reg, n in sorted(byregion.items(),
                             key=lambda kv: (-kv[1], kv[0])):
            f.write("  {:<8s} {:>9d}  ({:.2f}%)\n"
                    .format(reg, n, 100.0 * n / total if total else 0.0))
        if not byregion.get("itcm"):
            f.write("  NOTE: no ITCM call was seen. Either the capture really "
                    "made none, or\n        romtrace was run with --lo above "
                    "0x01ff8000, which makes every\n        division in the "
                    "game invisible. Check the capture before\n        "
                    "believing an empty ITCM row.\n")

        f.write("\n--- overlay residency, decided per frame from the "
                "fingerprints ---\n")
        f.write("An overlay swapped in and out inside a single frame is "
                "MISSED. This is\nresolved at one-frame granularity and cannot "
                "be finer than the trace is.\n\n")
        if res.residency_frames:
            for i, n in sorted(res.residency_frames.items()):
                o = next(x for x in res.overlays if x["id"] == i)
                f.write("  ov{:03d}  base 0x{:08x}  size 0x{:x}  resident in "
                        "{} frames\n".format(i, o["base"], o["code"], n))
        else:
            f.write("  none\n")
        if res.fp_empty:
            f.write("\n  bases holding nothing (four zero words), skipped:\n")
            for base, n in sorted(res.fp_empty.items()):
                f.write("    0x{:08x}  {} frames\n".format(base, n))
        if res.fp_unknown:
            f.write("\n  bases holding something that is NOT one of our "
                    "overlays, skipped\n  rather than guessed at:\n")
            for base, n in sorted(res.fp_unknown.items()):
                f.write("    0x{:08x}  {} frames\n".format(base, n))
        if res.fp_ambiguous:
            f.write("\n  bases where SEVERAL overlays share the same first "
                    "four words, so\n  residency could not be narrowed to one. "
                    "All of them were consulted:\n")
            for base, n in sorted(res.fp_ambiguous.items()):
                f.write("    0x{:08x}  {} frames\n".format(base, n))

        if tag_bad:
            f.write("\n--- records with an unknown tag ---\n")
            for tag, n in tag_bad.most_common(20):
                f.write("  tag 0x{:08x}  {} records\n".format(tag, n))
        if unresolved:
            f.write("\n--- call targets that resolved to NO symbol ---\n")
            f.write("{} calls across {} distinct addresses\n"
                    .format(sum(unresolved.values()), len(unresolved)))
            for va, n in unresolved.most_common(40):
                f.write("  {:8d}  0x{:08x}\n".format(n, va))

        f.write("\n--- most called ---\n")
        for name, n in counts.most_common(60):
            f.write("{:8d}  {}\n".format(n, name))
        f.write("\n--- busiest edges ---\n")
        for (caller, callee), n in edges.most_common(60):
            f.write("{:8d}  {} -> {}\n".format(n, caller, callee))


# ------------------------------------------------------------------ main

def main():
    ap = argparse.ArgumentParser(
        description="resolve an instrumented-melonDS cartridge call trace "
                    "against the decomp's own symbol tables")
    ap.add_argument("trace", help="the .bin romtrace.exe wrote")
    ap.add_argument("--out-dir", required=True)
    ap.add_argument("--root", default=REPO,
                    help="repo root holding config/ and extracted/")
    ap.add_argument("--max-lines", type=int, default=400000,
                    help="cap on trace_named.txt only; the summary and the "
                         "per-frame report are always complete")
    ap.add_argument("--args", dest="show_args", action="store_true",
                    help="print r0-r3 on every line of trace_named.txt")
    ap.add_argument("--strict", dest="strict", action="store_true",
                    default=True,
                    help="stop on the first record with an unknown tag "
                         "(the default)")
    ap.add_argument("--no-strict", dest="strict", action="store_false",
                    help="count unknown-tag records and carry on")
    ap.add_argument("--require-named", type=float, default=None,
                    help="exit non-zero unless at least this percentage of "
                         "call targets resolved to a name. A resolver that "
                         "names nothing looks exactly like one that names "
                         "everything if you only read the exit code.")
    a = ap.parse_args()

    res = Resolver(a.root)
    words = read_records(a.trace)
    frames, unresolved, tag_bad, ncalls = walk(words, res, a)

    if not os.path.isdir(a.out_dir):
        os.makedirs(a.out_dir)
    write_named(frames, os.path.join(a.out_dir, "trace_named.txt"),
                a.max_lines, a.show_args)
    write_frames(frames, os.path.join(a.out_dir, "trace_frames.txt"))
    write_summary(frames, res, unresolved, tag_bad,
                  os.path.join(a.out_dir, "trace_summary.txt"),
                  a.trace, os.path.getsize(a.trace))

    bad = sum(unresolved.values())
    named = ncalls - bad
    pct = 100.0 * named / ncalls if ncalls else 0.0
    print("romtrace_resolve: {} frames, {} calls, {} named ({:.4f}%), "
          "{} unresolved -> {}"
          .format(len(frames), ncalls, named, pct, bad, a.out_dir))
    if tag_bad:
        print("romtrace_resolve: {} records carried an unknown tag"
              .format(sum(tag_bad.values())))

    if a.require_named is not None and pct < a.require_named:
        print("romtrace_resolve: FAIL, {:.4f}% named is below the {:.4f}% "
              "this run required".format(pct, a.require_named))
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
