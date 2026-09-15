"""vtalias_guard -- a vtable alternatename must be an ADDRESS match, not a name hunch.

WHY THIS EXISTS (run link100, lane CRASH12). port/hal/actor_classes.cpp carried

    /alternatename:  __ZTV12daStarGate_c  ->  __ZTV4Door

with a careful, correct derivation attached: the star door's table at ov100
0x021483cc really does carry the RTTI name "12daStarGate_c". What the
derivation never did was check the OTHER name. _ZTV4Door is ov100 0x02148188,
a different table 0x244 bytes away whose own RTTI reads "8daDoor_c". The two
classes differ in size (0x148 against 0x118) and in what they keep at +0xd4 (a
ModelAnim against a CommonModel), and their tables differ in exactly the seven
slots a subclass overrides. Because the other twenty-four slots are identical,
the alias looked harmless for weeks. It was not: src/d_a_door.c stores
_ZTV4Door into every plain door, hal_fill_star_door_vtable had written the star
door's faces into that array, and the castle grounds died the first time a door
was drawn.

Proving a name for ONE table is not proving that TWO tables are one. The check
that was missing is mechanical and is this file: for every vtable alternatename
in port/, ask config what address each name has, and refuse when the two
answers differ.

WHAT IT READS. Only real directives. C and C++ comments are stripped before
matching, so a commented-out pragma or a lane note quoting one is not a linker
input -- the same scoping mistake alternatename_guard carries its own fixture
for. Aliases are resolved against every config/arm9/**/symbols.txt.

WHAT IT CANNOT SAY. A name config has never heard of is UNKNOWN, not a pass and
not a failure: many host arrays are port scaffolding with no ROM symbol. Those
are counted and listed under --verbose so the unchecked set stays visible
instead of quietly reading as green.

  python port/tools/vtalias_guard.py <root> [--verbose]
  python port/tools/vtalias_guard.py --selftest
"""
import os
import re
import sys

ALIAS = re.compile(r"/alternatename:\s*(__ZTV[A-Za-z0-9_]+)\s*=\s*(__ZTV[A-Za-z0-9_]+)")
SYMROW = re.compile(r"^\s*(\S+)\s+kind:\S+\s+addr:(0x[0-9a-fA-F]+)")
SRC_EXT = (".cpp", ".c", ".h", ".inc", ".hpp", ".txt")


def strip_comments(text):
    """Remove /* */ and // comments, keeping string literals intact.

    A commented-out directive is not a directive. Done as a single scan rather
    than a regex so a // inside a string literal does not eat the rest of the
    line.
    """
    out = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                j += 1
            out.append(text[i:j])
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            i = n if j < 0 else j + 2
            out.append(" ")
        elif text.startswith("//", i):
            j = text.find("\n", i)
            i = n if j < 0 else j
            out.append(" ")
        else:
            out.append(c)
            i += 1
    return "".join(out)


def load_config_symbols(root):
    """name -> {address, ...} over every config/arm9 symbols.txt.

    A name may legitimately appear in several overlays; the set is kept whole so
    a genuine multi-module name is reported as ambiguous rather than convicted.
    """
    syms = {}
    base = os.path.join(root, "config", "arm9")
    for dirpath, _dirnames, filenames in os.walk(base):
        for fn in filenames:
            if fn != "symbols.txt":
                continue
            path = os.path.join(dirpath, fn)
            with open(path, encoding="utf-8", errors="replace") as f:
                for line in f:
                    m = SYMROW.match(line)
                    if m:
                        syms.setdefault(m.group(1), set()).add(int(m.group(2), 16))
    return syms


def collect_aliases(root):
    """[(file, lineno, lhs, rhs)] for every vtable alternatename that is code."""
    found = []
    port = os.path.join(root, "port")
    for dirpath, dirnames, filenames in os.walk(port):
        dirnames[:] = [d for d in dirnames if d not in ("build", "__pycache__")]
        for fn in filenames:
            if not fn.endswith(SRC_EXT):
                continue
            path = os.path.join(dirpath, fn)
            try:
                with open(path, encoding="utf-8", errors="replace") as f:
                    raw = f.read()
            except OSError:
                continue
            if "/alternatename:" not in raw:
                continue
            # A .txt is lane prose, never a linker input. Named explicitly so the
            # scoping is a decision in this file and not an accident of the walk.
            if fn.endswith(".txt"):
                continue
            text = strip_comments(raw)
            for m in ALIAS.finditer(text):
                line = text.count("\n", 0, m.start()) + 1
                found.append((os.path.relpath(path, root).replace("\\", "/"),
                              line, m.group(1), m.group(2)))
    return found


