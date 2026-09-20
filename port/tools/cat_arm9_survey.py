#!/usr/bin/env python3
"""Why is each unlinked arm9 TU unlinked? One bucket per TU, with evidence.

WHAT THIS ANSWERS. linkage.py --by-module says arm9 is 3098 matched TUs with
1648 linked. "1450 unlinked" is not a worklist: it mixes a TU nobody ever
offered to the linker with one that was offered and dropped, and those two need
opposite fixes. This splits the 1450 by the FIRST fact that explains it, in an
order chosen so that each bucket is disjoint and the earlier ones are the ones
a lane can act on.

THE TWO FACTS EVERY VERDICT RESTS ON, and where they come from.

  OFFERED   the TU's source path appears in a port/slice_*.txt list, or is
            named directly in port/CMakeLists.txt. Those are the only two ways
            a src/ file reaches the walk_window compile. Offered-and-absent
            from the map means the linker SAW the object and /OPT:REF dropped
            it for want of a reference edge -- a missing bridge, not missing
            work. Never-offered means nobody has tried.

  CALLERS   who references the TU's ROM function. TWO SOURCES, and the second
            one is not optional.

            config/*/relocs.txt is the delinker's own record, and because
            delinks.txt cuts arm9 one function per file, every call between two
            functions crosses a file boundary and appears there. That is the
            ROM's real call graph, not a guess from names.

            BUT IT HOLDS ONLY CODE RELOCATIONS. Every one of arm9's 13891 rows
            is an arm_call or a PC-relative load; not one is a data word. arm9
            is not relocated at runtime, so a vtable's slots are plain absolute
            addresses in .data and the delinker never emitted them as relocs.
            Taking relocs.txt alone therefore reports "nothing references it"
            for every method that is only ever reached through a vtable, which
            in a C++ game is most of the interesting ones -- and it reports it
            in exactly the confident shape that gets a real target written off
            as dead. So the second source is a scan of the arm9 image's own
            data words: any word in .data/.rodata/.init landing exactly on a
            function symbol's first byte is a table reference, and the data
            symbol holding that word is recorded as the caller.

            A TU whose only callers live in overlays the port never mounts
            cannot be reached however much host wiring is added, and that is
            bucket (d) rather than work.

THE BUCKETS, in the order they are tested:

  (e) EXCEPTION    linkage.py already rules this a documented host-ABI
                   exception (PORT_HOST_ABI tag). Not work. Tested first so a
                   tagged symbol cannot be recounted as anything else.
  (a) SHADOW       a host file defines the symbol and the matched TU is not in
                   the binary. The host body is what runs. Direct replacement.
  (b) DROPPED      offered to the linker and not in the map. /OPT:REF took it.
                   A reference edge is missing; this is the workable set.
  (d) OV-ONLY      never offered, and every ROM caller of it lives in a module
                   the port does not mount. Unreachable by construction.
  (c) NO-CALLER    never offered, and no linked TU references it. Either its
                   subsystem never runs in walk_window or its callers are
                   themselves unlinked. Sub-split into UNREACHED (some arm9
                   caller exists but that caller is unlinked too) and ORPHAN
                   (nothing in the ROM references it at all -- a table entry,
                   a vtable slot body, or a genuine leaf).
  (f) CANDIDATE    never offered, and a LINKED TU references it directly. This
                   is the other workable set: the reference edge already exists
                   in the binary, so compiling the TU in should link it.

WHY (f) IS SEPARATE FROM (b). Both are workable, but the work differs. A (b)
TU is already compiled and needs a caller; an (f) TU already has a caller and
needs compiling. Conflating them produces a worklist where half the entries
have the wrong fix written next to them.

AND THE PART THE BUCKETS ON THEIR OWN GET WRONG. UNREACHED is by far the
biggest bucket, and read as a list it invites a lane to pick TUs off it one at
a time, which does nothing: each one's caller is unlinked too, so seating it
adds an object the linker drops again. Those TUs are not 900-odd separate
problems. They are a few hundred CLUSTERS hanging off a much smaller set of
ROOTS -- the members with no unlinked caller of their own -- and the root is
the only place a lane can push. --roots prints them, classified by what would
have to be true for the root to be reached at all, which is the difference
between "this subsystem needs a vtable slot seated" and "this subsystem's only
callers are in an overlay the port does not mount".

THE LIMIT, stated plainly. A "linked caller references it" edge is proof the
ROM call exists, NOT proof the port's copy of that caller still makes the call:
the host build may reach the same point through a host body. So (f) is a
candidate list, not a promise. The only way to settle one is to compile it and
read the map, which is what phase 2 does, one batch at a time.

    python port/tools/cat_arm9_survey.py [repo-root] [--list BUCKET] [--all]
                                                     [--roots]
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import linkage  # noqa: E402
import objsrc_check  # noqa: E402


def repo_root():
    """The checkout this script lives in: <root>/port/tools/cat_arm9_survey.py.

    Two dirnames off the file gives port/, three gives the root. Same rule as
    linkage.py and gate.py, and for the same reason -- a tool rooted at CWD
    reads correct from the repo root and silently measures another tree from
    anywhere else.
    """
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(os.path.dirname(here))


SYM_LINE = re.compile(
    r"^(\S+)\s+kind:(\w+)\(([^)]*)\)\s+addr:0x([0-9a-fA-F]+)")


def parse_symbols(path, module):
    """[(name, addr, size, module)] for one config symbols.txt."""
    out = []
    try:
        f = open(path, errors="replace")
    except OSError:
        return out
    with f:
        for line in f:
            m = SYM_LINE.match(line)
            if not m:
                continue
            name, kind, attrs, addr = m.groups()
            if kind != "function":
                continue
            size = 0
            ms = re.search(r"size=0x([0-9a-fA-F]+)", attrs)
            if ms:
                size = int(ms.group(1), 16)
            out.append((name, int(addr, 16), size, module))
    return out


def all_symbols(root):
    """Every function symbol in the config, tagged with its module."""
    syms = []
    syms += parse_symbols(os.path.join(root, "config", "arm9", "symbols.txt"),
                          "arm9")
    for sub in ("itcm", "dtcm"):
        syms += parse_symbols(
            os.path.join(root, "config", "arm9", sub, "symbols.txt"), sub)
    ovdir = os.path.join(root, "config", "arm9", "overlays")
    for ov in sorted(os.listdir(ovdir)) if os.path.isdir(ovdir) else []:
        syms += parse_symbols(os.path.join(ovdir, ov, "symbols.txt"), ov)
    return syms


def owner_index(syms):
    """{module: sorted [(addr, end, name)]} for address -> symbol lookup.

    Kept per module because overlays share address windows: 0x020ad6ac is a
    real address in a dozen different overlays and resolving it globally picks
    whichever sorted first. The reloc file a `from:` came out of names the
    module, so the lookup is always scoped.
    """
    out = {}
    for name, addr, size, mod in syms:
        out.setdefault(mod, []).append((addr, addr + max(size, 4), name))
    for mod in out:
        out[mod].sort()
    return out


def owner_of(spans, addr):
    """The symbol whose span contains addr, or None. Bisect on a sorted list."""
    lo, hi = 0, len(spans)
    while lo < hi:
        mid = (lo + hi) // 2
        if spans[mid][0] <= addr:
            lo = mid + 1
        else:
            hi = mid
    if lo == 0:
        return None
    start, end, name = spans[lo - 1]
    return name if addr < end else None


RELOC = re.compile(r"^from:0x([0-9a-fA-F]+)\s+kind:(\S+)\s+to:0x([0-9a-fA-F]+)"
                   r"\s+module:(\S+)")
MOD_OVS = re.compile(r"overlays?\((.*)\)")


def reloc_files(root):
    """[(path, from-module)] for every relocs.txt in the config."""
    out = [(os.path.join(root, "config", "arm9", "relocs.txt"), "arm9")]
    for sub in ("itcm", "dtcm"):
        out.append((os.path.join(root, "config", "arm9", sub, "relocs.txt"),
                    sub))
    ovdir = os.path.join(root, "config", "arm9", "overlays")
    for ov in sorted(os.listdir(ovdir)) if os.path.isdir(ovdir) else []:
        out.append((os.path.join(ovdir, ov, "relocs.txt"), ov))
    return [(p, m) for p, m in out if os.path.isfile(p)]


def callers(root, spans):
    """{callee symbol: set(caller symbol)} from the delinker's own relocs.

    A reloc's `to:` names a destination module which may be a SET of overlays
    (`module:overlays(6,99)`) when the delinker could not pin one. Those
    resolve against each candidate and the first module that owns the address
    wins; an ambiguous edge is still a real edge, and for this survey the
    question is only "does anything reach it", so one resolution is enough.
    """
    out = {}
    for path, frommod in reloc_files(root):
        with open(path, errors="replace") as f:
            for line in f:
                m = RELOC.match(line)
                if not m:
                    continue
                src, _kind, dst, dmod = m.groups()
                src, dst = int(src, 16), int(dst, 16)
                frm = owner_of(spans.get(frommod, []), src)
                if frm is None:
                    continue
                cands = []
                if dmod == "main":
                    cands = ["arm9"]
                elif dmod in ("itcm", "dtcm"):
                    cands = [dmod]
                else:
                    mo = MOD_OVS.search(dmod)
                    if mo:
                        cands = ["ov%03d" % int(x) for x in
                                 mo.group(1).split(",") if x.strip().isdigit()]
                for c in cands:
                    to = owner_of(spans.get(c, []), dst)
                    if to is not None:
                        out.setdefault(to, set()).add(frm)
                        break
    return out


ARM9_BASE = 0x02004000  # extracted/arm9_dec.bin is flat at this address
# romdata.py's anchor: the image is only trusted when Copy36Bytes reads as the
# function it should be. Borrowing the same check rather than inventing a
# second one, so a wrong image is caught here the way it is caught there.
ANCHOR_NAME = "Copy36Bytes"


def data_refs(root, spans, syms):
    """{callee: set(data symbol holding a word that points at it)}.

    Scans arm9's initialized image for absolute pointers into code. A word is
    counted only when it lands on a function symbol's FIRST byte: an interior
    hit is far more likely to be an integer that happens to look like an
    address than a real entry point, and this survey would rather miss a table
    than invent a caller.

    Only arm9 is scanned. An overlay's tables would need each overlay image
    loaded at its own base, and the answer would not change any bucket here:
    an overlay-only reference already puts a TU in OV-ONLY, and an overlay
    table that points into arm9 is a rarity the code relocs already carry.
    """
    path = os.path.join(root, "extracted", "arm9_dec.bin")
    if not os.path.isfile(path):
        return {}, "no extracted/arm9_dec.bin -- table references not scanned"
    with open(path, "rb") as f:
        img = f.read()

    starts = {}
    data_syms = []
    for name, addr, size, mod in syms:
        if mod != "arm9":
            continue
        starts[addr] = name
    for name, addr, size, mod in all_data_symbols(root):
        data_syms.append((addr, name))
    data_syms.sort()

    anchor = [a for a, n in ((a, n) for a, n in starts.items())
              if n == ANCHOR_NAME]
    if not anchor:
        return {}, "no %s symbol -- image anchor not checked" % ANCHOR_NAME

    out = {}
    # the delink section table says which windows hold initialized data
    windows = []
    dl = os.path.join(root, "config", "arm9", "delinks.txt")
    with open(dl, errors="replace") as f:
        for line in f:
            m = re.match(r"\s+\.(\w+)\s+start:0x([0-9a-fA-F]+)\s+"
                         r"end:0x([0-9a-fA-F]+)\s+kind:(\w+)", line)
            # DATA WINDOWS ONLY. Including the code sections looks harmless and
            # is not: a .text literal pool holds function addresses too, and
            # those are already in relocs.txt as kind:load, so scanning code
            # here would double-count real call edges as table references and
            # move TUs out of the bucket that describes them.
            # .exception and .exceptix are mwcc's exception tables. They are
            # rodata full of function addresses and NONE of them is a caller:
            # a function is listed there because it has a cleanup range, not
            # because anything reaches it. Left in, they hand 53 arm9 functions
            # a fictional table reference.
            if m and m.group(1) in ("exception", "exceptix"):
                continue
            if m and m.group(4) in ("data", "rodata"):
                windows.append((int(m.group(2), 16), int(m.group(3), 16)))
            if m and m.group(1) == "bss":
                break

    def data_owner(addr):
        lo, hi = 0, len(data_syms)
        while lo < hi:
            mid = (lo + hi) // 2
            if data_syms[mid][0] <= addr:
                lo = mid + 1
            else:
                hi = mid
        return data_syms[lo - 1][1] if lo else None

    for start, end in windows:
        off = start - ARM9_BASE
        n = min(end, ARM9_BASE + len(img)) - start
        for i in range(0, n - 3, 4):
            w = int.from_bytes(img[off + i:off + i + 4], "little")
            name = starts.get(w)
            if name is None:
                continue
            holder = data_owner(start + i)
            out.setdefault(name, set()).add(holder or "data_%08x" % (start + i))
    return out, None


def all_data_symbols(root):
    """[(name, addr, 0, module)] for arm9's data and bss symbols."""
    out = []
    p = os.path.join(root, "config", "arm9", "symbols.txt")
    try:
        f = open(p, errors="replace")
    except OSError:
        return out
    with f:
        for line in f:
            m = re.match(r"^(\S+)\s+kind:(\w+)", line)
            ma = re.search(r"addr:0x([0-9a-fA-F]+)", line)
            if m and ma and m.group(2) in ("data", "bss"):
                out.append((m.group(1), int(ma.group(1), 16), 0, "arm9"))
    return out


