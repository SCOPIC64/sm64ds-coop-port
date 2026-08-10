#!/usr/bin/env python3
"""First-run: produce build/assets/romdata.bin from a user's SM64DS ROM.

ROM-CLEAN (2026-08-10): walk_window.exe embeds no ROM bytes; it loads its ROM
tables from build/assets/romdata.bin at boot. On the DEV tree that file is a
build product (CMake runs the emitters + romblob against the repo's extracted/).
On a USER's machine the launcher runs THIS, once, against the user's own .nds:

    python tools/romextract.py <user.nds> [--root <dir>]

It reuses the exact byte-identical Python already in the tree, so the file it
writes is bit-for-bit the one the dev build ships:

  1. tools/unpack.py <rom>  -> extracted/arm9_dec.bin + extracted/overlays/*.bin
     (ndspy: BLZ-decompresses arm9, writes each overlay image).
  2. port/tools/romdata.py + ovdata.py --rom-clean, for the SAME overlay set the
     shipped walk_window links (read out of port/CMakeLists.txt so this can
     never drift from the link line) -> one .blobpart per emitter.
  3. port/tools/romblob.py -> build/assets/romdata.bin + .manifest.

The launcher points walk_window at <root> via SM64DS_ASSET_ROOT (the same seam
fs.cpp uses for the catalogs + asset tree), so romdata.bin lands beside the
files.tsv/handles.tsv catalogs the launcher already produces.

Dependency: Python 3 + ndspy (`pip install ndspy`) -- the same pair
tools/asset_catalog.py already needs. No dsd.exe. See
out/rom-clean/RECIPE.md for the launcher contract.
"""

import pathlib
import re
import subprocess
import sys


def cmake_ov_list(cml, var):
    """Every ovNNN named in `set(<var> ...)` / `list(APPEND <var> ...)`, in order."""
    got = []
    for m in re.finditer(rf'(?:set|list\(APPEND)\s*\(?\s*{var}\s+([^\)]*)\)', cml):
        got += re.findall(r'ov\d+', m.group(1))
    seen = set()
    return [o for o in got if not (o in seen or seen.add(o))]


def run(argv):
    r = subprocess.run(argv, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(r.stdout + r.stderr)
        sys.exit(f"romextract: step failed: {' '.join(str(a) for a in argv)}")
    return r


def main():
    args = sys.argv[1:]
    root = None
    if "--root" in args:
        i = args.index("--root")
        root = pathlib.Path(args[i + 1])
        del args[i:i + 2]
    if len(args) != 1:
        sys.exit(__doc__)
    rom = pathlib.Path(args[0])
    if not rom.is_file():
        sys.exit(f"romextract: ROM not found: {rom}")

    src_root = pathlib.Path(__file__).resolve().parents[2]   # the repo root
    root = root or src_root                                  # where to write build/
    tools = src_root / "port/tools"
    ports = src_root / "port"
    py = sys.executable

    # 1. Unpack the ROM -> extracted/arm9_dec.bin + extracted/overlays/*.bin.
    #    unpack.py always writes under <src_root>/extracted, which is where the
    #    emitters read from.
    print("romextract: unpacking ROM (ndspy)...")
    run([py, str(src_root / "tools/unpack.py"), str(rom)])

    host_src = root / "build/port/host-src"
    host_src.mkdir(parents=True, exist_ok=True)
    cml = (ports / "CMakeLists.txt").read_text()

    # 2. Run every emitter in --rom-clean mode into the build tree. The set is
    #    read from CMakeLists so it always matches the shipped link line.
    parts = []

    def emit(argv, out_c):
        run([py, str(tools / argv[0]), *argv[1:], "--rom-clean"])
        parts.append(f"{out_c}.blobmap")

    print("romextract: emitting ROM tables...")
    emit(["romdata.py", str(host_src / "romdata.c")], host_src / "romdata.c")
    emit(["ovdata.py", "ov002", str(host_src / "ov002_data.c"),
          "--from-list", str(ports / "ov002_syms.txt")], host_src / "ov002_data.c")
    for ov in cmake_ov_list(cml, "PORT_LEVEL_OVERLAYS"):
        emit(["ovdata.py", ov, str(host_src / f"{ov}_data.c"), "--whole"],
             host_src / f"{ov}_data.c")
    emit(["ovdata.py", "ov009", str(host_src / "ov009_syms.c"), "--from-list",
          str(ports / "ov009_syms.txt"), "--pack"], host_src / "ov009_syms.c")
    for ov in cmake_ov_list(cml, "PORT_ACTOR_OVERLAYS"):
        emit(["ovdata.py", ov, str(host_src / f"{ov}_syms.c"), "--from-list",
              str(ports / f"{ov}_syms.txt"), "--pack"], host_src / f"{ov}_syms.c")

    # 3. Consolidate -> build/assets/romdata.bin + manifest (+ a dispatch we do
    #    not need at runtime but romblob writes anyway).
    assets = root / "build/assets"
    assets.mkdir(parents=True, exist_ok=True)
    print("romextract: consolidating -> romdata.bin...")
    run([py, str(tools / "romblob.py"),
         str(assets / "romdata.bin"), str(assets / "romdata.manifest"),
         str(host_src / "romdata_dispatch.c"), *parts])

    print(f"romextract: done -> {assets / 'romdata.bin'}")


if __name__ == "__main__":
    main()
