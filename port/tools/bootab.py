"""Headless, silent boot sweep, same knobs for every binary, so builds compare.
usage: python bootab.py <walk_window.exe> <outdir> [frames] [budget_s]
       [levels=2,4,5 | scenes=4,5 | levels=all | scenes=all] [idle=1] [aspect=<ratio>]
The SM64DS_TEST_LOCK* variables are passed through (every other inherited SM64DS_* is dropped),
so exporting SM64DS_TEST_LOCK=1 SM64DS_TEST_LOCK_PATH=C:/tmp/sm64ds-test-slot/slot.lock
SM64DS_TEST_LOCK_TIMEOUT=10800 before running makes every row take the machine-wide test slot.
Levels: SM64DS_LEVEL=<id> SM64DS_WINDOW_SELFTEST=<frames> (the battery's own level path)
Scenes: SM64DS_SCENE=<id> SM64DS_SCENE_FRAMES=<frames>
Always: SM64DS_FAULTS_FATAL=1 SM64DS_NO_FOCUS=1 SM64DS_VOLUME=0, no SCENE_WINDOW,
idle=1 adds SM64DS_SELFTEST_IDLE=1 to the LEVEL rows, which walk_window.cpp uses to
leave dz at 0 so the selftest neither holds forward nor hops at frame 30: the level
boots and then sits still. Scene rows have no selftest and are unaffected.
aspect=<ratio> sets SM64DS_ASPECT to that ratio on EVERY row, level and scene alike,
so one sweep boots the whole table at one presentation width. It is the only way to
reach the wide path from here: the environment scrub above drops an inherited
SM64DS_ASPECT with the rest of the SM64DS_* block, so a sweep that did not name it
is always the native 4:3 picture. 0 (or no aspect= at all) is that native sentinel;
the game clamps the ratio itself (port/hal/host_settings.cpp aspect_sanitise), so a
value this tool passes through unchanged is still sanitised before it sizes a buffer.
CREATE_NO_WINDOW + SW_SHOWMINNOACTIVE, every inherited SM64DS_* dropped.
Failing rows keep crash.txt + exit.txt + stdout tail under <outdir>/<kind><id>/.

warpin=1 and reentry=1 change HOW a LEVEL row is entered, and nothing else. Every
other arm of this tool boots the level directly (SM64DS_LEVEL=<id>), which is not
how the game is played: a course is entered by a LEVEL CHANGE out of the castle,
and state that is right on a direct boot can be wrong after a change (wave 13's
texture, minimap and star-select bugs, and lane SEAT14B's Jolly Roger Bay fault,
were all of that family, invisible to every direct-boot gate).
  warpin=1  boots castle grounds (SM64DS_LEVEL=1) and warps into the row's level
            at frame 600 with SM64DS_WARP_SEQ, selftest 1500. Row 1 is entered
            from level 6 instead, since it cannot warp into itself.
  reentry=1 boots the row's level, warps OUT to castle grounds at frame 400 and
            back IN at frame 800 (SM64DS_WARP_SEQ=1@400,<id>@800), selftest 1400,
            which is the leave-and-come-back shape SEAT14B faulted on. Row 1
            leaves to level 6 and back.
Both drive the game's own LoadLevelNoReturn through walk_window.cpp's warp-seq
block, so the teardown, the level-change poll and the boot half all run for real.
press=<spec> is the scripted pad both arms hand to SM64DS_PROBE_INPUT, default
200:A: entering a COURSE from the castle takes the painting route, so
hal/level_change.cpp runs the star select inline between the teardown and the
boot half (SM64DS_STARSEL_PAINTING, default on), and without an A press nothing
chooses an act and the interlude only ends at its 1800-frame backstop. press=none
drops the variable, which is the straight-in A/B of the same row.
Their FRAMES (argv[3]) is fixed by the arm, so a warp row is the same length on
every binary; the SCENE rows of a sweep are untouched by both.
"""
import os, sys, time, shutil, subprocess
EXE = os.path.abspath(sys.argv[1]); OUT = os.path.abspath(sys.argv[2])
FRAMES = sys.argv[3] if len(sys.argv) > 3 else "600"
BUDGET = int(sys.argv[4]) if len(sys.argv) > 4 else 90
EXEDIR = os.path.dirname(EXE); ROOT = os.path.dirname(os.path.dirname(EXEDIR))
sys.path.insert(0, os.path.join(ROOT, "port", "tools"))
import battery as B
LEVELS = B.mounted_levels(ROOT); SCENES = B.hosted_scenes(ROOT)
os.makedirs(OUT, exist_ok=True)
SI = subprocess.STARTUPINFO(); SI.dwFlags |= subprocess.STARTF_USESHOWWINDOW; SI.wShowWindow = 7
NOCON = getattr(subprocess, "CREATE_NO_WINDOW", 0)
base_env = {k: v for k, v in os.environ.items() if not k.startswith("SM64DS_") or k.startswith("SM64DS_TEST_LOCK")}
FILTER = sys.argv[5] if len(sys.argv) > 5 else ""
IDLE = any(a == "idle=1" for a in sys.argv[5:])
ASPECT = ""
for a in sys.argv[5:]:
    if a.startswith("aspect="): ASPECT = a[7:]
WARPIN = any(a == "warpin=1" for a in sys.argv[5:])
REENTRY = any(a == "reentry=1" for a in sys.argv[5:])
PRESS = "200:A"
for a in sys.argv[5:]:
    if a.startswith("press="): PRESS = a[6:]
