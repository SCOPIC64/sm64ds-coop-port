#!/usr/bin/env python3
"""The port's full verification battery, one command.

Everything the merge gate runs, in order, stopping at the first failure:

  1. build            port/build-port.cmd (32-bit MSVC, ninja)
  2. smoke suite      every smoke_*.exe in build/port, exit 0 each
  3. level selftests  walk_window.exe, SM64DS_WINDOW_SELFTEST=300 and
                      SM64DS_FAULTS_FATAL=1, on every mounted level -- the ids
                      are read out of port_level_table[] in hal/level_boot.cpp
                      at run time, so a new mount is covered the moment it
                      lands and no list here can go stale. A level whose
                      blocker belongs to another lane runs with that lane's
                      class skipped, named in LEVEL_SKIPS below and re-probed
                      bare on every run so the skip cannot outlive the bug
  4. linkage          port/tools/linkage.py -- the linked count is printed and
                      compared against --linked-floor if given (a merge must
                      never lower it)
  5. ptr_audit        port/tools/ptr_audit.py -- unhosted code pointers must
                      stay at zero

    python port/tools/battery.py [repo-root] [--linked-floor N] [--skip-build]

Exit 0 all green, 1 first red, with a one-line verdict per step so a log tail
reads as a checklist.

THE SELFTEST BMP IS ONLY BYTE-IDENTICAL AT AN EQUAL .dsstate SECTION BASE.

Read this before treating a walk_window_selftest.bmp diff as a rendering
regression. The rendered frame carries a dependence on the ABSOLUTE ADDRESSES
of the hosted DS globals, so it moves when the .dsstate section base moves --
and that base moves whenever a change pushes the preceding sections across a
4 KB page boundary, however unrelated the change is to rendering.

Controlled experiment (2026-08-14, walk_window.exe, SM64DS_WINDOW_SELFTEST=300,
the .dsstate span as printed by tools/dsstate_guard.py at link time). Every one
of these runs produced the IDENTICAL player position
pos=(-4915200, 2805556, 9342995):

    base source                     .dsstate 0x9ee000   md5 eb32dcab4915...
    base + 16 bytes of inert .bss   .dsstate 0x9ee000   md5 eb32dcab4915...
    base + 4 KB of inert .data      .dsstate 0x9ef000   md5 518ba22ae260...
    the drag-resize change          .dsstate 0x9ef000   md5 518ba22ae260...
    base + 8 KB of inert .data      .dsstate 0x9f0000   md5 15fde8a893d0...

Base logic and a real change produce the SAME BMP at the same section base, and
base logic alone produces THREE DIFFERENT BMPs at three different bases. The
delta is 3 pixels of water blue in the 512x384 frame, max channel delta 12.

So: a PR whose selftest positions match to the digit but whose BMP differs, and
whose dsstate_guard line reports a different section base than the baseline
build, is NOT a render regression. Rebuild the baseline with inert padding that
lands .dsstate at the same base and compare there; that is the comparison with
meaning. Reproduce the padding with, at file scope in any hal TU:

    extern "C" const char pad[0x1000];
    const char pad[0x1000] = {1};

EQUAL .dsstate BASE IS NECESSARY, NOT SUFFICIENT. A later review measured the
sharper form with controls: an inert shift INTERIOR to .dsstate (the section
base unmoved, only its contents and size changing) leaves the BMP byte-
identical, while host-global layout OUTSIDE .dsstate perturbs the frame even
at an equal base. In that case a touch-hosting PR's BMP delta -- 318 pixels,
max channel delta 13 -- was reproduced exactly by keeping the PR's probe code
and REMOVING the change under test, which is what proved the delta belonged to
the probe's own footprint and not to the fix. The general method: hold
everything but the change under test constant and see whether the BMP follows
the change or follows the footprint.

The address dependence itself is real and unexplained -- something in the
render path decides on a pointer value or reads an uninitialised field. It is
tracked separately; this note exists so the gate is not read as the bug.

A SELFTEST DOES NOT ALWAYS END ON THE LEVEL IT STARTED.

Every selftest log carries TWO [census] blocks -- one after boot and one at the
end of the run -- and the census reports the live actor set at print time, so
on a level that stays put the two are identical. Level 1 prints
"82 spawned (22 classes), 0 skipped" twice.

Levels 19, 20, 26, 34, 35, 39 and 49 warp within the 300 frames. Their logs carry a
"[lvl] change: level N -> M" line, and the second census is then M's, not N's:
level 26 warps to 1 and its second block is level 1's 82/22 exactly, level 39
warps to 5 and drops from 96 spawned to 64. (26 and 39 confirmed on this tree
2026-08-15; the rest are as reported by the mount lanes.)

Level 34 joined that list the day it was mounted, and it is the clearest case
of why the rule is the rule rather than a curiosity: rainbow_mario is Wing
Mario Over the Rainbow, a stage made of platforms over nothing, so the
selftest's idle player walks off and the game sends him to the castle grounds
-- "[lvl] change: level 34 -> 1, entrance 10, reason 0" -- exactly as it
should. Its second census is level 1's.

The battery only reads exit codes and does not care. Anything that reads a
census OFF one of these logs must take the FIRST [census] block, before the
first "[lvl] change:" line -- taking the last one silently files the
destination level's actors under the level that was asked for. Two blocks by
itself is not the warp signal; a "[lvl] change:" line is.
"""

