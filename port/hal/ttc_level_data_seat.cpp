/* run linkw wave 18 (lane w18): the TICK TOCK CLOCK level-data seat.
 *
 * THE JOB. Every Ttc class in ov065 reaches a CLPS collision block or an
 * animation descriptor that lives in whichever LEVEL overlay is loaded -- ov035
 * on level 27. Wave 17 seated the whole Ttc cluster without closing those reads
 * and measured what it costs: ids 108 (x8) and 114 (x5) ticked 300 frames
 * WITHOUT faulting, silently, on a garbage CLPS, and id 112 divided by zero in
 * Animation::Advance because its descriptor read zero frames. The quiet pair is
 * the worse half and is why that seat came back out.
 *
 * WHY THE MOUNT ALONE IS NOT THE ANSWER, WHICH IS THIS LANE'S CORRECTION.
 * The ranking this lane inherited said the remedy was "a per-symbol dual mount
 * alongside the whole mount -- ov022 already does exactly this". The dual mount
 * (port/ov035_syms.txt) is NECESSARY: it is what makes the ten CLPS blocks and
 * two descriptors exist as named host storage at all. It is NOT SUFFICIENT, and
 * tools/ovdata.py's own cross pass says why in as many words:
 *
 *     "Every level overlay is linked at the SAME base, because the DS only ever
 *      has one loaded ... Binding it to the ov009 copy would be right on the
 *      castle grounds and a walk over another level's bytes everywhere else,
 *      which is the class of bug this pass exists to remove. Resolving it
 *      correctly needs a seat that re-patches per loaded level; until there is
 *      one, raw is the honest answer and the sweep keeps it visible."
 *
 * MEASURED, not argued: each of the twelve target addresses lands inside
 * EIGHTEEN mounted overlay windows (ov009 ov010 ov012 ov013 ov014 ov015 ov016
 * ov018 ov019 ov020 ov021 ov022 ov025 ov035 ov045 ov052 ov056 ov060). The cross
 * pass drops a target covered by more than one window, so adding ov035 takes
 * that count from seventeen to eighteen and rebases nothing. `cross: ov065 9
 * pointers` before the mount and `cross: ov065 9` after it -- the same nine,
 * all into ov002 gaps. So the Ttc cluster's cost was re-ranked DOWNWARD on a
 * premise that does not hold; this file is the machinery that was missing.
 *
 * WHY A SEAT IS ALLOWED TO DO WHAT THE CROSS PASS REFUSES. The cross pass must
 * be conservative because it cannot know which level is loaded. A seat run FROM
 * THE LEVEL-27 MOUNT ROW knows exactly that, which is the disambiguation the
 * pass is missing. The Ttc classes exist only in Tick Tock Clock, so binding
 * their level-window reads to ov035's copy is right wherever they can run, and
 * on every other level these words keep their raw ROM value and nothing reads
 * them -- ov065's other residents (Snufit 236, Swoop 237, Dorrie 168,
 * DorrieCap 169) do not touch these four blocks.
 *
 * THE PRECEDENT is port_ov089_keymodels_fixup() in hal/actor_overlays.cpp,
 * which closes ov089's six LoadKeyModels cross pointers by hand for the same
 * reason and with the same ROM-value check. That seat is even named in the
 * cross pass's own HAND_SEATED table.
 *
 * ==== WHAT IS SEATED, AND WHAT IS NOT ======================================
 *
 * ov065 makes TWELVE reads into the level window. They split by WHERE the word
 * lives, and only one half is a seat's business:
 *
 *   EIGHT are words inside four ov065 DATA blocks, all four already in ov065's
 *   per-symbol mount. Those are this file's eight rewrites.
 *
 *   FOUR are literal-pool words inside ov065 FUNCTION bodies --
 *   func_ov065_0211a358 +0xfc, func_ov065_0211b1d4 +0x140,
 *   _ZN15TtcRotatingGear13InitResourcesEv +0xe0 and
 *   _ZN14TtcMovingCubeA13InitResourcesEv +0x138. The port does not mount
 *   ov065's .text, so there is no word to rewrite: those resolve at LINK time,
 *   because the matched TU names data_ov035_02112118 / _02112198 / _021121b8 /
 *   _02112258 and the dual mount is what makes those names resolve. Nothing to
 *   do here, but they are the other half of why the mount is required.
 *
 * The four blocks and their eight words, read out of
 * extracted/overlays/overlay_0065.bin and cross-checked against
 * config/arm9/overlays/ov065/relocs.txt. Each block is 0x10 (or 0x8) and EVERY
 * word in it is a relocation, so the reloc run and the next-symbol landing
 * agree on the extent and the run terminates exactly at the boundary -- the one
 * place in this lane where both width routes told the same story:
 *
 *   data_ov065_0211cfd8  +0x0 -> 0x02112138  CLPS, ids 108/109
 *                        +0x4 -> ov065 SharedFilePtr (mount's own pass)
 *                        +0x8 -> ov065 SharedFilePtr (mount's own pass)
 *                        +0xc -> 0x02112178  CLPS, ids 108/109
 *   data_ov065_0211d16c  +0x0 -> 0x0211208c  animation descriptor, id 111
 *                        +0x4 -> 0x021120a4  animation descriptor, id 112
 *   data_ov065_0211d19c  +0x0 -> 0x021120d8  CLPS
 *                        +0xc -> 0x021121f8  CLPS
 *   data_ov065_0211d364  +0x0 -> 0x021120f8  CLPS, ids 114/115
 *                        +0xc -> 0x02112158  CLPS
 *
 * IDEMPOTENT BY VALUE, not by a done-guard. Level 27 can be entered more than
 * once, and a re-entry may or may not have reset .dsstate back to ROM values, so
 * a `static int done` would be wrong in one of the two cases. Instead each word
 * is accepted if it holds EITHER its ROM address or the host address this seat
 * would write; anything else is a third value nobody predicted and aborts. That
 * is strictly stronger than the done-guard the sibling-fill rule asks for.
 *
 * THE READBACK IS THE PROOF. Wave 17's finding was that a class ticking cleanly
 * proves nothing -- ids 108 and 114 ran 300 clean frames on garbage. So after
 * rewriting, this seat DEREFERENCES each seated pointer and checks the storage
 * it now names really is what the ROM says it is: the six CLPS blocks must read
 * the 'CLPS' magic (0x53504c43) and a sane {entrySize, count} header, and the
 * two animation descriptors must read a nonzero frame count -- which is exactly
 * the field whose zero divided by zero in Animation::Advance. A pointer that
 * lands on plausible-looking garbage fails here instead of ticking quietly.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

/* the four ov065 blocks, already in ov065's per-symbol mount */
extern unsigned char data_ov065_0211cfd8[];
extern unsigned char data_ov065_0211d16c[];
extern unsigned char data_ov065_0211d19c[];
extern unsigned char data_ov065_0211d364[];

