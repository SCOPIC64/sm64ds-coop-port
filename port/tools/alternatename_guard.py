"""Build-time guard against DEFEATED /alternatename aliases in the port.

/alternatename:_LHS=_RHS tells the linker "if _LHS is undefined, use _RHS".
The directive only fires while _LHS is UNDEFINED. The moment a real
definition of _LHS enters the link -- a newly sliced src/ TU, a hostgen
output, a host copy -- the alias goes inert and every reference that used to
route to _RHS silently binds to the new definition instead. Nothing warns:
the link succeeds, the bytes are wrong.

Wave 5 hit this class twice (w5b_review.md R1/R2):

  R1  slicing src/RollingRock_Spawn.c defeated
      /alternatename:_RollingRock_Spawn=_data_ov010_02112d64 and
      PeachPainting::InitResources' SharedFilePtr became a code address --
      level 2 faulted (c0000005 at _RollingRock_Spawn+2).
  R2  slicing src/func_ov065_02117994.c (Swoop's helper, a genuinely
      different body) defeated the ov062 sibling alias and three ov062 TUs
      silently ran Swoop's code. No crash, censuses unchanged.

The same flip is latent for _ZN6EyerokD0Ev and the data_ov075 aliases if
their overlays ever land. This guard turns the whole class into a build
failure.

HOW THE MAP TELLS THE TWO CASES APART. When an alias FIRES, link.exe
publishes the LHS as a public at the RHS's address, so both names appear in
the map AT THE SAME ADDRESS -- that is the healthy shape and it covers
nearly every alias in the build. When the alias is DEFEATED, the LHS is a
real definition with its OWN address (and the RHS keeps its own, or is
/OPT:REF-stripped entirely). So the mechanical test is per directive:

  LHS absent from the map ................................. OK (unused)
  LHS present, RHS present, SAME address .................. OK (alias fired)
  LHS present at a DIFFERENT address than RHS ............. FAIL (defeated)
  LHS present, RHS absent ................................. FAIL (defeated;
      the LHS definition satisfied every reference and the RHS died to
      /OPT:REF -- exactly R1's shape when the data symbol goes unreferenced)

WHAT COUNTS AS A LINKER INPUT. An /alternatename reaches link.exe two ways,
and only the first one exists in this tree:

  (a) #pragma comment(linker, "/alternatename:...") in a .c/.cpp/.h under
      port/. The compiler copies the string into the object's .drectve
      section and the linker reads it from there. Only real pragma lines are
      parsed -- prose that merely mentions a directive in a comment is not a
      linker input.
  (b) a DIRECTIVE FILE handed to the linker directly: a response file
      (@file), a /DEF: module-definition file, or an /alternatename written
      straight into the link options. THE PORT HAS NONE OF THESE.
      REGISTERED_DIRECTIVE_FILES is therefore empty, and that emptiness is
      re-derived from port/CMakeLists.txt on every run by
      audit_linker_inputs() rather than believed.

WHY (b) IS A REGISTERED LIST AND NOT A FILE SWEEP. This guard used to walk
EVERY port/**.txt and treat any line whose lstrip started with
"/alternatename:" as a real linker input. port/ is mostly prose (lane maps,
seat notes, syms files), so a lane that quoted a directive at the start of a
line turned its own documentation into a build input. That was never
theoretical:

  * port/stage_lifecycle_map.txt:995 quotes lane SL0's FaderWipe vtable
    alias mid-paragraph, and the sweep parsed it.
  * port/ov007_seat.txt:444 quotes the ov007/ov002 alias to explain why that
    TU stays out of the slice, and the sweep parsed it.
  * lane LK4 deleted the real Heap::SetDefault alias from
    hal/lk4_solidheap_seat.cpp and replaced it with a receiver-bridging
    face. The build stayed broken afterwards, because the QUOTE of the dead
    directive in port/stage_lifecycle_map.txt:1084 was still being read as a
    live one: LHS defined at its own address, RHS at another, so the guard
    called a deleted alias defeated. It only went green once that quote was
    annotated out of the sweep's way. Prose had become load-bearing.

Docs are not linker inputs. The sweep is gone. A line that quotes a
directive in a file nothing hands the linker is prose, and the guard prints
how many it saw and where. A count nobody prints is how the next lane plants
the next landmine.

Loudness moved to where it belongs. If CMake hands the linker a directive
file that is NOT in REGISTERED_DIRECTIVE_FILES, the guard FAILS and names
the file and the registration mechanism; and a registered file CMake no
longer feeds fails the same way, so the list cannot rot in either direction.

WHAT THAT DERIVATION COVERS, EXACTLY. It reads port/CMakeLists.txt itself and
catches the three DIRECT spellings: a response file, a /DEF: file, and an
/alternatename written into the link options. It does not follow indirection.
A directive file reached through a CMake variable, through
target_link_libraries as a raw item, through INTERFACE_LINK_OPTIONS or
STATIC_LIBRARY_OPTIONS, through /WHOLEARCHIVE, through an include()d .cmake
file or an add_subdirectory, or through a generator expression, is a route
this single-file reader does not see. None of those exist in the tree today,
which is why the registry is empty and the build is green, but "cannot dodge
the guard" would be a bigger claim than the code earns. A follow-up lane is
queued to widen the derivation: walk every CMakeLists.txt and *.cmake under
port/, add target_link_libraries, and loosen the LINK_OPTIONS match. Until
that lands, treat this as a checked fact about the direct spellings and a
promise about nothing else.

ONE KNOWN GAP IN (a), REPORTED AND NOT YET CLOSED. The pragma test runs a
line at a time, so a pragma split with a backslash,

    #pragma comment(linker, \
        "/alternatename:_LHS=_RHS")

is a real linker input that this reader does not measure. Four exist:
hal/actor_classes_ccm.cpp:190, hal/w2_dtor_heads.cpp:335, and
unmatched/func_02043fdc_hostcopy.cpp:197 and :201. Against the current map
two fire and two read as defeated fallback-stub pairs (_port_rich_dump_ex
and _port_crash_dir_get, the reduced-binary idiom the baseline exists for),
so closing the gap means a REVIEWED baseline growth and belongs in its own
change rather than smuggled through a scoping fix. Until then the guard
counts these and prints them under "not scanned", because the failure this
whole file is about is a measurement nobody was told was missing.

The map is required: this guard needs the LINKED symbol table, so it runs
POST-LINK (build-port.cmd wires it after ninja; the pre-configure guards
cannot see what the linker resolved). A missing, empty, or truncated map is
itself a FAIL -- a failed link truncates walk_window.map to zero bytes, and
measuring nothing must never read as clean. --selftest needs no map and no
build, and build-port.cmd runs it BEFORE configure alongside closure.py's
and facegen.py's, so a broken guard fails in seconds rather than after a
full link.

THE BASELINE. The full binary carries a reviewed set of aliases whose LHS
is deliberately defined -- the port's weak-symbol idiom: fallback stubs that
only fire in reduced binaries (tests/fault_probe.h, the cxxname_bridge
fallbacks, the l7 particle stub), and MSVC-mangle faces/forwarders that
coexist with the Itanium body on purpose (the w4-c LoadFile forwarder
shape). Those read as "defeated" in the map but are the design. They are
frozen in alternatename_baseline.txt, the inferred_stub_guard ratchet
pattern: the guard FAILS on any defeated pair NOT in the baseline (that is
the R1/R2 arrival shape -- a newly sliced TU defining an alias LHS), and the
baseline may only shrink. Regenerate after a reviewed change with --update;
never hand-add a pair to unblock a build.

Exit 0: no defeated alias outside the baseline. Exit 1: at least one new
defeated alias, an unregistered directive source, or an unreadable map; each
listed with its source file and the fix wave 5 used: delete the dead
alternatename and, if the old routing is still needed, compile the
referencing TUs with a per-source -DLHS=RHS rename (see the R1/R2 blocks in
port/CMakeLists.txt).
"""

