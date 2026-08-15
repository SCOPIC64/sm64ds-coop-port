#!/usr/bin/env python3
"""One command a lane runs before it writes a status line or claims a result.

THE FAILURE THIS EXISTS FOR IS "A GREEN THAT WAS NEVER EARNED".

Not a hypothetical. Run linkw wave 8 produced four of them in one session:

  * a thunk pair was reported wired when only one half had been checked
  * a symbol was claimed by two lanes and each read the other's link as its own
  * the battery's level list was a hand-maintained literal, so it tested 19 of
    the 35 mounted levels and every lane that ran it read a green over a set
    nobody had looked at in weeks
  * linkage.py answered "0 linked (0.0%)" for a build that did not exist,
    because a wrapper script returned success on a link failure and nothing
    downstream asked whether the map was real

and a fifth shape on top: a lane parked mid-gate holding a binary that was one
source row stale, then quoted numbers off it the next day.

Every one of those is the same bug. A measurement was taken off an artifact
nobody proved was the artifact under test. So this tool does not measure
anything until it has proved the build is this tree's, is current, and linked.

WHAT IT PROVES, IN ORDER, STOPPING AT THE FIRST REFUSAL:

  1. the build directory belongs to THIS checkout. CMakeCache.txt's
     CMAKE_HOME_DIRECTORY must be <root>/port. A build dir shared with, or
     inherited from, another worktree reads green while measuring that other
     tree, and nothing else in the pipeline notices.
  2. the build runs and exits 0.
  3. ninja agrees there is nothing left to do. This is the positive form of
     "the artifacts are current": a dry run that reports work remaining means
     the outputs on disk are NOT what the sources say they should be, whatever
     the build script's exit code claimed. An exit code is a promise; the dry
     run is a check.
  4. walk_window.exe and walk_window.map exist, and if this build relinked
     them their mtimes are after the build started. An incremental build that
     legitimately relinks nothing leaves older mtimes, so the mtime rule is
     applied only when the build log shows the link ran, and step 3 carries
     the freshness proof in the no-op case.
  5. the map is a real map, not a truncated one. linkage.map_symbols already
     refuses anything under 4096 bytes with a specific message; this calls it
     rather than growing a second, worse copy of that rule.

THEN it measures: battery verdict, linkage headline, delta against the lane's
recorded baseline. And it stamps a manifest (git SHA, dirty flag, exe SHA256,
map size, the numbers, the verdict, a UTC timestamp) so a number pasted into a
status file can be traced back to a specific binary.

    python port/tools/gate.py [--record] [--skip-battery] [repo-root]

  --record         write the current numbers as this lane's baseline and exit
                   after the run. A lane records once, at the commit it starts
                   from, then every later run reads as a delta against it.
  --skip-battery   measure without running the battery. The manifest records
                   battery "skipped" and the verdict becomes INCOMPLETE, never
                   GREEN. A gate that can be told to skip its checks and still
                   say GREEN is the bug this file was written against.

With no repo-root argument this gates the checkout the script itself lives in,
not the current directory. Same rule as linkage.py, for the same reason: a tool
rooted at CWD reads correct from the repo root and silently measures another
tree from anywhere else.

The manifest lands at build/port/gate_manifest.json and the baseline at
build/port/gate_baseline.json, both inside the gated tree, so they travel with
the build they describe and cannot be confused for another lane's.
"""

import datetime
import hashlib
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import linkage  # noqa: E402  (path set above so the sibling module resolves)

BUILD_TIMEOUT = 1800
BATTERY_TIMEOUT = 3600
MEASURE_TIMEOUT = 600


def default_root():
    """The checkout this script lives in: <root>/port/tools/gate.py."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(os.path.dirname(here))


def refuse(msg):
    print()
    print("gate: REFUSED -- %s" % msg)
    return 2


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def git(root, *args):
    """A git command's stdout, or None if git is unavailable or errors."""
    try:
        r = subprocess.run(("git", "-C", root) + args, capture_output=True,
                           text=True, timeout=60)
    except (OSError, subprocess.SubprocessError):
        return None
    return r.stdout.strip() if r.returncode == 0 else None


