#!/usr/bin/env python3
"""Rewrite sm64ds-decomp sources for the host build.

The decomp's `src/` is byte-verified against the ROM and must not change. This
reads it and emits a transformed copy into build/host-src/, so the port never
edits the matched tree. See notes/assessment.md section 0c.

Currently rewrites direct MMIO cast-derefs:

    *(volatile unsigned short *)0x4000280      ->  NTR_MMIO(unsigned short, 0x4000280)

which routes the access through include/ntr/mmio.h so that write-triggered
registers run their side effect.

It also rewrites registers reached through a POINTER BOUND TO A LITERAL:

    volatile int *p = (volatile int *)0x400046c;
    *p = x;                                    ->  NTR_MMIO(int, 0x400046c) = x

The header used to say those were harmless because the host maps real memory
at the DS I/O addresses. They are not. func_0204488c -- the ordinary part
walk, and the one place a model's scale is spent -- writes MTX_SCALE through
exactly that shape, so every scale in the render walk latched into dead memory
and the geometry engine never saw it: the level lost its BMD shift and every
actor lost the Vector3 scale its Render asked for. See MMIO_PTR below.

    python tools/hostgen.py --decomp ../sm64ds-decomp _ZN4cstd3divEii
    python tools/hostgen.py --decomp ../sm64ds-decomp --all
"""

import argparse
import pathlib
import re
import sys

# *(volatile TYPE *)0xADDR   ->  NTR_MMIO(TYPE, 0xADDR)
# TYPE is a plain builtin spelling; the decomp never casts to anything else
# here. `volatile` is optional: matched code also reaches registers through
# plain casts (the material bind stores POLYGON_ATTR/TEXIMAGE without it),
# and any literal-address deref in the 0x04xxxxxx window is MMIO by
# definition.
# Two textual shapes: `*(T *)0xADDR` and `*((T *)0xADDR)`. The conditional
# group consumes the trailing paren only when the extra leading one matched,
# so computed addresses like `*(volatile u32 *)(0xADDR + x)` are left alone
# (they resolve through the mapped latch window, which is correct for reads).
MMIO_DEREF = re.compile(
    r"\*\s*(\()?\s*\(\s*(?:volatile\s+)?"
    r"((?:unsigned|signed|long|short|int|char|u8|u16|u32|u64|s8|s16|s32|s64)"
    r"(?:\s+(?:unsigned|signed|long|short|int|char))*)"
    r"\s*\*\s*\)\s*(0x0?4[0-9A-Fa-f]{6})\b(?(1)\s*\))"
)

# A REGISTER REACHED THROUGH A POINTER, which the cast-deref rewrite above
# cannot see. The decomp writes this whenever mwccarm kept the address in a
# register across a block:
#
#     volatile int *mtxScale = (volatile int *)0x400046c;   func_0204488c
#     ...
#     *mtxScale = c0;                                       MTX_SCALE, x3
#
#     volatile unsigned int *r2b8 = (volatile unsigned int *)0x40002b8;
#     r2b8[0] = 0;  r2b8[1] = n;                            func_02053008
#
# Both forms latch into mapped memory and trigger nothing, which is silent and
# total: the geometry engine simply never receives the command.
#
# The rewrite is deliberately narrow, because a pointer is a pointer and this
# transform is textual. A name qualifies only when
#   * it is bound EXACTLY ONCE, from a literal cast in the 0x04xxxxxx window,
#   * every other mention of it is `*name` or `name[<decimal literal>]`, and
#   * nothing else -- no address-of, no passing it along, no arithmetic.
# Anything outside that and the name is left completely alone; the port keeps
# the behaviour it had (a latch that does not trigger) rather than gaining a
# rewrite the tool cannot justify. The binding statement itself stays: it
# becomes an unused local, which is cheaper to read than a hole in the source.
MMIO_TYPE = (r"(?:unsigned|signed|long|short|int|char|"
             r"u8|u16|u32|u64|s8|s16|s32|s64)"
             r"(?:\s+(?:unsigned|signed|long|short|int|char))*")
