#!/usr/bin/env python
"""Yoshi's egg lay, driven headless, with a verdict.

WHAT IT PROVES. Yoshi tongues a Goomba on Bob-omb Battlefield, swallows it, lays
an egg, and walks away. Six assertions, all of which the pre-fix binary fails:

  1. the Player reaches St_Swallow          (state 0x020d666c / 0x020d6474)
  2. NOTHING is quarantined                 (a faulting actor is a frozen actor)
  3. a YOSHI_EGG actor exists and TICKS     (the egg was really laid)
  4. the Player LEAVES St_Swallow           (he is not stuck in the lay)
  5. the Player is still MOVING at the end  (he is not frozen in place)
  6. the YOSHI_EGG actor is DRAWN           (a [actor] Render line with a real
                                             model file and non-zero transforms)

Assertion 5 is the one that speaks to the "yoshi freezes" half of the report.
Assertion 6 is the one that speaks to the "doesn't show up" half: assertion 3
only counts the egg's Behavior ticks, which an egg that is never drawn still
produces, so a driver that steps over the frame-0xa mouth transfer (see below)
can tick an egg forever without ever putting a picture of it on screen.
Assertions 3, 4 and 6 are the ones that stop a "no longer hangs" non-fix from
passing: an egg that is never laid, is never drawn, or a Yoshi who never
returns to walking, is not a fix.

WHY THERE IS A DRIVER AT ALL. One env knob, off by default, in the host test
layer, not touching game logic:

  SM64DS_SELFTEST_TONGUE_ONCE  press B once instead of every 40 frames. A second
                               B while an enemy is in the mouth is the SPIT
                               (St_YoshiPower_Main case 4), and the swallow
                               hand-off needs the 0x5a-frame lockout at
                               Player+0x6c6 to run down first -- 90 frames,
                               longer than the 40-frame period -- so the
                               repeating press can never reach a swallow.

This tool used to also carry SM64DS_YOSHI_SWALLOW, which made by hand the two
writes St_YoshiPower_Main case 1 makes at body-anim frame 0xa (unk_0b0 |=
0x40000, &= ~0x20000: the enemy moves from the tongue to the mouth). That is
precisely the step a played swallow does not reach, so the knob made the proof
step over the real failure and report PASS on a binary where the egg never
shows up. The knob is gone. What replaces it is arming the tongue target
earlier: SM64DS_YOSHI_EGG_REPRO now reads 213, not 200. The eat animation's
start frame comes from data_ov002_020ff0f8, indexed by the tongue animation's
frame at the moment of the grab; those ROM bytes are 11, 10, 9, 8, 7, and index
0 -- the frame-200 grab -- is the only entry that lands past the frame-0xa
mouth transfer, so a grab that early can never hand the enemy to the mouth. A
grab at f213 lands on tongue-animation frame 3, whose table entry is 8, which
is before the transfer, so the real transfer fires and the whole swallow runs
on the game's own frames with no driver standing in for it at all.

RECIPE (reproducible from a clean tree):

    python tools/asset_catalog.py generate "<path to the .nds>"
    cmd /c port\\build-port.cmd
    python -u port/tools/yoshi_egg_proof.py

Exit 0 = all six hold. Exit 1 = at least one fails, with the reason printed.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
EXE = os.path.join(REPO, "build", "port", "walk_window.exe")

# The Player's swallow state, both halves, as the state trace spells them.
ST_SWALLOW = ("020d666c", "020d6474")

ENV = {
    # BoB, as Yoshi, headless, 600 frames.
    "SM64DS_CHARACTER": "3",
    "SM64DS_LEVEL": "6",
    "SM64DS_WINDOW_SELFTEST": "600",
    # one tongue flick at f210, then leave him alone
    "SM64DS_SELFTEST_TONGUE": "1",
    "SM64DS_SELFTEST_TONGUE_ONCE": "1",
    # keep the tongue pointed at the first live Goomba (id 200) from f213, so
    # the grab lands on tongue-animation frame 3 (table entry 8, before the
    # frame-0xa mouth transfer) instead of frame 0 (table entry 11, after it)
    "SM64DS_YOSHI_EGG_REPRO": "213",
    "SM64DS_YOSHI_EGG_CLASS": "200",
    "SM64DS_YOSHI_EGG_WIN": "400",
    # readers
    "SM64DS_TRACE_STATE": "2",
    "SM64DS_RS_PROBE": "1",
    "SM64DS_ACTOR_PROBE": "1",
    # quiet spawner: no window, no sound, never activated
    "SM64DS_NO_FOCUS": "1",
    "SM64DS_VOLUME": "0",
    # other lanes run concurrently
    "SM64DS_TEST_LOCK": "1",
    "SM64DS_TEST_LOCK_PATH": r"C:\tmp\sm64ds-test-slot\slot.lock",
    "SM64DS_TEST_LOCK_TIMEOUT": "5400",
}

# The player's per-frame line: [f317] pos=(-3695.0,0.0,4030.1) ... st=020d666c ...
FRAME = re.compile(r"^\[f(\d+)\] pos=\(([-\d.]+),([-\d.]+),([-\d.]+)\).*?st=([0-9a-f]{8})")

# The egg's RENDER face, not its Behavior face: [actor] YOSHI_EGG model 3002432C
# file 301774CC transforms 3017749C mat.t (-509,3,545) scene
DRAWN = re.compile(r"^\[actor\] YOSHI_EGG\s+model (\S+) file (\S+) transforms (\S+)")


def run(log_path):
    env = dict(os.environ)
    env.update(ENV)
    SI = subprocess.STARTUPINFO()
    SI.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    SI.wShowWindow = 7  # SW_SHOWMINNOACTIVE
    NOCON = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    with open(log_path, "wb") as fh:
        rc = subprocess.call([EXE], cwd=REPO, env=env, stdout=fh,
                             stderr=subprocess.STDOUT,
                             creationflags=NOCON, startupinfo=SI)
    with open(log_path, "r", errors="ignore") as fh:
        return rc, fh.read()


def check(text, rc):
    fails = []

    frames = []
    for line in text.splitlines():
        m = FRAME.match(line)
        if m:
            frames.append((int(m.group(1)),
                           (float(m.group(2)), float(m.group(3)), float(m.group(4))),
                           m.group(5)))

    # 1. reached the swallow
    sw = [f for f in frames if f[2] in ST_SWALLOW]
    if not sw:
        fails.append("1 REACH: the Player never entered St_Swallow -- the run "
                     "did not exercise the egg lay at all, so it proves nothing")
    else:
        print("  1 REACH   ok: St_Swallow from f%d to f%d (%d frames)"
              % (sw[0][0], sw[-1][0], len(sw)))

    # 2. nothing quarantined
    q = [l for l in text.splitlines() if "[quarantine]" in l]
    if q:
        fails.append("2 FAULT: %d quarantine(s): %s" % (len(q), q[0].strip()))
    else:
        print("  2 FAULT   ok: no actor was quarantined")

    # 3. the egg exists and ticks
    ticks = text.count("[rsprobe] YOSHI_EGG")
    if ticks < 30:
        fails.append("3 EGG: only %d YOSHI_EGG Behavior ticks -- the egg was "
                     "not laid, or did not survive" % ticks)
    else:
        print("  3 EGG     ok: %d YOSHI_EGG Behavior ticks" % ticks)

    # 4. left the swallow again
    if sw:
        after = [f for f in frames if f[0] > sw[-1][0] and f[2] not in ST_SWALLOW]
        if not after:
            fails.append("4 RELEASE: the Player never left St_Swallow")
        else:
            print("  4 RELEASE ok: left St_Swallow at f%d into state 0x%s"
                  % (after[0][0], after[0][2]))

    # 5. still moving at the end -- the actual "yoshi freezes" assertion
    tail = frames[-90:]
    if len(tail) < 30:
        fails.append("5 ALIVE: only %d frames of player trace" % len(tail))
    else:
        xs = set(round(f[1][0], 1) for f in tail)
        zs = set(round(f[1][2], 1) for f in tail)
        if len(xs) < 3 and len(zs) < 3:
            fails.append("5 ALIVE: the Player has not moved for the last %d "
                         "frames (stuck at %s) -- FROZEN"
                         % (len(tail), tail[-1][1]))
        else:
            print("  5 ALIVE   ok: %d distinct x / %d distinct z over the last "
                  "%d frames" % (len(xs), len(zs), len(tail)))

    # 6. drawn -- the egg's RENDER face, not just its Behavior face
    def _all_zero(s):
        return re.fullmatch(r"0+", s) is not None

    drawn_line = None
    drawn_match = None
    for line in text.splitlines():
        m = DRAWN.match(line)
        if m:
            drawn_line = line
            drawn_match = m
            break
    if not drawn_match or _all_zero(drawn_match.group(2)) or _all_zero(drawn_match.group(3)):
        fails.append("6 DRAWN: the egg ticked but was never drawn (no Render), "
                     "or its model file did not load")
    else:
        print("  6 DRAWN   ok: %s" % drawn_line.strip())

    if rc != 0:
        fails.append("0 EXIT: walk_window returned %d" % rc)
    return fails


def main():
    if not os.path.exists(EXE):
        print("no walk_window.exe -- build the port first")
        return 2
    log = os.path.join(REPO, "build", "port", "yoshi_egg_proof.log")
    print("yoshi_egg_proof: BoB, Yoshi, one tongue flick at f210, 600 frames")
    rc, text = run(log)
    fails = check(text, rc)
    print("  log: %s" % log)
    if fails:
        print("YOSHI EGG PROOF: FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print("YOSHI EGG PROOF: PASS (6/6)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
