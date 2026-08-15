"""w13-adjudicate: pair each debt symbol's src body with its ROM disassembly.

For one symbol (or the whole debt queue) prints the src/ body, then the ROM
body disassembled out of extracted/overlays/overlay_NNNN.bin at the
overlays.yaml base, with branch targets and pc-relative literals resolved to
names. That pairing is the evidence a per-body ruling rests on.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import w13_rom
import w13_syms
import inferred_stub_guard as guard

ROOT = w13_rom.ROOT

SYM_RE = re.compile(r"^func_ov([0-9]{3})_([0-9a-fA-F]{8})$")


def sym_map():
    """debt symbol -> repo-relative src path."""
    port_dir = os.path.join(ROOT, "port")
    out = {}
    for rel, ab in guard.build_tu_set(port_dir, ROOT):
        if not rel.startswith("src/"):
            continue
        with open(ab, encoding="utf-8", errors="replace") as fh:
            txt = fh.read()
        if guard.MARKER not in txt:
            continue
        s = guard.marker_symbol(ab)
        if s:
            out[s] = rel
    return out


def literal_targets(data, addr, ovid):
    """pc-relative literal loads -> (insn addr, literal addr, word, name)."""
    hits = []
    for a, raw, mn, ops in w13_rom.disasm(data, addr):
        m = re.match(r"^(\w+), \[pc, #(?:0x)?([0-9a-fA-F]+)\]$", ops)
        if not (mn.startswith("ldr") and m):
            continue
        delta = int(m.group(2), 16) if m.group(2).lower().startswith(("a", "b", "c", "d", "e", "f")) or "0x" in ops else int(m.group(2))
        lit = a + 8 + delta
        off = lit - addr
        if 0 <= off + 4 <= len(data):
            word = int.from_bytes(data[off:off + 4], "little")
            hits.append((a, lit, word, w13_syms.resolve(word, ovid)))
    return hits


def dump(symbol, src_rel):
    m = SYM_RE.match(symbol)
    if not m:
        print("!! cannot parse {}".format(symbol))
        return
    ovid = int(m.group(1), 10)
    addr = int(m.group(2), 16)

    print("=" * 78)
    print("SYMBOL  {}".format(symbol))
    print("SRC     {}".format(src_rel))
    print("=" * 78)
    print("--- src body ---")
    with open(os.path.join(ROOT, src_rel), encoding="utf-8", errors="replace") as fh:
        print(fh.read().rstrip())

    print("--- ROM body (ov{:03d} @ {:#x}) ---".format(ovid, addr))
    try:
        data, base, how = w13_rom.body_bytes(ovid, addr)
    except Exception as exc:
        print("!! UNREADABLE: {}".format(exc))
        return
    print("# base={:#x} len={:#x} end-by={}".format(base, len(data), how))
    lines = w13_rom.disasm(data, addr)
    for a, raw, mn, ops, note in w13_syms.annotate(lines, ovid):
        print("{:08X}  {:<8}  {:<8} {}{}".format(a, raw, mn, ops, note))
    lits = literal_targets(data, addr, ovid)
    if lits:
        print("# literals:")
        for a, lit, word, name in lits:
            print("#   @{:08X} pool {:08X} = {:08X}{}".format(
                a, lit, word, "  <{}>".format(name) if name else ""))


def main(argv):
    m = sym_map()
    want = argv[1:] or guard.read_debt()
    for s in want:
        dump(s, m.get(s, "?"))
        print()


if __name__ == "__main__":
    main(sys.argv)
