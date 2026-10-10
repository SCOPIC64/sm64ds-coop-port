#!/usr/bin/env python3
"""Adapt the flat constructor call without editing the matched source.

The flat game ABI expects a constructor to return its receiver. SysV C++
constructors return void, so callers must use an explicit returning bridge.
"""
import argparse
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    options = parser.parse_args()
    source = options.root / 'src/_ZN4Heap28CreateExpandingHeapAllocatorEPvjj.cpp'
    text = source.read_text(encoding='utf-8')
    name = '_ZN22ExpandingHeapAllocatorC1EPvj'
    if text.count(name) != 2:
        raise ValueError('Constructor declaration/call changed; review the native bridge')
    options.out.parent.mkdir(parents=True, exist_ok=True)
    options.out.write_text('// Host-only generated source; the matched file is untouched.\n'
        + text.replace(name, 'port_native_expanding_heap_ctor'), encoding='utf-8')


if __name__ == '__main__':
    main()
