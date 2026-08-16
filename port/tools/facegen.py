#!/usr/bin/env python3
"""Generate the mechanical faces and aliases a slice's link wall asks for.

WHY THIS EXISTS. When a seat lane's link (or port/tools/closure.py) names its
unresolved externals, most of the wall is not judgment, it is transcription.
The MG1 pathfinder enumerated its wall as "30 ordinary /alternatename faces,
2 argument-landing faces, 7 pointer-to-member symbols"; the S1 spine wrote 24
faces in one round and 12 aliases in another; hal/actor_class_faces.cpp is a
whole file of the same shape written by hand. Every one of those rows is a
pure function of two mangled names, and lanes have been spending their
attention writing them one at a time.

WHAT IT EMITS, sorted from an unresolved-external list plus a universe of
available definitions (a map, a targets file, or both):

  ALIAS    a decorated DATA name over a hosted C symbol:
           #pragma comment(linker, "/alternatename:?d@@3PAHA=_d").
           Data only, plus __cdecl free functions onto a same-named C
           definition. NEVER a method: an alias cannot change a calling
           convention, the ov007 lane's law.
  FACE     a __thiscall method whose body is a matched C-name TU: the
           shadow-struct forwarder of hal/actor_class_faces.cpp, declared
           from the CALLER's mangle so the symbol reproduces exactly, in a
           file that includes NOTHING (that file's header says why).
  RFACE    the reverse: an unresolved Itanium C name whose body is a matched
           MSVC method (the TrackInDeathTable shape). Zero-argument methods
           only; a reverse face with arguments re-lands them and is written
           by hand against the cxxname_bridge precedent.

WHAT IT REFUSES, because a generator that guesses saves nothing (the law:
every generator carries its verifier, and judgment rows go to a human):

  * anything pointer-to-member typed (P8/Q8 in the TYPE TEXT, under any
    wrapping: P8 direct, PAP8 as an array) -- the gate-16 wall; a host
    copy, never a face. The boundary is the consumer-spelling law
    (port/mg_fanout_costs.txt section 4): a pair whose consumer spells it
    as two ints is safe as an alias, and a consumer naming a member-pointer
    type needs a host copy WHETHER IT IS CALLED OR ONLY COPIED, because the
    four-byte MSVC member pointer shifts every field after it;
  * struct-typed globals in ANY spelling -- @@3U by value AND @@3PAU as a
    pointer or array. The mangle cannot show the struct's members, and a
    struct wrapper hides a member-pointer table: @@3PAUEntry@@A slipped the
    first version's two guards and was the twenty-five-entry dScMgCurling_c
    state table (mg_fanout_costs.txt s10 finding 1). Where the tool cannot
    see the consumer it refuses with the reason rather than aliasing;
  * data spellings outside the scalar subset -- ALIAS is now a whitelist
    (scalars and pointer/array-of-scalar chains), not "anything that is not
    a struct". A spelling this tool has not proven safe is a refusal, not
    an alias;
  * namespace-scope free functions, both directions (the @@Y-after-a-
    qualifier spelling). Memory is a NAMESPACE, and the first version read
    ?Allocate@Memory@@YAPAXIH@Z as a method of class Memory and refused
    the Itanium side as "reverse face with 2 args" -- right refusal, wrong
    reason (s10 finding 4). Detected now and refused with the true one:
    when both sides are __cdecl with the same arguments a plain hand
    /alternatename is the whole fix (hal/scene_mg_faces.cpp section 2d is
    the ruling). Auto-emitting that alias was considered and DECLINED: it
    would need a cross-mangling parameter-type comparison, and the failure
    mode of getting one row wrong is an alias that links and misreads the
    stack. One row has ever needed it;
  * arity disagreements between the two mangles -- ModelBase::ApplyOpacity
    takes 2 on the caller's side and 1 in the ROM body, and the hand face
    that drops the argument was a RULING, not plumbing;
  * ambiguous joins (two candidates for one class::method), float/double
    signatures, struct-by-value returns (SRET is trap territory), templates,
    statics, non-thiscall conventions.

THE VERIFIER (--verify) IS MANDATORY, not an extra: no generated file is
wired into a slice without a PASS from it, and the fan-out briefs carry
that as a rule. It compiles the emitted file and reads the object back
with dumpbin: the defined-external set must equal the requested rows
EXACTLY and the undefined set must be a subset of the target universe.
That is the same check A1 ran on WallSign's Render by hand, and it is
load-bearing: a class-typed parameter spelled as a struct silently
produces a wrong-mangling face (MSVC mangles class V where struct is U,
proven in review), and nothing but the surface check catches it. What no
verifier checks is method_faces.cpp's failure mode 1 (a forward to a
plausible SIBLING): the generator's exact-name join refuses lookalike
siblings instead of choosing between them, and every emitted row still
gets the per-face checklist at review -- target, arity, receiver.

    python port/tools/facegen.py --unresolved wall.txt --map build/port/walk_window.map --out faces_gen.cpp --verify
    python port/tools/facegen.py --unresolved wall.txt --targets have.txt --out faces_gen.cpp --verify
    python port/tools/facegen.py --selftest
"""