import argparse
import os
import re
import sys
import tempfile

PORT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A real linker directive. Group 1 = LHS, group 2 = RHS. Decorated names
# (leading-underscore cdecl, ?...@@ MSVC C++) are matched as-is against the
# map, which lists decorated names too.
ALT_RE = re.compile(r'/alternatename:([^\s"=]+)=([^\s")]+)')
PRAGMA_RE = re.compile(r'#\s*pragma\s+comment\s*\(\s*linker\s*,')

SRC_EXTS = ('.c', '.cpp', '.h')

# Mechanism (b): files the linker reads directives OUT OF. Port-relative,
# forward slashes. Empty on purpose: the port feeds link.exe objects and
# libraries and nothing else. check_registration() re-derives that from
# CMake every run, so this stays a checked fact rather than a claim.
REGISTERED_DIRECTIVE_FILES = ()

# Files a quoted directive can hide in. This is the old sweep's surface
# (.txt) plus the two other doc formats under port/, so the lines the sweep
# used to turn into build input still get counted and named. They are not
# linker inputs and are never measured against the map.
DOC_EXTS = ('.txt', '.md', '.inc')

# CMake constructs that hand link.exe something whose CONTENTS it reads as
# directives, i.e. every way an /alternatename can enter the link without
# passing through a compiled TU. Scanned only inside link-option commands.
CMAKE_COMMENT_RE = re.compile(r'#.*$')
LINKOPT_CTX_RE = re.compile(
    r'target_link_options|add_link_options|\blink_options\b|'
    r'CMAKE_EXE_LINKER_FLAGS|CMAKE_MODULE_LINKER_FLAGS|'
    r'CMAKE_SHARED_LINKER_FLAGS|LINK_FLAGS', re.IGNORECASE)
