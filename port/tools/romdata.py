#!/usr/bin/env python3
"""Emit the ROM data constants the gate-4b slice references.

These are read-only tables in the arm9 image (sine table, identity matrices,
texture-matrix constants). They are Nintendo's bytes, so they are never
committed: this reads the local decompressed arm9 (extracted/arm9_dec.bin,
flat image based at 0x02000000) and writes a C file under the build tree.
"""

import pathlib
import struct
import sys

# extracted/arm9_dec.bin: the DECOMPRESSED arm9, flat at base 0x02004000
# (verified by two code anchors: Copy36Bytes @0x0205a548, DecompressLZ16
# @0x0205a754). extracted/dsd/arm9/arm9.bin is the COMPRESSED payload
# (arm9.yaml: compressed true) and must not be read directly. Addresses at
# or past bss_start are runtime-initialized and belong in the HAL.
#
# BSS_START WAS 0x02094640 AND THAT WAS TOO LOW BY 0x69c0, which quietly
# pushed a whole run of file-backed .data into the HAL as zeroed storage.
# The ROM settles it twice over: extracted/dsd/arm9/arm9.yaml records
# bss_start 34162240 = 0x0209b000, and config/arm9/delinks.txt lays the
# sections out as .data 0x02086bc0..0x0209b000, .bss 0x0209b000..0x020aa420.
# The image itself reaches 0x020a0f78, so every address below 0x0209b000 has
# real bytes behind it. Two of the symbols the old boundary excluded had
# already been reconstructed by hand in port/hal/model_host.cpp, and the ROM
# bytes agree with both reconstructions exactly -- which is the check that
# the new boundary is the right one.
BASE = 0x02004000
BSS_START = 0x0209B000

# (symbol address, byte length, C element type). BSS addresses do not belong
# here -- they are runtime-initialized and live in the HAL as storage.
TABLES = [
    (0x02082128, 48, "int"),        # Model ctor default Matrix4x3
    (0x02082190, 48, "int"),        # matrix Render uses for a NULL argument
    (0x02082214, 0x4000, "short"),  # s16 trig table the material bind indexes
    (0x020755A0, 12, "char"),       # OBJ width table (shape x size)
    (0x020755AC, 12, "char"),       # OBJ height table
]