import os
import re
import subprocess
import sys

SELFTEST_FRAMES = "300"
STEP_TIMEOUT = 600

TABLE_OPEN = "static const PortLevelDesc port_level_table[] = {"

# A MOUNTED LEVEL WHOSE BLOCKER IS NOT THE MOUNT, AND THE CLASS THAT BLOCKS IT.
#
# level id -> (SM64DS_SKIP_CLASS value, who owns the fix, what it looks like)
#
# The alternative to this table is worse in both directions. Leave the level
# out of the table in level_boot.cpp and a proven mount goes unshipped because
# somebody else's actor is broken; leave it in and run it bare and the battery
# is red on every lane until that actor is fixed. So the mount lands, the
# battery covers it, and the one class that is not the mount's responsibility
# is named here in the open.
#
# A SKIP HERE IS A DEBT, AND THE BATTERY COLLECTS IT. Every entry is re-probed
# BARE on every run (retire_probe below). The moment the owning lane's fix
# lands, the bare run goes green and the battery says so in capitals, so the
# skip cannot quietly become permanent -- which is the only way a mechanism
# like this stays honest. Deleting a retired entry is the whole maintenance
# burden.
LEVEL_SKIPS = {
    33: ("SNUFIT",
         "the actors lane",
         "SNUFIT (actor id 236) faults in RENDER. Measured on THIS tree, not "
         "quoted: rc 139, FAULT c0000005 at +0x0003c6ec accessing 0, which "
         "faultmap.py resolves to Model::Virtual10+0xc, walker actor id 0xec = "
         "236 = SNUFIT. Wave 8 and lane w21 read the same fault at a different "
         "offset because the binary moved, so the class is long unfixed rather "
         "than newly broken. Level 33's mount itself is proven: with the class "
         "skipped it runs 300 frames clean, census 72 spawned over 17 classes "
         "with 4 SNUFIT declined."),
}
# The bare re-probe is expected to FAULT while the debt stands, and a fault
# under FAULTS_FATAL exits fast. A probe that instead hangs is not evidence of
# anything, so it gets a short leash and is read as "still needed".
RETIRE_PROBE_TIMEOUT = 120