SLICE_ENTRY = re.compile(r"([A-Za-z_]\w*)\.(?:c|cpp)\s*$")


def offered_stems(root):
    """Stems the port hands to the walk_window compile, from the CMake inputs.

    Two sources, because the build has two ways of naming a src/ file: the
    slice_*.txt lists (one repo-relative path per line) and CMakeLists.txt
    itself (the hostgen symbol sets and a handful of direct src/ paths). A
    stem named either way was offered.
    """
    out = set()
    pdir = os.path.join(root, "port")
    for fn in sorted(os.listdir(pdir)):
        if not (fn.startswith("slice_") and fn.endswith(".txt")):
            continue
        with open(os.path.join(pdir, fn), errors="replace") as f:
            for line in f:
                line = line.split("#", 1)[0].strip()
                if not line:
                    continue
                m = SLICE_ENTRY.search(line.replace("\\", "/"))
                if m:
                    out.add(m.group(1))
    cml = os.path.join(pdir, "CMakeLists.txt")
    with open(cml, errors="replace") as f:
        text = f.read()
    # direct `src/<stem>.c` / `.cpp` mentions
    for m in re.finditer(r"src/([A-Za-z_]\w*)\.(?:c|cpp)\b", text):
        out.add(m.group(1))
    # hostgen symbol sets: `set(GATE4A_SYMS  func_... func_...)`
    for m in re.finditer(r"set\((\w*(?:SYMS|_SYMS))\b([^)]*)\)", text):
        for tok in m.group(2).split():
            if re.fullmatch(r"[A-Za-z_]\w*", tok):
                out.add(tok)
    return out