import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import closure  # noqa: E402

SCALARS = {"int", "unsigned int", "short", "unsigned short", "char",
           "unsigned char", "signed char", "long", "unsigned long", "bool"}
REFUSED_SCALARS = {"float", "double", "__int64", "unsigned __int64"}

# (?!Y) is finding 4's fix: after a single qualifier, Y is "function", not a
# member access specifier, so ?Allocate@Memory@@YA... is a free function in
# NAMESPACE Memory and never was a method of a class Memory. MSVC_NSFREE
# catches exactly what MSVC_METHOD now excludes. Deeper nestings
# (?f@Inner@Outer@@...) match neither and fall to UNKNOWN, a safe refusal.
MSVC_METHOD = re.compile(r"^\?(\w+)@(\w+)@@(?!Y)[A-Z]")
MSVC_NSFREE = re.compile(r"^\?(\w+)@(\w+)@@Y([A-Z])")
MSVC_DATA = re.compile(r"^\?(\w+)@(?:(\w+)@)?@3(.+)$")
MSVC_FREE = re.compile(r"^\?(\w+)@@Y([A-Z])")
ITANIUM = re.compile(r"^_?(_ZNK?\d.*)$")

# The ALIAS whitelist for data spellings: scalars, and pointer/array-of-
# scalar chains ([PQ][AB] prefixes), with a trailing cv/storage letter.
# These are the spellings that CANNOT hide a member pointer -- the two-ints
# law's mechanical form. Everything else refuses: U/T structs in any
# wrapping (the members are invisible in the mangle), W4 enums (safe in
# principle, never seen on a wall, so not claimed), and anything this
# expression has no opinion on.
SAFE_DATA_TYPE = re.compile(r"^(?:[PQ][AB])*(?:[CDEFGHIJKMNX]|_[JKN])[A-D]$")


def member_ptr_typed(sym):
    """True when an MSVC mangle names a pointer-to-member TYPE anywhere in
    its type text -- P8/Q8 direct, or under any wrapping such as PAP8 (an
    array of member pointers). Only the text after the first '@@' is
    scanned, so an identifier that happens to contain 'P8' cannot
    false-positive; the first version scanned the whole symbol.
    R8/S8 (the cv-qualified member-pointer spellings) are NOT tested for
    because none has ever appeared on a wall; if one arrives it falls to
    the struct or whitelist refusals below, which is still a refusal."""
    parts = sym.split("@@", 1)
    return len(parts) == 2 and ("P8" in parts[1] or "Q8" in parts[1])


# ---------------------------------------------------------------------------
# name parsing

def itanium_parse(name):
    """_ZN5Actor9UpdatePosEP12CylinderClsn -> (cls, meth, const, arity).

    Returns None for anything outside the two-component non-structor shape:
    nested namespaces, ctors/dtors and operators are not face material.
    Arity counting handles the subset the repo's mangles use -- builtins,
    P/R/K prefixes, <len>Name types with one template block, S substitutions.
    Anything else returns arity -1 and the row is refused rather than
    miscounted.
    """
    m = re.match(r"^_ZN(K?)(\d.*)$", name)
    if not m:
        return None
    is_const = m.group(1) == "K"
    comps, rest = [], m.group(2)
    while rest and rest[0].isdigit():
        n = re.match(r"(\d+)", rest)
        length = int(n.group(1))
        body = rest[n.end():n.end() + length]
        if len(body) != length:
            return None
        comps.append(body)
        rest = rest[n.end() + length:]
    if not rest.startswith("E") or len(comps) != 2:
        return None
    cls, meth = comps
    if re.match(r"^[CD][0-3]$", meth) or not meth[0].isalpha():
        return None
    return cls, meth, is_const, itanium_arity(rest[1:])


def itanium_arity(params):
    if params == "v":
        return 0
    i, n = 0, 0
    while i < len(params):
        c = params[i]
        if c in "PRK":
            i += 1
            continue
        if c.isdigit():
            m = re.match(r"(\d+)", params[i:])
            length = int(m.group(1))
            i += m.end() + length
            if i < len(params) and params[i] == "I":
                depth, i = 1, i + 1
                while i < len(params) and depth:
                    if params[i] == "I":
                        depth += 1
                    elif params[i] == "E":
                        depth -= 1
                    i += 1
            n += 1
            continue
        if c == "S":
            j = params.find("_", i)
            if j < 0:
                return -1
            i = j + 1
            n += 1
            continue
        if c in "abcdefghijlmnostuvwxyz":
            i += 1
            n += 1
            continue
        return -1
    return n


