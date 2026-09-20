"""Headless, silent boot sweep, same knobs for every binary, so builds compare.
usage: python bootab.py <walk_window.exe> <outdir> [frames] [budget_s]
       [levels=2,4,5 | scenes=4,5 | levels=all | scenes=all] [idle=1] [aspect=<ratio>]
       [smooth=<0..3>]
The SM64DS_TEST_LOCK* variables are passed through (every other inherited SM64DS_* is dropped),
so exporting SM64DS_TEST_LOCK=1 SM64DS_TEST_LOCK_PATH=C:/tmp/sm64ds-test-slot/slot.lock
SM64DS_TEST_LOCK_TIMEOUT=10800 before running makes every row take the machine-wide test slot.
Levels: SM64DS_LEVEL=<id> SM64DS_WINDOW_SELFTEST=<frames> (the battery's own level path)
Scenes: SM64DS_SCENE=<id> SM64DS_SCENE_FRAMES=<frames>
Always: SM64DS_FAULTS_FATAL=1 SM64DS_NO_FOCUS=1 SM64DS_VOLUME=0, no SCENE_WINDOW,
idle=1 adds SM64DS_SELFTEST_IDLE=1 to the LEVEL rows, which walk_window.cpp uses to
leave dz at 0 so the selftest neither holds forward nor hops at frame 30: the level
boots and then sits still. Scene rows have no selftest and are unaffected.
smooth=<0..3> sets SM64DS_SMOOTH_MODELS on EVERY row, level and scene alike, which
is the "SmoothModels" setting (run hd1, lane MDL). Same shape and same reason as
aspect= below: the environment scrub drops an inherited one, so a sweep that does
not name it is byte-identical to one from before this argument existed.
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

exit=<kind> changes how a LEVEL row is LEFT, which warpin/reentry do not cover:
they prove a course's boot after a change, and every one of them ends with the
course still standing. Nothing else in this run has ever run a course's exit.
  exit=void   SM64DS_VOID_DROP=200 -- below the death plane, so the cartridge's
              own func_ov002_020c5d60 -> ST_DEAD_PIT -> HitDeathPlane -> the
              level change back to the hub with the death reason (2)
  exit=star   SM64DS_STAR_DROP=#0,200 plus A presses -- stand on the level's
              first PowerStar, let its collision run the star-get sequence and
              the course-clear exit (reason 1)
  exit=pause  START then the fourth pause button on the touch screen -- exit
              course (reason 0)
All three set selftest 1200 and SM64DS_EXIT_WATCH=1, and a row is FAIL unless
the run carries a "[lvl] change:" line WITH THE REASON THAT ARM ASKED FOR (void
2, star 1, pause 0), which is printed in the note. Both halves are needed: an
exit that quietly does nothing exits 0 with the level still up, and a selftest
that walks a course for 1200 frames falls off plenty of them on its own, so a
change with the wrong reason measured the fall and not the arm. Scene rows are
untouched.

entrance=<n|all> changes WHICH ENTRANCE RECORD a LEVEL row is entered at, which
no other arm of this tool has ever moved: every row above enters at record 0.
Until port/hal/level_boot.cpp was fixed (the entrance the change asked for, not
always record 0) the port could not enter a level at any other record at all, so
every mode in the cartridge's entry-mode table except the two that record 0
happens to carry is code no gate in this run has executed.
  entrance=<n>   every level row boots with SM64DS_ENTRANCE=<n>, i.e. record n
  entrance=all   each level row is EXPANDED into one row per entrance record the
                 cartridge gives that level, read here out of the ROM itself
SM64DS_ENTRANCE seats data_0209f268, the PENDING entrance, and Stage's own
`data_0209f264 = data_0209f268` then latches it, so a direct boot selects the
record exactly the way a level change does. Rows carry the record in their own
column and in their output directory name (level<id>_e<rec>), and a level row's
note carries the run's `selftest: N frames, pos=(x, y, z)` line, because an
entrance whose mode leaves the player stuck in his arrival animation exits 0 and
is still a finding: its position barely moves under the held-forward walk while
record 0's travels. Scene rows are untouched by the arm, and a sweep that does
not name it is byte-identical to one from before the arm existed.
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
SMOOTH = ""
for a in sys.argv[5:]:
    if a.startswith("smooth="): SMOOTH = a[7:]
WARPIN = any(a == "warpin=1" for a in sys.argv[5:])
REENTRY = any(a == "reentry=1" for a in sys.argv[5:])
PRESS = "200:A"
for a in sys.argv[5:]:
    if a.startswith("press="): PRESS = a[6:]
EXIT = ""
for a in sys.argv[5:]:
    if a.startswith("exit="): EXIT = a[5:]
ENTRANCE = ""
for a in sys.argv[5:]:
    if a.startswith("entrance="): ENTRANCE = a[9:]
# the row filter is positional but the flags are not, so a run that passes only a
# flag must not have that flag read as a filter (it would then match no prefix and
# sweep everything by accident)
if FILTER in ("idle=1", "warpin=1", "reentry=1") or FILTER.startswith(("aspect=", "press=", "exit=", "entrance=", "smooth=")): FILTER = ""
if FILTER.startswith("levels="):
    sel = FILTER[7:]; SCENES = ()
    if sel != "all": LEVELS = tuple(i for i in LEVELS if str(i) in sel.split(","))
elif FILTER.startswith("scenes="):
    sel = FILTER[7:]; LEVELS = ()
    if sel != "all": SCENES = tuple(i for i in SCENES if str(i) in sel.split(","))
def entrance_counts(root):
    """How many entrance records each level has, read off the cartridge.

    The walk is the ROM's own and the same one port/tools/ov_places.py uses for
    object tables: data_020758c8[level] is the level's object overlay id,
    data_02092208[level] its LVL_Overlay; the LVL_Overlay's misc table is at +4
    and its sub-table array at +0x10 with the count at +0x14; every 8-byte
    sub-table entry is {descriptor, count, pad2, records} and the descriptor's
    low five bits are the sub-loader index, of which 1 is LOADER_ENTRANCE.
    """
    import re, struct, pathlib
    rt = pathlib.Path(root)
    rng = {}
    for p in sorted((rt / "config/arm9/overlays").glob("ov*/delinks.txt")):
        ov = int(re.search(r"ov(\d+)", str(p)).group(1))
        t = p.read_text()
        s = [int(x, 16) for x in re.findall(r"start:(0x[0-9a-fA-F]+)", t)]
        rng[ov] = min(s)
    arm9 = (rt / "extracted/arm9_dec.bin").read_bytes()
    a32 = lambda x: struct.unpack_from("<I", arm9, x - 0x02004000)[0]
    out = {}
    for lvl in range(52):
        ovid = a32(0x020758C8 + lvl * 4)
        lo = a32(0x02092208 + lvl * 4)
        f = rt / ("extracted/overlays/overlay_%04d.bin" % ovid)
        if ovid not in rng or not f.exists():
            continue
        base = rng[ovid]; d = f.read_bytes()
        has = lambda a, n=1: base <= a and a + n <= base + len(d)
        u8 = lambda a: d[a - base]
        u16 = lambda a: struct.unpack_from("<H", d, a - base)[0]
        u32 = lambda a: struct.unpack_from("<I", d, a - base)[0]
        if not has(lo, 0x18):
            continue
        tables = [u32(lo + 4)]
        subs = u32(lo + 0x10); nsub = u8(lo + 0x14)
        if has(subs, nsub * 0xC):
            tables += [u32(subs + s * 0xC) for s in range(nsub)]
        n_ent = 0
        for t in tables:
            if not t or not has(t, 8):
                continue
            n = u16(t); ents = u32(t + 4)
            if not has(ents, n * 8):
                continue
            for j in range(n):
                e = ents + j * 8
                if (u8(e) & 0x1F) != 1:
                    continue
                cnt = u8(e + 1)
                if has(u32(e + 4), cnt * 0x10):
                    n_ent += cnt
        if n_ent:
            out[lvl] = n_ent
    return out


ENTCOUNT = entrance_counts(ROOT) if ENTRANCE == "all" else {}
ART = ("crash.txt", "exit.txt")
def clear():
    for a in ART:
        p = os.path.join(EXEDIR, a)
        if os.path.exists(p):
            try: os.remove(p)
            except OSError: pass
def run(kind, ident, label, ent=None):
    clear()
    env = dict(base_env)
    env[kind] = str(ident)
    if ent is not None:
        # the PENDING entrance; Stage's own latch copies it to the current one
        # and level_boot passes that to LoadClsnAndObjects as the record index
        env["SM64DS_ENTRANCE"] = str(ent)
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
        if EXIT:
            # LEAVING a course, which no other arm of this tool does. Each one
            # drives the cartridge's own exit and writes nothing but a position
            # or a pad word; walk_window.cpp's EXIT ARMS block is the whole of
            # them. The row is graded on the level change as well as the exit
            # code, because an exit that quietly does nothing leaves a live
            # level behind and exits 0.
            env["SM64DS_WINDOW_SELFTEST"] = "1200"
            env["SM64DS_EXIT_WATCH"] = "1"
            if EXIT == "void":
                # the death plane: drop below it at 200 and let
                # func_ov002_020c5d60 -> ST_DEAD_PIT -> HitDeathPlane run
                env["SM64DS_VOID_DROP"] = "200"
            elif EXIT == "star":
                # stand on the level's first PowerStar and press A through the
                # star-get prompts
                env["SM64DS_STAR_DROP"] = "#0,200"
                env["SM64DS_PROBE_INPUT"] = \
                    "260:A,300:A,340:A,380:A,420:A,460:A"
            elif EXIT == "pause":
                # START, then the fourth pause button (exit course). The four
                # buttons are touch boxes x 8..247, y 0x20/0x48/0x70/0x98 each
                # 0x20 tall; the tap has to carry a press EDGE, so it is a
                # short range, not a hold.
                #
                # SEVEN STARTS, NOT ONE, and that is the cartridge's doing.
                # Stage::PS_Update case 2 (src/_ZN5Stage9PS_UpdateEv.cpp:404)
                # forks on Player::CanPause (0x020bd828 through
                # src/func_02029408.c), which returns 0 while the player is
                # AIRBORNE, taking damage or under no control. A START pressed
                # mid-stride lands in pause sub-state 0xb, which has no menu
                # buttons at all, and the run then reads as "the pause menu
                # ignores every tap" -- measured on 14, 15, 28, 33, 34 and
                # thirteen more. Retrying every 60 frames finds a grounded one.
                env["SM64DS_PROBE_INPUT"] = ",".join(
                    "%d:START" % f for f in range(200, 620, 60))
                env["SM64DS_TOUCH_PROBE"] = ",".join(
                    "%d-%d:128:168" % (f, f + 1) for f in range(230, 650, 60))
            else:
                sys.exit("unknown exit=%s (void, star, pause)" % EXIT)
    else: env["SM64DS_SCENE_FRAMES"] = FRAMES
    env["SM64DS_FAULTS_FATAL"] = "1"; env["SM64DS_NO_FOCUS"] = "1"; env["SM64DS_VOLUME"] = "0"
    # set on level and scene rows alike: the aspect is latched at boot, before
    # either path picks its presentation, so both read the same key
    if ASPECT: env["SM64DS_ASPECT"] = ASPECT
    # smooth=N, the same shape and the same reason as aspect= above: the
    # environment scrub drops an inherited SM64DS_SMOOTH_MODELS with the rest
    # of the SM64DS_* block, so a sweep that does not name it is byte-identical
    # to one from before this argument existed, and naming it is the only way
    # to reach the model smoother from here.
    if SMOOTH: env["SM64DS_SMOOTH_MODELS"] = SMOOTH
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
    if EXIT and kind == "SM64DS_LEVEL":
        # an exit row that exits 0 but never left the level is a FAIL: the
        # whole point of the arm is the change, and a course that swallows it
        # leaves the player standing in a level he asked to leave.
        #
        # AND IT HAS TO BE THE RIGHT EXIT. data_0209f26c, the reason the change
        # carries, is 2 for a death, 1 for a course cleared and 0 for anything
        # else including exit-course. A selftest that walks a course for 1200
        # frames falls off plenty of them by itself, so a pause row that came
        # back with reason 2 measured a fall, not the pause menu -- eleven of
        # the first star sweep's twenty-six "passes" were that.
        want = {"void": 2, "star": 1, "pause": 0}.get(EXIT)
        chg = ""
        for line in out.splitlines():
            if line.startswith("[lvl] change:"): chg = line.strip(); break
        if not chg:
            ok = False
        elif want is not None and ("reason %d" % want) not in chg:
            ok = False
            chg += "  (WRONG EXIT: wanted reason %d)" % want
        note = (note + " | " if note else "") + (chg or "NO LEVEL CHANGE")
    if ent is not None and kind == "SM64DS_LEVEL":
        # how far the held-forward walk actually got. An entrance whose mode
        # wedges the player in his arrival animation exits 0 with a position
        # that barely left the record's own spawn point, which no return code
        # can show.
        for line in out.splitlines():
            if line.startswith("selftest:"):
                note = (note + " | " if note else "") + line.strip(); break
    if not ok:
        d = os.path.join(OUT, "%s%d%s" % (label, ident,
                                          "" if ent is None else "_e%d" % ent))
        os.makedirs(d, exist_ok=True)
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
        if ENTRANCE and kind == "SM64DS_LEVEL":
            recs = list(range(ENTCOUNT.get(i, 0))) if ENTRANCE == "all" else [int(ENTRANCE)]
        else:
            recs = [None]
        for ent in recs:
            ok, rc, dt, note = run(kind, i, label, ent)
            verdict = "PASS" if ok else "FAIL"
            if ENTRANCE:
                rows.append((label, i, "" if ent is None else ent, verdict, rc, dt, note))
            else:
                rows.append((label, i, verdict, rc, dt, note))
            print("%-5s %-3d %-4s %-4s rc=%-12s %6.1fs %s"
                  % (label, i, "" if ent is None else "e%d" % ent, verdict, rc, dt, note)
                  if ENTRANCE else
                  "%-5s %-3d %-4s rc=%-12s %6.1fs %s" % (label, i, verdict, rc, dt, note),
                  flush=True)
with open(os.path.join(OUT, "sweep.tsv"), "w") as f:
    f.write("kind\tid\tentrance\tverdict\trc\tseconds\tnote\n" if ENTRANCE
            else "kind\tid\tverdict\trc\tseconds\tnote\n")
    for r in rows: f.write("\t".join(str(x) for x in r) + "\n")
lv = [r for r in rows if r[0] == "level"]; sc = [r for r in rows if r[0] == "scene"]
V = 3 if ENTRANCE else 2   # the entrance arm puts the record between the id and the verdict
print("SUMMARY exe=%s%s%s%s%s levels %d/%d scenes %d/%d" % (EXE, " idle" if IDLE else "",
      (" warpin press=%s" % (PRESS or "none")) if WARPIN else ((" reentry press=%s" % (PRESS or "none")) if REENTRY else ""),
      " entrance=" + ENTRANCE if ENTRANCE else "",
      " aspect=" + ASPECT if ASPECT else " aspect=native", sum(r[V] == "PASS" for r in lv), len(lv), sum(r[V] == "PASS" for r in sc), len(sc)), flush=True)
