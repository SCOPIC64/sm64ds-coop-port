// The 2D register audit: which unmodelled hardware a scene ACTUALLY touches.
//
// WHY THIS EXISTS. port/ov004_ov007_2d_map.txt section 7 lists six clusters of
// 2D hardware the ntr layer does not model. That list says what COULD matter.
// It was written from what two overlays' source pokes, not from a run. This
// samples what the live scenes really leave in the register file, so the gaps
// get closed in need order instead of encyclopedia order.
//
// WHY IT SAMPLES INSTEAD OF INTERCEPTING. ntr/io.cpp maps real memory at the DS
// addresses, so a 2D register store is an ordinary store to an ordinary page.
// It does not pass through io_read/io_write unless the storing function happens
// to be hostgen'd, which for the 2D surface is the exception. There is no seam
// to intercept at, and the same fact is why L1's GXSTAT fix had to normalise the
// LATCH rather than a returned value.
//
// WHAT SAMPLING SEES, AND WHAT IT CANNOT. It sees the register state at
// scan-out, which is the state that decides the image: on hardware the 2D unit
// reads these registers as it draws, and the port has no scanline timing at all,
// so a value written and overwritten before scan-out cannot reach the screen in
// either one. It does NOT see such a mid-frame value, and it cannot distinguish
// "never written" from "written zero". Zero is the off state for every register
// in the gap set (BLDCNT mode 0 is no effect, MASTER_BRIGHT factor 0 is no
// change, the window enables live in DISPCNT bits 13-15 and are sampled), so
// zero reads as "not in use" throughout, which is sound for the question asked
// and is NOT sound for any question about writes.
//
// The proxy hook is the exact half: anything that does reach io_write is logged
// with its value. It is a strict subset and is reported separately so the two
// are never confused.

