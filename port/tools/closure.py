#!/usr/bin/env python3
"""Measure a slice's link closure without paying for a link.

WHY THIS EXISTS. Every seat lane answers the same question over and over:
"if I add these TUs, what will the linker still want, and what will it
refuse as already defined?" The lanes have been paying for that answer with
REAL LINK WAVES -- the MG1 pathfinder ran FOUR of them to close a 276-TU
slice and a fifth to enumerate its wall, the S1 spine ran link rounds 4
through 8, and SL0 finally wrote the cheap version as a worktree-local
script (C:/tmp/g2/build/closure.py). This is that script promoted: compile
the candidate TUs with walk_window's own flags, read each object's symbol
table, and diff the union against the current walk_window.map. Same answer
as a link for "what does this slice still want", at the cost of a compile.

WHAT IT REPORTS, in three sections:

  COMPILE FAILURES   TUs cl.exe refused, with the tail of each log. These
                     are data, not noise -- the gate-16 PMF wall and the
                     C2733 declaration conflicts announce themselves here
                     before any link would have found them.
  UNRESOLVED         externals no object in the set defines and the image
                     does not carry. The slice's real closure frontier.
  DUP-DEF            externals the set defines that the image ALSO defines:
                     the LNK2005 list. SL0 measured the gate-36 slice at
                     FIFTEEN duplicate definitions where the planning map
                     said one; this section is why no slice should be wired
                     from a map's say-so again.

TWO CAVEATS THAT ARE PART OF THE ANSWER, both paid for by lanes:

  * The UNRESOLVED count is a FLOOR, not the bill (lane L4's stage map).
    A TU that resolves through an /alternatename face, or one /OPT:REF
    drops for want of a reference edge, never shows up here and still has
    to be carried. Closure says what the linker will ASK for; it does not
    say what will finally LINK.
  * Names are compared RAW, exactly as dumpbin and the map spell them.
    linkage.py's old map_symbols() lstrip("_") bug made _ZN* host
    definitions invisible by over-stripping; nothing here strips anything.

    python port/tools/closure.py --root C:/tmp/g2 src/_ZN5Stage13InitResourcesEv.cpp
    python port/tools/closure.py --root C:/tmp/g2 --slice port/slice_gate36.txt
    python port/tools/closure.py --selftest
"""

import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile

# The compile line is walk_window's own, transcribed from the lane probe
# (C:/tmp/g2/build/probe.cmd) which transcribed it from build.ninja. If the
# port's flags change in CMakeLists.txt this line follows them or the
# measurement is of some other build.
CL_FLAGS = ("/nologo /c /DWIN32 /D_WINDOWS /GR /EHsc /O2 /Ob2 /DNDEBUG "
            "-std:c++17 -MT /Oy- /wd4311 /wd4312 /wd4068 /wd4576 "
            "-DPORT_CXX_ALIASES_LINKED -DSM64DS_PLATFORM_PC")

VCVARS_CANDIDATES = [
    r"%s\Microsoft Visual Studio\2022\%s\VC\Auxiliary\Build\vcvars32.bat"
    % (pf, ed)
    for pf in (os.environ.get("ProgramFiles(x86)",
                              r"C:\Program Files (x86)"),)
    for ed in ("BuildTools", "Community", "Professional", "Enterprise")
]

# One map row: "  0001:00012345  _name  10012345  obj". Same expression the
# SL0 probe used; it is deliberately anchored on the section:offset pair so
# header lines and the static-symbol tail never match.
MAP_ROW = re.compile(
    r"\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+[0-9a-fA-F]{8}")


def find_vcvars():
    for c in VCVARS_CANDIDATES:
        if os.path.exists(c):
            return c
    return None


