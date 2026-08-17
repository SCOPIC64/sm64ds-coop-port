#!/usr/bin/env python3
"""aritycheck -- the third face of the class: the frame balances, the
arguments are still wrong.

C has no cross-translation-unit prototype checking for extern "C" symbols.
The linker matches on the NAME alone, so one TU may declare

    extern void _ZN9ActorBase18MarkForDestructionEv(void);   /* src/... */

while another DEFINES

    void _ZN9ActorBase18MarkForDestructionEv(void *self)     /* port/hal/... */
    { ((ActorBase *)self)->ActorBase::MarkForDestruction(); }

and nothing complains. The caller pushes nothing, the callee reads `self` off
[esp+4], and what is sitting there is THE CALLER'S OWN RETURN ADDRESS. Under
__cdecl every `ret` in that chain is the right size, so abicheck passes it and
aliascheck never sees it: no /alternatename is involved.

THE RULE: for every extern "C" symbol, every declaration's parameter count
must equal the definition's. A disagreement is a dropped or an invented
argument, and the DROPPED direction is the dangerous one -- the callee reads a
stack slot the caller never wrote.

    DROPS    declaration has FEWER parameters than the definition. The callee
             reads uninitialised stack. When the definition's first parameter
             is the receiver, this is the receiver-dropping class in source
             form rather than in linker-directive form.
    INVENTS  declaration has MORE. Under __cdecl the caller cleans up, so the
             extra push is wasted rather than fatal; it usually means the
             matched ROM body genuinely takes no receiver and a host thunk is
             passing one anyway.

THE FULL CENSUS DOES NOT GATE, AND THAT IS A MEASURED DECISION RATHER THAN
TIMIDITY. The RULE is sound and its named instances are real. The
IMPLEMENTATION is a regular-expression reader over C and C++ text, and on this
tree it reports about two thousand disagreements, of which an unknown but
large fraction are parser artefacts: K&R-style spellings, macro-expanded
declarations, C++ overloads that share an Itanium prefix, and method
definitions inside class bodies. A gate at that number would be switched off
within a day, and a gate with a two-thousand-row baseline is not a gate.

ONE SUBSET DOES GATE, AND IT IS THE ONE THAT KEEPS RECURRING.

    THE RECEIVER SHAPE: a declaration with NO parameters at all, against a
    definition of an Itanium-mangled MEMBER name (_ZN...) that takes at least
    one. A member's receiver rides r0 on ARM like any other first argument, so
    a body transcribed from the ROM takes it as a parameter -- and a caller
    that spells the declaration `(void)` passes nothing and the callee reads
    the caller's return address as `this`.

    This is not a hypothetical subset. It is exactly what landed on cons twice
    on 2026-08-16, hours before this checker did:

      #1539  include/decl_FaderColor.h
             -extern void _ZN10FaderColor11AdvanceFadeEv(void);
             +extern void _ZN10FaderColor11AdvanceFadeEv(void*);
      #1543  src/ChainChomp_Spawn.cpp, ChiefChilly_Spawn.cpp, Wiggler_Spawn.c
             -void _ZN5EnemyC2Ev(void);          -> _ZN5EnemyC2Ev();
             +void _ZN5EnemyC2Ev(void*);         -> _ZN5EnemyC2Ev(c);

    Both were found by a person reading a fault. Both are in this subset, and
    both are ABSENT from it now, which is what makes it a usable ratchet. The
    live rows are frozen in aritycheck_receiver_baseline.txt; a row that is
    not in that file fails --gate-receiver. The list may only shrink.

    The frozen rows are DEBT, not clearance. Nobody has adjudicated them, and
    some will be parser artefacts. The ratchet's claim is narrow and true: the
    tree does not grow a NEW one without somebody seeing it.

WHAT WOULD MAKE THE FULL CENSUS GATE. Not a bigger regex. The honest route is
to read the arity out of the emitted code -- the callers' push counts, the way
abicheck reads emitted rets -- rather than out of the text. Until someone does
that, the two-thousand number is a pointer, not a verdict.

    python port/tools/aritycheck.py [repo-root]        census, always exit 0
    python port/tools/aritycheck.py --drops-only       the receiver subset
    python port/tools/aritycheck.py --gate-receiver    ratchet, exit 1 on new
    python port/tools/aritycheck.py --selftest
    python port/tools/aritycheck.py --json out.json
"""

import argparse
import json
import os
import re
import sys
from collections import defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_ROOT = os.path.dirname(os.path.dirname(HERE))
DIRS = ('src', 'include', os.path.join('port', 'unmatched'),
        os.path.join('port', 'hal'))
RECEIVER_BASELINE = os.path.join(HERE, 'aritycheck_receiver_baseline.txt')