def undname_batch(symbols):
    """{decorated: undecorated} via the toolchain's own undname.exe."""
    if not symbols:
        return {}
    vcvars = closure.find_vcvars()
    if vcvars is None:
        return {}
    with tempfile.TemporaryDirectory() as td:
        runner = pathlib.Path(td) / "und.cmd"
        outp = pathlib.Path(td) / "und.txt"
        lines = ["@echo off", 'call "%s" >nul' % vcvars]
        for s in symbols:
            lines.append('undname "%s" >> "%s"' % (s, outp))
        runner.write_text("\r\n".join(lines) + "\r\n")
        subprocess.run(["cmd", "/c", str(runner)], capture_output=True)
        text = outp.read_text(errors="replace") if outp.exists() else ""
    result, key = {}, None
    for line in text.splitlines():
        m = re.match(r'Undecoration of :- "(.+)"', line)
        if m:
            key = m.group(1)
        m = re.match(r'is :- "(.+)"', line)
        if m and key:
            result[key] = m.group(1)
            key = None
    return result


SIG = re.compile(
    r"^(?:public|protected|private): "
    r"(virtual )?(.+?)__thiscall (\w+)::(\w+)\((.*)\)(const )?\s*$")


def parse_param(text):
    """One undname param -> (kind, spelling, classname).

    kind: 'scalar' | 'ptr' | 'ref'; classname set for struct/class types.
    None means the row must be refused (the type is beyond the mechanical
    subset -- float, function pointer, template, by-value struct).
    """
    t = text.strip()
    if "<" in t or "(" in t:
        return None
    is_const = False
    m = re.match(r"^(?:struct|class) (\w+) (const )?([*&])$", t)
    if m:
        cls, is_const = m.group(1), bool(m.group(2))
        kind = "ptr" if m.group(3) == "*" else "ref"
        return (kind, cls, is_const)
    m = re.match(r"^(?:const )?(?:struct|class) (\w+) ([*&])$", t)
    if m:
        return ("ptr" if m.group(2) == "*" else "ref", m.group(1),
                t.startswith("const "))
    if t in SCALARS:
        return ("scalar", t, False)
    if t in ("void *", "void const *", "const void *"):
        return ("ptr", "void", "const" in t)
    return None


def parse_msvc_sig(und):
    """undname text -> dict or a refusal string."""
    m = SIG.match(und)
    if m is None:
        if "static" in und:
            return "static method"
        if "__thiscall" not in und:
            return "not __thiscall"
        return "unparsed signature"
    virtual, ret, cls, meth, params, constness = m.groups()
    ret = ret.strip()
    if ret in REFUSED_SCALARS or "<" in ret:
        return "refused return type %r" % ret
    if ret not in SCALARS and ret != "void":
        pm = parse_param(ret + " *")  # 'struct X *' shape check via reuse
        if ret.endswith("*") or ret.endswith("&"):
            rp = parse_param("struct " + ret if not
                             ret.startswith(("struct", "class")) else ret)
            if rp is None:
                return "refused return type %r" % ret
        else:
            return "struct-by-value return (SRET territory)"
    plist = []
    if params.strip() not in ("void", ""):
        for p in params.split(","):
            parsed = parse_param(p)
            if parsed is None:
                return "refused param %r" % p.strip()
            if parsed[0] == "scalar" and parsed[1] in REFUSED_SCALARS:
                return "refused param %r" % p.strip()
            plist.append(parsed)
    return {"virtual": bool(virtual), "ret": ret, "cls": cls, "meth": meth,
            "params": plist, "const": bool(constness)}


# ---------------------------------------------------------------------------
# classification

def build_universe(targets):
    """targets (raw map/list spellings) -> lookup structures.

    One leading underscore is the x86 C decoration and is stripped for the
    JOIN KEY ONLY; raw spellings are kept for reporting. Nothing here strips
    more than one underscore (linkage.py's old lstrip("_") bug is the
    counterexample this comment exists for).

    Namespace-scope free functions (the @@Y-after-a-qualifier spelling) get
    their own table so the Itanium side of such a pair can be refused with
    the TRUE reason instead of "no MSVC method in the universe" -- the
    definition is there, it just is not a method (finding 4).
    """
    cnames, itanium_by_cm, msvc_by_cm, nsfree_by_cm = {}, {}, {}, {}
    for raw in targets:
        if raw.startswith("?"):
            m = MSVC_METHOD.match(raw)
            if m:
                msvc_by_cm.setdefault((m.group(2), m.group(1)),
                                      []).append(raw)
            else:
                m = MSVC_NSFREE.match(raw)
                if m:
                    nsfree_by_cm.setdefault((m.group(2), m.group(1)),
                                            []).append(raw)
            continue
        ident = raw[1:] if raw.startswith("_") else raw
        cnames[ident] = raw
        if ident.startswith("_ZN"):
            parsed = itanium_parse(ident)
            if parsed:
                cls, meth, is_const, arity = parsed
                itanium_by_cm.setdefault((cls, meth), []).append(
                    (ident, is_const, arity))
    return cnames, itanium_by_cm, msvc_by_cm, nsfree_by_cm