def map_defined(mappath):
    defined = set()
    with open(mappath, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = MAP_ROW.match(line)
            if m:
                defined.add(m.group(1))
    return defined


def read_syms(symspath):
    """dumpbin /SYMBOLS -> (undefined externals, defined externals)."""
    und, dfn = set(), set()
    with open(symspath, encoding="utf-8", errors="replace") as f:
        for line in f:
            if "External" not in line:
                continue
            m = re.search(r"\|\s+(\S+)", line)
            if not m:
                continue
            if "UNDEF" in line:
                und.add(m.group(1))
            else:
                dfn.add(m.group(1))
    return und, dfn


def slice_lines(path):
    out = []
    with open(path, encoding="utf-8-sig", errors="replace") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if line:
                out.append(line)
    return out


def compile_batch(root, sources, outdir, extra_flags=""):
    """Compile every source with the port's flags; one vcvars call total.

    Returns {source: syms-file-or-None}; the .log beside each object holds
    cl's own words for the failures.
    """
    vcvars = find_vcvars()
    if vcvars is None:
        sys.exit("no vcvars32.bat found under Visual Studio 2022; "
                 "closure needs cl.exe and dumpbin")
    outdir = pathlib.Path(outdir)
    outdir.mkdir(parents=True, exist_ok=True)
    root = pathlib.Path(root)
    inc = "-I%s -I%s -I%s" % (root / "port", root / "include",
                              root / "port" / "ntr" / "include")
    rootdef = '-DPORT_REPO_ROOT="%s"' % str(root).replace("\\", "/")

    script = [u"@echo off", u'call "%s" >nul' % vcvars]
    plan = []
    for i, src in enumerate(sources):
        srcp = pathlib.Path(src)
        if not srcp.is_absolute():
            srcp = root / srcp
        stem = "%03d_%s" % (i, srcp.name)
        obj = outdir / (stem + ".obj")
        syms = outdir / (stem + ".syms")
        log = outdir / (stem + ".log")
        plan.append((src, srcp, obj, syms))
        script.append('cl %s %s %s %s /Fo"%s" "%s" > "%s" 2>&1'
                      % (CL_FLAGS, extra_flags, rootdef, inc, obj, srcp, log))
        script.append('if exist "%s" dumpbin /nologo /SYMBOLS "%s" > "%s"'
                      % (obj, obj, syms))
    runner = outdir / "closure_run.cmd"
    runner.write_text("\r\n".join(script) + "\r\n", encoding="ascii",
                      errors="replace")
    subprocess.run(["cmd", "/c", str(runner)], capture_output=True)

    results = {}
    for src, srcp, obj, syms in plan:
        results[src] = syms if syms.exists() and obj.exists() else None
    return results


def measure(root, mappath, sources, outdir, extra_flags=""):
    have = map_defined(mappath)
    results = compile_batch(root, sources, outdir, extra_flags)
    allund, alldef, failed = set(), set(), []
    for src, syms in results.items():
        if syms is None:
            log = pathlib.Path(str(pathlib.Path(outdir)))
            tail = []
            for cand in log.glob("*_%s.log" % pathlib.Path(src).name):
                tail = cand.read_text(errors="replace").splitlines()[-3:]
            failed.append((src, tail))
            continue
        u, d = read_syms(syms)
        allund |= u
        alldef |= d
    unresolved = sorted(x for x in allund if x not in have
                        and x not in alldef)
    dupdef = sorted(x for x in alldef if x in have)
    return have, failed, unresolved, dupdef


def report(have, failed, unresolved, dupdef):
    print("map defines %d symbols" % len(have))
    if failed:
        print("\n=== COMPILE FAILURES (%d) ===" % len(failed))
        for src, tail in failed:
            print("  ", src)
            for line in tail:
                print("       ", line)
    print("\n=== UNRESOLVED after this slice: %d ===" % len(unresolved))
    for x in unresolved:
        print("   ", x)
    print("\n=== DUP-DEF against the image (LNK2005): %d ===" % len(dupdef))
    for x in dupdef:
        print("   ", x)


def selftest():
    """Three tiny TUs against a fabricated map; every section exercised.

    a.cpp defines a_def and wants missing_sym, present_sym and dup_sym.
    b.cpp defines dup_sym (also in the map: the LNK2005 case) and wants
    nothing. c.cpp does not compile. The fabricated map carries present_sym
    and dup_sym. Expected: failures == {c.cpp}, unresolved == {_missing_sym}
    (satisfied-in-set a_def proves the set-union rule, satisfied-in-map
    present_sym proves the map rule), dupdef == {_dup_sym}.
    """
    with tempfile.TemporaryDirectory() as td:
        tdp = pathlib.Path(td)
        (tdp / "port").mkdir()
        (tdp / "include").mkdir()
        (tdp / "port" / "ntr" / "include").mkdir(parents=True)
        (tdp / "a.cpp").write_text(
            'extern "C" void missing_sym(); extern "C" void present_sym();\n'
            'extern "C" void dup_sym();\n'
            'extern "C" void a_def() { missing_sym(); present_sym(); '
            'dup_sym(); }\n')
        (tdp / "b.cpp").write_text('extern "C" void dup_sym() {}\n')
        (tdp / "c.cpp").write_text("this does not compile\n")
        fakemap = tdp / "fake.map"
        fakemap.write_text(
            " 0001:00000000       _present_sym               10000000 x.obj\n"
            " 0001:00000010       _dup_sym                   10000010 x.obj\n")
        have, failed, unresolved, dupdef = measure(
            tdp, fakemap,
            [str(tdp / "a.cpp"), str(tdp / "b.cpp"), str(tdp / "c.cpp")],
            tdp / "probe")
        ok = True
        if [s for s, _ in failed] != [str(tdp / "c.cpp")]:
            print("FAIL: compile-failure section wrong:", failed)
            ok = False
        if unresolved != ["_missing_sym"]:
            print("FAIL: unresolved wrong:", unresolved)
            ok = False
        if dupdef != ["_dup_sym"]:
            print("FAIL: dupdef wrong:", dupdef)
            ok = False
        print("selftest %s" % ("PASS" if ok else "FAIL"))
        return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(
        description="slice closure without a link; see the docstring")
    ap.add_argument("sources", nargs="*",
                    help="TU paths, absolute or root-relative")
    ap.add_argument("--root", default=None,
                    help="repo root (default: two dirs above this file)")
    ap.add_argument("--map", dest="mapfile", default=None,
                    help="image map (default: <root>/build/port/"
                         "walk_window.map)")
    ap.add_argument("--slice", action="append", default=[],
                    help="read TU paths from a slice_*.txt (repeatable)")
    ap.add_argument("--out", default=None,
                    help="object/scratch dir (default: a temp dir)")
    ap.add_argument("--extra-flags", default="",
                    help="appended verbatim to the cl line")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        sys.exit(selftest())

    root = pathlib.Path(args.root) if args.root else \
        pathlib.Path(__file__).resolve().parents[2]
    mapfile = args.mapfile or (root / "build" / "port" / "walk_window.map")
    if not pathlib.Path(mapfile).exists():
        sys.exit("no map at %s; build walk_window first or pass --map"
                 % mapfile)
    sources = list(args.sources)
    for s in args.slice:
        sources += slice_lines(s)
    if not sources:
        sys.exit("nothing to measure: pass TU paths or --slice")

    if args.out:
        report(*measure(root, mapfile, sources, args.out,
                        args.extra_flags))
    else:
        with tempfile.TemporaryDirectory() as td:
            report(*measure(root, mapfile, sources, td, args.extra_flags))


if __name__ == "__main__":
    main()