RSP_RE = re.compile(r'(?:^|[\s"\'])@([A-Za-z0-9_.${}/\\-]+)')
DEF_RE = re.compile(r'/DEF:([^\s"\')]+)', re.IGNORECASE)


def collect_directives(port_dir, registered=None):
    """Sort every /alternatename under port_dir into three buckets.

    Returns (directives, quoted, unscanned).

      directives  [(lhs, rhs, relpath, lineno)], what the LINK carries, and
                  the only bucket measured against the map.
      quoted      [(relpath, lineno)], a complete directive written out in
                  a DOC file. Prose. Named rather than dropped, because a
                  count nobody prints is how the next landmine gets planted.
      unscanned   [(relpath, lineno)], a complete directive inside a
                  LINE-CONTINUED pragma, so a real linker input this
                  line-at-a-time reader does not see. A known gap, reported
                  rather than pretended away; see the docstring.
    """
    if registered is None:
        registered = REGISTERED_DIRECTIVE_FILES
    registered = set(registered)
    out = []
    quoted = []
    unscanned = []
    for root, dirs, files in os.walk(port_dir):
        dirs[:] = sorted(d for d in dirs if d not in ('__pycache__',))
        for name in sorted(files):
            path = os.path.join(root, name)
            rel = os.path.relpath(path, port_dir).replace('\\', '/')
            lower = name.lower()
            is_registered = rel in registered
            is_src = lower.endswith(SRC_EXTS)
            is_doc = lower.endswith(DOC_EXTS)
            if not (is_registered or is_src or is_doc):
                continue
            try:
                with open(path, 'r', encoding='utf-8', errors='replace') as f:
                    continued = False
                    for lineno, line in enumerate(f, 1):
                        was_continued = continued
                        continued = (is_src and line.rstrip().endswith('\\')
                                     and (PRAGMA_RE.search(line)
                                          or was_continued))
                        if '/alternatename:' not in line:
                            continue
                        if is_registered:
                            pass          # a real directive file: every line
                        elif is_src and PRAGMA_RE.search(line):
                            pass          # mechanism (a): a real pragma
                        elif is_src:
                            # A pragma split over lines IS an input; this
                            # reader just cannot see it. Say so.
                            if was_continued and ALT_RE.search(line):
                                unscanned.append((rel, lineno))
                            continue      # otherwise a comment mention
                        else:
                            if ALT_RE.search(line):
                                quoted.append((rel, lineno))
                            continue
                        for m in ALT_RE.finditer(line):
                            out.append((m.group(1), m.group(2), rel, lineno))
            except OSError as e:
                print('alternatename_guard: cannot read %s: %s' % (rel, e))
                sys.exit(1)
    return out, quoted, unscanned


