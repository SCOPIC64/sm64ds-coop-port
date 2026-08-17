#!/usr/bin/env python3
"""abi_prove -- prove the ABI checkers still catch every instance of the class
that a person had to find by hand.

WHY THIS IS SHAPED THE WAY IT IS, AND WHY THE ORIGINAL WAS THROWN AWAY

    The version recovered from branch port-abi-sweep asserted that all nine
    known instances of the class were CAUGHT ON THE TREE IT RAN ON.  That
    worked because that worktree was branched from cons e39d7b5d7, which
    predated every fix, so all nine were still live in it.  Six days later
    every one of them is FIXED, and the same assertions now fail -- not
    because the checkers regressed but because the tree got better.  A proof
    that goes red when the bug is fixed is not a proof, it is a snapshot.

    So the shape is inverted.  Each fixture is a defect that WAS real and IS
    fixed, and each is proved twice:

        GREEN   the checker passes on the tree as it stands, and the fixture's
                own defect is gone from it.
        RED     the fixture is re-broken IN A SCRATCH COPY of the tree, and
                the checker is run against that copy and must fail, naming
                that fixture.

    A checker that cannot go red on demand has not been shown to work.  Both
    halves run every time; a checker that only ever passes has proved nothing
    and this says so.

    THE SCRATCH COPY IS A COPY.  The real tree is never edited, not even
    transiently, so an interrupted run cannot leave a re-broken directive
    behind.  port/ is about 11 MB and copies once for the whole run.

WHAT IS PROVED, PER CHECKER

    aliascheck   nine alias-form fixtures.  Six are the receiver bridges that
                 landed on cons 2026-08-16 (the three Heap methods, _Destroy,
                 and the SolidHeapAllocator pair); three are the instances the
                 original sweep pinned (Player::TryGrab,
                 BlendModelAnim::SetAnim, Platform::KillByMegaChar).  Each is
                 re-injected as a real `#pragma comment(linker, ...)` in a new
                 file under the scratch copy's port/hal/, which is exactly how
                 a real one arrives.

    aritycheck   two fixtures, and they are the ones that make the point: the
                 receiver-shape defects fixed by #1539 (FaderColor::AdvanceFade)
                 and #1543 (Enemy's base ctor) on 2026-08-16.  Both are
                 re-broken by rewriting the real declaration back to `(void)`
                 in the scratch copy.

    abicheck     six vtable-form fixtures -- the thunks the original sweep
                 pinned by name.  These are proved against DISASSEMBLY rather
                 than source, because that is abicheck's input: a thunk's
                 `ret 4` is rewritten to a bare `ret` in a scratch copy of the
                 real disassembly of THIS build, and abicheck must call the
                 result UNDERPOP.  Needs --disasm-dir; without it the abicheck
                 arm is SKIPPED and the run says so rather than passing.

USAGE
    python port/tools/abi_prove.py [repo-root] [--disasm-dir DIR] [--keep]
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
PY = sys.executable
DEFAULT_ROOT = os.path.dirname(os.path.dirname(HERE))

FIXTURE_FILE = "abi_prove_reinjected.cpp"
NS_FIXTURE_FILE = "abi_prove_nsdecl.cpp"

# --------------------------------------------------------------------------
# alias-form fixtures: (name, directive text, what it cost, where it was fixed)
# --------------------------------------------------------------------------
ALIAS_FIXTURES = [
    ("Heap::SetDefault",
     "__ZN4Heap10SetDefaultEv=?SetDefault@Heap@@QAEHXZ",
     "published a register nobody set into the default-heap pointer; the boot "
     "carried on with it and faulted later in Heap::Allocate",
     "cons 8af183ddb, 2026-08-16"),
    ("Heap::Destroy",
     "__ZN4Heap7DestroyEv=?Destroy@Heap@@QAEXXZ",
     "the body dereferences the receiver, so this one faults rather than "
     "publishing a wrong word",
     "cons 0936784c9, 2026-08-16"),
    ("Heap::_Destroy",
     "?_Destroy@Heap@@QAEXXZ=__ZN4Heap8_DestroyEv",
     "the other direction: the caller sets ECX, the flat body reads the stack",
     "cons 0936784c9, 2026-08-16"),
    ("Heap::ResizeToFit",
     "__ZN4Heap11ResizeToFitEv=?ResizeToFit@Heap@@QAEIXZ",
     "reached from Virtual34, slot 13 of _ZTV5Actor",
     "cons cf428a19a, 2026-08-16"),
    ("SolidHeapAllocator::Allocate",
     "?Allocate@SolidHeapAllocator@@QAEPAXIH@Z="
     "__ZN18SolidHeapAllocator8AllocateEji",
     "thiscall left, flat right: both arguments shift by one",
     "cons 44b78e90d, 2026-08-16"),
    ("SolidHeapAllocator::Reallocate",
     "?Reallocate@SolidHeapAllocator@@QAEPAXPAXI@Z="
     "__ZN18SolidHeapAllocator10ReallocateEPvj",
     "the sibling of the above, same shape",
     "cons 44b78e90d, 2026-08-16"),
    ("Player::TryGrab",
     "__ZN6Player7TryGrabER5Actor=?TryGrab@Player@@QAE_NAAUActor@@@Z",
     "grabbing an actor, e.g. the penguin catch",
     "the King Bob-omb lane, 2026-08-10"),
    ("BlendModelAnim::SetAnim",
     "__ZN14BlendModelAnim7SetAnimER8SharedFilePtr="
     "?SetAnim@BlendModelAnim@@QAEXAAUSharedFilePtr@@@Z",
     "King Bob-omb animation change; 11 crash reports from 5 reporters",
     "the King Bob-omb lane, 2026-08-10"),
    ("Platform::KillByMegaChar",
     "__ZN8Platform15KillByMegaCharER6Player="
     "?KillByMegaChar@Platform@@QAEXAAUPlayer@@@Z",
     "a Mega character kills a platform",
     "the King Bob-omb lane, 2026-08-10"),
    # RULE P's own fixture, deliberately invisible to RULE R. Both sides are
    # public __thiscall, so the receiver AGREES and the receiver rule is right
    # to stay silent; the ARGUMENT COUNT disagrees, so the left side is
    # compiled for a callee that rets 4 and the right side rets 0. This row is
    # what proves rule P is running at all -- it was not until 2026-08-17,
    # because the helper piped the name list into undname's stdin, undname
    # takes filenames, and the resulting exit 1 was read as "undname is not
    # installed" on every machine including ones where it is.
    ("a pop mismatch RULE R cannot see (rule P only)",
     "?SetAnim@BlendModelAnim@@QAEXAAUSharedFilePtr@@@Z="
     "?SetAnim@BlendModelAnimFace@@QAEXXZ",
     "thiscall both sides, so the receiver agrees; the argument count does "
     "not, and the caller's frame runs 4 bytes high on every call",
     "never live on cons -- a constructed discriminator, not a past defect"),
]

# --------------------------------------------------------------------------
# arity-form fixtures: (name, file, broken text, fixed text, why, where)
# The broken text is what the file said BEFORE the fix. Re-breaking is a
# literal reverse substitution, so if the fixed text is no longer in the file
# the fixture REFUSES rather than silently proving nothing.
# --------------------------------------------------------------------------
ARITY_FIXTURES = [
    ("FaderColor::AdvanceFade", "include/decl_FaderColor.h",
     "extern void _ZN10FaderColor11AdvanceFadeEv(void);",
     "extern void _ZN10FaderColor11AdvanceFadeEv(void*);",
     "a member's receiver rides r0; declared (void) the caller passes nothing "
     "and the body reads its own caller's return address as `this`",
     "PR #1539, cons ba1b0a670, 2026-08-16"),
    ("Enemy base ctor", "src/RollingRock_Spawn.c",
     "extern void _ZN5EnemyC2Ev(void);",
     "extern void _ZN5EnemyC2Ev(void *);",
     "spawn sites that already had the receiver in hand and were not passing "
     "it; the definition src/_ZN5EnemyC2Ev.cpp:5 takes one",
     "PR #1543, cons b74cf657d, 2026-08-16 (that PR fixed the ChainChomp / "
     "ChiefChilly / Wiggler spellings; this row re-breaks a sibling that "
     "carries the identical declaration today)"),
]

# --------------------------------------------------------------------------
# vtable-form fixtures: thunk substring, slot, what a player does to reach it
# --------------------------------------------------------------------------
VTABLE_FIXTURES = [
    ("crate_pounded", 21, "ground-pound a crate (HMC and elsewhere)"),
    ("ssb_pounded", 21, "ground-pound the Bob-omb Battlefield seesaw"),
    ("ps_s19", 19, "Yoshi tries to turn a Power Star into an egg"),
    ("bp_egg", 19, "Yoshi tries to turn a Baby Penguin into an egg"),
    ("fb_slot27", 27, "a Mega character hits the WF fall block"),
    ("whomp_s30", 30, "egg-aim query against a Whomp"),
]


def run(argv, cwd=None):
    p = subprocess.run(argv, cwd=cwd, capture_output=True, text=True)
    return p.returncode, p.stdout + p.stderr


def make_scratch(root, dest):
    """A copy of the tree deep enough for the no-build checkers to run.

    aliascheck walks <root>/port. aritycheck walks src, port/unmatched and
    port/hal, and reads include/ only through those. So the copy carries port,
    src and include, and nothing else -- no build directory, no extracted ROM.
    """
    for sub in ("port", "src", "include"):
        s = os.path.join(root, sub)
        if not os.path.isdir(s):
            continue
        shutil.copytree(s, os.path.join(dest, sub),
                        ignore=shutil.ignore_patterns("__pycache__", "build"))
    return dest


def inject_alias(scratch, directive, tag):
    """Write the re-broken directive into the scratch tree as a real pragma."""
    path = os.path.join(scratch, "port", "hal", FIXTURE_FILE)
    with open(path, "w", encoding="utf-8") as f:
        f.write("/* abi_prove re-injected fixture: %s\n"
                "   This file exists only inside a scratch copy. If you are\n"
                "   reading it in a real checkout, delete it. */\n" % tag)
        f.write('#pragma comment(linker, "/alternatename:%s")\n' % directive)
    return path


def clear_alias(scratch):
    path = os.path.join(scratch, "port", "hal", FIXTURE_FILE)
    if os.path.exists(path):
        os.remove(path)


def rebreak_arity(scratch, rel, fixed, broken):
    """Reverse the fix in the scratch copy. (ok, message)"""
    path = os.path.join(scratch, rel.replace("/", os.sep))
    if not os.path.isfile(path):
        return False, "%s is not in the tree" % rel
    src = open(path, encoding="utf-8", errors="replace").read()
    if fixed not in src:
        return False, ("%s no longer contains the FIXED text, so re-breaking "
                       "it would prove nothing: %r" % (rel, fixed))
    open(path, "w", encoding="utf-8").write(src.replace(fixed, broken, 1))
    return True, "rewrote %s back to the pre-fix spelling" % rel


def restore_arity(scratch, root, rel):
    shutil.copyfile(os.path.join(root, rel.replace("/", os.sep)),
                    os.path.join(scratch, rel.replace("/", os.sep)))


# --------------------------------------------------------------------------
def prove_alias(root, scratch, results):
    print("\n" + "=" * 74)
    print("## aliascheck -- %d alias-form fixtures" % len(ALIAS_FIXTURES))
    print("=" * 74)

    rc, out = run([PY, os.path.join(HERE, "aliascheck.py"), root])
    green = rc == 0
    print("\n  GREEN on the real tree: aliascheck exit %d  %s"
          % (rc, "PASS" if green else "FAIL"))
    for ln in out.splitlines():
        if "receiver crossings:" in ln or "ALIASCHECK" in ln:
            print("      %s" % ln.strip())
    results.append(("aliascheck GREEN on cons", green))

    # Every crossing the real tree carries, read once. A fixture's own defect
    # must be ABSENT from this, or the red proof below would be proving
    # something that was already there.
    _, live = run([PY, os.path.join(HERE, "aliascheck.py"), root, "--list"])

    for name, directive, cost, fixed_in in ALIAS_FIXTURES:
        lhs = directive.split("=", 1)[0]
        rhs = directive.split("=", 1)[1]
        absent = not (lhs in live and rhs in live)
        inject_alias(scratch, directive, name)
        rc1, out1 = run([PY, os.path.join(HERE, "aliascheck.py"), scratch])
        clear_alias(scratch)
        # WHICH RULE fires is part of the fixture. The first version of this
        # asked for the substring "NEW", which appears in aliascheck's own
        # count line ("0 NEW, 5 KNOWN-OPEN, ...") on every run including a
        # clean one, so it was very nearly no assertion at all. The markers
        # below are the two rules' report lines and neither appears on a clean
        # run. Pinning one per fixture is what stops a receiver fixture being
        # credited to the pop rule or the reverse -- the entire reason for
        # having two rules is that each sees what the other cannot.
        marker = "POP MISMATCH" if "rule P" in name else "NEW  <<<<<<"
        named = lhs in out1
        caught = rc1 == 1 and named and marker in out1
        ok = absent and caught
        results.append(("aliascheck RED: %s" % name, ok))
        print("\n  %-4s %s" % ("PASS" if ok else "FAIL", name))
        print("       gone from cons: %s   (fixed in %s)"
              % ("yes" if absent else "NO -- still present!", fixed_in))
        print("       re-broken in the scratch copy: aliascheck exit %d, "
              "names it: %s, reports '%s': %s"
              % (rc1, "yes" if named else "NO", marker,
                 "yes" if marker in out1 else "NO"))
        print("       cost when live: %s" % cost)


def prove_arity(root, scratch, results):
    print("\n" + "=" * 74)
    print("## aritycheck --gate-receiver -- %d arity-form fixtures"
          % len(ARITY_FIXTURES))
    print("=" * 74)

    rc, out = run([PY, os.path.join(HERE, "aritycheck.py"), root,
                   "--gate-receiver"])
    green = rc == 0
    print("\n  GREEN on the real tree: aritycheck --gate-receiver exit %d  %s"
          % (rc, "PASS" if green else "FAIL"))
    for ln in out.splitlines():
        if "RATCHET" in ln or "baselined," in ln:
            print("      %s" % ln.strip())
    results.append(("aritycheck GREEN on cons", green))

    for name, rel, broken, fixed, why, fixed_in in ARITY_FIXTURES:
        ok_break, msg = rebreak_arity(scratch, rel, fixed, broken)
        if not ok_break:
            results.append(("aritycheck RED: %s" % name, False))
            print("\n  FAIL %s\n       %s" % (name, msg))
            continue
        rc1, out1 = run([PY, os.path.join(HERE, "aritycheck.py"), scratch,
                         "--gate-receiver"])
        restore_arity(scratch, root, rel)
        caught = rc1 == 1 and "NEW RECEIVER-SHAPE" in out1
        results.append(("aritycheck RED: %s" % name, caught))
        print("\n  %-4s %s" % ("PASS" if caught else "FAIL", name))
        print("       fixed in %s; %s" % (fixed_in, msg))
        print("       re-broken: aritycheck --gate-receiver exit %d, new "
              "row(s) reported: %s" % (rc1,
                                       "yes" if caught else "NO"))
        print("       cost when live: %s" % why)


def prove_nsdecl(root, scratch, results):
    """The namespaced C++ declaration spelling, taught to aritycheck
    2026-08-17.

    aritycheck read flat Itanium declarations only, so

        namespace Player { void St_EndingFly_Main(); }

    was invisible even though it emits exactly the symbol four sibling TUs
    declare flatly. That blind spot hid the SIXTH live receiver defect on cons
    (src/func_ov007_020b7764.cpp:2 declares it, :9 calls it with nothing,
    the definition takes a receiver). aliascheck cannot see that one either,
    and is right not to: the matching alias at hal/scene_boot.cpp:1126 is
    ?St_EndingFly_Main@Player@@YAXXZ, a free function on both sides, so the
    receiver agrees and there is no crossing. Lane ABR1's review found the
    hole.

    That defect is LIVE, so it cannot be re-broken and the green/red shape the
    other fixtures use does not fit it. What is proved instead is that the
    detector fires on the SPELLING: a scratch file carrying nothing but a
    namespaced declaration of a symbol whose definition takes a receiver must
    produce a new ratchet row naming that file, and removing the file must
    take the row away again. The second half matters as much as the first --
    a detector that never goes quiet is a detector nobody can ratchet.
    """
    print("\n" + "=" * 74)
    print("## aritycheck, the namespaced C++ declaration spelling")
    print("=" * 74)

    path = os.path.join(scratch, "src", NS_FIXTURE_FILE)
    with open(path, "w", encoding="utf-8") as f:
        f.write("//cpp\n"
                "/* abi_prove fixture, scratch copy only. The exact shape of\n"
                "   src/func_ov007_020b7764.cpp:2, which nothing in this\n"
                "   suite could see before 2026-08-17. */\n"
                "namespace Player { void St_EndingFly_Main(); }\n")
    rc, out = run([PY, os.path.join(HERE, "aritycheck.py"), scratch,
                   "--gate-receiver"])
    os.remove(path)
    caught = (rc == 1 and "NEW RECEIVER-SHAPE" in out
              and NS_FIXTURE_FILE in out)
    results.append(("aritycheck RED: namespaced C++ declaration", caught))
    print("\n  %-4s a namespaced declaration in a scratch file"
          % ("PASS" if caught else "FAIL"))
    print("       aritycheck --gate-receiver exit %d, new row naming the "
          "fixture file: %s" % (rc, "yes" if caught else "NO"))

    rc2, _ = run([PY, os.path.join(HERE, "aritycheck.py"), scratch,
                  "--gate-receiver"])
    back = rc2 == 0
    results.append(("aritycheck GREEN again once the fixture is removed",
                    back))
    print("  %-4s and green again with the fixture removed (exit %d)"
          % ("PASS" if back else "FAIL", rc2))


def prove_vtable(root, disasm, results):
    print("\n" + "=" * 74)
    print("## abicheck -- %d vtable-form fixtures" % len(VTABLE_FIXTURES))
    print("=" * 74)
    if not disasm:
        print("\n  SKIPPED: no --disasm-dir. abicheck reads emitted code, so "
              "its fixtures need this build's disassembly. Generate it with\n"
              "      port/tools/gen_disasm.cmd <build-dir> <out-dir>\n"
              "  and pass --disasm-dir <out-dir>. This arm is SKIPPED, not "
              "passed.")
        results.append(("abicheck arm ran at all", False))
        return

    rc, out = run([PY, os.path.join(HERE, "abicheck.py"),
                   "--disasm-dir", disasm])
    green = rc == 0
    fills = re.search(r"(\d+) vtable slot fills", out)
    nfills = int(fills.group(1)) if fills else 0
    print("\n  GREEN on this build: abicheck exit %d over %d slot fills  %s"
          % (rc, nfills, "PASS" if green and nfills else "FAIL"))
    results.append(("abicheck GREEN on this build", green and nfills > 0))

    # RED: rewrite one thunk's `ret <n>` to a bare `ret` in a scratch copy of
    # the real disassembly. That is precisely the emitted shape of the bug --
    # crate_pounded shipped exactly like this -- so the proof runs against
    # real input rather than a hand-written fixture.
    tmp = tempfile.mkdtemp(prefix="abi_prove_dis_")
    try:
        for name, slot, why in VTABLE_FIXTURES:
            for f in os.listdir(tmp):
                os.remove(os.path.join(tmp, f))
            hit = None
            for fn in sorted(os.listdir(disasm)):
                if not fn.endswith(".txt"):
                    continue
                src = open(os.path.join(disasm, fn), "rb").read().decode(
                    "utf-16-le" if open(os.path.join(disasm, fn), "rb").read(2)
                    == b"\xff\xfe" else "utf-8", "replace")
                if name not in src:
                    open(os.path.join(tmp, fn), "w", encoding="utf-8").write(
                        src)
                    continue
                # break the terminating `ret <n>` of the fixture's thunk only
                broken, n = re.subn(
                    r"(?m)^(\s+[0-9A-F]+:\s+)ret\s+[0-9A-F]+h?\s*$",
                    r"\1ret", src)
                open(os.path.join(tmp, fn), "w", encoding="utf-8").write(
                    broken)
                if n:
                    hit = (fn, n)
            if hit is None:
                results.append(("abicheck RED: %s" % name, False))
                print("\n  FAIL %s -- no thunk of that name in the "
                      "disassembly, so nothing could be re-broken" % name)
                continue
            rc1, out1 = run([PY, os.path.join(HERE, "abicheck.py"),
                             "--disasm-dir", tmp])
            caught = rc1 == 1 and "UNDERPOP" in out1
            results.append(("abicheck RED: %s" % name, caught))
            print("\n  %-4s %s (slot %d)" % ("PASS" if caught else "FAIL",
                                             name, slot))
            print("       %d ret sizes blanked in %s" % (hit[1], hit[0]))
            print("       abicheck exit %d, UNDERPOP reported: %s"
                  % (rc1, "yes" if "UNDERPOP" in out1 else "NO"))
            print("       a player reaches it by: %s" % why)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main(argv):
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("root", nargs="?", default=DEFAULT_ROOT)
    ap.add_argument("--disasm-dir", metavar="DIR")
    ap.add_argument("--keep", action="store_true",
                    help="do not delete the scratch copy (for debugging)")
    args = ap.parse_args(argv[1:])
    root = os.path.abspath(args.root)

    for stray in (os.path.join(root, "port", "hal", FIXTURE_FILE),
                  os.path.join(root, "src", NS_FIXTURE_FILE)):
        if not os.path.exists(stray):
            continue
        print("abi_prove: REFUSED -- %s exists in the REAL tree. An earlier "
              "run was interrupted while it should have been writing only "
              "into a scratch copy, or somebody committed a fixture. Delete "
              "it before proving anything." % stray)
        return 2

    print("abi_prove -- the ABI checkers, proved GREEN on this tree and RED "
          "on a re-broken copy of it")
    print("  tree    : %s" % root)
    print("  disasm  : %s" % (args.disasm_dir or "(none: abicheck arm will "
                                                 "be SKIPPED)"))

    scratch = tempfile.mkdtemp(prefix="abi_prove_")
    results = []
    try:
        print("  scratch : %s (copying port, src, include)" % scratch)
        make_scratch(root, scratch)
        prove_alias(root, scratch, results)
        prove_arity(root, scratch, results)
        prove_nsdecl(root, scratch, results)
        prove_vtable(root, args.disasm_dir, results)
    finally:
        if args.keep:
            print("\n  scratch copy KEPT at %s" % scratch)
        else:
            shutil.rmtree(scratch, ignore_errors=True)

    print("\n" + "=" * 74)
    npass = sum(1 for _, ok in results if ok)
    for label, ok in results:
        print("  %-4s %s" % ("PASS" if ok else "FAIL", label))
    print("=" * 74)
    print("%d of %d proofs passed" % (npass, len(results)))
    if npass != len(results):
        print("ABI_PROVE FAILED. A checker that cannot be made to go red has "
              "not been shown to work, and a fixture whose defect is still "
              "live on the tree is not a fixture.")
        return 1
    print("ABI_PROVE PASSED: every checker is green on this tree and red on a "
          "deliberately re-broken copy of it.")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