def classify(unresolved, universe):
    cnames, itanium_by_cm, msvc_by_cm, nsfree_by_cm = universe
    rows = {"ALIAS": [], "ALIAS_FN": [], "FACE": [], "RFACE": [],
            "WALL": [], "REFUSED": [], "MISSING": [], "UNKNOWN": []}
    need_undname = []
    for raw in unresolved:
        sym = raw
        if sym.startswith("?"):
            if member_ptr_typed(sym):
                rows["WALL"].append((raw, "pointer-to-member typed (P8/Q8 "
                                     "in the type text): the consumer names "
                                     "a member-pointer type, so it needs a "
                                     "host copy whether the pair is called "
                                     "or only copied -- the consumer-"
                                     "spelling law, mg_fanout_costs.txt s4; "
                                     "the gate-16 wall"))
                continue
            m = MSVC_DATA.match(sym)
            if m and m.group(2) is None:
                t = m.group(3)
                if re.match(r"^(?:[PQ][AB])*[UT]", t):
                    rows["REFUSED"].append((raw, "struct-typed global (%s): "
                                            "the mangle cannot show the "
                                            "struct's members, and a struct "
                                            "wrapper hides a member-pointer "
                                            "table -- @@3PAUEntry@@A was the "
                                            "dScMgCurling_c state table "
                                            "(s10 finding 1); rule the "
                                            "consumer's source by hand" % t))
                elif not SAFE_DATA_TYPE.match(t):
                    rows["REFUSED"].append((raw, "data spelling %r outside "
                                            "the scalar subset this tool "
                                            "has proven safe (the two-ints "
                                            "law); rule it by hand" % t))
                elif m.group(1) in cnames:
                    rows["ALIAS"].append((raw, m.group(1)))
                else:
                    rows["MISSING"].append((raw, "no C definition named %s"
                                            % m.group(1)))
                continue
            m = MSVC_FREE.match(sym)
            if m:
                if m.group(2) != "A":
                    rows["REFUSED"].append((raw, "non-cdecl free function: "
                                            "an alias cannot change a "
                                            "calling convention"))
                elif m.group(1) in cnames:
                    rows["ALIAS_FN"].append((raw, m.group(1)))
                else:
                    rows["MISSING"].append((raw, "no C definition named %s"
                                            % m.group(1)))
                continue
            m = MSVC_NSFREE.match(sym)
            if m:
                rows["REFUSED"].append((raw, "free function at NAMESPACE "
                                        "scope (%s::%s, the @@Y spelling): "
                                        "not a method and not face "
                                        "material, and ALIAS_FN joins "
                                        "global-scope names only -- a "
                                        "namespace member joined onto a "
                                        "bare C name could land on a "
                                        "different function; rule it by "
                                        "hand" % (m.group(2), m.group(1))))
                continue
            if MSVC_METHOD.match(sym):
                need_undname.append(raw)
                continue
            rows["UNKNOWN"].append((raw, "unclassified MSVC mangle"))
            continue
        ident = sym[1:] if sym.startswith("_") else sym
        if ident.startswith("_ZN"):
            parsed = itanium_parse(ident)
            if not parsed:
                rows["UNKNOWN"].append((raw, "Itanium shape outside the "
                                        "two-component subset"))
                continue
            cls, meth, is_const, arity = parsed
            cands = msvc_by_cm.get((cls, meth), [])
            if not cands:
                nsf = nsfree_by_cm.get((cls, meth), [])
                if nsf:
                    # Finding 4's original shape: the first version read
                    # this as a class method and refused the row as
                    # "reverse face with N args" -- right refusal, wrong
                    # reason, and a wrong reason in a refusal sends a lane
                    # to hand-write the wrong thing.
                    rows["REFUSED"].append((raw, "the MSVC definition %s "
                                            "is a namespace-scope __cdecl "
                                            "free function, not a method: "
                                            "no face and no argument "
                                            "re-landing is needed; when "
                                            "both sides are __cdecl with "
                                            "the same arguments a plain "
                                            "hand /alternatename is the "
                                            "whole fix -- the "
                                            "Memory::Allocate ruling, "
                                            "hal/scene_mg_faces.cpp 2d"
                                            % nsf[0]))
                else:
                    rows["MISSING"].append((raw, "no MSVC method %s::%s in "
                                            "the universe" % (cls, meth)))
            elif len(cands) > 1:
                rows["REFUSED"].append((raw, "ambiguous: %d MSVC candidates"
                                        % len(cands)))
            elif arity != 0:
                rows["REFUSED"].append((raw, "reverse face with %d args: "
                                        "hand-write it against the "
                                        "cxxname_bridge precedent" % arity))
            else:
                rows["RFACE"].append((raw, ident, cands[0]))
            continue
        rows["UNKNOWN"].append((raw, "plain C name: a face cannot supply "
                                "it, host or mount it"))

    und = undname_batch(need_undname + [c for r in rows["RFACE"]
                                        for c in [r[2]]])
    for raw in need_undname:
        sig = und.get(raw)
        if sig is None:
            rows["REFUSED"].append((raw, "undname produced nothing"))
            continue
        parsed = parse_msvc_sig(sig)
        if isinstance(parsed, str):
            rows["REFUSED"].append((raw, parsed + " [" + sig + "]"))
            continue
        cands = itanium_by_cm.get((parsed["cls"], parsed["meth"]), [])
        if not cands:
            rows["MISSING"].append((raw, "no Itanium body for %s::%s"
                                    % (parsed["cls"], parsed["meth"])))
            continue
        if len(cands) > 1:
            rows["REFUSED"].append((raw, "ambiguous: %d Itanium candidates"
                                    % len(cands)))
            continue
        ident, t_const, t_arity = cands[0]
        if t_arity < 0:
            rows["REFUSED"].append((raw, "target arity unparseable: %s"
                                    % ident))
            continue
        if t_arity != len(parsed["params"]):
            rows["REFUSED"].append((raw, "ARITY MISMATCH: caller %d vs "
                                    "body %d (%s) -- a ruling, not "
                                    "plumbing" % (len(parsed["params"]),
                                                  t_arity, ident)))
            continue
        rows["FACE"].append((raw, parsed, ident))

    # resolve the RFACE signatures now that undname has run
    resolved_rfaces = []
    for raw, ident, cand in rows["RFACE"]:
        sig = und.get(cand)
        parsed = parse_msvc_sig(sig) if sig else "undname produced nothing"
        if isinstance(parsed, str):
            rows["REFUSED"].append((raw, parsed))
            continue
        resolved_rfaces.append((raw, ident, cand, parsed))
    rows["RFACE"] = resolved_rfaces
    return rows