def cache_value(cachefile, key):
    """One CMakeCache.txt entry's value, or None."""
    try:
        with open(cachefile, errors="replace") as f:
            for line in f:
                name = line.split(":", 1)[0]
                if name == key and "=" in line:
                    return line.split("=", 1)[1].strip()
    except OSError:
        return None
    return None


def check_build_dir_is_ours(root, build):
    """Refuse a build directory configured for a different checkout.

    The wrong-tree trap: two worktrees, one build dir, and a binary that passes
    every check while being the other tree's. CMake records the source dir it
    was configured against, so the check is exact and costs nothing.
    """
    cachefile = os.path.join(build, "CMakeCache.txt")
    if not os.path.isfile(cachefile):
        return None  # a first build has no cache yet; configure will make one
    home = cache_value(cachefile, "CMAKE_HOME_DIRECTORY")
    if home is None:
        return "no CMAKE_HOME_DIRECTORY in %s" % cachefile
    want = os.path.normcase(os.path.abspath(os.path.join(root, "port")))
    got = os.path.normcase(os.path.abspath(home))
    if want != got:
        return ("%s was configured for %s, not %s. That build dir belongs to "
                "another checkout" % (cachefile, home,
                                      os.path.join(root, "port")))
    return None


def ninja_says_up_to_date(build):
    """(ok, detail) from a ninja dry run over the gated build directory.

    The build script's exit code is a promise. This is the check: if ninja
    still has work to do after a successful build, the outputs on disk are not
    what the sources say they should be, and no number taken off them is
    quotable.
    """
    make = cache_value(os.path.join(build, "CMakeCache.txt"),
                       "CMAKE_MAKE_PROGRAM")
    if not make or not os.path.isfile(make):
        return None, ("CMAKE_MAKE_PROGRAM missing or not on disk (%s), cannot "
                      "cross-check the build" % make)
    try:
        r = subprocess.run([make, "-C", build, "-n"], capture_output=True,
                           text=True, timeout=MEASURE_TIMEOUT)
    except (OSError, subprocess.SubprocessError) as e:
        return None, "ninja dry run did not run: %s" % e
    out = (r.stdout or "") + (r.stderr or "")
    if r.returncode != 0:
        return False, "ninja dry run exited %d: %s" % (r.returncode,
                                                       out.strip()[-400:])
    if "no work to do" in out:
        return True, "ninja: no work to do"
    pending = [ln for ln in out.splitlines() if ln.strip()][:6]
    return False, ("ninja still has work after a successful build:\n    "
                   + "\n    ".join(pending))


def run_build(root):
    """(returncode, combined output) for port/build-port.cmd on this tree."""
    script = os.path.join(root, "port", "build-port.cmd")
    if not os.path.isfile(script):
        return None, "no build script at %s" % script
    r = subprocess.run(["cmd", "/c", script], cwd=root, capture_output=True,
                       text=True, timeout=BUILD_TIMEOUT)
    return r.returncode, (r.stdout or "") + (r.stderr or "")


def run_battery(root):
    """(verdict, returncode, tail) for the battery over an already-built tree.

    --skip-build is deliberate: the gate has just built, and letting the
    battery build again would both double the wall clock and, worse, mean the
    artifacts the gate proved fresh are not the artifacts the battery tested.
    """
    r = subprocess.run([sys.executable,
                        os.path.join(root, "port", "tools", "battery.py"),
                        root, "--skip-build"],
                       cwd=root, capture_output=True, text=True,
                       timeout=BATTERY_TIMEOUT)
    out = (r.stdout or "") + (r.stderr or "")
    lines = [ln for ln in out.splitlines() if ln.strip()]
    if r.returncode == 0 and "battery: ALL GREEN" in out:
        verdict = "ALL GREEN"
    else:
        fails = [ln for ln in lines if "FAIL" in ln]
        verdict = "FAIL: " + (fails[0] if fails else "rc=%d" % r.returncode)
    return verdict, r.returncode, "\n".join(lines[-12:])


