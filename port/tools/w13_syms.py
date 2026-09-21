"""w13-adjudicate: whole-image symbol resolver.

Builds one addr -> name map across main/itcm/dtcm and every overlay so a
disassembled `bl #0x...` reads as a name instead of a number. Overlays overlap
in address space, so a lookup takes the overlay being disassembled first and
falls back to main/itcm/dtcm, which is how the loaded image actually resolves.
"""

import functools
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CFG = os.path.join(ROOT, "config", "arm9")

SYM_RE = re.compile(r"^(\S+)\s+(.*)$")


def _load(path):
    out = {}
    if not os.path.isfile(path):
        return out
    with open(path, encoding="utf-8", errors="replace") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            m = SYM_RE.match(line)
            if not m:
                continue
            am = re.search(r"addr:(0x[0-9a-fA-F]+)", m.group(2))
            if am:
                out.setdefault(int(am.group(1), 16), m.group(1))
    return out


@functools.lru_cache(maxsize=None)
def main_syms():
    d = {}
    for rel in ("symbols.txt", "itcm/symbols.txt", "dtcm/symbols.txt"):
        d.update(_load(os.path.join(CFG, rel)))
    return d


@functools.lru_cache(maxsize=None)
def ov_syms(ovid):
    return _load(os.path.join(CFG, "overlays", "ov{:03d}".format(ovid),
                              "symbols.txt"))


def resolve(addr, ovid=None):
    """Name at addr as the loaded image would see it, or None."""
    if ovid is not None:
        n = ov_syms(ovid).get(addr)
        if n:
            return n
    return main_syms().get(addr)


def annotate(lines, ovid):
    """Attach a resolved name to every branch target in a disasm listing."""
    out = []
    for a, raw, mn, ops in lines:
        note = ""
        m = re.match(r"^#(0x[0-9a-fA-F]+)$", ops.strip())
        if m and mn.startswith(("bl", "b")):
            n = resolve(int(m.group(1), 16), ovid)
            if n:
                note = "   <{}>".format(n)
        out.append((a, raw, mn, ops, note))
    return out