# ---------------------------------------------------------------------------
# emission

def c_type(kind, name, is_const):
    if kind == "scalar":
        return name
    return ("const void *" if is_const else "void *")


def cpp_type(kind, name, is_const):
    if kind == "scalar":
        return name
    base = ("const %s" % name) if is_const else name
    return base + (" *" if kind == "ptr" else " &")


def emit(rows, out):
    lines = [
        "// GENERATED by port/tools/facegen.py -- the mechanical rows only.",
        "// Every face here passed the exact-name join and the arity gate;",
        "// what no generator checks is method_faces.cpp's checklist item",
        "// (a), the plausible-sibling trap, so review each row's target",
        "// against its caller before wiring. This file includes NOTHING,",
        "// for hal/actor_class_faces.cpp's reasons.",
        ""]
    fwd, externs, structs, bodies, pragmas = set(), [], {}, [], []

    for raw, ident in rows["ALIAS"]:
        pragmas.append('#pragma comment(linker, "/alternatename:%s=_%s")'
                       % (raw, ident))
    for raw, ident in rows["ALIAS_FN"]:
        pragmas.append('#pragma comment(linker, "/alternatename:%s=_%s")'
                       % (raw, ident))

    for raw, sig, ident in rows["FACE"]:
        cls, meth = sig["cls"], sig["meth"]
        for kind, name, k in sig["params"]:
            if kind != "scalar" and name != "void":
                fwd.add(name)
        cparams = ["void *self"] + [c_type(*p) for p in sig["params"]]
        cret = sig["ret"] if sig["ret"] in SCALARS or sig["ret"] == "void" \
            else "void *"
        externs.append("%s %s(%s);" % (cret, ident, ", ".join(cparams)))
        argn = ["a%d" % i for i in range(len(sig["params"]))]
        mparams = ", ".join("%s %s" % (cpp_type(*p), n)
                            for p, n in zip(sig["params"], argn))
        structs.setdefault(cls, []).append(
            "    %s %s(%s)%s;" % (sig["ret"], meth, mparams,
                                  " const" if sig["const"] else ""))
        callargs = ["(void *)this" if sig["const"] else "this"]
        for p, n in zip(sig["params"], argn):
            kind = p[0]
            callargs.append("&" + n if kind == "ref" else n)
        retkw = "return " if sig["ret"] != "void" else ""
        bodies.append("%s %s::%s(%s)%s\n{ %s%s(%s); }"
                      % (sig["ret"], cls, meth, mparams,
                         " const" if sig["const"] else "",
                         retkw, ident, ", ".join(callargs)))

    for raw, ident, cand, sig in rows["RFACE"]:
        cls, meth = sig["cls"], sig["meth"]
        structs.setdefault(cls, []).append(
            "    %s %s()%s;  /* declared, DEFINED BY THE MATCHED TU */"
            % (sig["ret"], meth, " const" if sig["const"] else ""))
        cast = "(const %s *)" % cls if sig["const"] else "(%s *)" % cls
        selft = "const void *" if sig["const"] else "void *"
        retkw = "return " if sig["ret"] != "void" else ""
        bodies.append('extern "C" %s %s(%s self)\n{ %s(%sself)->%s::%s(); }'
                      % (sig["ret"], ident, selft, retkw, cast, cls, meth))

    for name in sorted(fwd):
        lines.append("struct %s;" % name)
    if externs:
        lines += ["", 'extern "C" {'] + externs + ["}"]
    for cls in sorted(structs):
        lines += ["", "struct %s {" % cls] + structs[cls] + ["};"]
    lines += [""] + bodies
    if pragmas:
        lines += [""] + pragmas
    text = "\n".join(lines) + "\n"
    pathlib.Path(out).write_text(text)
    return text


