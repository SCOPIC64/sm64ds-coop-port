#!/usr/bin/env python3
"""The address binding's own tests: port/tools/facegen.py --sync.

WHY THESE AND NOT MORE. facegen.py's own --selftest covers the classifier and
the forward-face emitter it shipped with; this file covers the part lane FACES1
added, which is the ADDRESS BINDING -- the rule that a face for flat name F
binds F to exactly the member the ROM name names and never a sibling. Each case
below is a shape that has cost this tree something real, or that would if the
rule were dropped:

  1. A Q TARGET (public non-virtual). The shadow must declare it NON-virtual:
     ?Kill@X@@QAEXXZ and ?Kill@X@@UAEXXZ are different symbols, so a shadow
     that guesses turns a closed row into a fresh unresolved external.
  2. A U TARGET (public virtual). The same in the other direction, and this is
     the majority case on the sync wall: 1340 of the 1691 rows measured by lane
     ALIAS2 are public virtual. The generator that shipped declared every
     shadow member non-virtual, which is why none of them could be used.
  3. A SIBLING THAT MUST BE REFUSED. Two ROM addresses joining one decorated
     definition -- the D1/D2 destructor pair is the mechanical form, and
     ApproachLinear/ApproachLinear2 is the one that shipped (signs spinning
     forever, 156 src TUs). Both rows refuse; neither is chosen.
  4. AN ARG-COUNT MISMATCH. ModelBase::ApplyOpacity takes 2 on the caller's
     side and 1 in the ROM body; that was a RULING, not plumbing, and the tool
     must refuse rather than drop an argument.
  5. A TWO-ARGUMENT REVERSE FACE. Arguments re-landed from the stack in order,
     receiver first. The pre-FACES1 tool refused every one of these ("reverse
     face with N args: hand-write it"); 211 rows of the sync wall are that
     shape.
  6. A LEDGER WHOSE ADDRESS DISAGREES WITH config/. The checked-in row is
     evidence, not an instruction: a row that no longer derives is refused.
  7. A FORWARD FACE, the other direction: a caller's decorated member DEFINED
     here and forwarded to the ROM body's flat name, receiver first.
  8. THE D0 RULE (lane FACES2), four arms on a synthetic ROM: a Deallocate
     class whose heap argument comes from the pooled word's OWN relocation, an
     operator_delete2 class that must carry no heap at all, a D0 that makes a
     call its D1 does not, and a D0 that stores one more relocated word than
     its D1 does. The last two are the shapes that make a D0 something other
     than "its D1 plus one deallocation", and both must refuse: a wrong heap
     pointer is heap corruption, and it would link.

Run: python port/tools/test_facegen.py          (add --verify to compile too)
"""

import os
import pathlib
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import facegen  # noqa: E402


FAILED = []


def check(cond, what):
    if cond:
        print("  ok   %s" % what)
    else:
        print("  FAIL %s" % what)
        FAILED.append(what)


def fake_root(td, symbols):
    """A repo root carrying just config/arm9/symbols.txt."""
    root = pathlib.Path(td) / "root"
    cfg = root / "config" / "arm9"
    cfg.mkdir(parents=True, exist_ok=True)
    rows = ["%s kind:function(arm,size=0x40) addr:0x%08x" % (n, a)
            for n, a in symbols]
    (cfg / "symbols.txt").write_text("\n".join(rows) + "\n")
    facegen._ROM_INDEX_CACHE.clear()
    facegen._JOIN_CACHE.clear()
    facegen._SPAN_CACHE.clear()
    facegen._RELOC_CACHE.clear()
    return str(root)


