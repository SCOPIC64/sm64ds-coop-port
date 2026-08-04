// HOST COPY of src/_ZN3HUD13InitResourcesEv.cpp -- the matched source
// verbatim except that its file-scope globals are declared rather than
// DEFINED. Retires the moment the port has a way to compile a matched TU
// whose tentative definitions collide with storage it already owns.
//
// WHY THIS FILE EXISTS. The matched TU opens with
//
//     extern "C" {
//         ...
//         u8 data_0209f2d8;
//         s8 data_0209f2f8;
//         ...
//         CAA0 data_0209caa0;
//     }
//
// and in C++ every one of those is a DEFINITION, not a declaration. Seven of
// them are storage the port already owns somewhere else, so linking the TU as
// it stands is seven LNK2005s:
//
//     data_0209f2d8 f2f8 f2fc f250   port/unmatched/Player_InitResources.cpp
//     data_ov002_0211117c 02111184   the ov002 per-symbol mount
//     data_0209caa0                  hal/level_boot.cpp
//
// The last one is the one that cannot be settled by moving the storage here.
// data_0209caa0 is the SAVE BLOCK, and level_boot owns it as a grouped-section
// run of four symbols because LoadEntranceObjects reads byte 0x41 -- past this
// symbol's own 0x14 and into the third symbol of the run. The HUD's view of it
// is a 12-byte struct whose only live field is `unk8`, the "intro has played"
// bit. Two definitions of different sizes, and the one with the contiguity
// contract has to win.
//
// So the whole block becomes extern and nothing else changes. Player's own
// InitResources host copy already does exactly this for data_0209caa0 alone
// ("storage moved to hal/level_boot.cpp"); this is the same move for the rest.
//cpp
typedef signed char s8;
typedef unsigned char u8;
typedef short s16;
typedef int s32;
typedef unsigned int u32;

struct CAA0 { char pad[8]; int unk8; };

extern "C" {
    int GetOwnerLanguage(void);
    /* THE RAW LOADER, not LoadFile. See the note below. */
    void* func_0201817c(unsigned handle);
    void DecompressLZ16(void* handle, int addr);
    void Deallocate(void* handle);
    u8 NumStars(void);
    void func_0203da4c(void);

    /* EVERY ONE OF THESE IS `extern` HERE AND A DEFINITION IN THE MATCHED
       TU. That single word is the whole difference between the two files. */
    extern u8 data_0209f2d8;
    extern s8 data_0209f2f8;
    extern u8 data_0209f2fc;
    extern u8 data_0209f250;
    extern void* data_0209f394[];
    extern s8 data_ov002_02111178;
    extern u8 data_ov002_0211117c;
    extern s8 data_ov002_02111184;
    extern s16 data_ov002_02111188;
    extern CAA0 data_0209caa0;
}

/* GetHealth's own TU defines `int Player::GetHealth()`, and MSVC puts the
   return type in a member's mangled name -- declaring it u8 here (which the
   matched TU does) asks the linker for ?GetHealth@Player@@QAEEXZ and gets
   nothing. The value is assigned to a u8 either way. */
#include <cstdio>

/* THE LOADER, and why it is not LoadFile.

   TWO reasons, and the first is a bug this file would otherwise have shipped.

   1. DOUBLE DECOMPRESSION. On the ROM, LoadFile hands back the file still
      COMPRESSED and the caller expands it -- which is why every load below is
      followed by DecompressLZ16. The port's LoadFile is SharedFilePtr-backed
      and already expands LZ77 on the way out, so calling DecompressLZ16 on its
      result would decode an already-decoded buffer straight into VRAM.
      func_0201817c is the port's raw loader (hal/fs.cpp, gate 25): file bytes
      on the game heap, still compressed, exactly the ROM's contract. It is
      also what makes the trailing Deallocate correct -- a fresh allocation per
      call rather than a cached SharedFilePtr the cache still points at.

   2. A MISSING FILE IS NOT FATAL HERE. This extraction is 12% short of the
      catalog (248 of 2072 files), and three of the HUD's belong to the gap:
      handle 0x269 is data/2D_cad/EUR/ENG/d_2d_cmn_icon_E_ncg.bin and 0xA00A /
      0xA00E live in ARCHIVE/cee.narc, none of which extracted/ carries. The
      port's LoadFile aborts on a null, which would take the whole boot down
      for artwork. The BOTTOM screen's own tiles and palette -- 0x229 and
      0x22A -- are present, and Stage::LoadGraphics2D has already filled sub
      OBJ VRAM and its palette, so skipping the absent loads leaves the panel
      with real graphics rather than no game.

   Both notes retire together the day the extraction is complete. */