#include "ntr/ppu_audit.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ntr {
namespace {

// ---- the register file ------------------------------------------------------
//
// Offsets are engine-relative: engine A at 0x04000000, engine B at 0x04001000.
// GBATEK's 2D display block, cross-read against the decomp's own accessors
// where one exists (ppu.cpp's header note records which shifts came from
// G2::GetBG0ScrPtr and G2S::GetBG0CharPtr rather than from docs).
//
// `eng` is which engines have the register at all. `modelled` is whether the
// port's LIVE path for that engine reads it, which is not the same question and
// differs between the two engines:
//
//   engine A live path = hal/message_compositor.cpp (2D over the 3D frame).
//                        ntr/ppu.cpp's ppu_scanout is NOT in any live path; it
//                        fills the whole framebuffer and would erase the 3D.
//   engine B live path = ntr/ppu_sub.cpp, via hal/sub_screen.cpp.
//
// so a register can be modelled on B and a gap on A. The minimap's affine
// registers are exactly that.

enum { E_A = 1, E_B = 2 };

struct Reg {
    uint16_t off;
    uint8_t width;      // 2 or 4
    uint8_t eng;        // which engines have it
    uint8_t modelled;   // which engines' live path reads it
    const char *name;
};

const Reg kRegs[] = {
    {0x00, 4, E_A | E_B, E_A | E_B, "DISPCNT"},
    {0x08, 2, E_A | E_B, E_A | E_B, "BG0CNT"},
    {0x0A, 2, E_A | E_B, E_A | E_B, "BG1CNT"},
    {0x0C, 2, E_A | E_B, E_A | E_B, "BG2CNT"},
    {0x0E, 2, E_A | E_B, E_A | E_B, "BG3CNT"},
    {0x10, 2, E_A | E_B, E_A | E_B, "BG0HOFS"},
    {0x12, 2, E_A | E_B, E_A | E_B, "BG0VOFS"},
    {0x14, 2, E_A | E_B, E_A | E_B, "BG1HOFS"},
    {0x16, 2, E_A | E_B, E_A | E_B, "BG1VOFS"},
    {0x18, 2, E_A | E_B, E_A | E_B, "BG2HOFS"},
    {0x1A, 2, E_A | E_B, E_A | E_B, "BG2VOFS"},
    {0x1C, 2, E_A | E_B, E_A | E_B, "BG3HOFS"},
    {0x1E, 2, E_A | E_B, E_A | E_B, "BG3VOFS"},
    // BG2/BG3 rotation and scaling. Modelled on B (ppu_sub does affine and
    // extended affine); a gap on A, where the compositor is text-mode only.
    {0x20, 2, E_A | E_B, E_B, "BG2PA"},
    {0x22, 2, E_A | E_B, E_B, "BG2PB"},
    {0x24, 2, E_A | E_B, E_B, "BG2PC"},
    {0x26, 2, E_A | E_B, E_B, "BG2PD"},
    {0x28, 4, E_A | E_B, E_B, "BG2X"},
    {0x2C, 4, E_A | E_B, E_B, "BG2Y"},
    {0x30, 2, E_A | E_B, E_B, "BG3PA"},
    {0x32, 2, E_A | E_B, E_B, "BG3PB"},
    {0x34, 2, E_A | E_B, E_B, "BG3PC"},
    {0x36, 2, E_A | E_B, E_B, "BG3PD"},
    {0x38, 4, E_A | E_B, E_B, "BG3X"},
    {0x3C, 4, E_A | E_B, E_B, "BG3Y"},
    // The window unit. Modelled on both, which the gap list did not know.
    {0x40, 2, E_A | E_B, E_A | E_B, "WIN0H"},
    {0x42, 2, E_A | E_B, E_A | E_B, "WIN1H"},
    {0x44, 2, E_A | E_B, E_A | E_B, "WIN0V"},
    {0x46, 2, E_A | E_B, E_A | E_B, "WIN1V"},
    {0x48, 2, E_A | E_B, E_A | E_B, "WININ"},
    {0x4A, 2, E_A | E_B, E_A | E_B, "WINOUT"},
    {0x4C, 2, E_A | E_B, 0, "MOSAIC"},
    // Colour special effects: nothing reads these on either engine.
    {0x50, 2, E_A | E_B, 0, "BLDCNT"},
    {0x52, 2, E_A | E_B, 0, "BLDALPHA"},
    {0x54, 2, E_A | E_B, 0, "BLDY"},
    // Engine A only.
    {0x60, 4, E_A, 0, "DISP3DCNT"},
    {0x64, 4, E_A, 0, "DISPCAPCNT"},
    {0x68, 4, E_A, 0, "DISP_MMEM_FIFO"},
    // Master brightness. ppu_sub applies it; the engine-A compositor does not,
    // though walk_window's own fade composite runs after it (hal/fader_wipes.cpp
    // reads BLDCNT/BLDY itself, which is a THIRD reader and is why the audit
    // reports the raw values rather than a verdict).
    {0x6C, 2, E_A | E_B, E_B, "MASTER_BRIGHT"},
};
const unsigned kRegN = sizeof kRegs / sizeof kRegs[0];

// ---- what is remembered per register ----------------------------------------

const unsigned kMaxVals = 6;

struct Slot {
    uint32_t val[kMaxVals];
    uint32_t hits[kMaxVals];
    unsigned n;
    unsigned nonzero;   // samples where the value was not 0
    int first_frame;    // sample index of the first non-zero, -1 if never
};

Slot g_slot[2][kRegN];

void note(Slot &s, uint32_t v, int frame) {
    if (v) {
        ++s.nonzero;
        if (s.first_frame < 0) s.first_frame = frame;
    }
    for (unsigned i = 0; i < s.n; ++i)
        if (s.val[i] == v) { ++s.hits[i]; return; }
    if (s.n < kMaxVals) {
        s.val[s.n] = v;
        s.hits[s.n] = 1;
        ++s.n;
    }
}

// ---- the OAM object-mode census ---------------------------------------------
//
// Attribute 0 bits 10-11 are the object mode and bit 12 is mosaic. That layout
// is not taken from docs alone: the decomp's own OAM::Render composes the word
// as `(mode << 10) | (mosaic << 12)` over a `u32 objMode : 2` bitfield
// (src/_ZN3OAM6RenderEbP7OamAttriiii5Fix12IiEi.c), which pins both fields.
//
// The four DS modes are counted separately on purpose. Both ppu.cpp and
// ppu_sub.cpp skip mode 3 with the comment "OBJ window", and the gap list
// repeats it. Counting all four says which mode the game really uses rather
// than assuming the comment names it.

struct ObjCensus {
    uint32_t mode[4];   // enabled objects seen in each mode, summed over samples
    uint32_t mosaic;
    uint32_t frames_with[4];   // samples in which the mode appeared at all
};

ObjCensus g_obj[2];

void census_oam(int e) {
    const uint32_t base = e == 0 ? 0x07000000u : 0x07000400u;
    unsigned seen[4] = {0, 0, 0, 0};
    for (int i = 0; i < 128; ++i) {
        const uint16_t a0 = *reinterpret_cast<volatile uint16_t *>(base + i * 8u);
        const bool affine = (a0 & 0x100) != 0;
        if (!affine && (a0 & 0x200)) continue;      // disabled
        const unsigned m = (a0 >> 10) & 3;
        ++g_obj[e].mode[m];
        ++seen[m];
        if ((a0 >> 12) & 1) ++g_obj[e].mosaic;
    }
    for (int m = 0; m < 4; ++m)
        if (seen[m]) ++g_obj[e].frames_with[m];
}

// ---- the touch surface ------------------------------------------------------
//
// NOT the stylus record. TouchInfo[4] at DS 0x020a0de8 is a hosted global living
// in the port's .dsstate section, not at that DS address, so it is not memory
// this file can read, and it is already driven from the mouse by
// hal/sub_screen.cpp's poll_touch. What IS memory here is the hardware surface:
// the ARM7 SPI ports and the shared-WRAM words the IPC touch handler reads.
// Sampling them answers the only open question, which is whether any LINKED
// code is waiting on a panel model.

struct TouchReg { uint32_t addr; uint8_t width; const char *name; };

const TouchReg kTouch[] = {
    {0x04000130, 2, "KEYINPUT"},
    {0x04000136, 2, "EXTKEYIN"},
    // The pad is TWO latches. func_02013f4c reads
    //     (KEYINPUT | *(u16 *)0x27fffa8) ^ 0x2fff
    // because X, Y and the hinge are ARM7-side and not visible at 0x04000130.
    // Sampling only the first would report half a surface, and the keypad is
    // active low, so a zero on either half reads as "held", not as "absent".
    {0x027FFFA8, 2, "SHARED_PAD"},
    {0x040001C0, 2, "SPICNT"},
    {0x040001C2, 2, "SPIDATA"},
    {0x027FFFAA, 2, "SHARED_TP_LO"},   // func_0205f300's two reads
    {0x027FFFAC, 2, "SHARED_TP_HI"},
};
const unsigned kTouchN = sizeof kTouch / sizeof kTouch[0];

Slot g_touch[kTouchN];

// ---- proxy hits (the exact half) --------------------------------------------

struct ProxyHit { uint32_t addr; uint32_t value; uint8_t width; uint8_t write; uint32_t n; };
const unsigned kMaxProxy = 64;
ProxyHit g_proxy[kMaxProxy];
unsigned g_proxy_n;
uint32_t g_proxy_dropped;

// ---- state ------------------------------------------------------------------

int g_on = -1;
const char *g_path;
int g_frames;
char g_tag[32];
bool g_registered;

uint32_t rd(uint32_t a, unsigned w) {
    return w == 4 ? *reinterpret_cast<volatile uint32_t *>(a)
                  : *reinterpret_cast<volatile uint16_t *>(a);
}

}  // namespace