# Initialized arm9 data referenced by name: address + size resolve from
# config/arm9/symbols.txt (size = delta to the next symbol, the ovdata.py
# convention). Emitted as aligned byte arrays so any struct view works.
NAMED = [
    "data_0208a178", "data_0208c178",
    "data_0208e504", "data_0208e538", "data_0208e548", "data_0208e54c",
    "data_0208e55c", "data_0208e56c", "data_0208e57c", "data_0208e58c",
    "data_0208e59c", "data_0208e5b0", "data_0208e5c0", "data_0208e5d4",
    "data_0208e5e4", "data_0208e5f0", "data_0208e5fc", "data_0208e608",
    "data_0208e610", "data_0208e624", "data_0208e634", "data_0208e648",
    "data_02092120",
    "data_02082178", "data_02090e80", "data_020914a0",
    "data_02092584", "data_02092654", "data_02092668", "data_0208e500", "data_02086758", "data_02086a58", "data_0208e430", "data_02086b58",
    "data_0208e434", "data_0208e438", "data_0208e43c", "data_0208e440",
    "data_0208e444", "data_02092124", "data_02086384", "data_0208e448", "data_02086bc0", "data_02086bd8", "data_02086c20", "data_02086c38", "data_02086c50", "data_02086c60", "data_02086c78", "data_02086c80", "data_02086c88", "data_02086c90", "data_02086ca0", "data_02086ca8", "data_02086cb0", "data_02086cb8", "data_02086cc0", "data_02086cd0", "data_02086cd8", "data_02086ce8", "data_02086cf0", "data_02086cf8", "data_02086d00", "data_02086d08", "data_02086d18", "data_02086d20", "data_02086d38", "data_02086d40", "data_02086d60", "data_02086d68", "data_02086d80", "data_02086d88", "data_02086d98", "data_02086da0", "data_02086dc0", "data_02086dd8", "data_02086df0", "data_02086e08", "data_02086e20", "data_02086e30", "data_02086e38", "data_02088fb8", "data_020890a0", "data_02092118", "data_02092110", "data_020876e4", "data_0208eecc", "data_02092134",
    "data_0208e42c",
    "data_0208ee44",
    "data_0208ee8c",
    "data_0208eeac",
    "data_0208f074",
    "data_0208f174",
    # cstd::atan2's lookup table: atan(i/1024) in binangs, i = 0..0x400, so
    # 0..0x2000 (45 degrees) and the quadrant fixups do the rest. EVERY
    # heading in the game runs through it -- Vec3_HorzAngle, Vec3_VertAngle,
    # the camera's own rotation, the walk's slope math. Zeroed storage in the
    # HAL made atan2 answer 0 for every off-axis direction.
    "data_020994e0",
    "data_020756b0",   # D-pad direction -> binang table (Stage::CheckInput)
    # Per-character voice-id offset. Sound::PlayCharVoice is
    # Play(1, baseId + data_02075250[ch], v), and the bytes read 00/40/80/c0
    # -- the VOICE SEQARC is laid out in 0x40-entry blocks, one per
    # character, so zeroed storage would give every character Mario's voice.
    "data_02075250",
    "data_0209214c",   # per-mode button remap pointer table (CheckInput)
    # tier-2 state wave: fader/level/message tables
    "data_02086f20",
    "data_0208e420",
    "data_0208eeec",
    "data_0208eeee",
    "data_02086f14",
    "data_02086f2c",
    # death states: SetNextLevel's sublevel -> (next level, entrance) table
    "data_02075638",
    "data_02092664",   # Scene::SetSceneToSpawn's pending-scene ID
    "data_020889b0",   # func_0200ee68's demo/cutscene flag block (NoControl)
    "data_020755bc",   # character -> wipe type (StartExitCharacterWipe)
    # gate 13, the real Camera: the constant vectors the follow chain reads.
    # The mode-preset table itself is a CONTIG run, not a NAMED entry.
    "CAM_SPACE_CAM_POS_ASR_3",   # camera-space eye offset, Behavior's weather
    "data_02086efc",   # up vector {0, 0x1000, 0}    -- Render's LookAt_
    "data_02086f08",   # {0, 0, -0x1000}, the degenerate-look fallback
    "data_02086f38",   # per-level XZ bounds, low
    "data_02086f48",   # per-level XZ bounds, high
    "data_02086cc8",
    "data_02086d50",
    "data_02086e84",
    "data_02086f58",
    "data_02086e90", "data_02086e9c", "data_02086ea8", "data_02086eb4",
    "data_020874f4",   # first record past the mode table
    # gate 14, the level boot: the two kuppa scripts
    # ContinueKuppaScriptIfNecessary compares the pending pointer against.
    "data_02087c00", "data_02089608",
    # the Camera's SpawnInfo, for the entrance path's actor registry
    "Camera_SpawnInfo",
    # sublevel -> level-part table (GetLevelPart, the death-table index)
    "data_02075264",
    # The model-walk constants the old BSS boundary hid. data_02099f88 is the
    # DEFAULT SCALE VECTOR {0x1000, 0x1000, 0x1000} func_02044534 copies when
    # Render is given no scale argument; with it zeroed the billboard part
    # walk builds an all-zero 3x3 and every vertex of every billboard model
    # lands on the model's translation point.
    "data_02099f80", "data_02099f84",   # BCA keyframe weight tables
    "data_02099f88",                    # billboard/part-walk default scale
    "data_02099f94",                    # texture-matrix size table
    # THE STAGE'S OWN RENDER SCALE. Stage::RenderModel ends in
    # Model::Render(&data_020755d4) and Stage::RenderModelTransparent in the
    # same call through slot 5, so this Vector3 is the only scale the level
    # model is ever drawn with. {125.0, 125.0, 125.0} in Fix12i.
    "data_020755d4",
]


# Address RUNS whose symbols must stay adjacent in host memory because the
# ROM code walks ACROSS the symbol boundaries. func_0200cb58 sets the camera's
# mode pointer to `index * 0x28 + &data_02086fcc`, and Camera::Behavior /
# Camera::Render then compare that pointer against &data_0208733c and
# &data_0208738c -- symbols 15 and 18 records further along. Emitted as one
# array per symbol is byte-correct but places them at unrelated addresses, so
# every mode past the first reads garbage and no comparison can ever hold.
#
# MSVC grouped sections put them back together: the linker concatenates
# contributions to `.name$suffix` sorted by the full section name, so one
# $NNN per symbol in address order lays the run out in ROM order. Every delta
# in these runs is a multiple of 8, so align(8) packs with no interior
# padding. (romdata_contig_check() in the HAL asserts the result at runtime.)
CONTIG = [
    # the camera-mode preset table: 33 records of 0x28, 0x528 total
    ("cammod", 0x02086FCC, 0x020874F4),
]


def symbol_table(root):
    import re
    syms = []
    for line in (root / "config/arm9/symbols.txt").read_text().splitlines():
        m = re.search(r"^(\S+)\s+kind:\S+\s+addr:0x([0-9a-fA-F]+)", line)
        if m:
            syms.append((int(m.group(2), 16), m.group(1)))
    syms.sort()
    return syms


def contig_entries(syms):
    """[(tag, [(name, addr, size), ...])] for every run in CONTIG."""
    runs = []
    for tag, start, end in CONTIG:
        members = [(a, n) for a, n in syms if start <= a < end]
        out = []
        for i, (a, n) in enumerate(members):
            nxt = members[i + 1][0] if i + 1 < len(members) else end
            size = nxt - a
            if size % 8:
                sys.exit(f"{tag}: {n} is {size:#x} bytes, not a multiple of 8 "
                         "-- ordered sections would pad and break the run")
            out.append((n, a, size))
        if not out or out[0][1] != start:
            sys.exit(f"{tag}: no symbol at the run start {start:#x}")
        runs.append((tag, out))
    return runs