MMIO_BIND = re.compile(
    r"\b(\w+)\s*=\s*\(\s*(?:volatile\s+)?(?:const\s+)?(" + MMIO_TYPE + r")"
    r"\s*\*\s*\)\s*(0x0?4[0-9A-Fa-f]{6})\s*;")
MMIO_WIDTH = {
    "char": 1, "signed char": 1, "unsigned char": 1, "u8": 1, "s8": 1,
    "short": 2, "short int": 2, "signed short": 2, "unsigned short": 2,
    "unsigned short int": 2, "u16": 2, "s16": 2,
    "int": 4, "signed": 4, "signed int": 4, "unsigned": 4, "unsigned int": 4,
    "long": 4, "long int": 4, "unsigned long": 4, "u32": 4, "s32": 4,
    "long long": 8, "signed long long": 8, "unsigned long long": 8,
    "unsigned long long int": 8, "u64": 8, "s64": 8,
}


def mmio_ptr(text):
    """Rewrite derefs of names bound to a literal register address."""
    binds = {}
    for m in MMIO_BIND.finditer(text):
        name = m.group(1)
        ctype = " ".join(m.group(2).split())
        binds[name] = None if name in binds else (ctype, int(m.group(3), 16))

    edits = []
    for name, info in binds.items():
        if info is None or info[0] not in MMIO_WIDTH:
            continue
        ctype, addr = info
        width = MMIO_WIDTH[ctype]
        # spans this pass must not touch: the declarator and the binding.
        skip = [m.span() for m in re.finditer(
            r"(?:(?:volatile|const)\s+)*(?:" + MMIO_TYPE + r")\s*\*\s*" +
            re.escape(name) + r"\b", text)]
        skip += [m.span() for m in MMIO_BIND.finditer(text)
                 if m.group(1) == name]
        mine = []
        ok = True
        for m in re.finditer(r"\b" + re.escape(name) + r"\b", text):
            if any(a <= m.start() < b for a, b in skip):
                continue
            d = re.match(r"\s*\[\s*(\d+)\s*\]", text[m.end():])
            if d:
                mine.append((m.start(), m.end() + d.end(),
                             addr + int(d.group(1)) * width))
                continue
            s = re.search(r"(?<![)\]\w])\*\s*$", text[:m.start()])
            if s:
                mine.append((s.start(), m.end(), addr))
                continue
            ok = False
            break
        if ok and mine:
            edits += [(a, b, f"NTR_MMIO({ctype}, {c:#x})") for a, b, c in mine]

    for a, b, rep in sorted(edits, reverse=True):
        text = text[:a] + rep + text[b:]
    return text, len(edits)


HEADER = """// GENERATED by tools/hostgen.py from {src}
// Do not edit. The source of truth is the byte-verified decomp; edit the
// transform or the host I/O layer instead.
#include "ntr/mmio.h"

"""


# mwccarm allows void*+int (byte arithmetic, the pret void*-arith idiom);
# MSVC C++ rejects it. Retype the pointee to char* only where the deref
# feeds an addition -- the value is identical, the arithmetic becomes legal.
# Two spellings reach here and they park their parentheses differently:
#     (*((void **) obj)) + 0x18                    -- func_02044b30
#     *(void**)(c + id * 4 + 0xdc) + 0x50          -- func_ov002_020c897c
# The first wraps the deref and leaves the operand bare; the second wraps
# the operand and leaves the deref bare. Same rewrite either way, and the
# operand is substituted VERBATIM, parens and all -- dropping them turns
# *(void**)(c + 0x160) into *((char **) c + 0x160), which is a scaled
# index, not the same address.
#
# The second spelling needs the lookbehind. Casting the deref before the
# addition is already legal C++, and the decomp writes that far more often
# than the bare form:
#     (char*)*(void**)(c + 0x160) + 0x58     legal, leave it alone
#            *(void**)(c + 0x160) + 0x58     void* arithmetic, rewrite
# The only textual difference is what sits in front of the `*`, so a cast's
# closing paren (and a preceding identifier or subscript, which would make
# the `*` a multiply) blocks the match.
VOIDPP_ARITH = re.compile(
    r"\(\s*\*\s*\(\s*\(\s*void\s*\*\*\s*\)\s*([^()]+?)\s*\)\s*\)\s*\+"
    r"|(?<![)\]\w])\*\s*\(\s*void\s*\*\*\s*\)\s*(\([^()]+?\))\s*\+")