static void *hud_load(int handle)
{
    void *p = func_0201817c((unsigned)handle);
    if (!p) {
        static int said;
        if (said < 8) {
            ++said;
            printf("  [hud] file 0x%x is not in this extraction; skipped\n",
                   handle);
        }
    }
    return p;
}

struct Player { int GetHealth(); };
namespace GX { void LoadOBJPltt(const void*, u32, u32); }
namespace GXS { void LoadOBJPltt(const void*, u32, u32); }


/* ...and the consumers, so a skipped load is a skipped use rather than a null
   dereference three lines later. */
static void hud_decomp(void *h, int addr) { if (h) DecompressLZ16(h, addr); }
static void hud_free(void *h) { if (h) Deallocate(h); }
static void hud_gx_pal(void *h, u32 o, u32 n) { if (h) GX::LoadOBJPltt(h, o, n); }
static void hud_gxs_pal(void *h, u32 o, u32 n) { if (h) GXS::LoadOBJPltt(h, o, n); }

struct HUD {
    char pad0[0x60];
    s16 unk60;
    s16 unk62;
    s16 unk64;
    s16 unk66;
    s16 unk68;
    char pad6a[4];
    s16 unk6E;
    s16 unk70;
    u8 unk72;
    u8 unk73;
    char pad74[4];
    u8 unk78;
    int InitResources();
};