# a definition: <ret> name(params) {
DEFN = re.compile(
    r'^[ \t]*(?:extern\s+"C"\s+)?(?:static\s+)?'
    r'(?:const\s+)?[A-Za-z_]\w*(?:\s*\*)*[\s*]+'
    r'(?P<name>_ZN\w+|func_\w+|[A-Za-z_]\w*)\s*\((?P<p>[^;{)]*)\)\s*\{', re.M)
# a declaration: <ret> name(params) ;
DECL = re.compile(
    r'^[ \t]*(?:extern\s+(?:"C"\s+)?)?'
    r'(?:const\s+)?[A-Za-z_]\w*(?:\s*\*)*[\s*]+'
    r'(?P<name>_ZN\w+|func_\w+)\s*\((?P<p>[^;{)]*)\)\s*;', re.M)

KEYWORDS = {'if', 'for', 'while', 'switch', 'return', 'else', 'sizeof', 'do'}


def nparams(p):
    p = p.strip()
    if p in ('', 'void'):
        return 0
    depth, cur, out = 0, '', []
    for ch in p:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        if ch == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    out.append(cur)
    return len([x for x in out if x.strip()])


def strip_comments(src):
    src = re.sub(r'/\*.*?\*/', '', src, flags=re.S)
    return re.sub(r'//[^\n]*', '', src)


def scan(root):
    defs, decls = {}, defaultdict(list)
    for d in DIRS:
        for dirpath, _, files in os.walk(os.path.join(root, d)):
            for fn in sorted(files):
                if not fn.endswith(('.c', '.cpp', '.h')):
                    continue
                p = os.path.join(dirpath, fn)
                try:
                    src = open(p, encoding='utf-8', errors='replace').read()
                except OSError:
                    continue
                rel = os.path.relpath(p, root).replace(os.sep, '/')
                src = strip_comments(src)
                for m in DEFN.finditer(src):
                    n = m.group('name')
                    if n in KEYWORDS:
                        continue
                    defs.setdefault(n, (nparams(m.group('p')), rel,
                                        src.count('\n', 0, m.start()) + 1))
                for m in DECL.finditer(src):
                    n = m.group('name')
                    if n in KEYWORDS:
                        continue
                    decls[n].append((nparams(m.group('p')), rel,
                                     src.count('\n', 0, m.start()) + 1))
    rows = []
    for name, (dn, dfile, dline) in sorted(defs.items()):
        for cn, cfile, cline in decls.get(name, []):
            if cn == dn:
                continue
            rows.append(dict(sym=name, def_n=dn, def_file=dfile,
                             def_line=dline, decl_n=cn, decl_file=cfile,
                             decl_line=cline,
                             kind='DROPS' if cn < dn else 'INVENTS',
                             # the receiver-dropping shape specifically: a
                             # declaration with NO parameters against a
                             # definition whose first one is the receiver
                             receiver=(cn == 0 and dn >= 1
                                       and name.startswith('_ZN'))))
    return defs, decls, rows


def receiver_key(r):
    """A ratchet key that survives edits above the row.

    Symbol plus the DECLARING file, never the line number: a row keyed by
    line moves every time somebody adds a comment, and a baseline that churns
    is a baseline nobody trusts. Two declarations of the same symbol in one
    file collapse to one key, which is the right resolution here -- fixing
    the file fixes both.
    """
    return '%s|%s' % (r['sym'], r['decl_file'])


def load_receiver_baseline(path=RECEIVER_BASELINE):
    rows = set()
    if os.path.isfile(path):
        with open(path, 'r', encoding='utf-8') as f:
            for ln in f:
                ln = ln.split('#', 1)[0].strip()
                if ln:
                    rows.add(ln)
    return rows


def selftest():
    bad = 0
    print('aritycheck --selftest')
    cases = [
        ('int,int', 2, 'two parameters'),
        ('void', 0, 'explicit void'),
        ('', 0, 'empty list'),
        ('void *self', 1, 'one pointer'),
        ('void *self, struct A (*cb)(int,int), int n', 3,
         'a function-pointer parameter is ONE parameter'),
        ('T<int,int> a, int b', 2, 'a template argument list is not a comma'),
    ]
    print('\n  PARAMETER COUNTING, %d fixtures' % len(cases))
    for text, want, note in cases:
        got = nparams(text)
        ok = got == want
        bad += 0 if ok else 1
        print('    %-4s want %-3s got %-3s  %s'
              % ('ok' if ok else 'FAIL', want, got, note))

    print('\n  DECL/DEFN RECOGNITION')
    sample = ('extern void _ZN9ActorBase18MarkForDestructionEv(void);\n'
              'void _ZN9ActorBase18MarkForDestructionEv(void *self)\n'
              '{ return; }\n'
              '/* void _ZN4Fake4CommentEv(void); */\n')
    s = strip_comments(sample)
    d = [m.group('name') for m in DECL.finditer(s)]
    f = [m.group('name') for m in DEFN.finditer(s)]
    ok = d == ['_ZN9ActorBase18MarkForDestructionEv'] and \
        f == ['_ZN9ActorBase18MarkForDestructionEv']
    bad += 0 if ok else 1
    print('    %-4s one declaration, one definition, comment ignored '
          '(decls=%s defns=%s)' % ('ok' if ok else 'FAIL', d, f))

    print('\n%s' % ('SELFTEST PASSED' if not bad
                    else 'SELFTEST FAILED (%d)' % bad))
    return 1 if bad else 0


