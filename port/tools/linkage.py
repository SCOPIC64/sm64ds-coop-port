"""How much of the port is the real decomp, and what is still scaffolding?

The goal for the port is that it IS the decompiled game: every host file under
port/hal and port/unmatched replaced by the byte-matched translation unit it
stands in for, step by step, until there is nothing left standing in.

This measures that two ways.

  headline   how many matched TUs from src/ actually reach the binary
  queue      for every symbol a HOST file defines, whether src/ already has a
             matched TU defining the same symbol -- those are the direct
             replacement candidates, the work list for getting to 100%

The queue is the useful half. A host file whose symbols have no matched
counterpart is genuine host code (the ntr layer, the window, the SDAT host,
MSVC ABI shims for register ride-throughs and pointer-to-member dispatch) and
is not a defect. A host file whose symbols DO have matched counterparts is
scaffolding that outlived its reason, and port_stage_a_boot is the example
that motivated this file: a hand-written subset of Stage::InitResources, while
Stage::InitResources sat in src/ decompiled and unlinked.

NOT every queue symbol is scaffolding, though. This tool sorts every queue hit
into three buckets, and only the first of them is work:

  SHADOWS     a host file defines the symbol and the matched TU of that name
              is NOT in the binary. The host body is what runs. This is the
              replacement work list -- the number the port drives to zero.

  FACES       a host file defines the symbol AND the matched TU of that name
              IS in the binary. Both can coexist because they are different
              symbols to the linker: the matched .cpp compiles a real C++
              method (MSVC mangles it `?Allocate@Heap@@...`) while the host
              file defines the ROM's Itanium C name (`_ZN4Heap8AllocateEji`)
              that the ROM's own vtables and call sites use. hal/heap_vtable
              and hal/cxxname_bridge are full of these, and they are the
              OPPOSITE of scaffolding -- they are the bridge that lets a
              C-named ROM caller reach the decompiled method.
              READ THE CAVEAT: a map cannot prove forwarding. All this bucket
              knows is that both definitions are linked. That is consistent
              with a face that forwards into the matched body, and equally
              consistent with a host body that duplicates it while the matched
              TU is kept alive by some other caller -- which WOULD be
              scaffolding. Deciding takes reading the host definition. The
              bucket is listed separately so it is neither counted as work nor
              silently forgiven.

  EXCEPTIONS  a matched src TU that is an ARM asm primitive, an ARM register
              ride-through, an mwcc pointer-to-member dispatch, or a poke of DS
              hardware the ntr layer does not model cannot compile-and-behave
              under MSVC no matter how much wiring is added -- the host
              definition IS the faithful stand-in. A host definition carrying a
              `PORT_HOST_ABI:` tag in the comment right above it is one of
              these. The tag is a human ruling, so it wins over the other two
              buckets: a tagged symbol is an exception whether or not the
              matched TU also links.

THE DECORATION RULE, and the bug it replaced. This is a 32-bit MSVC link, so
an `extern "C"` function gets exactly ONE leading underscore in the /MAP:
`func_02049d60` appears as `_func_02049d60`. The ROM's C++ symbols are carried
through the port as C names that ALREADY start with an underscore, so
`_ZN4Heap8AllocateEji` appears as `__ZN4Heap8AllocateEji`. This file used to
undecorate with `sym.lstrip("_")`, which strips EVERY leading underscore and
turned that into `ZN4Heap8AllocateEji` -- a name no matched TU is called, so
every single `_ZN*`-named host definition fell out of the queue unseen. The
queue read 132 when the honest number was ~447. Strip exactly one.

    python port/tools/linkage.py [repo-root] [--queue] [--exceptions] [--faces]

  --queue        list every SHADOW symbol with its matched source
  --exceptions   also print the documented host-ABI exceptions and their reasons
  --faces        also print the face bucket, host object by host object

With no repo-root argument this measures the checkout the script itself lives
in, not the current directory. It used to root at CWD, which reads the same
from the repo root and silently measures nothing (or another checkout's map)
from anywhere else. The resolved root and map are printed above every run, so
a pasted reading always says which tree and which binary it came from.
"""
import os
import re
import sys

HOST_DIRS = ("hal", "unmatched")