bool ppu_audit_on() {
    if (g_on < 0) {
        g_path = std::getenv("SM64DS_PPU_AUDIT");
        g_on = g_path && *g_path ? 1 : 0;
    }
    return g_on != 0;
}

void ppu_audit_proxy(uint32_t addr, uint64_t value, unsigned width, bool is_write) {
    if (!ppu_audit_on()) return;
    // The 2D surface only: both engines' display blocks, plus OAM and palette
    // RAM, which are the other two things a 2D gap could hide in.
    const bool in2d = (addr >= 0x04000000u && addr < 0x04000070u)
                      || (addr >= 0x04001000u && addr < 0x04001070u)
                      || (addr >= 0x05000000u && addr < 0x05000800u)
                      || (addr >= 0x07000000u && addr < 0x07000800u);
    if (!in2d) return;
    for (unsigned i = 0; i < g_proxy_n; ++i)
        if (g_proxy[i].addr == addr && g_proxy[i].write == (is_write ? 1u : 0u)
            && g_proxy[i].value == (uint32_t)value) { ++g_proxy[i].n; return; }
    if (g_proxy_n >= kMaxProxy) { ++g_proxy_dropped; return; }
    ProxyHit &h = g_proxy[g_proxy_n++];
    h.addr = addr;
    h.value = (uint32_t)value;
    h.width = (uint8_t)width;
    h.write = is_write ? 1 : 0;
    h.n = 1;
}

void ppu_audit_sample(const char *tag) {
    if (!ppu_audit_on()) return;
    if (!g_registered) {
        g_registered = true;
        std::atexit(ppu_audit_dump);
        for (unsigned i = 0; i < kRegN; ++i)
            for (int e = 0; e < 2; ++e) g_slot[e][i].first_frame = -1;
        for (unsigned i = 0; i < kTouchN; ++i) g_touch[i].first_frame = -1;
    }
    if (tag && !g_tag[0]) {
        std::strncpy(g_tag, tag, sizeof g_tag - 1);
        g_tag[sizeof g_tag - 1] = 0;
    }

    for (int e = 0; e < 2; ++e) {
        const uint32_t base = e == 0 ? 0x04000000u : 0x04001000u;
        for (unsigned i = 0; i < kRegN; ++i) {
            if (!(kRegs[i].eng & (e == 0 ? E_A : E_B))) continue;
            note(g_slot[e][i], rd(base + kRegs[i].off, kRegs[i].width), g_frames);
        }
        census_oam(e);
    }
    for (unsigned i = 0; i < kTouchN; ++i)
        note(g_touch[i], rd(kTouch[i].addr, kTouch[i].width), g_frames);

    ++g_frames;
    // Flush on the first sample and then periodically. A run that faults skips
    // atexit entirely, and a table that only existed at a clean exit would have
    // nothing to say about exactly the runs whose registers are most
    // interesting. The first-sample flush also distinguishes "the seam never
    // ran" (no file at all) from "it ran and everything was zero".
    if (g_frames == 1 || (g_frames & 31) == 0) ppu_audit_dump();
}