def fake_d0_root(td):
    """A synthetic ROM for the D0 rule: four classes and their relocations.

    Demo   D0 = D1 + Memory::Deallocate, heap pooled at data_020a0eac.
    Leaf   D0 = D1 + Memory::operator_delete2, no heap.
    More   D0 = D1 + Deallocate AND one more call, so it is NOT the D1 plus
           one deallocation and must refuse.
    Store  D0 = D1 + Deallocate and one more RELOCATED WORD than the D1
           stores, the extra-store shape, which must refuse too.
    """
    root = pathlib.Path(td) / "d0root"
    cfg = root / "config" / "arm9"
    cfg.mkdir(parents=True, exist_ok=True)
    syms = [
        ("_ZN4DemoD1Ev", 0x02001000, 0x20), ("_ZN4DemoD0Ev", 0x02001020, 0x30),
        ("_ZN4LeafD1Ev", 0x02001100, 0x10), ("_ZN4LeafD0Ev", 0x02001120, 0x20),
        ("_ZN4MoreD1Ev", 0x02001200, 0x20), ("_ZN4MoreD0Ev", 0x02001220, 0x40),
        ("_ZN5StoreD1Ev", 0x02001300, 0x20),
        ("_ZN5StoreD0Ev", 0x02001320, 0x40),
        ("_ZN6Memory10DeallocateEPvP4Heap", 0x0203C1E8, 0x28),
        ("_ZN6Memory16operator_delete2EPv", 0x0203CBCC, 0x0C),
        ("_ZN5Inner4KillEv", 0x02011000, 0x10),
        ("_ZN5Other4KillEv", 0x02012000, 0x10),
        ("data_020a0eac", 0x020A0EAC, 0x04),
        ("data_020a1000", 0x020A1000, 0x04),
        ("_ZTV4Store", 0x020A2000, 0x40),
    ]
    (cfg / "symbols.txt").write_text(
        "\n".join("%s kind:function(arm,size=0x%x) addr:0x%08x" % (n, z, a)
                  for n, a, z in syms) + "\n")
    rel = [
        (0x02001008, "arm_call", 0x02011000),        # Demo D1 -> Inner::Kill
        (0x02001028, "arm_call", 0x02011000),        # Demo D0, the same call
        (0x02001034, "arm_call", 0x0203C1E8),        # ... plus Deallocate
        (0x02001040, "load", 0x020A0EAC),            # ... the pooled heap
        (0x02001128, "arm_call", 0x0203CBCC),        # Leaf D0 -> delete2
        (0x02001208, "arm_call", 0x02011000),        # More D1 -> Inner::Kill
        (0x02001228, "arm_call", 0x02011000),        # More D0, the same
        (0x02001234, "arm_call", 0x0203C1E8),        # ... plus Deallocate
        (0x02001238, "arm_call", 0x02012000),        # ... AND one more call
        (0x02001250, "load", 0x020A0EAC),
        (0x02001308, "arm_call", 0x02011000),        # Store D1 -> Inner::Kill
        (0x02001318, "load", 0x020A2000),            # ... stores its vtable
        (0x02001328, "arm_call", 0x02011000),        # Store D0, the same call
        (0x02001334, "arm_call", 0x0203C1E8),        # ... plus Deallocate
        (0x02001350, "load", 0x020A2000),
        (0x02001354, "load", 0x020A0EAC),
        (0x02001358, "load", 0x020A1000),            # ... AND one more word
    ]
    (cfg / "relocs.txt").write_text(
        "\n".join("from:0x%08x kind:%s to:0x%08x module:main" % r
                  for r in rel) + "\n")
    facegen._ROM_INDEX_CACHE.clear()
    facegen._JOIN_CACHE.clear()
    facegen._SPAN_CACHE.clear()
    facegen._RELOC_CACHE.clear()
    return str(root)


SYMBOLS = [
    # 1. a Q target: public non-virtual
    ("_ZN13BigBrickBlock4KillEv", 0x020B38A0),
    # 2. a U target: public virtual
    ("_ZN11BillBlaster8BehaviorEv", 0x02126F8C),
    # 3. the sibling pair: D1 and D2 of one class at two addresses
    ("_ZN10FaderColorD1Ev", 0x02017574),
    ("_ZN10FaderColorD2Ev", 0x020175C4),
    # a class with a D1 and NO D2 must still generate
    ("_ZN10BowserFireD1Ev", 0x02116484),
    # 4. the arity ruling
    ("_ZN9ModelBase12ApplyOpacityEj", 0x0201A0B0),
    # 5. a two-argument reverse face
    ("_ZN5Actor9TrackStarEjj", 0x02012340),
    # a flat name with no ROM address at all
    ("_ZN7Nowhere6AbsentEv", 0x02000000),
]