# Directories a host object's source may live in, most specific first. fs.cpp
# exists under both hal/ and ntr/; the queue's plain `fs.cpp.obj` is the hal one
# and the ntr copy is namespaced (`ntr_2x:runtime.cpp.obj`), so hal wins here.
HOST_SRC_DIRS = ("port/hal", "port/hal/sdat", "port/unmatched", "port/ntr",
                 "port/tests")

# A host definition is a documented host-ABI exception when the comment block
# right above it carries this tag. The text after the colon is the reason;
# trailing comment punctuation (*/) is stripped off.
ABI_TAG = re.compile(r"PORT_HOST_ABI:\s*(.+?)\s*(?:\*/\s*)?$")


def host_source_for(root, obj):
    """Resolve a host .obj name to its source file, or None."""
    # obj is like `cxx_aliases.cpp.obj` or `ntr_2x:runtime.cpp.obj`.
    base = obj.split(":", 1)[-1]
    stem = os.path.splitext(os.path.splitext(base)[0])[0]
    for d in HOST_SRC_DIRS:
        for ext in (".cpp", ".c"):
            p = os.path.join(root, d, stem + ext)
            if os.path.exists(p):
                return p
    return None


IDENT = re.compile(r"[A-Za-z_]\w*")
# Type/keyword tokens that share a definition line with the real symbol name.
_SKIP_IDENT = {"extern", "static", "void", "int", "unsigned", "char", "short",
               "long", "const", "signed", "struct", "return", "if", "for",
               "while", "C"}


def _is_comment_or_blank(stripped, in_block):
    """(is_comment_or_blank, new_in_block) for a stripped source line."""
    if stripped == "":
        return True, in_block
    if in_block:
        # inside /* ... */; the block ends on the line carrying */
        return True, (not stripped.endswith("*/"))
    if stripped.startswith("//"):
        return True, False
    if stripped.startswith("/*"):
        # a /* ... */ opener; stays open unless it also closes on this line
        return True, (not stripped.endswith("*/"))
    if stripped.startswith("*"):
        return True, in_block
    return False, in_block


def _reasons_in(source_path):
    """{symbol: reason} for every PORT_HOST_ABI tag in a host source file.

    A tag documents the DEFINITION right below it. The reason binds to every
    identifier on the first real code line after the tag -- skipping the rest of
    the comment block, tracking /* ... */ state so a wrapped prose line does not
    look like code. The definition may be `extern "C" void foo(...)`, a pointer
    return `void *foo(...)`, or a plain method; the caller only ever asks about
    names that are real queue symbols, so recording every identifier is safe.
    """
    try:
        with open(source_path, errors="replace") as f:
            lines = f.readlines()
    except (OSError, TypeError):
        return {}
    out = {}
    for i, line in enumerate(lines):
        m = ABI_TAG.search(line)
        if not m:
            continue
        reason = m.group(1)
        # Is this tag line itself the tail of an open /* block? Find out by
        # scanning down for the first genuine code line.
        in_block = line.strip().startswith(("/*", "*")) and not line.strip().endswith("*/")
        # If the tag is on a `// ...` line, no block is open.
        if line.strip().startswith("//"):
            in_block = False
        j = i + 1
        # Bind the reason to every identifier from the tag down to the end of
        # the definition it documents. `func_02059824`'s definition is preceded
        # by a forward-declaration of the body it calls, and several tags sit
        # above a prototype line before the real definition; collecting the
        # whole run to the opening `{` (or the closing `;` of a one-line body)
        # catches the real symbol whichever line carries it. Extra prototype
        # names are harmless -- the caller only ever asks about queue symbols.
        # A blank line or a new comment block ends the run (the next tagged def).
        run = 0
        while j < len(lines) and run < 12:
            st = lines[j].strip()
            is_c, in_block = _is_comment_or_blank(st, in_block)
            if is_c:
                if st == "" and not in_block:
                    break
                j += 1
                continue
            for name in IDENT.findall(lines[j]):
                if name not in _SKIP_IDENT:
                    out.setdefault(name, reason)
                    # /alternatename pragmas carry the MSVC `_` prefix the
                    # map parser strips; bind the stripped form too so a tag
                    # above an alias documents the symbol the queue asks for.
                    # Same one-underscore rule as the map parser, or a tag
                    # above a `__ZN...` alias would bind a name the queue
                    # never asks about.
                    out.setdefault(undecorate(name), reason)
            # A `{` opens the definition body -- stop here. A line ending in `;`
            # is a forward-declaration/prototype the definition sits below;
            # keep going. `}` closes a one-line body -- stop.
            if "{" in st or st.endswith("}"):
                break
            j += 1
            run += 1
    return out


