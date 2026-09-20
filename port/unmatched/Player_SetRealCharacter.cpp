// HOST COPY of src/_ZN6Player16SetRealCharacterEj.cpp: the live character
// switch, with three host additions (all consequences of files loading on
// demand from the host card instead of living in DS RAM):
//
// 1. No Release of the outgoing anim file: the ROM trusts the shared
//    refcount to keep it alive while stale slots still reference it, but on
//    the host it is freed outright. Skipping leaks kilobytes once per
//    switch, never per frame.
// 2. ensure_file on every slot the switch reads: slots for never-booted
//    characters can hold DS-address garbage from the overlay mount (never
//    Constructed on host). DS RAM range can never be a real host file, so
//    that range means garbage: reset and load for real.
// 3. A SetAnim ending that re-seats the primary body animation: non-boot
//    slots never had their primary frames set, so the next AdvanceAnims
//    would divide by zero frames. The current anim id is kept when readable
//    so the pose does not pop. ma->file is cleared first to force SetAnim's
//    slow path (it fast-paths on pointer equality).
#include "types.h"
#include "decl_ModelAnim2.h"
#include "decl_common.h"
#include "Player.h"
struct SharedFilePtr;

extern "C" void _ZN13SharedFilePtr7ReleaseEv(SharedFilePtr *self);
extern "C" void _ZN6Player18SetNewHatCharacterEjjb(void *self, u32 a, u32 b, bool c);
extern "C" void _ZN9Animation8LoadFileER13SharedFilePtr(SharedFilePtr &f);
extern "C" void _ZN6Player4HealEi(void *self, int hp);
extern "C" u32 _ZNK6Player14GetBodyModelIDEjb(void *self, u32 a, bool b);

extern SharedFilePtr *data_ov002_020ff480[];
extern "C" {
extern void *data_ov002_020ff2f0[];
}
extern u8 data_02092128[];
extern u8 data_0209caa0[];
extern "C" void func_ov002_020e6330(char *c);
extern "C" int _ZN6Player6IsAnimEj(void *c, unsigned a);
extern "C" void _ZN6Player7SetAnimEji5Fix12IiEj(void *c, unsigned a, int b, int f, unsigned d);
extern "C" void *_ZN13SharedFilePtr8LoadFileEv(SharedFilePtr *self);

/* Slots for characters that were never booted can hold DS-address garbage
   from the overlay mount (never Constructed on host) or a null filePtr.
   Real host files never live in DS RAM range, so that range means garbage:
   reset the slot and load it for real. Non-null host pointers are trusted
   (same bytes a reload would produce). */
static void *ensure_file(SharedFilePtr *slot)
{
    void *fp = *(void **)((char *)slot + 4);
    if (fp == 0 ||
        ((unsigned)(size_t)fp >= 0x02000000u &&
         (unsigned)(size_t)fp < 0x02800000u)) {
        *(unsigned char *)((char *)slot + 2) = 0;
        *(void **)((char *)slot + 4) = 0;
        _ZN9Animation8LoadFileER13SharedFilePtr(*slot);
        fp = *(void **)((char *)slot + 4);
    }
    return fp;
}

void Player::SetRealCharacter(unsigned int chr_)
{
    u32 chr = (u32)chr_;

    u32 base = mCharFileBase;
    u32 m1, m2;

    /* NO Release of data_ov002_020ff480[base + (param1 & 3)]: see header. */
    (void)base;
    _ZN6Player18SetNewHatCharacterEjjb(((char *)this), chr, 0, 1);
    param1 = chr;
    mCharacter = (u8)chr;
    data_02092128[mPlayerNo] = (u8)param1;
    data_0209caa0[0x41] = (u8)param1;
    ensure_file(data_ov002_020ff480[mCharFileBase + (param1 & 3)]);
    /* The target's body-anim file (slot 0xc4+char): load it for real --
       ensure_file resets DS-mount garbage instead of trusting it. */
    ensure_file(data_ov002_020ff480[0xc4 + (param1 & 3)]);
    func_ov002_020e6330(((char *)this));
    _ZN6Player4HealEi(((char *)this), 0x880);
    unk_73c = 0;

    m1 = _ZNK6Player14GetBodyModelIDEjb(((char *)this), chr, 0);
    m2 = _ZNK6Player14GetBodyModelIDEjb(((char *)this), param1 & 0xff, 0);
    _ZN10ModelAnim24CopyERKS_Pcj(
        *(void **)(((char *)this) + m1 * 4 + 0xdc),
        *(void **)(((char *)this) + m2 * 4 + 0xdc),
        *(char **)((char *)data_ov002_020ff480[mCharFileBase + chr] + 4),
        0);
    m1 = _ZNK6Player14GetBodyModelIDEjb(((char *)this), chr, 0);
    _ZN10ModelAnim213Func_020162C4Eji5Fix12IiEt(
        *(void **)(((char *)this) + m1 * 4 + 0xdc),
        *(int *)((char *)data_ov002_020ff480[chr + 0xc4] + 4), 0, 0x1000, 0);
    /* Re-seat the primary body animation through the real SetAnim: slots
       that were never the boot character never had their primary frames
       set (only Func_020162C4's secondary), so the next AdvanceAnims would
       divide by zero frames. Keep the current anim id when it can be read
       back so the pose does not pop, else fall back to the walk cycle --
       the running state re-asserts its own anim within a few frames. */
    {
        unsigned cur = 0x47;
        for (unsigned a = 0; a < 0x200; ++a) {
            if (_ZN6Player6IsAnimEj((char *)this, a)) {
                cur = a;
                break;
            }
        }
        /* The BCA + texture files SetAnim is about to read: ensure them
           first (SetAnim trusts their filePtrs; the texture one it never
           loads itself). sl mirrors SetAnim's own clamp. */
        ensure_file((SharedFilePtr *)data_ov002_020ff480[((param1 & 3) + cur * 4)]);
        {
            unsigned sl = cur * 4;
            unsigned f8 = param1 & 3;
            if ((int)sl >= 0x60) sl = 0x60;
            sl += f8;
            SharedFilePtr *tex =
                (SharedFilePtr *)data_ov002_020ff2f0[sl];
            void *fp = *(void **)((char *)tex + 4);
            if (fp == 0 ||
                ((unsigned)(size_t)fp >= 0x02000000u &&
                 (unsigned)(size_t)fp < 0x02800000u)) {
                *(unsigned char *)((char *)tex + 2) = 0;
                *(void **)((char *)tex + 4) = 0;
                _ZN13SharedFilePtr8LoadFileEv(tex);
            }
        }
        /* Force ModelAnim::SetAnim's slow path: ma->file can already equal
           the file word on entry, which takes the flags-only fast path and
           leaves the 0 boot frames behind. Clearing it re-seeds the frame
           count from the ensured-valid file below. */
        *(void **)(*(char **)(((char *)this) + m1 * 4 + 0xdc) + 0x60) = 0;
        _ZN6Player7SetAnimEji5Fix12IiEj((char *)this, cur, 0, 0x1000, 0);
    }
}