# the row filter is positional but the flags are not, so a run that passes only a
# flag must not have that flag read as a filter (it would then match no prefix and
# sweep everything by accident)
if FILTER in ("idle=1", "warpin=1", "reentry=1") or FILTER.startswith(("aspect=", "press=")): FILTER = ""
if FILTER.startswith("levels="):
    sel = FILTER[7:]; SCENES = ()
    if sel != "all": LEVELS = tuple(i for i in LEVELS if str(i) in sel.split(","))
elif FILTER.startswith("scenes="):
    sel = FILTER[7:]; LEVELS = ()
    if sel != "all": SCENES = tuple(i for i in SCENES if str(i) in sel.split(","))
ART = ("crash.txt", "exit.txt")
def clear():
    for a in ART:
        p = os.path.join(EXEDIR, a)
        if os.path.exists(p):
            try: os.remove(p)
            except OSError: pass
def run(kind, ident, label):
    clear()
    env = dict(base_env)
    env[kind] = str(ident)
    if kind == "SM64DS_LEVEL":
        env["SM64DS_WINDOW_SELFTEST"] = FRAMES
        if IDLE: env["SM64DS_SELFTEST_IDLE"] = "1"
        if WARPIN or REENTRY:
            # the row's level is REACHED, not booted: castle grounds first, then
            # the game's own LoadLevelNoReturn. Level 1 cannot warp into itself,
            # so it uses level 6 as the other end of the change.
            other = 6 if int(ident) == 1 else 1
            if WARPIN:
                env[kind] = str(other)
                env["SM64DS_WARP_SEQ"] = "%d@600" % int(ident)
                env["SM64DS_WINDOW_SELFTEST"] = "1500"
            else:
                env["SM64DS_WARP_SEQ"] = "%d@400,%d@800" % (other, int(ident))
                env["SM64DS_WINDOW_SELFTEST"] = "1400"
            if PRESS and PRESS != "none": env["SM64DS_PROBE_INPUT"] = PRESS
    else: env["SM64DS_SCENE_FRAMES"] = FRAMES
    env["SM64DS_FAULTS_FATAL"] = "1"; env["SM64DS_NO_FOCUS"] = "1"; env["SM64DS_VOLUME"] = "0"
    # set on level and scene rows alike: the aspect is latched at boot, before
    # either path picks its presentation, so both read the same key
    if ASPECT: env["SM64DS_ASPECT"] = ASPECT
    t0 = time.time(); out = ""; rc = "TIMEOUT"
    try:
        p = subprocess.run([EXE], cwd=EXEDIR, env=env, capture_output=True, text=True,
                           errors="replace", timeout=BUDGET, creationflags=NOCON, startupinfo=SI)
        rc = p.returncode; out = (p.stdout or "") + (p.stderr or "")
    except subprocess.TimeoutExpired as e:
        def s(v): return "" if v is None else (v if isinstance(v, str) else v.decode("utf-8", "replace"))
        out = s(e.stdout) + s(e.stderr)
    dt = time.time() - t0
    ok = rc == 0
    note = ""
    low = out.lower()
    for key in ("unhosted", "wrong bytes", "quarantine", "assert", "out of memory", "unhandled"):
        if key in low: note = key; break
    if rc == "TIMEOUT": note = (note + " " if note else "") + "timeout"
    elif isinstance(rc, int) and rc != 0: note = (note + " " if note else "") + "rc 0x%08x" % (rc & 0xFFFFFFFF)
    if not ok:
        d = os.path.join(OUT, "%s%d" % (label, ident)); os.makedirs(d, exist_ok=True)
        for a in ART:
            p = os.path.join(EXEDIR, a)
            if os.path.exists(p): shutil.copy2(p, d)
        with open(os.path.join(d, "stdout.txt"), "w", encoding="utf-8", errors="replace") as f: f.write(out)
        # one-line fault summary from the play log
        for line in out.splitlines():
            if line.startswith("FAULT") or "UNHOSTED" in line or "WRONG BYTES" in line:
                note += " | " + line.strip()[:160]; break
    return ok, rc, round(dt, 1), note
rows = []
plan = [("SM64DS_LEVEL", LEVELS, "level"), ("SM64DS_SCENE", SCENES, "scene")]
for kind, ids, label in plan:
    for i in ids:
        ok, rc, dt, note = run(kind, i, label)
        rows.append((label, i, "PASS" if ok else "FAIL", rc, dt, note))
        print("%-5s %-3d %-4s rc=%-12s %6.1fs %s" % (label, i, "PASS" if ok else "FAIL", rc, dt, note), flush=True)
with open(os.path.join(OUT, "sweep.tsv"), "w") as f:
    f.write("kind\tid\tverdict\trc\tseconds\tnote\n")
    for r in rows: f.write("\t".join(str(x) for x in r) + "\n")
lv = [r for r in rows if r[0] == "level"]; sc = [r for r in rows if r[0] == "scene"]
print("SUMMARY exe=%s%s%s%s levels %d/%d scenes %d/%d" % (EXE, " idle" if IDLE else "",
      (" warpin press=%s" % (PRESS or "none")) if WARPIN else ((" reentry press=%s" % (PRESS or "none")) if REENTRY else ""),
      " aspect=" + ASPECT if ASPECT else " aspect=native", sum(r[2] == "PASS" for r in lv), len(lv), sum(r[2] == "PASS" for r in sc), len(sc)), flush=True)