def audit_linker_inputs(port_dir):
    """Re-derive mechanism (b) from port/CMakeLists.txt.

    Returns [(mechanism, token, lineno)] for the DIRECT constructs that hand
    link.exe something it reads as directives: a response file, a /DEF: file,
    and an /alternatename in the link options, written literally in this one
    file. Indirect routes are out of reach and listed in the module
    docstring. CMake comments are stripped first: the CMakeLists prose
    discusses /alternatename in a dozen places and none of that reaches the
    linker.
    """
    found = []
    cml = os.path.join(port_dir, 'CMakeLists.txt')
    if not os.path.isfile(cml):
        return found
    depth = 0
    with open(cml, 'r', encoding='utf-8', errors='replace') as f:
        for lineno, raw in enumerate(f, 1):
            line = CMAKE_COMMENT_RE.sub('', raw)
            if not (depth or LINKOPT_CTX_RE.search(line)):
                continue
            for m in DEF_RE.finditer(line):
                found.append(('/DEF: module-definition file', m.group(1),
                              lineno))
            for m in RSP_RE.finditer(line):
                found.append(('@ response file', m.group(1), lineno))
            if '/alternatename:' in line:
                found.append(('/alternatename: in the link options',
                              line.strip(), lineno))
            depth = max(0, depth + line.count('(') - line.count(')'))
    return found


def check_registration(port_dir, registered=None):
    """Return a list of complaint lines; empty means the registry is honest.

    Two ways to be wrong, both fatal. CMake feeds a directive source that is
    not registered, so the guard would never read it. That is the silent hole
    the old txt sweep was flailing at. Or a registered file is not fed any more
    (or is gone), so the registry describes a build that no longer exists.
    """
    if registered is None:
        registered = REGISTERED_DIRECTIVE_FILES
    registered = list(registered)
    bad = []
    seen = set()
    for mechanism, token, lineno in audit_linker_inputs(port_dir):
        norm = token.replace('\\', '/').lstrip('./')
        seen.add(norm)
        if norm in registered:
            continue
        bad.append('port/CMakeLists.txt:%d hands the linker a directive '
                   'source that is NOT registered:' % lineno)
        bad.append('    mechanism : %s' % mechanism)
        bad.append('    token     : %s' % token)
        bad.append('    register it in REGISTERED_DIRECTIVE_FILES in '
                   'port/tools/alternatename_guard.py (port-relative path),')
        bad.append('    or move the directive into a compiled TU as '
                   '#pragma comment(linker, "/alternatename:...").')
    for rel in registered:
        if rel not in seen:
            bad.append('REGISTERED_DIRECTIVE_FILES lists %s but '
                       'port/CMakeLists.txt no longer feeds it to the '
                       'linker.' % rel)
            bad.append('    drop it from the list, or restore the link '
                       'option that fed it.')
        elif not os.path.isfile(os.path.join(port_dir, rel)):
            bad.append('REGISTERED_DIRECTIVE_FILES lists %s but that file '
                       'does not exist under port/.' % rel)
    return bad