_REASON_CACHE = {}


def abi_reason(source_path, sym):
    """The PORT_HOST_ABI reason tagged above sym's definition, or None."""
    if not source_path:
        return None
    reasons = _REASON_CACHE.get(source_path)
    if reasons is None:
        reasons = _reasons_in(source_path)
        _REASON_CACHE[source_path] = reasons
    return reasons.get(sym)


def matched_index(root):
    """{symbol stem: repo-relative src path} for every matched TU."""
    out = {}
    for dirpath, _d, files in os.walk(os.path.join(root, "src")):
        for fn in files:
            if fn.endswith((".c", ".cpp")):
                stem = os.path.splitext(fn)[0]
                rel = os.path.relpath(os.path.join(dirpath, fn), root)
                out.setdefault(stem, rel.replace("\\", "/"))
    return out


def undecorate(sym):
    """Undo MSVC's cdecl decoration: exactly ONE leading underscore.

    32-bit MSVC prefixes every `extern "C"` symbol with a single `_`. The ROM's
    C++ names travel through the port as C identifiers that already begin with
    an underscore, so `_ZN4Heap8AllocateEji` is emitted `__ZN4Heap8AllocateEji`
    and only the FIRST underscore is decoration. `lstrip("_")` eats both and
    loses the name; taking one character keeps `_ZN...` intact and still
    undecorates the plain `_func_020226d0` case correctly. Decorated C++ names
    (`?Allocate@Heap@@...`) carry no underscore prefix and pass through.
    """
    return sym[1:] if sym.startswith("_") else sym


def map_symbols(mapfile):
    """[(symbol, objfile)] out of an MSVC /MAP."""
    out = []
    with open(mapfile, errors="replace") as f:
        for line in f:
            # The flag column between the RVA and the object is `f`, `f i`, or
            # absent (data rows). A bare \S* placeholder backtracks into the
            # object name on flagless rows and truncates `ov010_syms.c.obj`
            # to `c.obj`, so spell the flags out and anchor the object at EOL.
            m = re.match(
                r"\s+[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+[0-9a-fA-F]{8}\s+(?:[fi]\s+)*(\S+\.obj)\s*$",
                line)
            if m:
                out.append((undecorate(m.group(1)), m.group(2)))
    return out


def default_root():
    """The checkout this script lives in: <root>/port/tools/linkage.py."""
    here = os.path.dirname(os.path.abspath(__file__))
    return os.path.dirname(os.path.dirname(here))


