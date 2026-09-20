#!/usr/bin/env python3
"""Build-time self-check: the ROM-CLEAN blob bytes == the baked-in bytes.

ROM-CLEAN emits two code paths from one source of truth (romblob_common.init_body
picks "0" vs the real bytes), so a drift between them would be a silent wrong-ROM
bug. This asserts they agree: for every symbol the manifest names, the bytes at
its blob offset must equal the bytes the SAME emitter prints in BAKED mode.

    python tools/romblob_verify.py <src_root> <build_root> <manifest> <bin>

It re-runs each emitter (romdata.py + the ov*_data / ov*_syms invocations) in
baked mode into a temp dir, parses the array initialisers, and compares. A
mismatch, a missing symbol, or a size disagreement exits non-zero and names the
offender -- the build fails loudly rather than shipping a blob the game will
read as garbage.
"""

import hashlib
import pathlib
import re
import struct
import subprocess
import sys
import tempfile

# The typed arrays romdata.py emits (everything else is `unsigned char[]`).
TYPED = {"int": "<i", "unsigned int": "<I", "short": "<h",
         "unsigned short": "<H", "char": "<b", "unsigned char": "<B"}


def parse_arrays(text):
    """{name: bytes} for every `... NAME[N] = { ... };` in a baked emitter C."""
    out = {}
    # typed romdata arrays with a SPACE-separated element list:
    # `int NAME[N] = { v, v, ... };` (romdata.py's TABLES use ", " joins).
    for m in re.finditer(
            r'^(int|unsigned int|short|unsigned short|char|unsigned char) '
            r'(\w+)\[\d+\] = \{ ([^}]*) \};', text, re.M):
        ctype, name, body = m.group(1), m.group(2), m.group(3)
        vals = [int(x) for x in body.split(",") if x.strip() != ""]
        out[name] = b"".join(struct.pack(TYPED[ctype], v) for v in vals)
    # byte arrays: `u8 NAME[N]` (ovdata) and `unsigned char NAME[N]` (romdata
    # NAMED/CONTIG), possibly __declspec/allocate/align/static prefixed. Bodies
    # are comma-joined ints (romdata uses "," not ", " here) or a `0` shorthand
    # meaning all-zero of the declared length. `unsigned char` typed rows above
    # re-parse to the same bytes, so an overwrite here is harmless.
    for m in re.finditer(
            r'\b(?:u8|unsigned char) (\w+)\[(\d+)\] = \{ ?([0-9,\s]*?) ?\};',
            text):
        name, n, body = m.group(1), int(m.group(2)), m.group(3)
        vals = [int(x) for x in body.split(",") if x.strip() != ""]
        out[name] = b"\x00" * n if (len(vals) == 1 and vals[0] == 0 and n > 1) \
            else bytes(vals)
    return out


def baked_bytes(src_root, tmp):
    """Run every emitter in BAKED mode; return {name: bytes} across all of them.

    The emitter argv mirrors CMakeLists (minus --rom-clean). Only the emitters
    that contribute ROM bytes are re-run; ov_cross has no data arrays.
    """
    tools = src_root / "port/tools"
    ports = src_root / "port"
    # each run is (argv-after-script, output C path). The output path is argv[0]
    # for romdata but argv[1] for ovdata, so track it explicitly.
    romdata_c = str(tmp / "romdata.c")
    ov002_c = str(tmp / "ov002_data.c")
    runs = [(["romdata.py", romdata_c], romdata_c)]
    runs.append((["ovdata.py", "ov002", ov002_c,
                  "--from-list", str(ports / "ov002_syms.txt")], ov002_c))

    # Level whole-image mounts + per-symbol packs come from the same overlay
    # lists CMake builds. Read them straight from CMakeLists so this check never
    # drifts from the real link line.
    cml = (ports / "CMakeLists.txt").read_text()

    def ov_list(var):
        # collect every `list(APPEND <var> ovNNN ...)` and `set(<var> ovNNN...)`
        got = []
        for m in re.finditer(rf'(?:set|list\(APPEND)\s*\(?\s*{var}\s+([^\)]*)\)',
                             cml):
            got += re.findall(r'ov\d+', m.group(1))
        # dedupe, keep order
        seen = set()
        return [o for o in got if not (o in seen or seen.add(o))]

    for ov in ov_list("PORT_LEVEL_OVERLAYS"):
        c = str(tmp / f"{ov}_data.c")
        runs.append((["ovdata.py", ov, c, "--whole"], c))
    ov009s = str(tmp / "ov009_syms.c")
    runs.append((["ovdata.py", "ov009", ov009s,
                  "--from-list", str(ports / "ov009_syms.txt"), "--pack"],
                 ov009s))
    for ov in ov_list("PORT_ACTOR_OVERLAYS"):
        c = str(tmp / f"{ov}_syms.c")
        runs.append((["ovdata.py", ov, c, "--from-list",
                      str(ports / f"{ov}_syms.txt"), "--pack"], c))

    arrays = {}
    for argv, out_c in runs:
        script = tools / argv[0]
        out = subprocess.run([sys.executable, str(script), *argv[1:]],
                             capture_output=True, text=True)
        if out.returncode != 0:
            sys.exit(f"romblob_verify: baked re-run failed: {' '.join(argv)}\n"
                     f"{out.stderr}")
        arrays.update(parse_arrays(pathlib.Path(out_c).read_text()))
    return arrays


def main():
    src_root = pathlib.Path(sys.argv[1])
    # sys.argv[2] (build_root) is unused directly; kept for CMake symmetry.
    manifest = pathlib.Path(sys.argv[3]).read_text().splitlines()
    blob = pathlib.Path(sys.argv[4]).read_bytes()

    with tempfile.TemporaryDirectory() as td:
        arrays = baked_bytes(src_root, pathlib.Path(td))

    bad = 0
    checked = 0
    for line in manifest:
        if not line.startswith("sym\t"):
            continue
        _, tag, name, off, size, ssha = line.split("\t")
        off, size = int(off), int(size)
        slice_ = blob[off:off + size]
        # whole-image parts name a <tag>_image symbol; its baked array is
        # image+bss but the blob slice is the image only. Compare on the slice
        # length, which is what the loader memcpys.
        ref = arrays.get(name)
        if ref is None:
            print(f"romblob_verify: {name} ({tag}) not found in baked output")
            bad += 1
            continue
        ref = ref[:size] if name.endswith("_image") else ref
        if len(ref) != size or ref != slice_:
            print(f"romblob_verify: MISMATCH {name} ({tag}): baked {len(ref)}B "
                  f"vs blob {size}B")
            bad += 1
        checked += 1

    if bad:
        sys.exit(f"romblob_verify: {bad} symbol(s) disagree -- the blob does "
                 "NOT match the baked bytes")
    print(f"romblob_verify: OK, {checked} symbols blob==baked "
          f"({len(blob)} bytes)")


if __name__ == "__main__":
    main()
