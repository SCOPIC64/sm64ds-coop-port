"""Find romdata symbols that are secretly POINTER TABLES.

romdata.py emits the ROM's raw bytes for every symbol in its NAMED list. That
is right for constants and tables of numbers. It is a latent crash for any
symbol whose bytes include a RELOCATED word, because a relocated word holds a
DS address: config/arm9/**/relocs.txt lists `from:<addr> to:<addr>` for every
one of them. Emitted raw, a code pointer called through is a jump into
unmapped host memory; a data pointer read through is ROM-space garbage.

data_02099fb4 and data_02099fbc were exactly this -- two words each, called
as `data_02099fbc[axis](...)` by the particle billboard builder -- and they
crashed a play session ten thousand frames in. They are hosted properly in
hal/particle_vtable.cpp now. This is the sweep that says what else looks the
same.

    python port/tools/ptr_audit.py [repo-root]

"seated" means some host translation unit under port/ names the symbol, so
somebody has at least thought about it. romdata.py itself does not count --
being on the list that emits the raw bytes is the bug, not the fix.
"""
import bisect
import os
import re
import sys


def load_symbols(root):
    """{name: addr}, sorted [addr], and the set of addrs that are functions."""
    names, addrs, funcs = {}, [], set()
    for dirpath, _dirs, files in os.walk(os.path.join(root, "config", "arm9")):
        if "symbols.txt" not in files:
            continue
        with open(os.path.join(dirpath, "symbols.txt"), errors="replace") as f:
            for line in f:
                m = re.match(r"(\S+)\s+kind:(\S+?)(?:\(.*?\))?\s+addr:0x([0-9a-fA-F]+)",
                             line)
                if not m:
                    continue
                addr = int(m.group(3), 16)
                names.setdefault(m.group(1), addr)
                addrs.append(addr)
                if m.group(2) == "function":
                    funcs.add(addr)
    return names, sorted(set(addrs)), funcs


def load_relocs(root):
    """{from_addr: (kind, to_addr)}"""
    out = {}
    for dirpath, _dirs, files in os.walk(os.path.join(root, "config", "arm9")):
        if "relocs.txt" not in files:
            continue
        with open(os.path.join(dirpath, "relocs.txt"), errors="replace") as f:
            for line in f:
                m = re.match(r"from:0x([0-9a-fA-F]+)\s+kind:(\S+)\s+to:0x([0-9a-fA-F]+)",
                             line)
                if m:
                    out[int(m.group(1), 16)] = (m.group(2), int(m.group(3), 16))
    return out


def load_named(root):
    """Everything romdata.py emits ROM BYTES for: the NAMED list, with explicit
    sizes where given, plus the CONTIG address runs.

    The runs matter. romdata.py emits a run by walking config/arm9/symbols.txt
    between two addresses, so its members appear nowhere in NAMED -- and a
    sweep that only reads NAMED would call the whole camera-mode table clean
    without ever looking at it. It happens to BE clean (no relocation in
    0x02086fcc..0x020874f4), but that is a fact this tool should establish
    rather than assume."""
    path = os.path.join(root, "port", "tools", "romdata.py")
    with open(path, errors="replace") as f:
        src = f.read()
    out = []
    for m in re.finditer(r'"([A-Za-z_]\w*)(?::(0x[0-9a-fA-F]+))?"', src):
        out.append((m.group(1), int(m.group(2), 16) if m.group(2) else None))
    return out


def load_contig(root, names):
    """[(name, size)] for every symbol inside a CONTIG run of romdata.py."""
    path = os.path.join(root, "port", "tools", "romdata.py")
    with open(path, errors="replace") as f:
        src = f.read()
    body = re.search(r"^CONTIG = \[(.*?)^\]", src, re.S | re.M)
    if not body:
        return []
    out = []
    by_addr = sorted((a, n) for n, a in names.items())
    for m in re.finditer(r'\(\s*"[^"]*"\s*,\s*(0x[0-9a-fA-F]+)\s*,'
                         r'\s*(0x[0-9a-fA-F]+)\s*\)', body.group(1)):
        start, end = int(m.group(1), 16), int(m.group(2), 16)
        members = [(a, n) for a, n in by_addr if start <= a < end]
        for i, (a, n) in enumerate(members):
            nxt = members[i + 1][0] if i + 1 < len(members) else end
            out.append((n, nxt - a))
    return out


def host_text(root):
    """Every host TU under port/, minus romdata.py and the build tree."""
    buf = []
    for dirpath, dirs, files in os.walk(os.path.join(root, "port")):
        dirs[:] = [d for d in dirs if d != "build"]
        for fn in files:
            if fn.endswith((".cpp", ".c", ".h")):
                try:
                    with open(os.path.join(dirpath, fn), errors="replace") as f:
                        buf.append(f.read())
                except OSError:
                    pass
    return "\n".join(buf)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    names, addrs, funcs = load_symbols(root)
    relocs = load_relocs(root)
    reloc_srcs = sorted(relocs)
    seated = host_text(root)

    rows = []
    for name, size in load_named(root) + load_contig(root, names):
        base = names.get(name)
        if base is None:
            continue
        if size is None:                      # delta to the next symbol
            i = bisect.bisect_right(addrs, base)
            size = (addrs[i] - base) if i < len(addrs) else 0
        lo = bisect.bisect_left(reloc_srcs, base)
        hi = bisect.bisect_left(reloc_srcs, base + size)
        inside = reloc_srcs[lo:hi]
        if not inside:
            continue
        code = [s for s in inside if relocs[s][1] in funcs]
        rows.append((len(code), len(inside), name, base, size, code,
                     name in seated))

    rows.sort(reverse=True)
    print("%-24s %-11s %-7s %-7s %s" %
          ("symbol", "addr", "relocs", "->code", "seated by host TU"))
    for ncode, nall, name, base, size, code, is_seated in rows:
        print("%-24s 0x%08x %-7d %-7d %s" %
              (name, base, nall, ncode, "yes" if is_seated else "NO"))
        for s in code[:4]:
            print("      from:0x%08x to:0x%08x (function)" % (s, relocs[s][1]))
        if len(code) > 4:
            print("      ... %d more code pointers" % (len(code) - 4))

    risky = [r for r in rows if r[0] and not r[6]]
    print("\n%d named romdata symbols carry ROM relocations" % len(rows))
    print("%d carry pointers to CODE" % len([r for r in rows if r[0]]))
    print("%d carry code pointers AND no host TU names them" % len(risky))
    return 1 if risky else 0


if __name__ == "__main__":
    sys.exit(main())
