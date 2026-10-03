"""Quiet, bounded gameplay checks for a built co-op release candidate."""
import argparse
import json
import pathlib
import re
import struct
import subprocess
import sys

import mp2_proof as harness


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("exe", type=pathlib.Path)
    parser.add_argument("--case", choices=["grounds", "door", "locked_entrance", "entrance", "interior", "cycle", "course"])
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[2]
    out = root / "build" / "coop-stability-proof"
    cases = {
        "grounds": (300, {"SM64DS_LEVEL": "1", "SM64DS_DOOR_PROBE": "1",
                          "SM64DS_EXIT_PROBE": "1"}),
        "door": (480, {"SM64DS_LEVEL": "1", "SM64DS_DOOR_DROP": "#0,60",
                       "SM64DS_DOOR_PROBE": "1"}),
        "locked_entrance": (480, {"SM64DS_LEVEL": "1", "SM64DS_DOOR_DROP": "#1,60",
                           "SM64DS_DOOR_PROBE": "1"}),
        "entrance": (720, {"SM64DS_LEVEL": "1", "SM64DS_DOOR_DROP": "#1,300",
                           "SM64DS_KEY_SPAWN_AT": "30:7",
                           "SM64DS_DOOR_PROBE": "1"}),
        "interior": (480, {"SM64DS_LEVEL": "2", "SM64DS_DOOR_DROP": "#8,60",
                           "SM64DS_DOOR_PROBE": "1"}),
        "cycle": (720, {"SM64DS_LEVEL": "1", "SM64DS_WARP_SEQ": "2@120,1@300,2@480"}),
        "course": (360, {"SM64DS_LEVEL": "6"}),
    }
    results = {}
    for name, (frames, knobs) in cases.items():
        if args.case and args.case != name:
            continue
        directory = out / name
        (directory / "tmp").mkdir(parents=True, exist_ok=True)
        env = harness.env_base(str(root), str(directory), "coop_" + name)
        env.update(knobs)
        env["SM64DS_WINDOW_SELFTEST"] = str(frames)
        env["SM64DS_SELFTEST_IDLE"] = "1"
        if name in ("locked_entrance", "entrance", "interior"):
            env["SM64DS_PROBE_INPUT"] = ",".join(f"{f}:A" for f in range(30, frames, 12))
        env["SM64DS_SAVE_PATH"] = str(directory / "test.sav")
        log = directory / "run.log"
        bmp = directory / "walk_window_selftest.bmp"
        bmp.unlink(missing_ok=True)
        with log.open("w") as stream:
            try:
                result = subprocess.run([str(args.exe.resolve())], cwd=directory,
                    env=env, stdout=stream, stderr=subprocess.STDOUT,
                    startupinfo=harness.SI_MIN, creationflags=harness.NO_CONSOLE,
                    timeout=180)
                rc = result.returncode
            except subprocess.TimeoutExpired:
                rc = "timeout"
        text = log.read_text(errors="replace")
        colors = 0
        if bmp.exists():
            data = bmp.read_bytes()
            if len(data) > 54 and data[:2] == b"BM":
                offset = struct.unpack_from("<I", data, 10)[0]
                stride = struct.unpack_from("<H", data, 28)[0] // 8
                if stride in (3, 4):
                    colors = len({data[i:i+3] for i in range(offset, len(data)-3, stride)})
        opened = "[door-open]" in text
        passed = rc == 0 and colors > 32 and f"selftest: {frames} frames" in text
        if name in ("door", "entrance", "interior"):
            passed = passed and opened and "[lvl] change:" in text
        if name == "locked_entrance":
            passed = (passed and not opened and "node 88c4 message" in text
                      and "[lvl] change:" not in text
                      and bool(re.search(r"\[door\] f(?:[2-4]\d\d).*nocontrol 0", text)))
        if name == "cycle":
            passed = passed and text.count("[lvl] change:") >= 3
        results[name] = {"pass": passed, "exit_code": rc, "colors": colors,
                         "door_opened": opened, "log": str(log)}
        print(name, json.dumps(results[name]), flush=True)
    out.mkdir(parents=True, exist_ok=True)
    (out / (args.case or "all") / "result.json").parent.mkdir(exist_ok=True)
    (out / (args.case or "all") / "result.json").write_text(json.dumps(results, indent=2))
    return 0 if all(row["pass"] for row in results.values()) else 1


if __name__ == "__main__":
    sys.exit(main())
