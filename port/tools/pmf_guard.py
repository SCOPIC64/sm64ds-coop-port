#!/usr/bin/env python
"""Refuse a build that seats a stack-receiver body in a pointer-to-member cell.

THE DEFECT THIS CATCHES, which nine lanes have now each found at a different
class (5ae983797 FlameChomp, 27a24ff5a ChainChomp, 651b5e853 daPgDfdr_c,
f9936e798 daGmch_c, 45ce69707 daPgMthr_c, 00732a5ab SnowmanHead and
daBgSnmBdy_c, then this lane at TreasureChest, Bird, PrincessPeach, Butterfly
and KingBobOmb):

  A ROM state record holds a pointer to member.  MSVC calls one by putting
  `this + delta` in ECX and pushing NOTHING, whether it reaches the record
  through a `call` or through a tail `jmp`.  A port seat that writes a __cdecl
  word into that record -- one of hal/faces_sync_gen.cpp's flat C faces, or a
  matched body taken raw -- gives the body a receiver it has to read off the
  caller's stack, and nobody put one there.  It shows up as a junk receiver, a
  null receiver, or control landing on a data word.

  The seat header is usually the thing that is wrong and it is wrong the same
  way each time: it reasons that the dispatcher is a tail jump, so the caller's
  own argument is still in place.  That is true of a FLAT C dispatcher whose
  first stack argument is the receiver (BabyPenguin's host copies, the path
  lift's func_ov002_020efa54, the puzzle piece's func_ov064_0211982c), and it
  is false as soon as the dispatcher is a __thiscall member, and false again as
  soon as /O2 inlines it into a caller in the same translation unit.

THE CRITERION, and why it is mechanical.  Every seat in port/ writes its cells
out of one static table in its own object's .rdata, so the code word that ends
up in each cell is IN THE BUILT IMAGE and can be read back.  For each seat
table this file names, the guard reads every code word out of the image, walks
the body's prologue, and asks where the receiver comes from:

  ECX      the body reads ecx before anything overwrites it: a __thiscall
           member, or the __fastcall thunk this campaign seats over a face.
  STACK    the body never reads ecx and does read [ebp+8] / [esp+4].
  NEITHER  neither: the body ignores its receiver.

A __fastcall face that ALSO takes a stack argument reads both, and ECX wins:
what decides is where the RECEIVER comes from.  A STACK word in a table this
file marks ECX is the defect, and the guard refuses.

The tables marked CDECL are the adjudicated exceptions, each with the flat C
dispatcher that does push the receiver named beside it.  A thunk on one of
those would break what works, so they are recorded here rather than left for
the next lane to rediscover.

usage: pmf_guard.py [--root <tree>] [--exe <exe>] [--map <map>] [--selftest]
                    [--list]
"""
import argparse
import bisect
import os
import re
import struct
import sys

try:
    import capstone
except ImportError:
    sys.stderr.write("pmf_guard: capstone is not installed\n")
    raise SystemExit(2)