/* the eight ov035 blocks they must name, from port/ov035_syms.txt */
extern unsigned char data_ov035_0211208c[];
extern unsigned char data_ov035_021120a4[];
extern unsigned char data_ov035_021120d8[];
extern unsigned char data_ov035_021120f8[];
extern unsigned char data_ov035_02112138[];
extern unsigned char data_ov035_02112158[];
extern unsigned char data_ov035_02112178[];
extern unsigned char data_ov035_021121f8[];

void port_ttc_level_data_seat(void);

}  /* extern "C" */

namespace {

const unsigned CLPS_MAGIC = 0x53504c43u;   /* 'CLPS' */

enum Kind { KIND_CLPS, KIND_ANIM };

struct Seat {
    unsigned char *block;      /* the ov065 block holding the word      */
    const char *blockName;
    unsigned off;              /* byte offset of the word in that block */
    unsigned rom;              /* the DS address the ROM word carries   */
    unsigned char *host;       /* the ov035 storage it must name        */
    Kind kind;
    const char *what;
};

const Seat g_seats[] = {
    {data_ov065_0211cfd8, "data_ov065_0211cfd8", 0x0, 0x02112138, data_ov035_02112138, KIND_CLPS, "CLPS ids 108/109"},
    {data_ov065_0211cfd8, "data_ov065_0211cfd8", 0xc, 0x02112178, data_ov035_02112178, KIND_CLPS, "CLPS ids 108/109"},
    {data_ov065_0211d16c, "data_ov065_0211d16c", 0x0, 0x0211208c, data_ov035_0211208c, KIND_ANIM, "anim descriptor id 111"},
    {data_ov065_0211d16c, "data_ov065_0211d16c", 0x4, 0x021120a4, data_ov035_021120a4, KIND_ANIM, "anim descriptor id 112"},
    {data_ov065_0211d19c, "data_ov065_0211d19c", 0x0, 0x021120d8, data_ov035_021120d8, KIND_CLPS, "CLPS"},
    {data_ov065_0211d19c, "data_ov065_0211d19c", 0xc, 0x021121f8, data_ov035_021121f8, KIND_CLPS, "CLPS"},
    {data_ov065_0211d364, "data_ov065_0211d364", 0x0, 0x021120f8, data_ov035_021120f8, KIND_CLPS, "CLPS ids 114/115"},
    {data_ov065_0211d364, "data_ov065_0211d364", 0xc, 0x02112158, data_ov035_02112158, KIND_CLPS, "CLPS"},
};

const int SEAT_COUNT = (int)(sizeof(g_seats) / sizeof(g_seats[0]));

unsigned rd32(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8) |
           ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

/* Does the storage this pointer now names really hold what the ROM says?
   This is the half wave 17 could not do: a Ttc class ticking cleanly on a
   garbage CLPS produced 300 green frames and told nobody. */
bool readback_ok(const Seat &s, char *why, unsigned long whyLen)
{
    if (s.kind == KIND_CLPS) {
        unsigned magic = rd32(s.host);
        unsigned hdr = rd32(s.host + 4);
        unsigned esz = hdr & 0xffffu;
        unsigned cnt = (hdr >> 16) & 0xffffu;
        if (magic != CLPS_MAGIC) {
            std::snprintf(why, (size_t)whyLen,
                          "magic %08x, wanted 'CLPS' %08x", magic, CLPS_MAGIC);
            return false;
        }
        if (esz != 8 || cnt == 0 || cnt > 2) {
            std::snprintf(why, (size_t)whyLen,
                          "header entrySize=%u count=%u, every ov035 block is 8/1..2",
                          esz, cnt);
            return false;
        }
        std::snprintf(why, (size_t)whyLen, "'CLPS' entrySize=%u count=%u", esz, cnt);
        return true;
    }
    /* the animation descriptor: word[0] is the frame count Animation::Advance
       divides by, and reading it as zero is the c0000094 wave 17 measured. */
    unsigned frames = rd32(s.host);
    if (frames == 0) {
        std::snprintf(why, (size_t)whyLen, "frame count 0 -- the divide-by-zero shape");
        return false;
    }
    std::snprintf(why, (size_t)whyLen, "frames=%u", frames);
    return true;
}

}  /* namespace */

