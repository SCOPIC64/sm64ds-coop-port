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
failure: it parses every /alternatename directive the build carries, parses
the linked map's defined symbols, and FAILS listing each alias whose LHS is
also a defined symbol in the map.

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

Exit 0: no alias LHS is defined in the map. Exit 1: at least one defeated
alias (or an unreadable map), each listed with its source file and the fix
that wave 5 used: delete the dead alternatename and, if the old routing is
still needed, compile the referencing TUs with a per-source
-DLHS=RHS rename (see the R1/R2 blocks in port/CMakeLists.txt).
"""

import argparse
import os
import re
import sys

PORT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# A real linker directive line. Group 1 = LHS, group 2 = RHS. Decorated names
# (leading underscore cdecl, ?...@@ MSVC C++) are matched as-is against the
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


def parse_map_defined(map_path):
    """Return the set of defined symbol names from an MSVC .map file.

    The publics section lists one defined symbol per line:
      0001:00000000  _symbol  00401000 f  lib:object
    Rows are recognized by the section:offset shape of the first field, which
    holds for every publics row and for no header/prologue line.
    """
    defined = set()
    addr_re = re.compile(r'^[0-9a-fA-F]{4}:[0-9a-fA-F]{8,16}$')
    with open(map_path, 'r', encoding='utf-8', errors='replace') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2 and addr_re.match(parts[0]):
                defined.add(parts[1])
    return defined


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    default_map = os.path.join(os.path.dirname(PORT_DIR), 'build', 'port',
                               'walk_window.map')
    ap.add_argument('--map', default=default_map,
                    help='linked MSVC map file (default: %(default)s)')
    ap.add_argument('--port', default=PORT_DIR,
                    help='port tree to scan for directives (default: %(default)s)')
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
    defined = parse_map_defined(args.map)
    if not defined:
        print('alternatename_guard: FAIL -- no defined symbols parsed from %s'
              % args.map)
        print('  (map exists but has no publics rows; the link is suspect)')
        return 1

    defeated = [(lhs, rhs, rel, ln) for (lhs, rhs, rel, ln) in directives
                if lhs in defined]
    if defeated:
        print('alternatename_guard: FAIL -- %d defeated alias(es): the LHS is'
              % len(defeated))
        print('a DEFINED symbol in the map, so the /alternatename is inert and')
        print('references bind to the new definition, not the intended RHS.')
        for lhs, rhs, rel, ln in defeated:
            print('  %s:%d' % (rel, ln))
            print('    /alternatename:%s=%s' % (lhs, rhs))
            print('    %s is defined in %s' % (lhs, os.path.basename(args.map)))
        print('Fix (the wave-5 R1/R2 recipe): delete the dead alternatename;')
        print('if the old routing is still needed, compile the referencing TUs')
        print('with a per-source -DLHS=RHS (see the R1/R2 blocks in')
        print('port/CMakeLists.txt).')
        return 1

    print('alternatename_guard: OK -- %d alias(es) scanned, none defeated '
          '(%d defined symbols)' % (len(directives), len(defined)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