# ---- the ledger -----------------------------------------------------------
# One row per seat table, matched against the map by REGEX because MSVC
# decorates a file-scope static with its own unnamed struct tag.  ECX rows are
# checked; CDECL rows are the adjudicated exceptions and are only reported.
LEDGER = [
    ("ECX", r"^\?g_treasure_chest_states@@",
     "TreasureChest: ?SetState@TreasureChest@@QAEXH@Z calls through "
     "_data_ov064_0211c98c with ecx = this + delta, ?CallStateBehavior tail "
     "jumps the same way; both are __thiscall members, so no receiver is ever "
     "on the stack"),
    ("ECX", r"^\?g_princess_peach_states@@",
     "PrincessPeach: ?CallStateInit and ?CallStateBehavior both tail jump "
     "through the record at +0x350 with ecx = this + delta, the shape "
     "00732a5ab fixed for SnowmanHead one overlay over"),
    ("ECX", r"^\?g_butterfly_states@@",
     "Butterfly: ?Behavior@Butterfly@@UAEHXZ carries the call inline through "
     "_data_ov100_02148628 with ecx = this + delta and nothing pushed"),
    ("ECX", r"^\?g_bird@@",
     "Bird: ?Behavior@Bird@@UAEHXZ calls through _data_ov009_02113c48 with "
     "ecx = this + delta and nothing pushed"),
    ("ECX", r"^\?g_king_states@@",
     "KingBobOmb: five of the class's own matched bodies inline the state "
     "change and call through _data_ov078_02126ffc with ecx = this + delta; "
     "_KingBobOmb_SetState carries ecx on its tail-jump path too"),
    ("ECX", r"^\?g_smb_cells@@",
     "daBgSnmBdy_c: ?Behavior and ?InitResources both carry the call inline "
     "with ecx = this + delta (00732a5ab)"),
    ("ECX", r"^\?g_smh_cells@@",
     "SnowmanHead: ?SetState calls ?CallStateInit with nothing pushed and "
     "both Call* helpers tail jump with ecx = this + delta (00732a5ab)"),

    ("CDECL", r"^\?g_bp_cells@@",
     "BabyPenguin: the dispatchers are the host copies "
     "BabyPenguin_StateEnter.cpp and BabyPenguin_StateTick.cpp, which call "
     "the cell as a plain function pointer and DO push the receiver, so a "
     "thunk here breaks what works (00732a5ab left these alone on purpose)"),
    ("CDECL", r"^\?seats@\?1\?\?port_pathlift_states_seat@@",
     "dPathLiftActor_c: the three TICK halves already carry __fastcall faces; "
     "the three ENTER halves are read by NOTHING but func_ov002_020efa54, a "
     "flat C dispatcher f(self, state) that tail jumps, so [esp+4] really is "
     "the receiver there (proved by a byte scan of .text for every reference "
     "to _data_ov002_0210af2c+0/+4)"),
    ("CDECL", r"^\?g_piece_states@@",
     "BowserPuzzlePiece: the .b half already carries a __fastcall face; the "
     ".a half's only reader is func_ov064_0211982c, a flat C dispatcher "
     "f(self, state) that tail jumps"),
]


# ---- image ----------------------------------------------------------------
MAPROW = re.compile(
    r"\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+(\S*)\s*(\S+)?\s*$")