extern "C" void port_ttc_level_data_seat(void)
{
    int seated = 0, already = 0;
    for (int i = 0; i < SEAT_COUNT; ++i) {
        const Seat &s = g_seats[i];
        unsigned char *word = s.block + s.off;
        unsigned have = rd32(word);
        unsigned host = (unsigned)(size_t)s.host;
        if (have == host) {
            ++already;                     /* re-entry, .dsstate not reset */
        } else if (have == s.rom) {
            word[0] = (unsigned char)(host & 0xff);
            word[1] = (unsigned char)((host >> 8) & 0xff);
            word[2] = (unsigned char)((host >> 16) & 0xff);
            word[3] = (unsigned char)((host >> 24) & 0xff);
            ++seated;
        } else {
            std::fprintf(stderr,
                         "FATAL: Ttc level-data seat: %s+%#x holds %08x, the ROM "
                         "says %08x and this seat writes %08x -- WRONG BYTES\n",
                         s.blockName, s.off, have, s.rom, host);
            std::abort();
        }
        char why[96];
        if (!readback_ok(s, why, sizeof why)) {
            std::fprintf(stderr,
                         "FATAL: Ttc level-data seat: %s+%#x now names %08x for "
                         "%s, but that storage reads %s\n",
                         s.blockName, s.off, host, s.what, why);
            std::abort();
        }
        std::fprintf(stderr, "  [ttc-seat] %s+%#-4x -> %s  (%s, %s)\n",
                     s.blockName, s.off, s.what, why,
                     have == host ? "already seated" : "seated");
    }
    std::fprintf(stderr,
                 "  [ttc-seat] %d level-window reads closed onto ov035 (%d newly "
                 "seated, %d already), all readbacks verified\n",
                 SEAT_COUNT, seated, already);
}