def main():
    positional = [a for a in sys.argv[1:] if not a.startswith("--")]
    root = positional[0] if positional else default_root()
    show_queue = "--queue" in sys.argv
    show_exceptions = "--exceptions" in sys.argv
    show_faces = "--faces" in sys.argv

    mapfile = os.path.join(root, "build", "port", "walk_window.map")
    if not os.path.exists(mapfile):
        print("no %s -- build walk_window first" % mapfile)
        return 2

    # Say which tree and which binary this reading came from. A linkage number
    # is only quotable next to these two lines.
    print("repo root : %s" % os.path.abspath(root))
    print("map       : %s" % os.path.abspath(mapfile))
    print()

    matched = matched_index(root)
    syms = map_symbols(mapfile)

    linked_stems = set()
    host_hits = {}
    for sym, obj in syms:
        stem = os.path.splitext(os.path.splitext(obj)[0])[0]
        if stem in matched:
            linked_stems.add(stem)
        # a host object is one whose name is not itself a matched TU
        if stem not in matched and sym in matched:
            host_hits.setdefault(obj, set()).add(sym)

    total = len(matched)
    linked = len(linked_stems)
    print("matched TUs in src/        : %d" % total)
    print("linked into walk_window    : %d  (%.1f%%)" % (linked, 100.0 * linked / total))
    print("still unlinked             : %d" % (total - linked))
    print()

    if not host_hits:
        print("no host object defines a symbol that src/ also has. Nothing to replace.")
        return 0

    # Split each host object's symbols three ways. Order matters: the
    # PORT_HOST_ABI tag is a human ruling and wins, then a matched TU that is
    # itself linked marks a face, and what is left is the work list.
    src_cache = {}
    documented = {}    # obj -> {sym: reason}   EXCEPTIONS, tagged
    faces = {}         # obj -> set(sym)        matched TU is linked too
    undocumented = {}  # obj -> set(sym)        SHADOWS, the work list
    for obj, s in host_hits.items():
        source = src_cache.get(obj)
        if source is None:
            source = host_source_for(root, obj)
            if source is None:
                # A host object with no source under the host dirs is a
                # generated seat (ovNNN_syms.c.obj). The only way a matched
                # TU's name lands in one is an /alternatename alias, and
                # cxx_aliases.cpp is the alias registry -- read the reason
                # from the tag above the alias.
                source = os.path.join(root, "port", "hal", "cxx_aliases.cpp")
            src_cache[obj] = source
        for sym in s:
            reason = abi_reason(source, sym)
            if reason:
                documented.setdefault(obj, {})[sym] = reason
            elif sym in linked_stems:
                faces.setdefault(obj, set()).add(sym)
            else:
                undocumented.setdefault(obj, set()).add(sym)

    n_all = sum(len(v) for v in host_hits.values())
    n_doc = sum(len(v) for v in documented.values())
    n_face = sum(len(v) for v in faces.values())
    n_undoc = sum(len(v) for v in undocumented.values())

    print("REPLACEMENT QUEUE: %d symbols across %d host objects are defined by a"
          % (n_all, len(host_hits)))
    print("host file while src/ carries a matched TU of the same name. Of those,")
    print("  %d are documented host-ABI exceptions (asm primitives, register"
          % n_doc)
    print("     ride-throughs, mwcc pointer-to-member, unmodelled DS hardware)")
    print("  %d are FACES -- the matched TU of that name is linked too, so the"
          % n_face)
    print("     host definition is most likely the C-name bridge INTO it, not a")
    print("     stand-in for it. Not work; not proven either (see --faces)")
    print("  %d are SHADOWS -- no matched TU of that name reaches the binary,"
          % n_undoc)
    print("     so the host body is what runs. This is the replacement work")
    print()

    # The shadow queue is the work list.
    if undocumented:
        print("UNDOCUMENTED QUEUE / SHADOWS (%d):" % n_undoc)
        for obj, s in sorted(undocumented.items(), key=lambda kv: -len(kv[1])):
            print("  %-40s %d symbol(s)" % (obj, len(s)))
            if show_queue:
                for sym in sorted(s):
                    print("        %-46s %s" % (sym, matched[sym]))
    else:
        print("UNDOCUMENTED QUEUE / SHADOWS (0): every queue symbol is a")
        print("documented host-ABI exception or a face over a linked matched")
        print("TU. The only way to shrink it further is a toolchain that can")
        print("run the ROM's ARM asm and hardware pokes.")

    print()
    print("FACES (%d): host definition and matched TU BOTH linked. The matched"
          % n_face)
    print("code is in the binary; these carry the ROM's C name to it. A map")
    print("cannot prove the host definition forwards rather than duplicates,")
    print("so read one before quoting it as either.")
    for obj, s in sorted(faces.items(), key=lambda kv: -len(kv[1])):
        print("  %-40s %d symbol(s)" % (obj, len(s)))
        if show_faces:
            for sym in sorted(s):
                print("        %-46s %s" % (sym, matched[sym]))

    if show_exceptions:
        print()
        print("DOCUMENTED HOST-ABI EXCEPTIONS (%d):" % n_doc)
        for obj, d in sorted(documented.items(), key=lambda kv: -len(kv[1])):
            print("  %-40s %d symbol(s)" % (obj, len(d)))
            for sym in sorted(d):
                print("        %-30s %s" % (sym, d[sym]))

    if not show_queue or not show_exceptions or not show_faces:
        print()
        print("re-run with --queue for shadow sources, --faces for the face")
        print("list, --exceptions for the documented reasons")
    return 0


if __name__ == "__main__":
    sys.exit(main())