def verify(rows, genfile, scratch):
    """Compile the emitted file; its symbol surface must equal the request.

    genfile is resolved to an ABSOLUTE path first, and that one line is
    finding 2's fix: compile_batch resolves a relative source against its
    root argument, which this function sets to the output's own parent, so
    a relative --out carrying a directory component (build/faces.cpp)
    used to become <parent>/<parent>/<file>, a path that does not exist.
    cl was handed a missing file and the tool reported a FALSE 'DID NOT
    COMPILE' on a correct one -- and under the standing rule that no
    generated file is wired without a PASS, a false fail costs a lane the
    whole hand-written alternative.
    """
    genfile = pathlib.Path(genfile).resolve()
    results = closure.compile_batch(genfile.parent, [str(genfile)], scratch)
    syms = results[str(genfile)]
    if syms is None:
        return False, "generated file DID NOT COMPILE (see %s)" % scratch
    und, dfn = closure.read_syms(syms)
    want_def = set(r[0] for r in rows["FACE"])
    want_def |= set("_" + r[1] for r in rows["RFACE"])
    want_und = set("_" + r[2] for r in rows["FACE"]) | \
        set(r[2] for r in rows["RFACE"])  # RFACE r[2] = decorated target
    problems = []
    if dfn != want_def:
        problems.append("defined set mismatch: extra=%s missing=%s"
                        % (sorted(dfn - want_def),
                           sorted(want_def - dfn)))
    if not und <= want_und:
        problems.append("unexpected undefineds: %s" % sorted(und - want_und))
    return (not problems), "; ".join(problems) or "surface exact"


def report(rows):
    for bucket in ("FACE", "RFACE", "ALIAS", "ALIAS_FN", "WALL", "REFUSED",
                   "MISSING", "UNKNOWN"):
        items = rows[bucket]
        if not items:
            continue
        print("=== %s (%d) ===" % (bucket, len(items)))
        for it in items:
            if bucket in ("FACE", "RFACE"):
                print("    %s  ->  %s" % (it[0], it[2] if bucket == "FACE"
                                          else it[1]))
            else:
                print("    %s  --  %s" % (it[0], it[1]))


# ---------------------------------------------------------------------------

def read_list(path):
    """A symbol per line; closure.py output is accepted as-is.

    If the text carries closure.py section headers, ONLY the UNRESOLVED
    section is ingested; everything else in that report is context, not a
    request. The first version of this gate was computed and never applied,
    and fed a real closure report it ingested 44 rows including all the
    duplicate-definition candidates and a header line -- generating a face
    for a symbol the image already defines, a manufactured LNK2005 that
    --verify cannot catch because the request itself was poisoned. A plain
    list (no headers anywhere) ingests every non-comment line.
    """
    lines = pathlib.Path(path).read_text(errors="replace").splitlines()
    has_headers = any(line.startswith("=== ") for line in lines)
    out, in_section = [], False
    for line in lines:
        if line.startswith("==="):
            in_section = line.startswith("=== UNRESOLVED")
            continue
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if in_section or not has_headers:
            out.append(s)
    return out