def named_entries(root, syms):
    addr_of = {n: a for a, n in syms}
    contig_names = {n for _, mem in contig_entries(syms) for n, _, _ in mem}
    out = []
    for name in NAMED:
        if name in contig_names:
            continue   # the run owns it
        a = addr_of[name]
        nxt = next((s for s, _ in syms if s > a), a + 4)
        out.append((name, a, max(4, nxt - a)))
    return out


def main():
    root = pathlib.Path(__file__).resolve().parents[2]
    img = root / "extracted/arm9_dec.bin"
    if not img.exists():
        sys.exit(f"missing {img} -- extract the ROM first")
    anchor = img.read_bytes()
    a = 0x0205A548 - BASE
    if anchor[a:a + 4] != b"\x0c\x10\xb0\xe8":
        sys.exit("arm9_dec.bin failed the Copy36Bytes anchor -- wrong image/base")
    for addr, length, _ in TABLES:
        if addr + length > BSS_START:
            sys.exit(f"{addr:#x}+{length:#x} crosses into BSS -- not file-backed")
    data = img.read_bytes()
    out_path = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else (
        root / "build/port/host-src/romdata.c")
    out_path.parent.mkdir(parents=True, exist_ok=True)

    lines = ["/* GENERATED by port/tools/romdata.py from extracted/arm9_dec.bin.",
             " * ROM constants stay out of git; regenerate locally. */", ""]
    for addr, length, ctype in TABLES:
        off = addr - BASE
        blob = data[off:off + length]
        if len(blob) != length:
            sys.exit(f"short read at {addr:#x}")
        width = 4 if ctype == "int" else 2
        fmt = "<i" if ctype == "int" else "<h"
        vals = [struct.unpack_from(fmt, blob, i)[0] for i in range(0, length, width)]
        name = f"data_{addr:08x}"
        body = ", ".join(str(v) for v in vals)
        lines.append(f"{ctype} {name}[{len(vals)}] = {{ {body} }};")
        lines.append("")
    syms = symbol_table(root)
    for name, addr, size in named_entries(root, syms):
        if addr + size > BSS_START:
            size = BSS_START - addr
        blob = data[addr - BASE:addr - BASE + size]
        body = ",".join(str(b) for b in blob)
        lines.append(f"__declspec(align(8)) unsigned char {name}[{size}] = "
                     f"{{ {body} }};")
    lines.append("")

    # Grouped-section runs (see CONTIG).
    for tag, members in contig_entries(syms):
        lines.append(f"/* run .{tag}: {members[0][1]:#010x} .. "
                     f"{members[-1][1] + members[-1][2]:#010x}, "
                     f"{len(members)} symbols, laid out in ROM order */")
        for i, (name, addr, size) in enumerate(members):
            sec = f".{tag}${i:04d}"
            lines.append(f'#pragma section("{sec}", read, write)')
            blob = data[addr - BASE:addr - BASE + size]
            body = ",".join(str(b) for b in blob)
            lines.append(f'__declspec(allocate("{sec}")) __declspec(align(8)) '
                         f"unsigned char {name}[{size}] = {{ {body} }};")
        first, last = members[0], members[-1]
        lines.append(f"unsigned {tag}_run_base = (unsigned)&{first[0]}[0];")
        lines.append(f"unsigned {tag}_run_end = (unsigned)&{last[0]}[0] + "
                     f"{last[2]};")
        lines.append(f"unsigned {tag}_run_span = "
                     f"{members[-1][1] + members[-1][2] - members[0][1]};")
    lines.append("")

    # The archive-mount table at data_0208ecf4: 13 entries of
    # {ptr, heap, u16 idBase, u16 idEnd, char *shortName, char *narcPath}.
    # Interior file IDs (>= 0x8000) resolve as narc[id - idBase]; the fs
    # seam consumes this host-shaped copy (see port/hal/fs.cpp).
    lines.append("struct port_arc_entry { unsigned short base, end;"
                 " const char *narc; };")
    ents = []
    off = 0x0208ECF4 - BASE
    for i in range(13):
        blob = data[off + i*0x14: off + (i+1)*0x14]
        base_id, end_id = struct.unpack_from("<HH", blob, 8)
        path_ptr = struct.unpack_from("<I", blob, 0x10)[0]
        p = data[path_ptr - BASE:path_ptr - BASE + 64].split(b"\0")[0]
        ents.append('    { %d, %d, "%s" },' % (base_id, end_id,
                                               p.decode("ascii").lstrip("/")))
    lines.append("struct port_arc_entry port_archive_map[13] = {")
    lines.extend(ents)
    lines.append("};")
    lines.append("")
    out_path.write_text("\n".join(lines), encoding="ascii")
    print(f"romdata -> {out_path}")


if __name__ == "__main__":
    main()