HEADLINE = re.compile(
    r"matched TUs in src/\s*:\s*(\d+).*?"
    r"linked into walk_window\s*:\s*(\d+)\s*\(([\d.]+)%\)", re.S)


def run_linkage(root):
    """(total, linked, pct, raw output) from linkage.py on this tree."""
    r = subprocess.run([sys.executable,
                        os.path.join(root, "port", "tools", "linkage.py"),
                        root],
                       cwd=root, capture_output=True, text=True,
                       timeout=MEASURE_TIMEOUT)
    out = (r.stdout or "") + (r.stderr or "")
    m = HEADLINE.search(out)
    if not m:
        return None, None, None, out
    return int(m.group(1)), int(m.group(2)), float(m.group(3)), out


def delta_line(label, now, was):
    if was is None:
        return "  %-10s %s   (no baseline)" % (label, now)
    d = now - was
    sign = "+" if d > 0 else ""
    return "  %-10s %s -> %s   (%s%s)" % (label, was, now, sign, d)


def main():
    args = list(sys.argv[1:])
    record = "--record" in args
    if record:
        args.remove("--record")
    skip_battery = "--skip-battery" in args
    if skip_battery:
        args.remove("--skip-battery")
    positional = [a for a in args if not a.startswith("--")]
    unknown = [a for a in args if a.startswith("--")]
    if unknown:
        return refuse("unknown flag(s): %s" % " ".join(unknown))

    root = os.path.abspath(positional[0] if positional else default_root())
    build = os.path.join(root, "build", "port")
    exe = os.path.join(build, "walk_window.exe")
    mapfile = os.path.join(build, "walk_window.map")
    manifest_path = os.path.join(build, "gate_manifest.json")
    baseline_path = os.path.join(build, "gate_baseline.json")

    print("gate root : %s" % root)
    print("build dir : %s" % build)
    print()

    # 1. the build dir belongs to this checkout
    bad = check_build_dir_is_ours(root, build)
    if bad:
        return refuse(bad)

    # 2. build
    started = datetime.datetime.now(datetime.timezone.utc).timestamp()
    print("building...")
    rc, out = run_build(root)
    if rc is None:
        return refuse(out)
    if rc != 0:
        print(out[-3000:])
        return refuse("port/build-port.cmd exited %d" % rc)
    print("build: ok (exit 0)")

    # the build may have reconfigured; re-check before trusting its outputs
    bad = check_build_dir_is_ours(root, build)
    if bad:
        return refuse(bad)

    # 3. ninja's own verdict on whether the outputs are current
    ok, detail = ninja_says_up_to_date(build)
    if ok is False:
        return refuse(detail)
    if ok is None:
        print("build check: SKIPPED -- %s" % detail)
    else:
        print("build check: %s" % detail)

    # 4. the artifacts exist, and are this build's if this build linked them
    for p in (exe, mapfile):
        if not os.path.isfile(p):
            return refuse("no %s after a successful build" % p)
    relinked = "walk_window.exe" in out and "Linking" in out
    exe_mtime = os.path.getmtime(exe)
    map_mtime = os.path.getmtime(mapfile)
    if relinked:
        stale = [p for p, t in ((exe, exe_mtime), (mapfile, map_mtime))
                 if t < started]
        if stale:
            return refuse("the build relinked walk_window but %s predates the "
                          "build start -- that artifact is not this build's"
                          % ", ".join(os.path.basename(p) for p in stale))
        print("artifacts: relinked by this build")
    else:
        print("artifacts: not relinked (ninja had nothing to do), current by "
              "the check above")

    # 5. the map is a map. linkage owns this rule; borrow it rather than
    #    reimplementing it slightly differently.
    map_size = os.path.getsize(mapfile)
    try:
        syms = linkage.map_symbols(mapfile)
    except SystemExit as e:
        return refuse(str(e))
    if not syms:
        return refuse("%s parsed to zero symbols at %d bytes -- that is not a "
                      "linked build" % (mapfile, map_size))
    exe_hash = sha256_of(exe)
    print("map       : %d bytes, %d symbol rows" % (map_size, len(syms)))
    print("exe sha256: %s" % exe_hash)
    print()

    # measure
    if skip_battery:
        battery_verdict, battery_rc, battery_tail = "skipped", None, ""
        print("battery   : SKIPPED by --skip-battery")
    else:
        print("battery   : running (build, smokes, every mounted level, "
              "linkage, ptr_audit)...")
        battery_verdict, battery_rc, battery_tail = run_battery(root)
        print("battery   : %s" % battery_verdict)
        if battery_rc:
            print(battery_tail)

    total, linked, pct, link_out = run_linkage(root)
    if linked is None:
        print(link_out[-2000:])
        return refuse("linkage.py printed no headline for this build")
    print()
    print("linkage   : %d / %d linked (%.1f%%), %d unlinked"
          % (linked, total, pct, total - linked))

    base = None
    if os.path.isfile(baseline_path):
        try:
            with open(baseline_path) as f:
                base = json.load(f)
        except (OSError, ValueError) as e:
            print("baseline  : unreadable (%s), treating as none" % e)
            base = None

    print()
    if base is None:
        print("delta     : no baseline recorded for this lane yet. Run "
              "`gate.py --record` to set one.")
    else:
        print("delta vs baseline recorded %s at %s%s:"
              % (base.get("utc", "?"), (base.get("git_sha") or "?")[:12],
                 " (dirty)" if base.get("git_dirty") else ""))
        print(delta_line("linked", linked, base.get("linked")))
        print(delta_line("total", total, base.get("total")))
        print(delta_line("unlinked", total - linked,
                         None if base.get("total") is None
                         else base["total"] - base["linked"]))
        if base.get("battery") and battery_verdict != "skipped":
            print("  battery    %s -> %s" % (base["battery"], battery_verdict))

    sha = git(root, "rev-parse", "HEAD")
    dirty = git(root, "status", "--porcelain")
    manifest = {
        "utc": datetime.datetime.now(datetime.timezone.utc)
                       .strftime("%Y-%m-%dT%H:%M:%SZ"),
        "root": root,
        "git_sha": sha,
        "git_branch": git(root, "rev-parse", "--abbrev-ref", "HEAD"),
        "git_dirty": bool(dirty) if dirty is not None else None,
        "exe": exe,
        "exe_sha256": exe_hash,
        "exe_size": os.path.getsize(exe),
        "map_size": map_size,
        "map_symbols": len(syms),
        "total": total,
        "linked": linked,
        "unlinked": total - linked,
        "pct": pct,
        "battery": battery_verdict,
        "build_relinked": bool(relinked),
    }
    # GREEN is reserved for a run that proved everything. A skipped battery is
    # an INCOMPLETE, never a green, or the flag becomes a way to launder one.
    if battery_verdict == "skipped":
        manifest["verdict"] = "INCOMPLETE (battery skipped)"
    elif battery_verdict != "ALL GREEN":
        manifest["verdict"] = "RED (%s)" % battery_verdict
    elif base is not None and linked < base.get("linked", 0):
        manifest["verdict"] = ("RED (linkage %d below the baseline's %d)"
                               % (linked, base["linked"]))
    else:
        manifest["verdict"] = "GREEN"

    with open(manifest_path, "w") as f:
        json.dump(manifest, f, indent=2, sort_keys=True)
        f.write("\n")

    print()
    print("manifest  : %s" % manifest_path)
    print(json.dumps(manifest, indent=2, sort_keys=True))

    if record:
        with open(baseline_path, "w") as f:
            json.dump(manifest, f, indent=2, sort_keys=True)
            f.write("\n")
        print()
        print("baseline  : recorded at %s" % baseline_path)

    print()
    print("gate: %s" % manifest["verdict"])
    return 0 if manifest["verdict"] == "GREEN" else 1


if __name__ == "__main__":
    sys.exit(main())