int HUD::InitResources()
{
    char* vram_a = (char*)0x6400000;
    char* vram_b = (char*)0x6600000;
    int cond1 = (data_0209f2d8 == 1);
    if (cond1 != 0) {
        void* h;
        if (GetOwnerLanguage() == 5) {
            h = hud_load(0xB003);
            hud_decomp(h, 0x6400000);
            hud_decomp(h, 0x6600000);
            hud_free(h);
            h = hud_load(0xB007);
            hud_decomp(h, (int)(vram_a + 0x2000));
            hud_free(h);
        } else if (GetOwnerLanguage() == 4) {
            h = hud_load(0xAC03);
            hud_decomp(h, 0x6400000);
            hud_decomp(h, 0x6600000);
            hud_free(h);
            h = hud_load(0xAC07);
            hud_decomp(h, (int)(vram_a + 0x2000));
            hud_free(h);
        } else if (GetOwnerLanguage() == 3) {
            h = hud_load(0xA803);
            hud_decomp(h, 0x6400000);
            hud_decomp(h, 0x6600000);
            hud_free(h);
            h = hud_load(0xA807);
            hud_decomp(h, (int)(vram_a + 0x2000));
            hud_free(h);
        } else if (GetOwnerLanguage() == 2) {
            h = hud_load(0xA403);
            hud_decomp(h, 0x6400000);
            hud_decomp(h, 0x6600000);
            hud_free(h);
            h = hud_load(0xA407);
            hud_decomp(h, (int)(vram_a + 0x2000));
            hud_free(h);
        } else {
            h = hud_load(0xA003);
            hud_decomp(h, 0x6400000);
            hud_decomp(h, 0x6600000);
            hud_free(h);
            h = hud_load(0xA007);
            hud_decomp(h, (int)(vram_a + 0x2000));
            hud_free(h);
        }
        h = hud_load(0x8000);
        hud_decomp(h, (int)(vram_b + 0x2000));
        hud_free(h);
        h = hud_load(0x8002);
        hud_gx_pal(h, 0, 0x120);
        hud_gxs_pal(h, 0, 0x120);
        hud_free(h);
        h = hud_load(0x8003);
        hud_gx_pal(h, 0x120, 0xE0);
        hud_free(h);
        h = hud_load(0x8001);
        hud_gxs_pal(h, 0x120, 0xE0);
        hud_free(h);
    } else {
        void* h;
        if (GetOwnerLanguage() == 5) h = hud_load(0x270);
        else if (GetOwnerLanguage() == 4) h = hud_load(0x26E);
        else if (GetOwnerLanguage() == 3) h = hud_load(0x26C);
        else if (GetOwnerLanguage() == 2) h = hud_load(0x26A);
        else h = hud_load(0x269);
        hud_decomp(h, 0x6400000);
        hud_decomp(h, 0x6600000);
        hud_free(h);
        if (GetOwnerLanguage() == 5) h = hud_load(0xB00E);
        else if (GetOwnerLanguage() == 4) h = hud_load(0xAC0E);
        else if (GetOwnerLanguage() == 3) h = hud_load(0xA80E);
        else if (GetOwnerLanguage() == 2) h = hud_load(0xA40E);
        else h = hud_load(0xA00E);
        hud_decomp(h, (int)(vram_a + 0x2000));
        hud_free(h);
        h = hud_load(0x229);
        hud_decomp(h, (int)(vram_b + 0x2000));
        hud_free(h);
        if (GetOwnerLanguage() == 5) h = hud_load(0xB00A);
        else if (GetOwnerLanguage() == 4) h = hud_load(0xAC0A);
        else if (GetOwnerLanguage() == 3) h = hud_load(0xA80A);
        else if (GetOwnerLanguage() == 2) h = hud_load(0xA40A);
        else h = hud_load(0xA00A);
        hud_gx_pal(h, 0, 0x120);
        hud_gxs_pal(h, 0, 0x120);
        hud_free(h);
        h = hud_load(0x980F);
        hud_gx_pal(h, 0x120, 0xE0);
        hud_free(h);
        h = hud_load(0x22A);
        hud_gxs_pal(h, 0x120, 0xE0);
        hud_free(h);
    }

    int var_r2 = 0;
    this->unk62 = 0;
    this->unk64 = 0xB4;
    this->unk66 = 0xA;
    if (data_0209f2d8 == 1) var_r2 = 1;
    if (var_r2 != 0) {
        if (data_0209f2f8 == 0x33) this->unk60 = 0x1E;
        else if (data_0209f2f8 == 0x2B) *(volatile s16*)&this->unk60 = 0x1E;
        else this->unk60 = 0x1E;
    } else {
        this->unk60 = 0x1E;
    }

    data_ov002_02111188 = 0xB4;
    data_ov002_02111184 = 0;
    {
        u8 idx = *(volatile u8*)&data_0209f250;
        data_ov002_0211117c = ((Player*)((idx ? (void* volatile*)data_0209f394 : (void* volatile*)data_0209f394)[idx]))->GetHealth();
    }
    if (data_ov002_0211117c == 8) {
        this->unk68 = -0x18;
        this->unk72 = 0;
        this->unk73 = 0;
    } else {
        this->unk68 = 0x18;
        this->unk73 = 1;
    }

    int var_r0_2 = 0;
    int var_r1;
    if (data_0209f2d8 == 1) var_r1 = 1; else var_r1 = 0;
    if (var_r1 == 0) {
        int n = NumStars();
        if (n > 0x63) var_r0_2 = 0x1B;
        else if (n > 9) var_r0_2 = 0x12;
        else var_r0_2 = 9;
    }

    int var_r1_2;
    if (data_0209f2d8 == 1) var_r1_2 = 1; else var_r1_2 = 0;
    if (var_r1_2 != 0) {
        data_ov002_02111178 = 0;
        this->unk6E = 0x10;
        this->unk70 = 0xF0;
        this->unk78 = 0;
    } else if (!(data_0209caa0.unk8 & 0x80)) {
        data_ov002_02111178 = 6;
        this->unk6E = -0x3A;
        this->unk70 = (s16)(var_r0_2 + 0x120);
        this->unk78 = 1;
    } else if (data_0209f2fc == 2) {
        data_ov002_02111178 = 1;
        this->unk6E = -0x3A;
        this->unk70 = 0xF0;
        this->unk78 = 0;
    } else if (data_0209f2fc == 1) {
        data_ov002_02111178 = 2;
        this->unk6E = -0x3A;
        this->unk70 = (s16)(var_r0_2 + 0x120);
        this->unk78 = 0;
    } else {
        data_ov002_02111178 = 0;
        this->unk6E = 0x10;
        this->unk70 = 0xF0;
        this->unk78 = 0;
    }

    func_0203da4c();
    return 1;
}