# Modules the port mounts. A TU whose only ROM callers live outside this set
# cannot be reached by any amount of host wiring, so it is not work.
# Derived from the ov*_syms.txt files the build actually consumes rather than
# hand-listed, so it tracks the tree instead of a comment going stale.
def mounted_modules(root):
    out = {"arm9", "itcm", "dtcm"}
    with open(os.path.join(root, "port", "CMakeLists.txt"),
              errors="replace") as f:
        text = f.read()
    for m in re.finditer(r"\bov(\d{3})_syms\.txt\b", text):
        out.add("ov" + m.group(1))
    for m in re.finditer(r'set\(LEVEL_OVS?\b([^)]*)\)', text):
        for tok in re.findall(r"\bov(\d{3})\b", m.group(1)):
            out.add("ov" + tok)
    return out


FUNC_ADDR = re.compile(r"^func_0[12][0-9a-fA-F]{6}$")


def main():
    args = [a for a in sys.argv[1:]]
    want_list = None
    if "--list" in args:
        i = args.index("--list")
        want_list = args[i + 1].upper()
        del args[i:i + 2]
    show_all = "--all" in args
    if show_all:
        args.remove("--all")
    show_roots = "--roots" in args
    if show_roots:
        args.remove("--roots")
    root = os.path.abspath(args[0]) if args else repo_root()

    mapfile = os.path.join(root, "build", "port", "walk_window.map")
    print("survey root: %s" % root)
    print("map:         %s" % mapfile)
    print()

    matched = linkage.matched_index(root)
    syms = linkage.map_symbols(mapfile)
    # DELIBERATELY THE SAME RULE linkage.py uses, over-count and all, so the
    # bucket table reconciles with the headline a lane quotes beside it. The
    # over-count is real (objsrc_check.py names the 18 host stand-ins counted
    # here as linked matched TUs) and it is reported separately rather than
    # silently corrected: two tools answering the same question with two
    # different numbers is how a status file ends up carrying both.
    linked_stems = set()
    for _sym, obj in syms:
        stem = os.path.splitext(os.path.splitext(obj.split(":")[-1])[0])[0]
        if stem in matched:
            linked_stems.add(stem)

    name_mod = linkage.module_index(root)

    def module_of(stem):
        m = linkage.FUNC_OV.match(stem)
        if m:
            return m.group(1)
        if FUNC_ADDR.match(stem):
            return "arm9"
        return name_mod.get(stem, "UNKNOWN")

    arm9 = sorted(s for s in matched if module_of(s) == "arm9")
    arm9_unlinked = [s for s in arm9 if s not in linked_stems]

    # HOST DEFINITIONS, so a shadow can be named. WHICH OBJECTS ARE HOST
    # OBJECTS IS ASKED OF THE BUILD, not of the object's name.
    #
    # Two ways the name lies, in opposite directions. Guessing from the name
    # alone gets both wrong, and each wrong answer names a fix that would not
    # work:
    #
    #   * port/ntr/runtime.cpp defines Copy36Bytes, the six IRQ entry points
    #     and the three CP15 cache ops. Restricting host directories to hal/
    #     and unmatched/ moves those twelve into CANDIDATE, reading as "a
    #     linked caller references it, compile it in" -- and compiling any of
    #     them in is a duplicate symbol against the ntr layer.
    #   * port/unmatched/func_0204322c.cpp was a HOST file whose stem was also
    #     a matched TU, so a name-based rule read it as the decompiled TU. That
    #     is the over-claim objsrc_check.py exists for, and here it would hide
    #     a real shadow instead of inventing a fake candidate. The eighteen
    #     files of that shape now carry a `_hostcopy` suffix, so no host stem
    #     collides with a matched TU today. The build.ninja route below is what
    #     keeps that true without depending on the naming convention holding.
    #
    # build.ninja carries each object's real source path and is generated from
    # the same CMakeLists the link uses, so it settles both.
    # AMBIGUITY IS FINE AS LONG AS EVERY CANDIDATE IS A HOST FILE. fs.cpp
    # exists under both port/hal and port/ntr and both compile to fs.cpp.obj,
    # so the basename has two sources and neither the map nor this tool can
    # say which one a symbol came from. It does not matter: the question here
    # is "is a host file answering this name", and both answers are yes.
    # Requiring exactly one source instead dropped fs.cpp entirely, and with
    # it SharedFilePtr::Load, DecompressLZ16 and the three card-loader
    # entry points -- five host bodies reported as work nobody had started.
    objsrc = objsrc_check.object_sources(root)
    hostdirs = tuple((root + "/port/" + d + "/").replace("\\", "/").lower()
                     for d in ("hal", "unmatched", "ntr", "tests"))
    host_defs = {}
    for sym, obj in syms:
        base = obj.split(":", 1)[-1]
        srcs = objsrc.get(base, ())
        if not srcs or not all(s.replace("\\", "/").lower().startswith(hostdirs)
                               for s in srcs):
            continue
        src = sorted(srcs)[0].replace("\\", "/")
        stem = os.path.splitext(os.path.splitext(base)[0])[0]
        host_defs.setdefault(sym, (stem, src))

    # A PORT_HOST_ABI TAG IS A RULING ABOUT THE SOURCE, so it is read from the
    # source and does not depend on the symbol surviving the link. func_02042ffc
    # is the case that forced this: port/unmatched/ActorDerived_Spawn.cpp defines
    # it and tags it, MSVC folds it into the one-line wrapper that calls it, and
    # no symbol of that name reaches the map. Keyed off the map alone the TU then
    # reads as DROPPED -- work, with no host body in sight -- when a human has
    # already ruled it an ARM register ride-through and written down why.
    tagged = {}
    for d in ("hal", "hal/sdat", "unmatched", "ntr"):
        hdir = os.path.join(root, "port", *d.split("/"))
        for fn in sorted(os.listdir(hdir)) if os.path.isdir(hdir) else []:
            if not fn.endswith((".c", ".cpp")):
                continue
            p = os.path.join(hdir, fn)
            for sym, reason in linkage._reasons_in(p).items():
                tagged.setdefault(sym, (fn, reason))

    allsyms = all_symbols(root)
    spans = owner_index(allsyms)
    call = callers(root, spans)
    tables, tblnote = data_refs(root, spans, allsyms)
    if tblnote:
        print("NOTE: %s" % tblnote)
    offered = offered_stems(root)
    mounted = mounted_modules(root)

    # A TU's ROM function name is its stem for func_* files; for a recovered
    # name the stem IS the config symbol. Either way the stem is the key the
    # caller map is built on.
    buckets = {k: [] for k in ("EXCEPTION", "SHADOW", "DROPPED", "OV-ONLY",
                               "UNREACHED", "ORPHAN", "CANDIDATE",
                               "TABLE-ONLY")}
    evidence = {}
    for stem in arm9_unlinked:
        cs = call.get(stem, set())
        tbl = tables.get(stem, set())
        src = matched[stem]
        host = host_defs.get(stem)
        if host and linkage.abi_reason(host[1], stem):
            buckets["EXCEPTION"].append(stem)
            evidence[stem] = host[0]
            continue
        if stem in tagged:
            buckets["EXCEPTION"].append(stem)
            evidence[stem] = "%s (tagged, host body folded)" % tagged[stem][0]
            continue
        if host:
            buckets["SHADOW"].append(stem)
            evidence[stem] = host[0]
            continue
        if stem in offered:
            buckets["DROPPED"].append(stem)
            evidence[stem] = src
            continue
        live = sorted(c for c in cs if c in linked_stems)
        if live:
            buckets["CANDIDATE"].append(stem)
            evidence[stem] = "called by " + ", ".join(live[:3])
            continue
        if not cs and tbl:
            # only a data table points at it: a vtable slot or a callback
            # table. Reaching it is a vtable-seat job, not a call-edge job.
            buckets["TABLE-ONLY"].append(stem)
            evidence[stem] = "in table " + ", ".join(sorted(tbl)[:2])
            continue
        if not cs:
            buckets["ORPHAN"].append(stem)
            evidence[stem] = "no call reloc and no arm9 table word"
            continue
        cmods = {module_of(c) for c in cs}
        if not (cmods & mounted):
            buckets["OV-ONLY"].append(stem)
            evidence[stem] = "callers in " + ", ".join(sorted(cmods)[:4])
            continue
        buckets["UNREACHED"].append(stem)
        evidence[stem] = "callers unlinked: " + ", ".join(sorted(cs)[:3])

    print("arm9 matched TUs: %d   linked: %d   unlinked: %d"
          % (len(arm9), len(arm9) - len(arm9_unlinked), len(arm9_unlinked)))
    print("mounted modules: %d (%s ...)"
          % (len(mounted), ", ".join(sorted(mounted)[:8])))
    print()
    order = ["SHADOW", "DROPPED", "CANDIDATE", "TABLE-ONLY", "UNREACHED",
             "OV-ONLY", "ORPHAN", "EXCEPTION"]
    work = {"SHADOW", "DROPPED", "CANDIDATE"}
    for b in order:
        print("  %-10s %5d %s" % (b, len(buckets[b]),
                                  "<- workable" if b in work else ""))
    print("  %-10s %5d" % ("TOTAL", sum(len(v) for v in buckets.values())))
    if sum(len(v) for v in buckets.values()) != len(arm9_unlinked):
        print("  RECONCILIATION FAILED -- do not quote these numbers.")

    if show_roots:
        print()
        print_roots(buckets, call, tables, linked_stems, module_of, mounted,
                    matched)

    for b in order:
        if want_list and b != want_list:
            continue
        if not want_list and not show_all:
            continue
        print()
        print("== %s (%d) ==" % (b, len(buckets[b])))
        for stem in buckets[b]:
            print("  %-34s %s" % (stem, evidence.get(stem, "")))