def voidpp_char(m):
    """Both alternatives land on the same char** deref."""
    return "((*((char **) %s))) +" % (m.group(1) or m.group(2))

# GCC-style attributes (long_call, target("thumb")) mean nothing to MSVC.
# One nesting level inside the (( )) is enough for every use in src/.
ATTRIBUTE = re.compile(r"__attribute__\s*\(\((?:[^()]|\([^()]*\))*\)\)")

# ENGINE BSS: `int data_0209b458;` at file scope.
#
# mwccarm and MSVC's C front end both make that a TENTATIVE definition, which
# the linker merges with whoever really owns the storage -- which is how the
# port's .c slices coexist with hal/auto_bss.cpp. MSVC's C++ front end does
# not: in a .cpp it is a strong definition, and the same symbol in the HAL is
# LNK2005. The port's rule is that engine BSS is the HAL's (real sizes live
# there; a src file's declared width is whatever that one function needed --
# Actor::BeforeBehavior spells the 0x5c-byte Clipper `char data_0209f43c;`),
# so the src-side definition becomes a declaration and the HAL keeps the
# storage. Opt-in per file: --extern-data, because a src file CAN be the
# intended owner and this rewrite would silently unhome it.
#
# Only `data_<hex>` names are touched, and only a plain scalar/array
# definition with no initializer.
EXTERN_DATA = re.compile(
    r"^([ \t]*)((?:(?:unsigned|signed|volatile|const|struct|long|short|int|char|"
    r"float|double|u8|u16|u32|u64|s8|s16|s32|s64|bool|Vector3|Matrix4x3|"
    r"[A-Z]\w*)[ \t]+)+\**[ \t]*)(data_(?:ov\d+_)?[0-9a-f]{6,8})"
    r"([ \t]*(?:\[[^\];=]*\])?)[ \t]*;",
    re.MULTILINE)


def transform(text, extern_data=False):
    """Return (new_text, n_rewrites)."""
    text, n3 = ATTRIBUTE.subn("", text)
    text, n2 = VOIDPP_ARITH.subn(voidpp_char, text)
    text, n1 = MMIO_DEREF.subn(lambda m: f"NTR_MMIO({m.group(2).strip()}, {m.group(3)})", text)
    text, n5 = mmio_ptr(text)
    n4 = 0
    if extern_data:
        text, n4 = EXTERN_DATA.subn(r"\1extern \2\3\4;", text)
    return text, n1 + n2 + n3 + n4 + n5


# ~110 files in the decomp are ARM assembly blocks -- CP15 cache ops, the CRT0,
# SWI wrappers, context switches. They are matched and correct, and they are also
# the one thing a host compiler can never consume. They are not transformed; they
# are reimplemented by hand in src/port/runtime.cpp. This is the port's shim
# surface, and keeping it enumerable is the point.
ASM_BLOCK = re.compile(r"^\s*(?:extern\s+\"C\"\s+)?(?:__)?asm\s", re.MULTILINE)


def is_asm(text):
    return bool(ASM_BLOCK.search(text))


# A shared header can declare a symbol with a different pointer parameter type
# than the TU that defines it uses. That was invisible while the decl_*.h
# headers were C++-mangled, because the two spellings were simply two different
# symbols; once main gave those headers C linkage (the 2026-08-03 sweep) both
# become the same extern "C" name and MSVC rejects the file outright:
#
#   include/decl_common.h  extern int func_ov002_020cfbdc(void*);
#   src/..._020cfbdc.cpp   extern "C" int func_ov002_020cfbdc(char *self)
#   -> error C2733: you cannot overload a function with 'extern "C"' linkage
#
# Neither side is wrong about the ROM -- void* and char* are one register --
# and neither src/ nor include/ may be edited for the port. So the emitted copy
# shadows the NAME across the header's include, which leaves the header
# declaring a dead alias and the TU's own definition untouched. One entry per
# symbol, listed rather than pattern-matched, so a new collision has to be
# looked at rather than silently absorbed.
HEADER_SHADOW = {
    "func_ov002_020cfbdc": "decl_common.h",
}


