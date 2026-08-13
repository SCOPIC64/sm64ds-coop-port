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

Directive sources covered, both ways an /alternatename enters the link:

  (a) #pragma comment(linker, "/alternatename:...") lines in any .c/.cpp/.h
      under port/. Only real pragma lines are parsed -- prose that merely
      mentions a directive in a comment is not a linker input.
  (b) bare /alternatename:... directive lines in any .txt under port/ (a
      response/directive file handed to the linker). None exist today; the
      reader is here so a future directive file cannot dodge the guard.

The map is required: this guard needs the LINKED symbol table, so it runs
POST-LINK (build-port.cmd wires it after ninja; the pre-configure guards
cannot see what the linker resolved). A missing, empty, or truncated map is
itself a FAIL -- a failed link truncates walk_window.map to zero bytes, and
measuring nothing must never read as clean.

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
defeated alias (or an unreadable map), each listed with its source file and
the fix wave 5 used: delete the dead alternatename and, if the old routing
is still needed, compile the referencing TUs with a per-source -DLHS=RHS
rename (see the R1/R2 blocks in port/CMakeLists.txt).
"""

import argparse
import os
import re
import sys

PORT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A real linker directive. Group 1 = LHS, group 2 = RHS. Decorated names
# (leading-underscore cdecl, ?...@@ MSVC C++) are matched as-is against the
# map, which lists decorated names too.
ALT_RE = re.compile(r'/alternatename:([^\s"=]+)=([^\s")]+)')
PRAGMA_RE = re.compile(r'#\s*pragma\s+comment\s*\(\s*linker\s*,')

SRC_EXTS = ('.c', '.cpp', '.h')


def collect_directives(port_dir):
    """Return [(lhs, rhs, relpath, lineno)] for every /alternatename the
    build carries."""
    out = []
    for root, dirs, files in os.walk(port_dir):
        dirs[:] = [d for d in dirs if d not in ('__pycache__',)]
        for name in files:
            path = os.path.join(root, name)
            rel = os.path.relpath(path, port_dir)
            lower = name.lower()
            is_src = lower.endswith(SRC_EXTS)
            is_txt = lower.endswith('.txt')
            if not (is_src or is_txt):
                continue
            try:
                with open(path, 'r', encoding='utf-8', errors='replace') as f:
                    for lineno, line in enumerate(f, 1):
                        if '/alternatename:' not in line:
                            continue
                        if is_src and not PRAGMA_RE.search(line):
                            continue  # prose mention, not a linker input
                        if is_txt and not line.lstrip().startswith('/alternatename:'):
                            continue  # comment mention in a syms/notes file
                        for m in ALT_RE.finditer(line):
                            out.append((m.group(1), m.group(2), rel, lineno))
            except OSError as e:
                print('alternatename_guard: cannot read %s: %s' % (rel, e))
                sys.exit(1)
    return out


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
    args = ap.parse_args()

    if not os.path.isfile(args.map):
        print('alternatename_guard: FAIL -- map not found: %s' % args.map)
        print('  (the guard runs post-link; a missing map means no link happened)')
        return 1
    if os.path.getsize(args.map) == 0:
        print('alternatename_guard: FAIL -- map is EMPTY: %s' % args.map)
        print('  (a failed link truncates the map to zero bytes; rebuild first)')
        return 1

    directives = collect_directives(args.port)
    publics = parse_map_publics(args.map)
    if not publics:
        print('alternatename_guard: FAIL -- no publics parsed from %s'
              % args.map)
        print('  (map exists but has no publics rows; the link is suspect)')
        return 1

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
    return 0


if __name__ == '__main__':
    sys.exit(main())