def print_roots(buckets, call, tables, linked_stems, module_of, mounted,
                matched):
    """Where the UNREACHED mass hangs from, and what would have to give.

    A root is an UNREACHED TU with no caller inside the unreached set. Pushing
    anywhere else in a cluster is wasted: the linker drops a TU whose caller it
    also dropped. Each root is classified by the ONE thing that stands between
    it and the linked core, which is what makes the row actionable.
    """
    cluster = set(buckets["UNREACHED"])
    # A root has no unlinked caller AT ALL, tested against every unlinked
    # bucket rather than against UNREACHED alone. Scoping the test to one
    # bucket promotes a TU whose only caller sits in TABLE-ONLY into a root it
    # is not, and a false root is the most expensive row on this list: it is
    # the one a lane spends an afternoon on.
    all_unlinked = set()
    for v in buckets.values():
        all_unlinked.update(v)
    roots = sorted(x for x in cluster
                   if not (call.get(x, set()) & all_unlinked))
    kinds = {}
    detail = {}
    for r in roots:
        cs = call.get(r, set())
        if tables.get(r):
            k = "table slot: seat it in a hosted vtable/dispatch table"
            detail[r] = ", ".join(sorted(tables[r])[:2])
        elif not cs:
            k = "nothing references it in arm9 at all"
            detail[r] = ""
        else:
            mods = {module_of(c) for c in cs}
            if not (mods & {"arm9", "itcm", "dtcm"}):
                inm = mods & mounted
                k = ("overlay callers, overlay MOUNTED" if inm
                     else "overlay callers, overlay not mounted")
                detail[r] = ", ".join(sorted(mods)[:4])
            else:
                unmatched = sorted(c for c in cs if c not in matched)
                k = ("arm9 caller has no matched TU" if unmatched
                     else "arm9 caller unlinked but matched")
                detail[r] = ", ".join((unmatched or sorted(cs))[:3])
        kinds.setdefault(k, []).append(r)

    print("UNREACHED CLUSTER ROOTS. %d TUs in %d clusters; a lane can only"
          % (len(cluster), len(roots)))
    print("push at a root, because every non-root's caller is dropped too.")
    print()
    for k in sorted(kinds, key=lambda x: -len(kinds[x])):
        print("  %-46s %4d" % (k, len(kinds[k])))
    print()
    for k in sorted(kinds, key=lambda x: -len(kinds[x])):
        print("-- %s (%d)" % (k, len(kinds[k])))
        for r in kinds[k]:
            print("   %-44s %s" % (r, detail.get(r, "")))


if __name__ == "__main__":
    main()
