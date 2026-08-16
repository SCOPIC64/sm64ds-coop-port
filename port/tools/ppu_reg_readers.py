"""Who reads the 2D display registers, and is that reader LINKED?

The audit in ntr/ppu_audit.cpp measures the register VALUES a run leaves behind.
That answers "is this gap live in the frame". It does not answer the other half:
whether some linked function is WAITING on a register the host never drives.
GXSTAT was found that way and not by looking at a frame, so this does the same
sweep over the 2D display block.

A src hit alone means nothing -- most of the tree is not linked into
walk_window. Every hit is checked against the link map, and only LINKED readers
are a finding.

    python port/tools/ppu_reg_readers.py [--map <walk_window.map>]
"""

import argparse
import os
import re
import sys

# The engine-A display block plus the two engines' 2D register files, by the
# spellings a decomp actually writes them in (with and without the leading zero,
# upper and lower case hex).
REGS = [
    (0x04000000, "DISPCNT_A"),
    (0x04000004, "DISPSTAT"),
    (0x04000006, "VCOUNT"),
    (0x04000040, "WIN0H_A"),
    (0x04000042, "WIN1H_A"),
    (0x04000044, "WIN0V_A"),
    (0x04000046, "WIN1V_A"),
    (0x04000048, "WININ_A"),
    (0x0400004A, "WINOUT_A"),
    (0x0400004C, "MOSAIC_A"),
    (0x04000050, "BLDCNT_A"),
    (0x04000052, "BLDALPHA_A"),
    (0x04000054, "BLDY_A"),
    (0x04000064, "DISPCAPCNT"),
    (0x0400006C, "MASTER_BRIGHT_A"),
    (0x04001000, "DISPCNT_B"),
    (0x04001040, "WIN0H_B"),
    (0x04001042, "WIN1H_B"),
    (0x04001044, "WIN0V_B"),
    (0x04001046, "WIN1V_B"),
    (0x04001048, "WININ_B"),
    (0x0400104A, "WINOUT_B"),
    (0x0400104C, "MOSAIC_B"),
    (0x04001050, "BLDCNT_B"),
    (0x04001052, "BLDALPHA_B"),
    (0x04001054, "BLDY_B"),
    (0x0400106C, "MASTER_BRIGHT_B"),
    (0x04000130, "KEYINPUT"),
    (0x04000136, "EXTKEYIN"),
    # The pad is two latches, not one. func_02013f4c ORs KEYINPUT with the
    # ARM7-posted word before inverting, because X, Y and the hinge are not
    # ARM9-visible. A sweep that watched only 0x04000130 would report half.
    (0x027FFFA8, "SHARED_PAD"),
    (0x040001C0, "SPICNT"),
    (0x040001C2, "SPIDATA"),
    (0x027FFFAA, "SHARED_TP_LO"),
    (0x027FFFAC, "SHARED_TP_HI"),
]


def spellings(addr):
    """Every literal a C file might write this address as."""
    out = set()
    for body in ("%x" % addr, "%X" % addr, "%08x" % addr, "%08X" % addr):
        out.add("0x" + body.lstrip("0") if body.lstrip("0") else "0x0")
        out.add("0x" + body)
    return out


def main():
    ap = argparse.ArgumentParser()
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.dirname(os.path.dirname(here))
    ap.add_argument("--map", default=os.path.join(root, "build", "port", "walk_window.map"))
    ap.add_argument("--src", default=os.path.join(root, "src"))
    args = ap.parse_args()

    if not os.path.exists(args.map):
        print("no link map at %s; build first" % args.map)
        return 2
    with open(args.map, "r", encoding="utf-8", errors="replace") as f:
        mapped = f.read()

    # One pass over src, one regex for every spelling of every register.
    pats = {}
    for addr, name in REGS:
        for s in spellings(addr):
            pats.setdefault(s.lower(), name)
    big = re.compile("|".join(re.escape(k) for k in sorted(pats, key=len, reverse=True)))

    hits = {}      # name -> {stem: count}
    for dirpath, _dirs, files in os.walk(args.src):
        for fn in files:
            if not fn.endswith((".c", ".cpp")):
                continue
            p = os.path.join(dirpath, fn)
            try:
                with open(p, "r", encoding="utf-8", errors="replace") as f:
                    body = f.read()
            except OSError:
                continue
            low = body.lower()
            if "0x4000" not in low and "0x0400" not in low and "0x27ff" not in low:
                continue
            stem = fn.rsplit(".", 1)[0]
            for m in set(big.findall(low)):
                nm = pats.get(m)
                if nm:
                    hits.setdefault(nm, {}).setdefault(stem, 0)
                    hits[nm][stem] += low.count(m)

    print("2D DISPLAY REGISTER READERS, src hits checked against the link map")
    print("Only LINKED rows can affect a run. A linked reader of a register the")
    print("host never drives is the GXSTAT shape and is what this hunts for.\n")
    print("%-18s %-6s %-6s %s" % ("register", "srcs", "linked", "linked readers"))
    any_linked = False
    for _addr, name in REGS:
        stems = hits.get(name, {})
        if not stems:
            continue
        linked = sorted(s for s in stems if ("\n" + s) in mapped or (" " + s) in mapped)
        if linked:
            any_linked = True
        print("%-18s %-6d %-6d %s" % (name, len(stems), len(linked),
                                      " ".join(linked[:6]) + (" ..." if len(linked) > 6 else "")))
    if not any_linked:
        print("\nno linked reader of any register in the list")
    return 0


if __name__ == "__main__":
    sys.exit(main())