def selftest():
    """The classifier and emitter against a curated request set.

    Ten forward faces and the TrackInDeathTable reverse face are drawn from
    hal/actor_class_faces.cpp (TouchesWater, that file's other reverse
    face, is exercised by the full reconstruction proof, not here); the
    ApplyOpacity arity refusal is that file's judgment row; the two ov004
    globals are the MG1 wall's, NOT that file's -- one pointer-to-member
    spelling that must be refused as the WALL and one PAHA spelling that
    must alias. Expected: ten FACE, one RFACE, one ALIAS, one WALL, the
    ARITY MISMATCH refusal, and a compiled symbol surface that equals the
    request exactly. A second case feeds read_list a real mixed closure.py
    report and expects ONLY the UNRESOLVED rows back.

    THE HARDENING ARMS, one per defect real use found (mg_fanout_costs.txt
    s10), each fed the exact symbol that slipped or misfired:

      1. ?data_ov006_02141950@@3PAUEntry@@A -- the dScMgCurling_c state
         table, a PMF table behind a struct wrapper, WITH its C name in
         the universe so the old alias temptation is live. Must be
         REFUSED, never ALIAS. Its by-value sibling @@3UPair@@A must
         refuse too (that one always did; the arm keeps it that way).
      2. verify() run twice more, on a relative --out WITH a directory
         component and on the bare-relative shape, both from a changed
         CWD. Both must PASS; the first used to be a false DID NOT
         COMPILE.
      3. a zero-byte map through closure.map_defined, the exact call
         --map makes: must refuse loudly, not return an empty universe.
      4. Memory::Allocate both ways -- the Itanium spelling against the
         @@Y definition must refuse WITH THE NAMESPACE REASON (the old
         reason was "reverse face with 2 args", a method misread), and
         the @@Y spelling as an unresolved row must refuse as a
         namespace-scope free function rather than dying in undname.
    """
    unresolved = [
        "?UpdatePos@Actor@@QAEXPAUCylinderClsn@@@Z",
        "?UpdatePosWithHorzSpeedAndAng@Actor@@QAEXXZ",
        "?ReflectAngle@Actor@@QAEFHHF@Z",
        "?DistToCPlayer@Actor@@QAEHXZ",
        "?UpdateWMClsn@Enemy@@QAEXAAUWithMeshClsn@@I@Z",
        "?Init@MovingCylinderClsn@@QAEXPAUActor@@HHII@Z",
        "?SetFile@MovingMeshCollider@@QAEXPAUKCL_File@@ABUMatrix4x3@@HFAAUCLPS_Block@@@Z",
        "?Init@WithMeshClsn@@QAEXPAUActor@@HHPAUVector3_16@@H@Z",
        "?IsOnWall@WithMeshClsn@@QBEHXZ",
        "?JustHitGround@WithMeshClsn@@QBEHXZ",
        "?ApplyOpacity@ModelBase@@QAEXIH@Z",
        "?data_ov004_020beb98@@3PAP8C@@AEXXZA",
        "?data_ov004_020beb68@@3PAHA",
        "__ZN5Actor17TrackInDeathTableEv",
        "?data_ov006_02141950@@3PAUEntry@@A",
        "?data_ov004_020bc904@@3UPair@@A",
        "__ZN6Memory8AllocateEji",
        "?Allocate@Memory@@YAPAXIH@Z",
        "?dataP8pad@@3PAHA",
        "?data_ov004_020bff00@@3PAP6AXXZA",
    ]
    targets = [
        "__ZN5Actor9UpdatePosEP12CylinderClsn",
        "__ZN5Actor28UpdatePosWithHorzSpeedAndAngEv",
        "__ZN5Actor12ReflectAngleE5Fix12IiES1_s",
        "__ZN5Actor13DistToCPlayerEv",
        "__ZN5Enemy12UpdateWMClsnER12WithMeshClsnj",
        "__ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj",
        "__ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block",
        "__ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_",
        "__ZNK12WithMeshClsn8IsOnWallEv",
        "__ZNK12WithMeshClsn13JustHitGroundEv",
        "__ZN9ModelBase12ApplyOpacityEj",
        "_data_ov004_020beb68",
        "?TrackInDeathTable@Actor@@QAEXXZ",
        "_data_ov006_02141950",
        "_data_ov004_020bc904",
        "?Allocate@Memory@@YAPAXIH@Z",
        "_dataP8pad",
        "_data_ov004_020bff00",
    ]
    rows = classify(unresolved, build_universe(targets))
    ok = True

    def expect(cond, what):
        nonlocal ok
        if not cond:
            print("FAIL:", what)
            ok = False

    expect(len(rows["FACE"]) == 10, "10 faces, got %d" % len(rows["FACE"]))
    expect(len(rows["RFACE"]) == 1 and
           rows["RFACE"][0][1] == "_ZN5Actor17TrackInDeathTableEv",
           "TrackInDeathTable reverse face")
    # dataP8pad is the WALL scan's name-exclusion arm: 'P8' in the NAME
    # portion of a scalar-spelled global must still alias -- the scan
    # covers the type text only.
    expect(set(r[1] for r in rows["ALIAS"]) ==
           {"data_ov004_020beb68", "dataP8pad"},
           "the two PAHA aliases (incl. P8-in-the-name): %s" % rows["ALIAS"])
    expect(len(rows["WALL"]) == 1 and "beb98" in rows["WALL"][0][0],
           "the P8 wall row and only it")
    expect(any("ARITY MISMATCH" in r[1] and "ApplyOpacity" in r[0]
               for r in rows["REFUSED"]), "ApplyOpacity arity refusal")
    expect(not rows["MISSING"] and not rows["UNKNOWN"],
           "no MISSING/UNKNOWN: %s %s" % (rows["MISSING"], rows["UNKNOWN"]))

    # arm 1: the struct-wrapped PMF table. Its C name IS in the universe,
    # so an ALIAS classification is one regex away; the len==1 assert on
    # ALIAS above catches that too, but this arm names the row.
    refused = {r[0]: r[1] for r in rows["REFUSED"]}
    expect("?data_ov006_02141950@@3PAUEntry@@A" in refused and
           "struct" in refused.get("?data_ov006_02141950@@3PAUEntry@@A", ""),
           "PAUEntry struct-wrapper refusal: %s" % rows["REFUSED"])
    expect("?data_ov004_020bc904@@3UPair@@A" in refused,
           "UPair by-value refusal held")
    # The whitelist's own arm: a function-pointer array (PAP6) is neither
    # struct-spelled nor scalar, its C name is in the universe, and its
    # words are DS code addresses -- it must refuse via the whitelist, not
    # fall through to ALIAS the way the old anything-not-a-struct rule did.
    expect("scalar subset" in
           refused.get("?data_ov004_020bff00@@3PAP6AXXZA", ""),
           "PAP6 function-pointer array refused by the whitelist: %s"
           % refused.get("?data_ov004_020bff00@@3PAP6AXXZA"))

    # arm 4: the namespace pair, both directions, and the REASON is the
    # assertion -- the old classifier refused the first row too, with a
    # method-shaped reason that sent the reader to the wrong precedent.
    r_it = refused.get("__ZN6Memory8AllocateEji", "")
    expect("namespace-scope" in r_it and "reverse face" not in r_it,
           "Memory::Allocate Itanium-side reason: %r" % r_it)
    r_ms = refused.get("?Allocate@Memory@@YAPAXIH@Z", "")
    expect("NAMESPACE" in r_ms,
           "Memory::Allocate MSVC-side reason: %r" % r_ms)

    with tempfile.TemporaryDirectory() as td:
        gen = pathlib.Path(td) / "faces_gen.cpp"
        emit(rows, gen)
        good, msg = verify(rows, gen, pathlib.Path(td) / "probe")
        expect(good, "verify: " + msg)
        if good:
            print("verify:", msg)

        # arm 2: the same request through a RELATIVE --out with a
        # directory component, from a changed CWD -- the shape finding 2
        # false-failed -- plus the bare-relative shape that always worked.
        oldcwd = os.getcwd()
        try:
            os.chdir(td)
            os.makedirs("sub", exist_ok=True)
            relout = os.path.join("sub", "faces_rel.cpp")
            emit(rows, relout)
            good, msg = verify(rows, relout, "probe_rel")
            expect(good, "verify on relative --out with a dir component: "
                   + msg)
            emit(rows, "faces_bare.cpp")
            good, msg = verify(rows, "faces_bare.cpp", "probe_bare")
            expect(good, "verify on bare relative --out: " + msg)
        finally:
            os.chdir(oldcwd)

        # arm 3: facegen's --map path is closure.map_defined, this exact
        # call; a zero-byte map (what a failed link leaves) must refuse
        # loudly rather than come back as an empty universe.
        zmap = pathlib.Path(td) / "zero.map"
        zmap.write_text("")
        try:
            closure.map_defined(zmap)
            expect(False, "zero-byte map ACCEPTED by the --map path")
        except SystemExit as e:
            expect("ZERO BYTES" in str(e),
                   "zero-map refusal message: %s" % e)

        # read_list against a real mixed closure.py report: only the
        # UNRESOLVED rows may come back. The first shipped version ingested
        # every section of exactly this shape and manufactured an LNK2005.
        mixed = pathlib.Path(td) / "closure_out.txt"
        mixed.write_text(
            "map defines 32535 symbols\n"
            "\n"
            "=== COMPILE FAILURES (1) ===\n"
            "   src/func_ov006_020e3528.cpp\n"
            "        error C2440: cannot convert\n"
            "\n"
            "=== UNRESOLVED after this slice: 2 "
            "(an estimate; confirm with a link) ===\n"
            "    ?UpdatePos@Actor@@QAEXPAUCylinderClsn@@@Z\n"
            "    _data_02075720\n"
            "\n"
            "=== DUP-DEF CANDIDATES, NEED A REAL LINK ===\n"
            "  already in the image from its own TU: 1\n"
            "    _Enable3dEngines  --  Enable3dEngines.c.obj compiles it "
            "already\n"
            "  FOREIGN-object rows, the LNK2005 candidates: 1\n"
            "    _LoadArchive  --  image copy is from scene_boot.cpp.obj\n")
        got = read_list(mixed)
        expect(got == ["?UpdatePos@Actor@@QAEXPAUCylinderClsn@@@Z",
                       "_data_02075720"],
               "read_list on mixed closure output: %s" % got)

        # and a plain list, no headers, still ingests every row
        plain = pathlib.Path(td) / "plain.txt"
        plain.write_text("# a comment\n?A@B@@QAEXXZ\n_c_name\n")
        got = read_list(plain)
        expect(got == ["?A@B@@QAEXXZ", "_c_name"],
               "read_list on a plain list: %s" % got)
    print("selftest %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser(
        description="generate mechanical faces/aliases; see the docstring")
    ap.add_argument("--unresolved", help="symbol list or closure.py output")
    ap.add_argument("--targets", action="append", default=[],
                    help="available-definition list (repeatable)")
    ap.add_argument("--map", dest="mapfile",
                    help="walk_window.map to use as the universe")
    ap.add_argument("--out", default="faces_gen.cpp")
    ap.add_argument("--verify", action="store_true",
                    help="compile the output and check its symbol surface")
    ap.add_argument("--selftest", action="store_true")
    args = ap.parse_args()

    if args.selftest:
        sys.exit(selftest())
    if not args.unresolved:
        sys.exit("pass --unresolved (or --selftest)")

    targets = []
    for t in args.targets:
        targets += read_list(t)
    if args.mapfile:
        targets += sorted(closure.map_defined(args.mapfile))
    if not targets:
        sys.exit("empty universe: pass --targets and/or --map")

    rows = classify(read_list(args.unresolved), build_universe(targets))
    report(rows)
    if rows["FACE"] or rows["RFACE"] or rows["ALIAS"] or rows["ALIAS_FN"]:
        emit(rows, args.out)
        print("\nwrote %s" % args.out)
        if args.verify:
            with tempfile.TemporaryDirectory() as td:
                good, msg = verify(rows, args.out, td)
                print("verify:", ("PASS " if good else "FAIL ") + msg)
                sys.exit(0 if good else 1)


if __name__ == "__main__":
    main()