def run():
    universe = [
        "?Kill@BigBrickBlock@@QAEXXZ",            # Q: public non-virtual
        "?Behavior@BillBlaster@@UAEHXZ",          # U: public virtual
        "??1FaderColor@@UAE@XZ",                  # one dtor for D1 AND D2
        "??1BowserFire@@UAE@XZ",
        "?ApplyOpacity@ModelBase@@QAEXIH@Z",      # 2 params, ROM name takes 1
        # THREE parameters where the ROM name takes two: TrackStar is the
        # second arity arm, and it must refuse here rather than drop one.
        "?TrackStar@Actor@@QAEIII@Z",
    ]

    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, SYMBOLS)
        flats = ["_" + n for n, _a in SYMBOLS]
        rows, refusals = facegen.derive_rows(flats, set(universe), root)
        by_flat = {r["flat"]: r for r in rows}
        why = dict(refusals)

        print("case 1: a Q target keeps a NON-virtual shadow")
        r = by_flat.get("__ZN13BigBrickBlock4KillEv")
        check(r is not None, "BigBrickBlock::Kill derived")
        if r:
            check(r["target"] == "?Kill@BigBrickBlock@@QAEXXZ",
                  "bound to the Q spelling")
            check(r["sig"]["virtual"] is False, "shadow is non-virtual")
            check(r["addr"] == 0x020B38A0, "carries the ROM address")
            decl = facegen._member_decl(r["rec"], r["sig"])
            check("virtual" not in decl, "declaration has no virtual: %s"
                  % decl)
            check(decl.startswith("public:"), "declared public")

        print("case 2: a U target gets a VIRTUAL shadow")
        r = by_flat.get("__ZN11BillBlaster8BehaviorEv")
        check(r is not None, "BillBlaster::Behavior derived")
        if r:
            check(r["sig"]["virtual"] is True, "shadow is virtual")
            decl = facegen._member_decl(r["rec"], r["sig"])
            check("virtual int Behavior()" in decl,
                  "declaration carries virtual: %s" % decl)

        print("case 3: a sibling by address is REFUSED, never chosen")
        for flat in ("__ZN10FaderColorD1Ev", "__ZN10FaderColorD2Ev"):
            check(flat not in by_flat, "%s not generated" % flat)
            check("rule 2" in why.get(flat, "") and
                  "plausible-sibling" in why.get(flat, ""),
                  "%s refused with the sibling reason" % flat)
        check("0x02017574" in why.get("__ZN10FaderColorD1Ev", "") and
              "0x020175c4" in why.get("__ZN10FaderColorD1Ev", ""),
              "the refusal names both ROM addresses")
        check("__ZN10BowserFireD1Ev" in by_flat,
              "a D1 with no D2 sibling still generates")

        print("case 4: an arg-count mismatch is REFUSED")
        flat = "__ZN9ModelBase12ApplyOpacityEj"
        check(flat not in by_flat, "ApplyOpacity not generated")
        check("ARITY MISMATCH" in why.get(flat, ""),
              "refused with the arity reason: %s" % why.get(flat, "")[:70])

        print("case 5: a name with no ROM address is REFUSED by rule 1")
        # the fake config lists it, so drop it from the index to prove rule 1
        idx = facegen.rom_index(root)
        idx.pop("_ZN7Nowhere6AbsentEv")
        rows2, refusals2 = facegen.derive_rows(["__ZN7Nowhere6AbsentEv"],
                                               set(universe), root)
        check(not rows2 and "rule 1" in dict(refusals2).get(
            "__ZN7Nowhere6AbsentEv", ""), "rule 1 refusal on a non-ROM name")

    print("case 6: a two-argument reverse face re-lands its arguments")
    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, [("_ZN5Actor9TrackStarEjj", 0x02012340)])
        rows, refusals = facegen.derive_rows(
            ["__ZN5Actor9TrackStarEjj"],
            {"?TrackStar@Actor@@QAEIII@Z"}, root)
        check(len(rows) == 1, "derived: %s" % (dict(refusals) or "yes"))
        if rows:
            r = rows[0]
            check(len(r["sig"]["params"]) == 2, "two parameters")
            out = pathlib.Path(td) / "two.cpp"
            text = facegen.emit_sync(rows, out)
            body = [ln for ln in text.splitlines()
                    if ln.startswith("{ return")]
            check(any("Actor::TrackStar(a0, a1)" in b for b in body),
                  "both arguments re-landed in order: %s" % body)
            check('extern "C" unsigned int __ZN5Actor9TrackStarEjj'
                  not in text, "the face is named without the C decoration")
            check("void *self, unsigned int a0, unsigned int a1" in text,
                  "receiver first, then the arguments")
            if "--verify" in sys.argv:
                good, msg = facegen.verify_sync(rows, out,
                                                pathlib.Path(td) / "probe")
                check(good, "verify: " + msg)

    print("case 7: the ledger round-trips and a disagreeing row is refused")
    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, [("_ZN13BigBrickBlock4KillEv", 0x020B38A0)])
        led = pathlib.Path(td) / "faces_sync.txt"
        led.write_text(
            "# a ledger\n"
            "__ZN13BigBrickBlock4KillEv  ?Kill@BigBrickBlock@@QAEXXZ  "
            "0x020b38a0 R\n")
        out = pathlib.Path(td) / "gen.cpp"
        rows, refusals = facegen.run_sync(str(led), root, str(out),
                                          strict=False)
        check(len(rows) == 1 and not refusals, "the good ledger generates")
        led.write_text(
            "__ZN13BigBrickBlock4KillEv  ?Kill@BigBrickBlock@@QAEXXZ  "
            "0x0deadbee R\n")
        rows, refusals = facegen.run_sync(str(led), root, str(out),
                                          strict=False)
        check(not rows and "LEDGER DISAGREES" in dict(refusals).get(
            "__ZN13BigBrickBlock4KillEv", ""),
            "a wrong address in the ledger is refused, not trusted")

    print("case 8: a forward face DEFINES the caller's member")
    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, [("_ZN13BigBrickBlock4KillEv", 0x020B38A0)])
        rows, refusals = facegen.derive_forward_rows(
            ["?Kill@BigBrickBlock@@UAEXXZ"],
            {"__ZN13BigBrickBlock4KillEv"}, root)
        check(len(rows) == 1, "derived: %s" % (dict(refusals) or "yes"))
        if rows:
            text = facegen.emit_sync(rows, pathlib.Path(td) / "fwd.cpp")
            check("void BigBrickBlock::Kill()" in text,
                  "the member is DEFINED, not declared")
            check("{ _ZN13BigBrickBlock4KillEv(this); }" in text,
                  "the body passes the receiver first")
            check("public: virtual void Kill();" in text,
                  "the shadow keeps the caller's virtualness")

    print("case 9: two faces that would call each other forever are broken up")
    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, [("_ZN13BigBrickBlock4KillEv", 0x020B38A0)])
        rev, _r = facegen.derive_rows(["__ZN13BigBrickBlock4KillEv"],
                                      {"?Kill@BigBrickBlock@@QAEXXZ"}, root)
        fwd, _f = facegen.derive_forward_rows(["?Kill@BigBrickBlock@@QAEXXZ"],
                                              {"__ZN13BigBrickBlock4KillEv"},
                                              root)
        check(len(rev) == 1 and len(fwd) == 1, "both directions derive alone")
        kept, cyc = facegen.refuse_face_cycles(rev + fwd)
        check(len(kept) == 1 and not kept[0].get("forward"),
              "the reverse row is kept")
        check(len(cyc) == 1 and "FACE CYCLE" in cyc[0][1],
              "the forward row is refused as a cycle: %s" % (cyc or None))

    # -----------------------------------------------------------------------
    # THE D0 RULE (lane FACES2). Four arms, all on a synthetic ROM so the
    # test says what it means rather than what today's config happens to hold:
    # a Deallocate class, an operator_delete2 class, a D0 that makes a call
    # its D1 does not (so it is not "the D1 plus one deallocation"), and a D0
    # that stores one more relocated word than its D1 -- the extra-store
    # shape, which at reloc level is an extra pooled address.
    print("case 9b: a forward face for a STRUCTOR is refused, and says why")
    with tempfile.TemporaryDirectory() as td:
        root = fake_root(td, [("_ZN10BowserFireD1Ev", 0x02116484)])
        rows, ref = facegen.derive_forward_rows(
            ["??1BowserFire@@UAE@XZ"], {"__ZN10BowserFireD1Ev"}, root)
        check(not rows and len(ref) == 1, "refused, not generated")
        check(ref and "??_7BowserFire@@6B@" in ref[0][1],
              "the reason names the vftable it would emit: %s"
              % (ref[0][1][:90] if ref else None))
        check(ref and "not a class-member mangle" not in ref[0][1],
              "and not the old wrong reason, which the gate produced")

    print("case 10: a Deallocate D0 derives, with the heap from the reloc")
    with tempfile.TemporaryDirectory() as td:
        root = fake_d0_root(td)
        rows, ref = facegen.derive_d0_rows(
            [("__ZN4DemoD0Ev", "_ZN4DemoD1Ev", 0x02001020, "D",
              "_ZN6Memory10DeallocateEPvP4Heap", "data_020a0eac")],
            {"__ZN4DemoD1Ev"}, root)
        check(len(rows) == 1, "derived: %s" % (dict(ref) or "yes"))
        if rows:
            text = facegen.emit_sync(rows, pathlib.Path(td) / "d0.cpp")
            check('extern "C" void _ZN4DemoD0Ev(void *self)' in text,
                  "the face DEFINES the flat D0 name")
            check("{ _ZN4DemoD1Ev(self); "
                  "_ZN6Memory10DeallocateEPvP4Heap(self, data_020a0eac); }"
                  in text,
                  "the body is the D1 and then the one deallocation")

    print("case 11: an operator_delete2 D0 derives and names no heap")
    with tempfile.TemporaryDirectory() as td:
        root = fake_d0_root(td)
        rows, ref = facegen.derive_d0_rows(
            [("__ZN4LeafD0Ev", "??1Leaf@@UAE@XZ", 0x02001120, "D",
              "_ZN6Memory16operator_delete2EPv", "-")],
            {"??1Leaf@@UAE@XZ"}, root)
        check(len(rows) == 1, "derived: %s" % (dict(ref) or "yes"))
        if rows:
            text = facegen.emit_sync(rows, pathlib.Path(td) / "d0b.cpp")
            check("{ ((Leaf *)self)->Leaf::~Leaf(); "
                  "_ZN6Memory16operator_delete2EPv(self); }" in text,
                  "the body runs the MSVC destructor and then the delete")
            check("public: virtual ~Leaf();" in text,
                  "the shadow keeps the destructor's virtualness")
        # the same row with a heap column must refuse: that call takes none
        _rows, ref2 = facegen.derive_d0_rows(
            [("__ZN4LeafD0Ev", "??1Leaf@@UAE@XZ", 0x02001120, "D",
              "_ZN6Memory16operator_delete2EPv", "data_020a0eac")],
            {"??1Leaf@@UAE@XZ"}, root)
        check(len(ref2) == 1 and "takes no heap pointer" in ref2[0][1],
              "a heap column on operator_delete2 is refused: %s"
              % (ref2 or None))

    print("case 12: a D0 that makes a call its D1 does not is REFUSED")
    with tempfile.TemporaryDirectory() as td:
        root = fake_d0_root(td)
        rows, ref = facegen.derive_d0_rows(
            [("__ZN4MoreD0Ev", "_ZN4MoreD1Ev", 0x02001220, "D",
              "_ZN6Memory10DeallocateEPvP4Heap", "data_020a0eac")],
            {"__ZN4MoreD1Ev"}, root)
        check(not rows and len(ref) == 1, "refused, not generated")
        check(ref and "not the one deallocation" in ref[0][1],
              "the reason names the extra call: %s"
              % (ref[0][1][:120] if ref else None))

    print("case 13: a D0 that stores one more relocated word is REFUSED")
    with tempfile.TemporaryDirectory() as td:
        root = fake_d0_root(td)
        rows, ref = facegen.derive_d0_rows(
            [("__ZN5StoreD0Ev", "_ZN5StoreD1Ev", 0x02001320, "D",
              "_ZN6Memory10DeallocateEPvP4Heap", "data_020a0eac")],
            {"__ZN5StoreD1Ev"}, root)
        check(not rows and len(ref) == 1, "refused, not generated")
        check(ref and "not the one heap pointer" in ref[0][1],
              "the reason names the extra pooled word: %s"
              % (ref[0][1][:140] if ref else None))
        # and a row whose heap column names a word the ROM does not pool
        rows2, ref2 = facegen.derive_d0_rows(
            [("__ZN4DemoD0Ev", "_ZN4DemoD1Ev", 0x02001020, "D",
              "_ZN6Memory10DeallocateEPvP4Heap", "data_020a1000")],
            {"__ZN4DemoD1Ev"}, root)
        check(not rows2 and ref2 and "not the one heap pointer" in ref2[0][1],
              "a wrong heap column is refused: %s"
              % (ref2[0][1][:120] if ref2 else None))

    print("")
    if FAILED:
        print("test_facegen FAIL (%d)" % len(FAILED))
        for f in FAILED:
            print("   ", f)
        return 1
    print("test_facegen PASS")
    return 0


if __name__ == "__main__":
    sys.exit(run())
