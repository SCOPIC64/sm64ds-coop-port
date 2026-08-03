/* HOST COPY of src/_ZN13OneUpMushroom8BehaviorEv.cpp -- the mushroom-type
 * dispatch read as a plain function pointer, and the table seated with host
 * addresses.
 *
 * OneUpMushroom is one class serving fourteen collectables (the 1-up, the
 * feather, the shell, the mega mushroom...), and Behavior picks the one it is
 * by indexing data_ov002_0210dc00 with mMushroomType. That table is fourteen
 * mwcc POINTER-TO-MEMBER-FUNCTION pairs, written by __sinit_ov002_02100adc out
 * of the fourteen statics at ov002 0x02108300..0x02108368. Every one of them
 * is the plain nonvirtual { function, 0 } form, so the host reads the first
 * word back as a function taking `this` and ignores the second -- the same
 * treatment the four processing-list walks already have
 * (port/unmatched/func_02043fdc.cpp).
 *
 * TWO THINGS THE HOST NEEDS THAT THE ROM DOES NOT.
 *
 * First, MSVC forms a pointer-to-member of an INCOMPLETE class as the
 * four-word general representation, which strides this table at 0x20 instead
 * of 0x10 and dispatches a neighbour's body. The source TU declares `struct
 * C;` before the typedef, so it hits that.
 *
 * Second, the sinit really did run -- it is matched src and it is on the boot
 * list -- but what it copied are the ov002 image's own words, which are DS
 * CODE ADDRESSES. That is the ovdata contract ("callers patch code pointers at
 * runtime"), and this is that patch: the seat below rewrites each pair's first
 * word with the host body, after checking that the word the sinit left agrees
 * with the ROM address that body was compiled from. A disagreement means the
 * mount is pointing at the wrong bytes and it says so rather than calling into
 * the overlay image -- which is exactly what the first run did, at 0x020aff10.
 *
 * Nothing else changes. The body below is the matched source's control flow
 * line for line: the Yoshi-eat early out, the two frame counters, the reset
 * when the sub-behaviour changed state, and the cylinder clear/update pair.
 *
 * Enemy::UpdateYoshiEat (ov002 0x020ade78) is UNMATCHED -- a 0x3cc-byte hole
 * in ov002's delink table with no C at all -- and it is called by name here.
 * hal/actor_vtables.cpp carries the stub and the reason.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

int _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(void *self, void *clsn);
void _ZN12CylinderClsn5ClearEv(void *self);
void _ZN12CylinderClsn6UpdateEv(void *self);
void func_ov002_020af4ec(void *self);

/* the fourteen type behaviours, in the order __sinit_ov002_02100adc seats
   them (mushroom type 0..13) */
void func_ov002_020aff10(void *);   /* 0  */
void func_ov002_020afe4c(void *);   /* 1  */
void func_ov002_020afd10(void *);   /* 2  */
void func_ov002_020afc44(void *);   /* 3  -- the castle grounds' three */
void func_ov002_020afbb4(void *);   /* 4  */
void func_ov002_020afa98(void *);   /* 5  */
void func_ov002_020afa6c(void *);   /* 6  */
void func_ov002_020af950(void *);   /* 7  */
void func_ov002_020af924(void *);   /* 8  */
void func_ov002_020af838(void *);   /* 9  */
void func_ov002_020af7cc(void *);   /* 10 */
void func_ov002_020afa50(void *);   /* 11 */
void func_ov002_020af908(void *);   /* 12 */
void func_ov002_020af724(void *);   /* 13 */

/* the sinit's fourteen {function, delta} pairs; storage is the ov002 mount */
struct PortPmf { unsigned fn; int delta; };
extern PortPmf data_ov002_0210dc00[];

}  /* extern "C" */

enum { PORT_MUSHROOM_TYPES = 14 };

static const struct { unsigned rom; void (*host)(void *); } g_types[] = {
    {0x020aff10, func_ov002_020aff10}, {0x020afe4c, func_ov002_020afe4c},
    {0x020afd10, func_ov002_020afd10}, {0x020afc44, func_ov002_020afc44},
    {0x020afbb4, func_ov002_020afbb4}, {0x020afa98, func_ov002_020afa98},
    {0x020afa6c, func_ov002_020afa6c}, {0x020af950, func_ov002_020af950},
    {0x020af924, func_ov002_020af924}, {0x020af838, func_ov002_020af838},
    {0x020af7cc, func_ov002_020af7cc}, {0x020afa50, func_ov002_020afa50},
    {0x020af908, func_ov002_020af908}, {0x020af724, func_ov002_020af724},
};

extern "C" void port_one_up_mushroom_types_seat(void)
{
    for (int i = 0; i < PORT_MUSHROOM_TYPES; ++i) {
        if (data_ov002_0210dc00[i].fn != g_types[i].rom) {
            std::fprintf(stderr, "FATAL: mushroom type %d: the sinit left "
                         "%08x, the ROM's own table says %08x -- WRONG "
                         "BYTES\n", i, data_ov002_0210dc00[i].fn,
                         g_types[i].rom);
            std::abort();
        }
        data_ov002_0210dc00[i].fn = (unsigned)(size_t)g_types[i].host;
        data_ov002_0210dc00[i].delta = 0;
    }
}

extern "C" int _ZN13OneUpMushroom8BehaviorEv(void *selfv)
{
    char *c = (char *)selfv;
    if (_ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(c, c + 0x144) != 0) {
        func_ov002_020af4ec(c);
        _ZN12CylinderClsn5ClearEv(c + 0x110);
        return 1;
    }
    *(int *)(c + 0xd0) = 0;                       /* mEatingPlayer */
    {
        int old = *(int *)(c + 0x388);
        unsigned type = (unsigned)*(int *)(c + 0x384);   /* mMushroomType */
        if (type >= PORT_MUSHROOM_TYPES) {
            std::fprintf(stderr, "FATAL: mushroom type %u out of range\n",
                         type);
            std::abort();
        }
        ((void (*)(void *))(size_t)data_ov002_0210dc00[type].fn)(c);
        ++*(unsigned short *)(c + 0x100);
        ++*(unsigned short *)(c + 0x38c);
        if (old != *(int *)(c + 0x388)) {
            *(unsigned short *)(c + 0x100) = 0;
            *(unsigned short *)(c + 0x300 + 0x8c) = 0;
        }
    }
    _ZN12CylinderClsn5ClearEv(c + 0x110);
    _ZN12CylinderClsn6UpdateEv(c + 0x110);
    func_ov002_020af4ec(c);
    return 1;
}
