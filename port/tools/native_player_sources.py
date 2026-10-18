#!/usr/bin/env python3
"""Generate native compiler adaptations without editing matched game sources."""
import argparse
from pathlib import Path


def adapt_hold_light(source: str) -> str:
    start = '    int var_r4 = 0;\n'
    end = '\nend:\n'
    if source.count(start) != 1 or source.count(end) != 1 or source.count('goto end;') != 1:
        raise ValueError('HoldLight control flow changed; review the native scope adaptation')
    # The early goto skips the later scalar initializer. ISO C++ rejects a
    # jump over initialization in the same scope. A nested block ends before
    # the shared epilogue, preserving exactly when var_r4 is initialized and
    # every original statement/branch. No constructor or expression is moved.
    return ('// Host-only native scope adaptation; matched source unchanged.\n'
            + source.replace(start, '    {\n' + start, 1).replace(end, '    }\n' + end, 1))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    source = args.root / 'src/_ZN6Player17St_HoldLight_MainEv.cpp'
    result = adapt_hold_light(source.read_text(encoding='utf-8'))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(result, encoding='utf-8')


if __name__ == '__main__':
    main()

