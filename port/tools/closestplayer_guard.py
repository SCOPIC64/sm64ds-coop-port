"""Build time guard against receiver dropping thiscall readers in the port.

Some src/ readers were decompiled as free functions that call an Actor
thiscall with empty parentheses, for example

    char* p = _ZN5Actor13ClosestPlayerEv();

Actor::ClosestPlayer is a thiscall: it reads `this` from the receiver
register. A zero argument call leaves that register holding whatever was
there, so on the host it dereferences a null base (null this + 0x5c) and
faults. The live readers were host copied to pass their own receiver (see
port/unmatched/Actor_ClosestPlayer_OverlayReaders.cpp and
Actor_ClosestPlayerWrappers.cpp). More raw readers still sit in src/ but
their overlays are not mounted in any slice gate yet, so they are latent.
A source comment cannot warn the future host author, because there is no
arrival line for a file that no gate compiles.

This guard turns that hope into a build failure. It reads the same
slice_gate*.txt files CMakeLists.txt reads, collects every src/ TU those
gates activate, and scans each one for a zero argument call to a known
receiver dropping symbol. If a gate ever activates such a TU without host
copying it first, the build stops here with the fix spelled out.

It is a build time check, wired ahead of cmake configure in build-port.cmd.

LIMITATION: the guard only sees TUs activated through slice_gate*.txt. A TU
added straight into CMakeLists.txt (a hardcoded add_executable / list append
line, or a generated symbol list) would not pass through a gate, so the
guard would not see it. Slice gates are the normal way TUs enter the build,
so this covers the normal path; a direct CMake edit that hosts a raw reader
would need its own review.
"""

import os
import re
import sys

# One row per receiver dropping symbol. Add a row here when a new thiscall
# reader that drops its receiver is found. Each row is:
#   symbol       the mangled name, for the human reading the report
#   pattern      a compiled regex matching the ZERO ARGUMENT call form. It
#                must match `SYMBOL()` (optional whitespace inside the
#                parens) and must NOT match the `(void)` declaration or any
#                call that passes an argument.
#   remedy       the fix text printed when this row finds an offender.
RULES = [
    {
        "symbol": "_ZN5Actor13ClosestPlayerEv",
        "pattern": re.compile(r"_ZN5Actor13ClosestPlayerEv\s*\(\s*\)"),
        "remedy": (
            "Actor::ClosestPlayer is thiscall. A zero argument call drops the "
            "receiver and faults on a null base (null this + 0x5c). Host copy "
            "this TU to pass its own receiver (see "
            "port/unmatched/Actor_ClosestPlayer_OverlayReaders.cpp) and comment "
            "the raw src out of its slice gate. Per seat gate item 4: a newly "
            "hosted overlay carrying a known raw reader."
        ),
    },
]


def port_root():
    """The port/ directory, resolved from this file, no absolute paths."""
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def repo_root():
    """The repo root, one level above port/."""
    return os.path.dirname(port_root())


def active_src_files(port_dir, repo_dir):
    """Every src/ TU that a slice gate activates.

    Mirrors the CMakeLists.txt reader exactly: strip the line, skip blank
    lines and lines that start with '#', treat the rest as a path relative to
    the repo root. Keep only the ones under src/ that exist on disk.
    """
    seen = set()
    ordered = []
    gate_names = sorted(
        n for n in os.listdir(port_dir)
        if n.startswith("slice_gate") and n.endswith(".txt")
    )
    for name in gate_names:
        gate_path = os.path.join(port_dir, name)
        with open(gate_path, "r", encoding="utf-8", errors="replace") as fh:
            for raw in fh:
                line = raw.strip()
                if not line or line.startswith("#"):
                    continue
                # Normalize slashes so src\ and src/ both resolve.
                rel = line.replace("\\", "/")
                if not rel.startswith("src/"):
                    continue
                abs_path = os.path.normpath(os.path.join(repo_dir, rel))
                if abs_path in seen:
                    continue
                seen.add(abs_path)
                if os.path.isfile(abs_path):
                    ordered.append((rel, abs_path))
    return ordered


def scan_file(rel, abs_path):
    """Return a list of (rel, lineno, remedy) offenders in one TU."""
    offenders = []
    with open(abs_path, "r", encoding="utf-8", errors="replace") as fh:
        for lineno, text in enumerate(fh, start=1):
            for rule in RULES:
                if rule["pattern"].search(text):
                    offenders.append((rel, lineno, rule["symbol"], rule["remedy"]))
    return offenders


def main():
    port_dir = port_root()
    repo_dir = repo_root()
    files = active_src_files(port_dir, repo_dir)

    offenders = []
    for rel, abs_path in files:
        offenders.extend(scan_file(rel, abs_path))

    if not offenders:
        print(
            "closestplayer_guard OK: no zero argument receiver dropping calls "
            "in {} slice gate active src TUs.".format(len(files))
        )
        return 0

    for rel, lineno, symbol, _remedy in offenders:
        print("{}:{}: zero-argument Actor::ClosestPlayer call".format(rel, lineno))

    # Print each distinct remedy once, so a multi row future stays readable.
    print("")
    print("REMEDY")
    printed = set()
    for _rel, _lineno, symbol, remedy in offenders:
        if symbol in printed:
            continue
        printed.add(symbol)
        print("  {}: {}".format(symbol, remedy))
    return 1


if __name__ == "__main__":
    sys.exit(main())