def shadow_header_decl(text, sym, header):
    """Hide the shared header's declaration of `sym` while it is included."""
    inc = '#include "%s"' % header
    if inc not in text:
        return text, 0
    return text.replace(
        inc,
        "#define %s %s__hdrshadow\n%s\n#undef %s" % (sym, sym, inc, sym),
        1), 1


def emit(src_path, out_dir, decomp_root, extern_data=False):
    text = src_path.read_text(encoding="utf-8", errors="replace")
    # The decomp marks C++ files with a leading `//cpp` line; the host build
    # compiles everything as C++ anyway, so drop it.
    text = re.sub(r"\A//cpp[^\n]*\n", "", text)
    sym = src_path.stem
    if sym in HEADER_SHADOW:
        text, _ = shadow_header_decl(text, sym, HEADER_SHADOW[sym])
    new, n = transform(text, extern_data)
    # Everything is emitted as C++ (NTR_MMIO expands to a template proxy), but
    # a .c source's symbols must keep C linkage: the port's other slices
    # compile the decomp's .c files as real C, and mixing linkages per-symbol
    # is exactly the bug class the HAL bridges exist to catch. The decomp's
    # own headers are C-only typedefs, so including them inside the wrap is
    # sound.
    if src_path.suffix == ".c":
        new = 'extern "C" {\n' + new + '\n}  /* extern "C" (hostgen: .c source) */\n'
    rel = src_path.relative_to(decomp_root)
    out = out_dir / rel.with_suffix(".cpp")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(HEADER.format(src=rel.as_posix()) + new, encoding="utf-8")
    return out, n


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("symbols", nargs="*", help="symbol names to transform (no extension)")
    # This tool lives at port/tools/hostgen.py inside the decomp itself.
    ap.add_argument("--decomp",
                    default=str(pathlib.Path(__file__).resolve().parents[2]))
    ap.add_argument("--out", default="build/host-src")
    ap.add_argument("--all", action="store_true", help="transform every file in src/")
    ap.add_argument("--extern-data", action="store_true",
                    help="turn file-scope data_XXXXXXXX definitions into "
                         "declarations (engine BSS is the HAL's -- see "
                         "EXTERN_DATA)")
    args = ap.parse_args()

    decomp = pathlib.Path(args.decomp).expanduser().resolve()
    src = decomp / "src"
    if not src.is_dir():
        sys.exit(f"no src/ under {decomp} -- pass --decomp")
    out_dir = pathlib.Path(args.out)

    if args.all:
        targets = [p for p in src.rglob("*") if p.suffix in (".c", ".cpp")]
    else:
        targets = []
        for name in args.symbols:
            hit = next((p for ext in (".c", ".cpp")
                        if (p := src / f"{name}{ext}").exists()), None)
            if hit is None:
                sys.exit(f"not found in decomp src/: {name}")
            targets.append(hit)
        if not targets:
            sys.exit("nothing to do -- pass symbol names or --all")

    total = 0
    skipped = []
    for path in targets:
        text = path.read_text(encoding="utf-8", errors="replace")
        if is_asm(text):
            skipped.append(path.stem)
            if not args.all:
                print(f"{path.name:<44}   ARM asm -- needs a host implementation")
            continue
        out, n = emit(path, out_dir, decomp, args.extern_data)
        total += n
        if not args.all:
            print(f"{path.name:<44} {n:>3} MMIO rewrite(s) -> {out}")

    if args.all:
        print(f"{len(targets) - len(skipped):,} files transformed, "
              f"{total:,} MMIO rewrites -> {out_dir}")
        print(f"{len(skipped):,} ARM asm files skipped (the port's shim surface)")
        manifest = out_dir / "asm-shims.txt"
        manifest.parent.mkdir(parents=True, exist_ok=True)
        manifest.write_text("\n".join(sorted(skipped)) + "\n", encoding="utf-8")
        print(f"  -> {manifest}")


if __name__ == "__main__":
    main()