void ppu_audit_dump() {
    if (!ppu_audit_on() || !g_frames) return;
    std::FILE *f = std::fopen(g_path, "w");
    if (!f) return;

    std::fprintf(f, "2D REGISTER AUDIT  seam=%s  samples=%d\n", g_tag, g_frames);
    std::fprintf(f, "M = the live path for that engine reads it; . = it does not.\n");
    std::fprintf(f, "A live path = hal/message_compositor.cpp, B = ntr/ppu_sub.cpp.\n\n");

    for (int e = 0; e < 2; ++e) {
        std::fprintf(f, "-- ENGINE %c (0x0400%04x) --\n", e == 0 ? 'A' : 'B',
                     e == 0 ? 0 : 0x1000);
        std::fprintf(f, "%-16s %-4s %-2s %-8s %-6s %s\n", "reg", "off", "m",
                     "nonzero", "first", "values seen (value xN)");
        for (unsigned i = 0; i < kRegN; ++i) {
            if (!(kRegs[i].eng & (e == 0 ? E_A : E_B))) continue;
            const Slot &s = g_slot[e][i];
            const bool mod = (kRegs[i].modelled & (e == 0 ? E_A : E_B)) != 0;
            // Everything unmodelled is printed even when dead, because "dead"
            // is the finding for most of the gap list. A modelled register is
            // printed only when it carried something.
            if (mod && !s.nonzero) continue;
            std::fprintf(f, "%-16s %04x %-2s %-8u ", kRegs[i].name, kRegs[i].off,
                         mod ? "M" : ".", s.nonzero);
            if (s.first_frame < 0) std::fprintf(f, "%-6s ", "-");
            else std::fprintf(f, "%-6d ", s.first_frame);
            for (unsigned k = 0; k < s.n; ++k)
                std::fprintf(f, "%0*x x%u%s", kRegs[i].width * 2, s.val[k],
                             s.hits[k], k + 1 < s.n ? ", " : "");
            if (s.n == kMaxVals) std::fprintf(f, " (+more)");
            std::fprintf(f, "\n");
        }
        const ObjCensus &c = g_obj[e];
        static const char *kModeName[4] = {"normal", "semi-transparent",
                                           "obj-window", "bitmap"};
        std::fprintf(f, "OAM object modes (enabled entries, summed over samples):\n");
        for (int m = 0; m < 4; ++m)
            std::fprintf(f, "  mode %d %-17s %8u  in %u/%d samples\n", m,
                         kModeName[m], c.mode[m], c.frames_with[m], g_frames);
        std::fprintf(f, "  mosaic bit set             %8u\n\n", c.mosaic);
    }

    std::fprintf(f, "-- TOUCH HARDWARE SURFACE --\n");
    std::fprintf(f, "The stylus record TouchInfo[4] is NOT here: it is a hosted\n");
    std::fprintf(f, "global, already driven from the mouse by poll_touch.\n");
    for (unsigned i = 0; i < kTouchN; ++i) {
        const Slot &s = g_touch[i];
        std::fprintf(f, "%-16s %08x nonzero=%-6u ", kTouch[i].name, kTouch[i].addr,
                     s.nonzero);
        for (unsigned k = 0; k < s.n; ++k)
            std::fprintf(f, "%0*x x%u%s", kTouch[i].width * 2, s.val[k], s.hits[k],
                         k + 1 < s.n ? ", " : "");
        std::fprintf(f, "\n");
    }

    std::fprintf(f, "\n-- io.cpp PROXY HITS on the 2D surface (exact, a strict subset) --\n");
    if (!g_proxy_n) {
        std::fprintf(f, "none. Every 2D access in this run reached mapped memory\n");
        std::fprintf(f, "directly, which is why the rest of this table is sampled.\n");
    }
    for (unsigned i = 0; i < g_proxy_n; ++i)
        std::fprintf(f, "%s %08x w%u = %08x  x%u\n", g_proxy[i].write ? "wr" : "rd",
                     g_proxy[i].addr, g_proxy[i].width, g_proxy[i].value, g_proxy[i].n);
    if (g_proxy_dropped)
        std::fprintf(f, "(%u further distinct hits dropped, table full)\n", g_proxy_dropped);

    std::fclose(f);
}

}  // namespace ntr