def mounted_levels(root):
    """Every mounted level id, read out of port_level_table[] at run time.

    This list used to be a literal here, and a literal is how the battery
    silently under-tests: run linkw wave 8 mounted sixteen levels and the tuple
    sat at nineteen ids for the rest of the wave, so the shipped battery
    covered 19 of 35 mounts and every lane that ran it read a green it had not
    earned. Deriving it is the only form that cannot go stale.

    A parse failure is fatal on purpose. Falling back to a built-in list would
    reintroduce exactly the bug this replaced -- a battery that keeps printing
    greens while testing a set nobody has checked in months.
    """
    path = os.path.join(root, "port", "hal", "level_boot.cpp")
    with open(path, encoding="utf-8", errors="replace") as f:
        text = f.read()

    i = text.find(TABLE_OPEN)
    if i < 0:
        raise SystemExit(f"battery: no port_level_table[] in {path}")
    i += len(TABLE_OPEN)
    j = text.find("\n};", i)
    if j < 0:
        raise SystemExit(f"battery: unterminated port_level_table[] in {path}")

    # The table carries per-wave comment blocks that quote ROM tables and row
    # fields, so strip comments before matching or a quoted id becomes a level.
    body = re.sub(r"/\*.*?\*/", " ", text[i:j], flags=re.S)
    body = re.sub(r"//[^\n]*", " ", body)

    # A row is {<id>, "<name>", "ov0NN", 0x0......., ...}; the name string is
    # what keeps the pattern off any other brace in the span.
    ids = [int(n) for n in re.findall(r"\{\s*(\d+)\s*,\s*\"", body)]
    if not ids:
        raise SystemExit(f"battery: port_level_table[] parsed empty in {path}")

    dupes = sorted({n for n in ids if ids.count(n) > 1})
    if dupes:
        # port_level_desc_for() returns the first match, so a duplicate id is a
        # dead row, not a harmless one.
        raise SystemExit(f"battery: duplicate level ids in port_level_table[]:"
                         f" {dupes}")

    return tuple(sorted(ids))


def run(cmd, cwd, env=None, timeout=STEP_TIMEOUT):
    return subprocess.run(cmd, cwd=cwd, env=env, timeout=timeout,
                          capture_output=True, text=True)


def selftest_env(lvl, skip=None):
    env = dict(os.environ,
               SM64DS_LEVEL=str(lvl),
               SM64DS_FAULTS_FATAL="1",
               SM64DS_WINDOW_SELFTEST=SELFTEST_FRAMES)
    if skip:
        env["SM64DS_SKIP_CLASS"] = skip
    else:
        # The battery's own environment must not decide what a level runs. An
        # SM64DS_SKIP_CLASS inherited from whoever invoked it would let a lane
        # skip its way to a green over levels this table says need nothing.
        env.pop("SM64DS_SKIP_CLASS", None)
    return env


def retire_probe(build, lvl):
    """Does level `lvl` still need its skip? (True still needed, False retired).

    Runs the level BARE. While the debt stands this faults under FAULTS_FATAL
    and returns quickly; when the owning lane's fix lands it returns 0 and the
    entry in LEVEL_SKIPS is dead weight.
    """
    try:
        r = run([os.path.join(build, "walk_window.exe")], build,
                env=selftest_env(lvl), timeout=RETIRE_PROBE_TIMEOUT)
    except subprocess.TimeoutExpired:
        return True, "the bare run did not finish inside %ds" % \
            RETIRE_PROBE_TIMEOUT
    if r.returncode:
        return True, "bare rc=%d" % r.returncode
    return False, "bare rc=0"