def parse_map_publics(map_path):
    """Return {symbol: 'SECTION:OFFSET'} from the MSVC map's publics section.

    Rows look like
        0001:000edd30       _func_020b5e58             004eed30 f   obj
    The section runs from the 'Publics by Value' header to the 'entry point
    at' line; the trailing 'Static symbols' block is deliberately excluded
    (TU-local names must not shadow an alias LHS).
    """
    publics = {}
    addr_re = re.compile(r'^[0-9a-fA-F]{4}:[0-9a-fA-F]{8,16}$')
    in_publics = False
    with open(map_path, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            if 'Publics by Value' in line:
                in_publics = True
                continue
            if line.lstrip().startswith('entry point at'):
                break
            if not in_publics:
                continue
            parts = line.split()
            if len(parts) >= 2 and addr_re.match(parts[0]):
                publics.setdefault(parts[1], parts[0])
    return publics


def evaluate(directives, publics):
    """Return (defeated, fired) for a directive set against a map.

    defeated rows are (lhs, rhs, relpath, lineno, lhs_addr, rhs_addr).
    """
    defeated = []
    fired = 0
    for lhs, rhs, rel, ln in directives:
        la = publics.get(lhs)
        if la is None:
            continue                      # unused alias: fine
        ra = publics.get(rhs)
        if ra is not None and la == ra:
            fired += 1                    # alias fired: LHS rides RHS's address
            continue
        defeated.append((lhs, rhs, rel, ln, la, ra))
    return defeated, fired


def baseline_path():
    return os.path.join(PORT_DIR, 'tools', 'alternatename_baseline.txt')


def load_baseline():
    pairs = set()
    try:
        with open(baseline_path(), 'r') as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith('#'):
                    pairs.add(line)
    except OSError:
        pass
    return pairs


def _old_txt_sweep(port_dir):
    """The DELETED sweep, transcribed, so the selftest can show what changed.

    Every port/**.txt line whose lstrip started with the directive was read
    as a linker input. This is the selftest's control arm and nothing on the
    live path calls it.
    """
    out = []
    for root, dirs, files in os.walk(port_dir):
        dirs[:] = sorted(d for d in dirs if d not in ('__pycache__',))
        for name in sorted(files):
            if not name.lower().endswith('.txt'):
                continue
            path = os.path.join(root, name)
            rel = os.path.relpath(path, port_dir).replace('\\', '/')
            with open(path, 'r', encoding='utf-8', errors='replace') as f:
                for lineno, line in enumerate(f, 1):
                    if not line.lstrip().startswith('/alternatename:'):
                        continue
                    for m in ALT_RE.finditer(line):
                        out.append((m.group(1), m.group(2), rel, lineno))
    return out


def selftest():
    """A synthetic port tree carrying every shape the scoping rule sorts.

    The fixture reproduces lane LK4's build break. A real directive is
    DELETED from the source that used to carry it and only a QUOTE of it
    survives in a lane map, while the map shows the LHS defined at its own
    address (LK4 replaced the alias with a receiver-bridging face). Under
    the old sweep that quote is a directive and the pair reads DEFEATED --
    the historical failure, reproduced. Under the new scope it is prose and
    the same tree is clean, in BOTH the bare and the "(DELETED by lane LK4)"
    annotated spelling, which is what makes that annotation optional.

    On top of that: a real pragma in a .cpp is still scanned; a directive in
    a REGISTERED directive file is scanned even though it is neither a
    pragma nor a source; and a CMakeLists that feeds the linker a response
    file nobody registered FAILS, naming the file and the mechanism.
    """
    ok = True

    def expect(cond, what, got):
        nonlocal ok
        if not cond:
            print("FAIL: %s: %s" % (what, got))
            ok = False

    lhs, rhs = '__ZN4Heap10SetDefaultEv', '?SetDefault@Heap@@QAEHXZ'
    directive = '/alternatename:%s=%s' % (lhs, rhs)
    # The LK4 map shape: LHS is a real definition, RHS lives elsewhere.
    publics = {lhs: '0001:0001cdb0', rhs: '0001:00060a20',
               '_real_lhs': '0001:00000100', '_real_rhs': '0001:00000100',
               '_rsp_lhs': '0001:00000200', '_rsp_rhs': '0001:00000200'}

    with tempfile.TemporaryDirectory() as td:
        port = os.path.join(td, 'port')
        os.makedirs(os.path.join(port, 'hal'))

        # (a) a REAL directive, the only mechanism the port uses.
        with open(os.path.join(port, 'hal', 'aliases.cpp'), 'w') as f:
            f.write('// a quoted mention on its own is not an input:\n')
            f.write('// /alternatename:_prose_lhs=_prose_rhs\n')
            f.write('#pragma comment(linker, '
                    '"/alternatename:_real_lhs=_real_rhs")\n')
            f.write('#pragma comment(linker, \\\n')
            f.write('    "/alternatename:_cont_lhs=_cont_rhs")\n')

        # (b) LK4's shape: the source that carried the directive no longer
        # does, and two lane maps quote it, bare and annotated.
        with open(os.path.join(port, 'hal', 'lk4_seat.cpp'), 'w') as f:
            f.write('// SetDefault USED TO BE THE FIRST LINE OF THIS BLOCK.\n')
            f.write('// The receiver-bridging face below replaces it.\n')
            f.write('extern "C" int %s(void *thiz) { return 0; }\n' % lhs)
        with open(os.path.join(port, 'bare_map.txt'), 'w') as f:
            f.write('  3  the cause is a mis-bridged receiver, in\n')
            f.write('     hal/lk4_seat.cpp:45:\n')
            f.write('         %s\n' % directive)
            f.write('     The RHS is __thiscall and takes ECX.\n')
        with open(os.path.join(port, 'annotated_map.txt'), 'w') as f:
            f.write('         (DELETED by lane LK4) %s\n' % directive)

        # A CMakeLists that feeds the linker nothing but flags: the shape
        # the empty registry is derived from.
        clean_cml = ('set(CMAKE_EXE_LINKER_FLAGS '
                     '"${CMAKE_EXE_LINKER_FLAGS} /MAP")\n'
                     '# prose: /alternatename:_x=_y and /DEF:not_real.def\n'
                     'target_link_options(walk_window PRIVATE '
                     '/DYNAMICBASE:NO)\n')
        cml = os.path.join(port, 'CMakeLists.txt')
        with open(cml, 'w') as f:
            f.write(clean_cml)

        directives, quoted, unscanned = collect_directives(port)
        old = _old_txt_sweep(port)

        # 1. The historical break: old scope parses the quote, new does not.
        expect([(d[0], d[2]) for d in old] == [(lhs, 'bare_map.txt')],
               'old sweep reads the LK4 quote as a directive', old)
        old_defeated, _ = evaluate(old, publics)
        expect(len(old_defeated) == 1 and old_defeated[0][0] == lhs,
               'old sweep FAILS on the deleted alias', old_defeated)
        new_defeated, new_fired = evaluate(directives, publics)
        expect(new_defeated == [], 'new scope has no defeats', new_defeated)

        # 2. The real pragma is still scanned, and it still fires.
        expect([(d[0], d[1]) for d in directives]
               == [('_real_lhs', '_real_rhs')],
               'the pragma is the only scanned directive', directives)
        expect(new_fired == 1, 'the real alias still fires', new_fired)

        # 3. Both quote spellings are inert, and both are still counted.
        #    This is what lets the "(DELETED by lane LK4)" annotation stop
        #    being load-bearing: the bare line is prose too.
        expect(sorted(quoted) == [('CMakeLists.txt', 2),
                                  ('annotated_map.txt', 1),
                                  ('bare_map.txt', 3)],
               'bare and annotated quotes, and a CMake comment, are prose '
               'and named', quoted)

        # 3b. The line-continued pragma is a real input, is NOT scanned, and
        #     is NOT silently misfiled as prose.
        expect(unscanned == [('hal/aliases.cpp', 5)],
               'the continued pragma is reported as an unscanned input',
               unscanned)
        expect(all(r != 'hal/aliases.cpp' for r, _ in quoted),
               'a real input is never reported as prose', quoted)

        # 4. A registered directive file IS scanned, pragma or not.
        rsp = 'link_aliases.rsp'
        with open(os.path.join(port, rsp), 'w') as f:
            f.write('/alternatename:_rsp_lhs=_rsp_rhs\n')
        reg, reg_quoted, _ = collect_directives(port, registered=(rsp,))
        expect(('_rsp_lhs', '_rsp_rhs', rsp, 1) in reg,
               'a registered directive file is scanned', reg)
        expect(reg_quoted == quoted,
               'registering a file does not change the prose count',
               reg_quoted)

        # 5. Registration is checked against CMake, not assumed. Clean tree
        #    with an empty registry: silent.
        expect(check_registration(port, registered=()) == [],
               'clean tree registers clean',
               check_registration(port, registered=()))

        # 6. CMake feeds a response file nobody registered: LOUD.
        with open(cml, 'w') as f:
            f.write(clean_cml)
            f.write('target_link_options(walk_window PRIVATE @%s)\n' % rsp)
        loud = check_registration(port, registered=())
        expect(any(rsp in l for l in loud)
               and any('response file' in l for l in loud)
               and any('REGISTERED_DIRECTIVE_FILES' in l for l in loud),
               'unregistered response file fails loudly, naming file and '
               'mechanism', loud)
        expect(check_registration(port, registered=(rsp,)) == [],
               'registering it clears the complaint',
               check_registration(port, registered=(rsp,)))

        # 7. /DEF: and an inline directive are caught the same way.
        with open(cml, 'w') as f:
            f.write(clean_cml)
            f.write('target_link_options(walk_window PRIVATE '
                    '/DEF:exports.def)\n')
            f.write('set(CMAKE_EXE_LINKER_FLAGS '
                    '"${CMAKE_EXE_LINKER_FLAGS} /alternatename:_a=_b")\n')
        loud = check_registration(port, registered=())
        expect(any('exports.def' in l for l in loud)
               and any('module-definition' in l for l in loud),
               'a /DEF: file is caught', loud)
        expect(any('in the link options' in l for l in loud),
               'an inline /alternatename in link options is caught', loud)

        # 8. A registry naming a file CMake does not feed is also wrong.
        with open(cml, 'w') as f:
            f.write(clean_cml)
        expect(any('no longer feeds it' in l
                   for l in check_registration(port, registered=(rsp,))),
               'a stale registration fails',
               check_registration(port, registered=(rsp,)))

    print("selftest %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    default_map = os.path.join(os.path.dirname(PORT_DIR), 'build', 'port',
                               'walk_window.map')
    ap.add_argument('--map', default=default_map,
                    help='linked MSVC map file (default: %(default)s)')
    ap.add_argument('--port', default=PORT_DIR,
                    help='port tree to scan for directives (default: %(default)s)')
    ap.add_argument('--update', action='store_true',
                    help='rewrite the baseline PAIRS block from the live '
                         'defeated set (reviewed changes only)')
    ap.add_argument('--selftest', action='store_true',
                    help='run the scoping fixture; needs no map and no build')
    args = ap.parse_args()

    if args.selftest:
        return selftest()

    if not os.path.isfile(args.map):
        print('alternatename_guard: FAIL -- map not found: %s' % args.map)
        print('  (the guard runs post-link; a missing map means no link happened)')
        return 1
    if os.path.getsize(args.map) == 0:
        print('alternatename_guard: FAIL -- map is EMPTY: %s' % args.map)
        print('  (a failed link truncates the map to zero bytes; rebuild first)')
        return 1

    unregistered = check_registration(args.port)
    if unregistered:
        print('alternatename_guard: FAIL -- the linker is fed a directive '
              'source this guard does not read.')
        print('An /alternatename arriving that way is invisible to the map '
              'check below, which is the')
        print('hole the old port/**.txt sweep was flailing at. Register it, '
              'or move it into a TU.')
        for line in unregistered:
            print('  %s' % line)
        return 1

    directives, quoted, unscanned = collect_directives(args.port)
    publics = parse_map_publics(args.map)
    if not publics:
        print('alternatename_guard: FAIL -- no publics parsed from %s'
              % args.map)
        print('  (map exists but has no publics rows; the link is suspect)')
        return 1

    defeated, fired = evaluate(directives, publics)

    if args.update:
        lines = sorted(set('%s=%s' % (lhs, rhs)
                           for lhs, rhs, _, _, _, _ in defeated))
        try:
            old = open(baseline_path(), 'r').read()
        except OSError:
            old = ''
        head = [l for l in old.splitlines() if l.startswith('#')]
        with open(baseline_path(), 'w') as f:
            for l in head:
                f.write(l + '\n')
            for l in lines:
                f.write(l + '\n')
        print('alternatename_guard: baseline rewritten, %d pair(s)'
              % len(lines))
        return 0

    baseline = load_baseline()
    live = {'%s=%s' % (lhs, rhs) for lhs, rhs, _, _, _, _ in defeated}
    stale = sorted(baseline - live)
    if stale:
        print('alternatename_guard: note -- %d baseline pair(s) no longer '
              'defeated (baseline can be tightened with --update):'
              % len(stale))
        for pair in stale:
            print('  %s' % pair)
    defeated = [d for d in defeated
                if '%s=%s' % (d[0], d[1]) not in baseline]

    if defeated:
        print('alternatename_guard: FAIL -- %d NEW defeated alias(es), not in'
              % len(defeated))
        print('the baseline: each LHS below is DEFINED at its own address, so')
        print('the /alternatename is inert and references bind to the')
        print('definition, not the intended RHS (the R1/R2 arrival shape).')
        for lhs, rhs, rel, ln, la, ra in defeated:
            print('  %s:%d' % (rel, ln))
            print('    /alternatename:%s=%s' % (lhs, rhs))
            print('    LHS at %s, RHS %s' % (la, ra if ra else 'NOT IN MAP'))
        print('Fix (the wave-5 R1/R2 recipe): delete the dead alternatename;')
        print('if the old routing is still needed, compile the referencing TUs')
        print('with a per-source -DLHS=RHS (see the R1/R2 blocks in')
        print('port/CMakeLists.txt).')
        return 1

    print('alternatename_guard: OK -- %d directive(s) scanned, %d fired, '
          '%d baseline-known, 0 new defeats (%d publics)'
          % (len(directives), fired, len(live), len(publics)))
    print('  sources: %d registered directive file(s) plus every real pragma '
          'in %s under port/'
          % (len(REGISTERED_DIRECTIVE_FILES), '/'.join(SRC_EXTS)))
    if quoted:
        print('  %d directive(s) quoted in prose, NOT linker inputs, not '
              'measured:' % len(quoted))
        for rel, ln in quoted:
            print('    port/%s:%d' % (rel, ln))
    if unscanned:
        print('  %d directive(s) in LINE-CONTINUED pragmas: real inputs this '
              'reader cannot see' % len(unscanned))
        print('  (known gap, see the docstring; closing it needs a reviewed '
              'baseline growth):')
        for rel, ln in unscanned:
            print('    port/%s:%d' % (rel, ln))
    return 0


if __name__ == '__main__':
    sys.exit(main())
