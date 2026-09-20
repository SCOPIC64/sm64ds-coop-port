"""w13-adjudicate: ROM body reader for the inferred-stub debt queue.

Reads the overlay image the port itself ships from (extracted/overlays/
overlay_NNNN.bin) at the base overlays.yaml gives for that overlay id, and
disassembles a function body starting at a RAM address.

Deliberately NOT a dsd export reader: the dsd arm9_overlays/ovNNN.bin exports
carry relocated words, so a word that reads as a live pointer there is a
rebased artifact, not what the ROM holds.

Function end is taken from config/arm9/overlays/ovNNN/symbols.txt when that
file gives a size, and otherwise from the next symbol boundary.
"""

import os
import re
import sys

from capstone import Cs, CS_ARCH_ARM, CS_MODE_ARM

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def overlay_bases():
    """id -> (base_address, code_size) parsed from the extracted overlays.yaml."""
    path = os.path.join(ROOT, "extracted", "dsd", "arm9_overlays", "overlays.yaml")
    bases = {}
    cur = None
    with open(path, encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            m = re.match(r"^-\s*id:\s*(\d+)$", line)
            if m:
                cur = int(m.group(1))
                bases[cur] = {}
                continue
            if cur is None:
                continue
            m = re.match(r"^(base_address|code_size|bss_size):\s*(\d+)$", line)
            if m:
                bases[cur][m.group(1)] = int(m.group(2))
    return bases


def overlay_image(ovid):
    """The raw overlay image the port ships from."""
    path = os.path.join(ROOT, "extracted", "overlays",
                        "overlay_{:04d}.bin".format(ovid))
    with open(path, "rb") as fh:
        return fh.read()


def read_symbols(ovid):
    """addr -> (name, size or None) from the delinker symbol table."""
    path = os.path.join(ROOT, "config", "arm9", "overlays",
                        "ov{:03d}".format(ovid), "symbols.txt")
    syms = {}
    if not os.path.isfile(path):
        return syms
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            m = re.match(r"^(\S+)\s+(.*)$", line)
            if not m:
                continue
            name = m.group(1)
            rest = m.group(2)
            am = re.search(r"addr:(0x[0-9a-fA-F]+)", rest)
            if not am:
                continue
            addr = int(am.group(1), 16)
            sm = re.search(r"size[=:](0x[0-9a-fA-F]+|\d+)", rest)
            size = int(sm.group(1), 0) if sm else None
            syms[addr] = (name, size)
    return syms


def body_bytes(ovid, addr, max_len=0x400):
    """The ROM bytes of the function at addr, and how the end was decided."""
    bases = overlay_bases()
    if ovid not in bases:
        raise KeyError("no overlay {} in overlays.yaml".format(ovid))
    base = bases[ovid]["base_address"]
    code_size = bases[ovid]["code_size"]
    img = overlay_image(ovid)
    off = addr - base
    if off < 0 or off >= code_size:
        raise ValueError(
            "addr {:#x} outside ov{:03d} [{:#x},{:#x})".format(
                addr, ovid, base, base + code_size))
    syms = read_symbols(ovid)
    how = "next-symbol"
    end = None
    if addr in syms and syms[addr][1]:
        end = off + syms[addr][1]
        how = "symbols.txt size"
    else:
        nxt = sorted(a for a in syms if a > addr)
        if nxt:
            end = off + (nxt[0] - addr)
    if end is None:
        end = off + max_len
        how = "max_len"
    end = min(end, code_size, len(img))
    return img[off:end], base, how


def disasm(data, addr):
    md = Cs(CS_ARCH_ARM, CS_MODE_ARM)
    md.detail = False
    out = []
    for insn in md.disasm(data, addr):
        out.append((insn.address, insn.bytes.hex(), insn.mnemonic, insn.op_str))
    return out


def show(ovid, addr, max_len=0x400):
    data, base, how = body_bytes(ovid, addr, max_len)
    print("# ov{:03d} base={:#x} addr={:#x} len={:#x} ({})".format(
        ovid, base, addr, len(data), how))
    for a, raw, mn, ops in disasm(data, addr):
        print("{:08X}  {:<8}  {:<8} {}".format(a, raw, mn, ops))
    tail = len(data) - 4 * len(disasm(data, addr))
    if tail > 0:
        print("# {} trailing byte(s) did not decode as ARM (literal pool?)".format(tail))


if __name__ == "__main__":
    ov = int(sys.argv[1], 0)
    ad = int(sys.argv[2], 0)
    ln = int(sys.argv[3], 0) if len(sys.argv) > 3 else 0x400
    show(ov, ad, ln)