def main():
    args = [a for a in sys.argv[1:]]
    floor = 0
    if "--linked-floor" in args:
        i = args.index("--linked-floor")
        floor = int(args[i + 1])
        del args[i:i + 2]
    skip_build = "--skip-build" in args
    if skip_build:
        args.remove("--skip-build")
    root = os.path.abspath(args[0] if args else ".")
    build = os.path.join(root, "build", "port")

    if not skip_build:
        r = run(["cmd", "/c", os.path.join(root, "port", "build-port.cmd")],
                root)
        if r.returncode:
            print("build: FAIL")
            print(r.stdout[-2000:])
            print(r.stderr[-2000:])
            return 1
        print("build: ok")

    smokes = sorted(f for f in os.listdir(build)
                    if f.startswith("smoke") and f.endswith(".exe"))
    for exe in smokes:
        r = run([os.path.join(build, exe)], build)
        tail = (r.stdout.strip().splitlines() or [""])[-1][:90]
        if r.returncode:
            print(f"{exe}: FAIL rc={r.returncode} {tail}")
            return 1
        print(f"{exe}: ok  {tail}")

    # FAULTS_FATAL is not optional here. Without it a level can take an access
    # violation inside a quarantined actor, freeze that actor, keep ticking and
    # exit 0 -- so the battery passes a level that is visibly broken. Every
    # lane's own census runs set it; the battery did not, which was the same
    # class of unearned green as the hand-maintained level list it used to run.
    levels = mounted_levels(root)
    print(f"levels: {len(levels)} mounted, from hal/level_boot.cpp")

    # A skip for a level that is not mounted is the staleness bug that killed
    # the hand-maintained level list, in miniature: the entry reads as covered
    # and tests nothing. Refuse it rather than skip past it.
    orphans = sorted(set(LEVEL_SKIPS) - set(levels))
    if orphans:
        print(f"levels: FAIL, LEVEL_SKIPS names unmounted level(s) {orphans}")
        return 1

    retired = []
    for lvl in levels:
        skip = LEVEL_SKIPS.get(lvl)
        env = selftest_env(lvl, skip[0] if skip else None)
        r = run([os.path.join(build, "walk_window.exe")], build, env=env)
        if r.returncode:
            print(f"selftest level {lvl}: FAIL rc={r.returncode}"
                  + (f" (SM64DS_SKIP_CLASS={skip[0]})" if skip else ""))
            print(r.stdout[-1500:])
            return 1
        if not skip:
            print(f"selftest level {lvl}: ok")
            continue
        still, how = retire_probe(build, lvl)
        print(f"selftest level {lvl}: ok with SM64DS_SKIP_CLASS={skip[0]}"
              f", owned by {skip[1]} ({how})")
        if not still:
            retired.append(lvl)

    for lvl in retired:
        skip = LEVEL_SKIPS[lvl]
        print(f"SKIP RETIRED: level {lvl} now runs 300 frames clean BARE. "
              f"{skip[0]} is fixed, so delete level {lvl} from LEVEL_SKIPS in "
              f"port/tools/battery.py -- the level is being tested with a "
              f"class switched off for no reason.")

    r = run([sys.executable, os.path.join(root, "port", "tools", "linkage.py"),
             root], root)
    m = re.search(r"linked into walk_window\s*:\s*(\d+)\s*\(([\d.]+)%\)",
                  r.stdout)
    if not m:
        print("linkage: FAIL (no linked count in output)")
        return 1
    linked = int(m.group(1))
    print(f"linkage: {linked} ({m.group(2)}%)")
    if floor and linked < floor:
        print(f"linkage: FAIL, below the floor of {floor}")
        return 1

    r = run([sys.executable, os.path.join(root, "port", "tools", "ptr_audit.py")],
            root)
    m = re.search(r"^(\d+) carry code pointers AND no host TU names them",
                  r.stdout, re.M)
    if not m:
        print("ptr_audit: FAIL (no verdict line)")
        return 1
    if int(m.group(1)) != 0:
        print(f"ptr_audit: FAIL, {m.group(1)} unhosted code pointers")
        return 1
    print("ptr_audit: 0 unhosted code pointers")

    # gate.py tails this output, so the debt is restated where a tail will see
    # it rather than only next to the level it belongs to.
    if LEVEL_SKIPS:
        print("skips: " + ", ".join(
            f"level {lvl} without {LEVEL_SKIPS[lvl][0]} ({LEVEL_SKIPS[lvl][1]})"
            for lvl in sorted(LEVEL_SKIPS)))
    if retired:
        print("skips: RETIRED and removable -- " +
              ", ".join(f"level {lvl}" for lvl in retired))

    print("battery: ALL GREEN")
    return 0


if __name__ == "__main__":
    sys.exit(main())
