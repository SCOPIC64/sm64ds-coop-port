// THE OAM SPRITE-TEMPLATE GUARD.
//
// OAM::Render takes an OamAttr* and walks it until it reads a3 == 0xffff:
//
//     ...
//     if (attr->a3 == 0xffff) return;
//     attr++;
//
// There is no other exit for an entry that gets CULLED. The `*pCount >= 0x80`
// guard at the top only stops a list that is EMITTING, and the four cull
// branches (x + w < 0, x > 0x100, y + h < 0, y > 0xc0) take the `attr++;
// continue` path without touching pCount. So a list with no terminator is a
// bounded walk while its sprites are on screen and an UNBOUNDED one the moment
// they go off, and the DS's flat memory is what the walk runs through.
//
// Every one of those lists comes from a POINTER TABLE in overlay data, and on
// the host every one of those pointers has to be rebased off its DS address
// onto the host mount (port/tools/ovdata.py's pointer pass, plus the
// cross-overlay fixups in hal/sub_actors.cpp). A pointer the rebase MISSES
// still holds its DS address, and the port reserves DS main RAM at 0x02000000
// as committed zeroed pages (ntr/io.cpp), so the miss does not trap where it
// happens. It reads as an all-zero OamAttr list -- no terminator anywhere --
// and the fault lands 2.9 MB later at the end of the reservation, inside
// OAM::Render, with nothing on the stack to say which table it came from. If
// the reservation is not mapped at all (VirtualAlloc loses the address to the
// host allocator, which io_init treats as non-fatal) the same miss faults on
// the first read instead.
//
// So: check the tables, once, up front, where the answer is still legible.
// This is a static check with no runtime cost after the first frame, and it
// names the table and the index rather than leaving a bare access violation
// at an address in nobody's module.
#include <cstdio>
#include <cstdlib>

extern "C" {

/* The minimap's, from Minimap::Render. */
extern void *_ZN3OAM15MM_PLAYER_ICONSE[];
extern void *_ZN3OAM18MM_VS_PLAYER_ICONSE[];
extern void *_ZN3OAM20MM_VS_PLAYER_ICONS_SE[];
extern void *_ZN3OAM15MM_STAR_MARKERSE[];
extern void *_ZN3OAM12MM_STAR_KEYSE[];
extern void *data_ov002_0210c748[];
extern void *data_ov002_0210cac8[];
/* The HUD's, from HUD::RenderHealthMeter and the VS timer. */
extern void *data_ov002_0210c230[];
extern void *_ZN3OAM17VS_YELLOW_NUMBERSE[];
/* ov001's digit table, which HUD::RenderCoinCount and its siblings index. */
extern void *_ZN3OAM7NUMBERSE[];

int hal_oam_templates_check(void);

}

namespace {

/* The DS main RAM window the port reserves. A template pointer landing in it
   is a pointer the rebase did not reach: host mounts are ordinary globals and
   live in the image, nowhere near 0x02000000. */
const unsigned kDsLo = 0x02000000u;
const unsigned kDsHi = 0x02400000u;

struct Table {
    const char *name;
    void **tbl;
    int count;          /* entries the render code can actually index */
};

/* Sizes are the ROM's own (next-symbol deltas in config/arm9/overlays/ovNNN/
   symbols.txt), clipped to the range the callers index:
     MM_PLAYER_ICONS   dsd sizes it 76 bytes = 19 words, but Minimap::Render
                       forms `character + capState * 4`, so 16 is the reach.
     MM_STAR_MARKERS   icon = flag + kind * 2, kinds 0..3, so 8.
     MM_STAR_KEYS      sel is 0 or 1.
     0210c748/0210cac8 12 bytes = 3, indexed by data_0209f370[i].
     0210c230          36 bytes = 9, HUD::RenderHealthMeter's segments.
     NUMBERS           80 bytes in ov001; ten digit pointers plus padding. */
const Table kTables[] = {
    {"OAM::MM_PLAYER_ICONS",       _ZN3OAM15MM_PLAYER_ICONSE,     16},
    {"OAM::MM_VS_PLAYER_ICONS",    _ZN3OAM18MM_VS_PLAYER_ICONSE,  16},
    {"OAM::MM_VS_PLAYER_ICONS_S",  _ZN3OAM20MM_VS_PLAYER_ICONS_SE, 16},
    {"OAM::MM_STAR_MARKERS",       _ZN3OAM15MM_STAR_MARKERSE,      8},
    {"OAM::MM_STAR_KEYS",          _ZN3OAM12MM_STAR_KEYSE,         2},
    {"data_ov002_0210c748",        data_ov002_0210c748,            3},
    {"data_ov002_0210cac8",        data_ov002_0210cac8,            3},
    {"data_ov002_0210c230",        data_ov002_0210c230,            9},
    {"OAM::VS_YELLOW_NUMBERS",     _ZN3OAM17VS_YELLOW_NUMBERSE,   10},
    {"OAM::NUMBERS",               _ZN3OAM7NUMBERSE,              10},
};

/* An OamAttr is eight bytes and the fourth halfword is the one the walk tests.
   A real template is a handful of entries; 64 is far past any of them and far
   short of a walk that would leave its own page. */
const int kMaxEntries = 64;

int terminated(const void *p)
{
    const unsigned short *a = (const unsigned short *)p;
    for (int i = 0; i < kMaxEntries; ++i)
        if (a[i * 4 + 3] == 0xffff) return 1;
    return 0;
}

}  // namespace

/* Returns the number of BAD entries, and prints one line per offender.
   Safe to call more than once; it only reports the first time. */
extern "C" int hal_oam_templates_check(void)
{
    static int done, bad;
    if (done) return bad;
    done = 1;

    const int quiet = std::getenv("SM64DS_NO_OAM_CHECK") != 0;
    for (unsigned t = 0; t < sizeof kTables / sizeof kTables[0]; ++t) {
        const Table &T = kTables[t];
        for (int i = 0; i < T.count; ++i) {
            void *e = T.tbl[i];
            const unsigned v = (unsigned)(unsigned long)e;
            const char *why = 0;
            if (e == 0)
                why = "NULL";
            else if (v >= kDsLo && v < kDsHi)
                why = "RAW DS ADDRESS (rebase missed it)";
            else if (!terminated(e))
                why = "NO 0xffff TERMINATOR in 64 entries";
            if (why) {
                ++bad;
                if (!quiet)
                    std::fprintf(stderr, "  [oam] %s[%d] = %08x -- %s\n",
                                 T.name, i, v, why);
            }
        }
    }
    if (!quiet)
        std::fprintf(stderr, "  [oam] sprite templates: %d bad entries\n", bad);
    return bad;
}