def sections(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    optsz = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    off = pe + 24 + optsz
    out = []
    for i in range(nsec):
        s = off + i * 40
        out.append((struct.unpack_from("<I", data, s + 12)[0],
                    struct.unpack_from("<I", data, s + 8)[0],
                    struct.unpack_from("<I", data, s + 20)[0],
                    struct.unpack_from("<I", data, s + 16)[0]))
    return base, out


class Image(object):
    def __init__(self, exe, mp):
        self.data = open(exe, "rb").read()
        self.base, self.secs = sections(self.data)
        self.syms = []
        base = 0x400000
        started = False
        with open(mp, encoding="utf-8", errors="replace") as f:
            for line in f:
                if "Preferred load address is" in line:
                    base = int(line.split()[-1], 16)
                if "Publics by Value" in line:
                    started = True
                    continue
                if not started:
                    continue
                m = MAPROW.match(line)
                if m:
                    obj = m.group(4) or m.group(3)
                    self.syms.append((int(m.group(2), 16) - base, m.group(1), obj))
        self.syms.sort()
        self.keys = [s[0] for s in self.syms]
        self.md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        va, vsz, raw, rsz = self.secs[0]
        self.text = (va, va + max(vsz, rsz))

    def off(self, rva):
        for va, vsz, raw, rsz in self.secs:
            if va <= rva < va + max(vsz, rsz):
                d = rva - va
                if d < rsz:
                    return raw + d
        return None

    def code(self, rva, n):
        o = self.off(rva)
        return b"" if o is None else self.data[o:o + n]

    def name_at(self, rva):
        i = bisect.bisect_right(self.keys, rva) - 1
        if i < 0:
            return ("?", "?", 0)
        r, n, o = self.syms[i]
        return (n, o, rva - r)


# ---- the classifier -------------------------------------------------------
STACKOP = re.compile(r"\[ebp \+ 8\]|\[esp \+ 4\]")
ECXWORD = re.compile(r"\becx\b")
READS_ITS_OPERAND = ("push", "jmp", "call", "test", "cmp")
KILLS_ITS_FIRST = ("mov", "lea", "pop", "movzx", "movsx", "xor")


def receiver_of_bytes(md, blob, rva, limit=60):
    stack_read = False
    ecx_dead = False
    for i in list(md.disasm(blob, rva))[:limit]:
        ops = i.op_str
        if STACKOP.search(ops):
            stack_read = True
        if ECXWORD.search(ops) and not ecx_dead:
            if i.mnemonic in READS_ITS_OPERAND:
                return "ECX"
            if ops.split(",")[0].strip() != "ecx":
                return "ECX"
        if ops.split(",")[0].strip() == "ecx" and i.mnemonic in KILLS_ITS_FIRST:
            ecx_dead = True
        if i.mnemonic == "ret":
            break
    return "STACK" if stack_read else "NEITHER"


def receiver_of(img, rva):
    return receiver_of_bytes(img.md, img.code(rva, 760), rva)


# ---- the census -----------------------------------------------------------
def cells_of(img, sym_rva, end_rva):
    """Every code word stored in [sym_rva, end_rva)."""
    out = []
    off = img.off(sym_rva)
    if off is None:
        return out
    n = end_rva - sym_rva
    blob = img.data[off:off + n]
    for k in range(0, len(blob) - 3, 4):
        w = struct.unpack_from("<I", blob, k)[0]
        if w < img.base:
            continue
        r = w - img.base
        if img.text[0] <= r < img.text[1]:
            out.append((sym_rva + k, r))
    return out


def census(img):
    datasyms = [(r, n, o) for r, n, o in img.syms
                if not (img.text[0] <= r < img.text[1])]
    datasyms.sort()
    rows = []
    for idx, (rva, name, obj) in enumerate(datasyms):
        for kind, pat, why in LEDGER:
            if re.match(pat, name):
                end = rva + 0x400
                for j in range(idx + 1, len(datasyms)):
                    if datasyms[j][0] > rva:
                        end = datasyms[j][0]
                        break
                for at, target in cells_of(img, rva, end):
                    rows.append((kind, name, obj, at, target,
                                 img.name_at(target), receiver_of(img, target),
                                 why))
                break
    return rows


# ---- the self-test --------------------------------------------------------
# Three real prologues, as bytes, so the check is proved without the image.
FLAT_FACE = bytes(bytearray([
    0x55,                          # push ebp
    0x8B, 0xEC,                    # mov ebp, esp
    0x8B, 0x4D, 0x08,              # mov ecx, dword ptr [ebp+8]
    0x5D,                          # pop ebp
    0xE9, 0x00, 0x00, 0x00, 0x00,  # jmp <member>
]))
CDECL_BODY = bytes(bytearray([
    0x55,                          # push ebp
    0x8B, 0xEC,                    # mov ebp, esp
    0x56,                          # push esi
    0x8B, 0x75, 0x08,              # mov esi, dword ptr [ebp+8]
    0xC3,                          # ret
]))
FASTCALL_THUNK = bytes(bytearray([
    0x51,                          # push ecx
    0xE8, 0x00, 0x00, 0x00, 0x00,  # call <face>
    0x83, 0xC4, 0x04,              # add esp, 4
    0xC3,                          # ret
]))
FASTCALL_WITH_STACK_ARG = bytes(bytearray([
    0x55,                          # push ebp
    0x8B, 0xEC,                    # mov ebp, esp
    0xFF, 0x75, 0x08,              # push dword ptr [ebp+8]   the int argument
    0x51,                          # push ecx                 the receiver
    0xE8, 0x00, 0x00, 0x00, 0x00,  # call <body>
    0xC2, 0x04, 0x00,              # ret 4
]))
THISCALL_MEMBER = bytes(bytearray([
    0x8B, 0xC1,                    # mov eax, ecx
    0x8B, 0x50, 0x10,              # mov edx, dword ptr [eax+0x10]
    0xC3,                          # ret
]))

SELFTESTS = [
    ("a flat C face from hal/faces_sync_gen.cpp", FLAT_FACE, "STACK"),
    ("a raw matched cdecl body", CDECL_BODY, "STACK"),
    ("the __fastcall thunk this campaign seats", FASTCALL_THUNK, "ECX"),
    ("a __fastcall face that also takes a stack argument",
     FASTCALL_WITH_STACK_ARG, "ECX"),
    ("a __thiscall member", THISCALL_MEMBER, "ECX"),
]


def selftest():
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    bad = 0
    for what, blob, want in SELFTESTS:
        got = receiver_of_bytes(md, blob, 0x401000)
        ok = "ok " if got == want else "FAIL"
        print("  %s %-52s want %-7s got %s" % (ok, what, want, got))
        if got != want:
            bad += 1
    if bad:
        print("pmf_guard --selftest: %d of %d classifications are WRONG, so the "
              "guard cannot see the defect it exists for" % (bad, len(SELFTESTS)))
        return 1
    print("pmf_guard --selftest OK: %d prologues, every one classified as the "
          "campaign's own evidence says" % len(SELFTESTS))
    return 0


# ---- main -----------------------------------------------------------------
def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--root", default=".")
    ap.add_argument("--exe")
    ap.add_argument("--map")
    ap.add_argument("--selftest", action="store_true")
    ap.add_argument("--list", action="store_true")
    a = ap.parse_args(argv)
    if a.selftest:
        return selftest()

    exe = a.exe or os.path.join(a.root, "build", "port", "walk_window.exe")
    mp = a.map or os.path.join(a.root, "build", "port", "walk_window.map")
    if not os.path.exists(exe) or not os.path.exists(mp):
        print("pmf_guard: no built image at %s -- nothing to check" % exe)
        return 0

    rc = selftest()
    if rc:
        return rc

    img = Image(exe, mp)
    rows = census(img)
    seen = set(name for _, name, _, _, _, _, _, _ in rows)
    missing = []
    for kind, pat, why in LEDGER:
        if not any(re.match(pat, n) for n in seen):
            missing.append(pat)
    if missing:
        print("pmf_guard REFUSES: %d seat table(s) in the ledger are not in the "
              "map, so the ledger has gone stale and the check is not running "
              "on them:" % len(missing))
        for m in missing:
            print("    %s" % m)
        return 1

    checked = [r for r in rows if r[0] == "ECX"]
    adjudicated = [r for r in rows if r[0] == "CDECL"]
    bad = [r for r in checked if r[6] == "STACK"]

    if a.list:
        for kind, name, obj, at, target, tn, klass, why in rows:
            print("%-6s %08x  %-44s -> %-8s %s+0x%x [%s]" % (
                kind, img.base + at, name[:44], klass, tn[0], tn[2], tn[1]))

    if bad:
        print("pmf_guard REFUSES: %d cell(s) in a pointer-to-member table hold "
              "a body that reads its receiver off the caller's stack. The call "
              "site passes it in ECX and pushes nothing, so the body will read "
              "whatever the caller last spilled." % len(bad))
        for kind, name, obj, at, target, tn, klass, why in bad:
            print("")
            print("  %s  [%s]" % (name, obj))
            print("    cell at %08x holds %08x = %s+0x%x [%s]" % (
                img.base + at, img.base + target, tn[0], tn[2], tn[1]))
            print("    its prologue reads [ebp+8]/[esp+4] and never reads ecx")
            print("    why this table needs ECX: %s" % why)
        print("")
        print("Fix: seat a __fastcall thunk that names the body, the shape "
              "5ae983797 / 27a24ff5a / 651b5e853 / f9936e798 / 45ce69707 / "
              "00732a5ab all use. If the cell is really reached only by a flat "
              "C dispatcher that pushes the receiver, move its table to a "
              "CDECL row in this file's ledger WITH the dispatcher named.")
        return 1

    print("pmf_guard OK: %d cell(s) across %d pointer-to-member table(s) all "
          "take their receiver in ECX; %d cell(s) across %d table(s) are the "
          "adjudicated __cdecl exceptions." % (
              len(checked), len(set(r[1] for r in checked)),
              len(adjudicated), len(set(r[1] for r in adjudicated))))
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
