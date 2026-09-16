"""Headless, silent boot sweep, same knobs for every binary, so builds compare.
usage: python bootab.py <walk_window.exe> <outdir> [frames] [budget_s] [levels=2,4,5 | scenes=4,5 | levels=all | scenes=all]
The SM64DS_TEST_LOCK* variables are passed through (every other inherited SM64DS_* is dropped),
so exporting SM64DS_TEST_LOCK=1 SM64DS_TEST_LOCK_PATH=C:/tmp/sm64ds-test-slot/slot.lock
SM64DS_TEST_LOCK_TIMEOUT=10800 before running makes every row take the machine-wide test slot.
Levels: SM64DS_LEVEL=<id> SM64DS_WINDOW_SELFTEST=<frames> (the battery's own level path)
Scenes: SM64DS_SCENE=<id> SM64DS_SCENE_FRAMES=<frames>
Always: SM64DS_FAULTS_FATAL=1 SM64DS_NO_FOCUS=1 SM64DS_VOLUME=0, no SCENE_WINDOW,
CREATE_NO_WINDOW + SW_SHOWMINNOACTIVE, every inherited SM64DS_* dropped.
Failing rows keep crash.txt + exit.txt + stdout tail under <outdir>/<kind><id>/.
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
    if kind == "SM64DS_LEVEL": env["SM64DS_WINDOW_SELFTEST"] = FRAMES
    else: env["SM64DS_SCENE_FRAMES"] = FRAMES
    env["SM64DS_FAULTS_FATAL"] = "1"; env["SM64DS_NO_FOCUS"] = "1"; env["SM64DS_VOLUME"] = "0"
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
print("SUMMARY exe=%s levels %d/%d scenes %d/%d" % (EXE, sum(r[2] == "PASS" for r in lv), len(lv), sum(r[2] == "PASS" for r in sc), len(sc)), flush=True)