def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('root', nargs='?', default=DEFAULT_ROOT)
    ap.add_argument('--drops-only', action='store_true',
                    help='print the RECEIVER subset rather than everything')
    ap.add_argument('--gate-receiver', action='store_true',
                    help='exit 1 on a RECEIVER-shape row that is not in '
                         'aritycheck_receiver_baseline.txt')
    ap.add_argument('--write-receiver-baseline', action='store_true',
                    help='rewrite the baseline body from this tree. Only ever '
                         'correct when the rows have been looked at.')
    ap.add_argument('--selftest', action='store_true')
    ap.add_argument('--json', metavar='PATH')
    ap.add_argument('--limit', type=int, default=40,
                    help='rows to print (0 for all); the census counts are '
                             'always complete')
    args = ap.parse_args(argv[1:])

    if args.selftest:
        return selftest()

    root = os.path.abspath(args.root)
    defs, decls, rows = scan(root)
    drops = [r for r in rows if r['kind'] == 'DROPS']
    invents = [r for r in rows if r['kind'] == 'INVENTS']
    recv = [r for r in rows if r['receiver']]

    if args.write_receiver_baseline:
        keys = sorted({receiver_key(r) for r in recv})
        with open(RECEIVER_BASELINE, 'a', encoding='utf-8') as f:
            for k in keys:
                f.write(k + '\n')
        print('appended %d receiver-shape keys to %s'
              % (len(keys), RECEIVER_BASELINE))
        return 0

    print('aritycheck -- cross-TU arity disagreement census')
    print('  %d definitions, %d declarations of _ZN*/func_* symbols'
          % (len(defs), sum(len(v) for v in decls.values())))
    print('  %d disagreements: %d DROPS, %d INVENTS'
          % (len(rows), len(drops), len(invents)))
    print('  %d of the DROPS are the RECEIVER shape (declared with no '
          'parameters at all against an _ZN member definition that takes '
          'one)' % len(recv))
    print('  The full census is REPORT ONLY. See the header for why.')

    if args.gate_receiver:
        base = load_receiver_baseline()
        keys = {receiver_key(r) for r in recv}
        new = sorted(keys - base)
        gone = sorted(base - keys)
        print('\n--- RECEIVER RATCHET (%s) ---'
              % os.path.basename(RECEIVER_BASELINE))
        print('  %d baselined, %d live, %d NEW, %d retired'
              % (len(base), len(keys), len(new), len(gone)))
        for k in gone:
            print('  retired (delete the row): %s' % k)
        if new:
            print('\n  NEW RECEIVER-SHAPE ROWS -- a declaration takes no '
                  'parameters where the definition takes a receiver:')
            for r in sorted(recv, key=lambda r: receiver_key(r)):
                if receiver_key(r) not in base:
                    print('    %s' % r['sym'])
                    print('        declared %d param(s) at %s:%d'
                          % (r['decl_n'], r['decl_file'], r['decl_line']))
                    print('        DEFINED  %d param(s) at %s:%d'
                          % (r['def_n'], r['def_file'], r['def_line']))
            print('\nARITYCHECK RECEIVER RATCHET FAILED: %d new row(s). A '
                  'member\'s receiver rides r0 on ARM like any other first '
                  'argument; declare it and pass it, the way #1539 and #1543 '
                  'did on 2026-08-16.' % len(new))
            return 1
        print('\nARITYCHECK RECEIVER RATCHET PASSED: no new receiver-shape '
              'row. The %d baselined rows are unadjudicated DEBT, not '
              'clearance.' % len(base))

    show = (recv if args.drops_only else rows)
    lim = len(show) if args.limit == 0 else min(args.limit, len(show))
    if args.gate_receiver:
        # A green ratchet has already said everything worth saying; dumping
        # the frozen rows behind it turns a one-line pass into 500 lines of
        # scroll nobody reads, and a gate nobody reads is not a gate.
        lim = 0
    if lim:
        print('\n--- %d of %d rows ---' % (lim, len(show)))
    for r in show[:lim]:
        print('  %-8s %s%s' % (r['kind'], r['sym'],
                               '   [RECEIVER SHAPE]' if r['receiver'] else ''))
        print('      declared %d param(s) at %s:%d'
              % (r['decl_n'], r['decl_file'], r['decl_line']))
        print('      DEFINED  %d param(s) at %s:%d'
              % (r['def_n'], r['def_file'], r['def_line']))

    if args.json:
        with open(args.json, 'w', encoding='utf-8') as f:
            json.dump(rows, f, indent=1)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
