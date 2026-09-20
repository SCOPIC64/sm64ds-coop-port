#!/usr/bin/env python3
"""Shared ROM-CLEAN blob plumbing for romdata.py and ovdata.py.

ROM-CLEAN (2026-08-10): walk_window.exe must embed ZERO Nintendo ROM bytes. The
generated C files (romdata.c, ov*_data.c, ov*_syms.c) declare every ROM-derived
array ZEROED and gain a per-file `port_<tag>_apply(const u8 *blob)` that memcpys
the real bytes in at boot. The bytes travel in one runtime file,
build/assets/romdata.bin, produced from the user ROM at first run and from the
repo's extracted/ at dev-build time -- BY THE SAME emitter code, so the runtime
bytes are byte-identical to the old baked-in build by construction.

Each emitter writes a per-file PART: <out>.blobpart (raw bytes, this file's
symbols concatenated in emission order) + <out>.blobmap (JSON: the ordered
(name, size) list and the emitter tag). romblob.py runs last (like ovdata's
--cross pass), concatenates every part into build/assets/romdata.bin, writes
build/assets/romdata.manifest, and generates romdata_dispatch.c, whose
port_romdata_load_all(blob) calls each part's apply with that part's base.

Why parts + a final consolidation rather than one blob per emitter: every
emitter is its own CMake custom command and cannot see the others, exactly like
the ovdata --cross situation; and the launcher must ship ONE runtime file, not
forty.
"""

import hashlib
import json
import pathlib


def init_body(raw):
    """The C brace body for one array: real bytes when baked, zero when clean.

    A single mode switch, `--rom-clean` on the emitter's argv, drives the whole
    pipeline: without it the arrays keep their ROM bytes exactly as before (dev
    default, every smoke harness untouched); with it they zero and the bytes go
    to the blob for the shipped exe. Kept here so romdata.py and ovdata.py agree.
    """
    return "0" if ROM_CLEAN else ",".join(str(b) for b in raw)


# Set once by each emitter's arg parse from "--rom-clean" on argv. Default OFF:
# a plain `python tools/romdata.py out.c` still bakes the bytes in, so nothing
# that links the generated C without loading a blob breaks.
ROM_CLEAN = False


def set_rom_clean(argv):
    global ROM_CLEAN
    ROM_CLEAN = "--rom-clean" in argv
    return ROM_CLEAN


def emit_apply(tag, entries):
    """C lines for `void port_<tag>_apply(const unsigned char *blob)`.

    entries: ordered [(symbol_name, raw_bytes)]. `blob` is the base of THIS
    part inside the consolidated romdata.bin (romblob.py adds the part base), so
    offsets here are part-relative and start at 0.
    """
    if not ROM_CLEAN:
        return []            # baked build: the arrays already hold their bytes
    lines = [f"void port_{tag}_apply(const unsigned char *blob)", "{"]
    cur = 0
    for name, raw in entries:
        lines.append(f"    memcpy((unsigned char *){name}, blob + {cur}, "
                     f"{len(raw)});")
        cur += len(raw)
    lines.append("}")
    return lines


def write_part(out_path, tag, entries):
    """Write <out>.blobpart + <out>.blobmap for one emitter's ordered entries.

    Returns the part's total byte length, or 0 in a baked build (no part). A
    baked build removes any stale part so a later --cross/romblob pass cannot
    pick up bytes the current .c no longer zeroes.
    """
    part_p = pathlib.Path(str(out_path) + ".blobpart")
    map_p = pathlib.Path(str(out_path) + ".blobmap")
    if not ROM_CLEAN:
        for p in (part_p, map_p):
            if p.exists():
                p.unlink()
        return 0
    payload = b"".join(raw for _, raw in entries)
    part_p.write_bytes(payload)
    doc = {
        "tag": tag,
        "sha256": hashlib.sha256(payload).hexdigest(),
        "syms": [[name, len(raw), hashlib.sha256(raw).hexdigest()]
                 for name, raw in entries],
    }
    map_p.write_text(json.dumps(doc, indent=1), encoding="ascii")
    return len(payload)