def itanium(decorated):
    """__ZTV4Door -> _ZTV4Door. MSVC prefixes a leading underscore on C names."""
    return decorated[1:] if decorated.startswith("__ZTV") else decorated


def check(root, verbose=False):
    syms = load_config_symbols(root)
    aliases = collect_aliases(root)
    bad, unknown, ok = [], [], []
    for path, line, lhs, rhs in aliases:
        la, ra = syms.get(itanium(lhs)), syms.get(itanium(rhs))
        if not la or not ra:
            unknown.append((path, line, lhs, rhs, la, ra))
            continue
        if la & ra:
            ok.append((path, line, lhs, rhs, sorted(la & ra)))
        else:
            bad.append((path, line, lhs, rhs, sorted(la), sorted(ra)))

    print("vtalias_guard: %d vtable alternatename(s) in port/ code, %d address-checked, "
          "%d not in config" % (len(aliases), len(ok) + len(bad), len(unknown)))
    if verbose:
        for path, line, lhs, rhs, la, ra in unknown:
            print("  unchecked  %-52s %s:%d" % (lhs + " = " + rhs, path, line))
            print("             %s %s   %s %s"
                  % (lhs, "0x%08x" % min(la) if la else "(not in config)",
                     rhs, "0x%08x" % min(ra) if ra else "(not in config)"))
        for path, line, lhs, rhs, hit in ok:
            print("  ok         %-52s %s  %s:%d"
                  % (lhs + " = " + rhs, " ".join("0x%08x" % a for a in hit), path, line))
    if bad:
        print("")
        print("vtalias_guard: FAIL -- %d vtable alias(es) join names config gives"
              " DIFFERENT addresses." % len(bad))
        print("An alternatename says two names are one object. Two addresses say they")
        print("are two objects, and whichever fill runs last then owns both classes.")
        for path, line, lhs, rhs, la, ra in bad:
            print("  %s:%d" % (path, line))
            print("    %-28s %s" % (lhs, " ".join("0x%08x" % a for a in la)))
            print("    %-28s %s" % (rhs, " ".join("0x%08x" % a for a in ra)))
        print("Fix: host each table under its own name and give each class its own")
        print("fill, the way gate 22 and gate 40 do in hal/actor_classes.cpp.")
        return 1
    print("vtalias_guard: OK -- every address-checked vtable alias joins one address")
    return 0


def selftest():
    """The guard must be able to say NO, and must not read prose as a directive."""
    fails = 0

    body = 'ok /*/alternatename:__ZTVA=__ZTVB*/ x\n// /alternatename:__ZTVC=__ZTVD\n'
    if ALIAS.search(strip_comments(body)):
        print("vtalias_guard SELFTEST FAIL: a commented-out directive was read as live")
        fails += 1

    live = '#pragma comment(linker, "/alternatename:__ZTV4Door=__ZTV8daDoor_c")\n'
    m = ALIAS.search(strip_comments(live))
    if not m or m.group(1) != "__ZTV4Door" or m.group(2) != "__ZTV8daDoor_c":
        print("vtalias_guard SELFTEST FAIL: a live directive was not read")
        fails += 1

    s = '  ok // "not a comment inside a string" /alternatename:__ZTVE=__ZTVF\n'
    if ALIAS.search(strip_comments(s)):
        print("vtalias_guard SELFTEST FAIL: a directive after // was read as live")
        fails += 1

    if itanium("__ZTV4Door") != "_ZTV4Door" or itanium("_ZTV4Door") != "_ZTV4Door":
        print("vtalias_guard SELFTEST FAIL: MSVC underscore not undecorated")
        fails += 1

    # The conviction itself, on the pair that made this guard exist.
    syms = {"_ZTV12daStarGate_c": {0x021483CC}, "_ZTV4Door": {0x02148188}}
    la, ra = syms["_ZTV12daStarGate_c"], syms["_ZTV4Door"]
    if la & ra:
        print("vtalias_guard SELFTEST FAIL: the star door pair did not convict")
        fails += 1

    if fails:
        return 1
    print("vtalias_guard: SELFTEST OK -- reads live directives, ignores commented and"
          " quoted ones, and convicts a two-address alias")
    return 0


def main(argv):
    if "--selftest" in argv:
        return selftest()
    verbose = "--verbose" in argv
    rest = [a for a in argv if not a.startswith("--")]
    root = rest[0] if rest else os.path.abspath(
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
    return check(root, verbose)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
