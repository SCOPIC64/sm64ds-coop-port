/* HOST COPIES of FOUR dScMgBase_c slots -- 1, 7, 29 and 30 -- being
 * func_ov004_020b0930, func_ov004_020b0620, func_ov004_020af094 and
 * func_ov004_020aeed8. Run link60, lane MG1.
 *
 * THREE OF THE FOUR ARE HERE FOR ONE REASON (a declaration conflict, below)
 * AND ONE OF THOSE THREE CARRIES A SECOND, WORSE DEFECT that this file also
 * repairs. Read the slot-7 note near its body before assuming all four copies
 * are the same kind of change: three are one token each, and slot 7 is one
 * token plus one argument.
 *
 * NOT AN ABI WORKAROUND AND NOT TAGGED AS ONE. All four bodies are copies of
 * their src TUs with ONE TOKEN CHANGED EACH -- the type of the single
 * parameter -- and the change is forced by a DECLARATION CONFLICT that has sat
 * in the tree unexercised because nothing ever compiled any of the four.
 *
 *   include/decl_common.h   extern void func_ov004_020aeed8(void*);
 *   include/decl_common.h   extern void func_ov004_020af094(void*);
 *   include/decl_common.h   extern int  func_ov004_020b0930(void*);
 *   include/decl_common.h   extern int  func_ov004_020b0620(void*);
 *   src/func_ov004_020aeed8.cpp   extern "C" void func_ov004_020aeed8(char* c)
 *   src/func_ov004_020af094.cpp   extern "C" void func_ov004_020af094(Obj *self)
 *   src/func_ov004_020b0930.cpp   int func_ov004_020b0930(char* c)
 *   src/func_ov004_020b0620.cpp   extern "C" int func_ov004_020b0620(char *self)
 *
 * The whole ov004 half of port/slice_mg1.txt was swept for the pattern rather
 * than found one build at a time, so these four are all of them and not the
 * ones that happened to surface: every .cpp TU in the slice that includes
 * decl_common.h had its definition's parameter types compared against that
 * header's declaration, and exactly four disagree. The .c TUs are compiled as
 * C, where the C++ overload rule does not apply, so they are correctly not
 * affected.
 *
 * All four src TUs include decl_common.h, so under MSVC each is a definition
 * whose parameter type disagrees with an extern "C" declaration in scope:
 *
 *   error C2733: 'func_ov004_020aeed8': you cannot overload a function with
 *   'extern "C"' linkage
 *
 * mwccarm accepts it, which is why the ROM built and why the byte gate has
 * never had an opinion. It is a C++-in-.c-with-a-shared-header defect of the
 * same family port/ov006_minigame_scout.txt section 3 counted twenty-two of,
 * and it is a DECOMP-SIDE defect rather than a port one.
 *
 * WHY THE HEADER IS NOT THE FIX, which was checked before this file was
 * written. Widening decl_common.h to char* moves the conflict rather than
 * removing it. Take slot 30 as the worked case: three TUs call
 * func_ov004_020aeed8 and func_ov004_020af094, and two of them --
 * src/func_ov006_020e6cac.c and src/func_ov006_020e6d24.cpp -- carry their OWN
 * local `void *` declarations, so a char* header line makes 020e6d24.cpp (a
 * .cpp) fail with the identical C2733. Making them agree is a multi-file
 * change inside the byte-gated tree, made by a port lane, to files the port
 * lane does not own. Routed rather than taken.
 *
 * WHAT CHANGED, EXACTLY, so a reviewer can diff rather than trust:
 *
 *   func_ov004_020aeed8  `char* c`   became `void *cv` plus a first line
 *                        `char *c = (char *)cv;`. Every other line is the src
 *                        file's, in order, including the `struct Obj` vtable
 *                        shim and both `o->f68()` guards.
 *   func_ov004_020af094  `Obj *self` became `void *cv` plus a first line
 *                        `Obj *self = (Obj *)cv;`. The src's own second line
 *                        `char *c = (char *)self;` is kept as it stands.
 *   func_ov004_020b0930  `char* c`   became `void *cv` plus a first line
 *                        `char *c = (char *)cv;`.
 *   func_ov004_020b0620  `char *self` became `void *cv` plus a first line
 *                        `char *self = (char *)cv;`  AND ONE ARGUMENT ADDED --
 *                        see the block above its body. It is the only one of
 *                        the four that is not purely a signature change.
 *
 * Apart from those, no constant, no offset, no call, no branch and no
 * statement order differs from src in any of the four. The four src TUs are
 * therefore NOT in port/slice_mg1.txt, and the slice says so where they would
 * have been.
 *
 * THE BODIES THEMSELVES WERE RULED AGAINST THE ROM before being seated, like
 * the other twenty-one framework slots. port/tools/inferred_stub_adjudicated
 * .txt has all four: func_ov004_020af094 at 0x1e8 with an eleven-word literal
 * pool, func_ov004_020aeed8 at 0x174 with an eight-word one,
 * func_ov004_020b0930 at 0x108 with a six-word one, and func_ov004_020b0620
 * at 0x220, the largest of the twenty. All four REAL_DECOMP.
 */
#include "types.h"
#include "decl_common.h"
#include "dScMgBase_c.h"

extern "C" u8 data_0209d45c;
extern "C" u8 data_0209d454;
extern "C" u8 data_0209d460;
extern "C" u8 data_0209d458;

extern "C" int _ZN2GX10LoadBGPlttEPKvjj(const void *p, u32 a, u32 b);
extern "C" int _ZN3GXS10LoadBGPlttEPKvjj(const void *p, u32 a, u32 b);
extern "C" void _ZN4CP1527FlushAndInvalidateDataCacheEjj(void *addr,
                                                         unsigned int size);
extern "C" void MultiStore16(unsigned short val, char *dst, int nbytes);
extern "C" void func_0201f32c(int arg0);
extern "C" int GetGameLanguage(void);
extern "C" void DecompressLZ16(void *src, void *dst);

/* ---- func_ov004_020aeed8, slot 30 -------------------------------------- */
namespace mgd8 {
struct Obj {
    virtual int f00();  virtual int f04();  virtual int f08();
    virtual int f0c();  virtual int f10();  virtual int f14();
    virtual int f18();  virtual int f1c();  virtual int f20();
    virtual int f24();  virtual int f28();  virtual int f2c();
    virtual int f30();  virtual int f34();  virtual int f38();
    virtual int f3c();  virtual int f40();  virtual int f44();
    virtual int f48();  virtual int f4c();  virtual int f50();
    virtual int f54();  virtual int f58();  virtual int f5c();
    virtual int f60();  virtual int f64();  virtual int f68();
};
}  /* namespace mgd8 */

extern "C" void func_ov004_020aeed8(void *cv)
{
    char *c = (char *)cv;                       /* the one added line */
    struct dScMgBase_c *self = (struct dScMgBase_c *)(void *)c;
    mgd8::Obj *o = (mgd8::Obj *)c;

    data_0209d45c = 0;
    data_0209d454 = 0;

    *(u32*)0x4000000 &= ~0x1f00;
    *(u32*)0x4001000 &= ~0x1f00;
    *(u16*)0x4000304 = (self->unk_224 << 15) | (*(u16*)0x4000304 & ~0x8000);
    *(u32*)0x4000000 = (*(u32*)0x4000000 & ~0xe000) | ((u32)data_0209d460 << 13);
    *(u32*)0x4001000 = (*(u32*)0x4001000 & ~0xe000) | ((u32)data_0209d458 << 13);

    _ZN2GX10LoadBGPlttEPKvjj(c + 0x4228, 0x20, 0x1e0);
    {
        char *p = c + 0x4228; p += 0x200;
        _ZN3GXS10LoadBGPlttEPKvjj(p, 0, 0x80);
    }

    if (o->f68() != 2)
    {
        char *src = (char*)0x6600000; src += 0x6000;
        MultiCopy_Int((int*)(c + 0x2228), (int*)src, 0x2000);
    }

    data_0209d45c = (u8)self->unk_21c;
    data_0209d454 = (u8)self->unk_220;

    if (o->f68() == 2)
        return;

    *(u32*)0x4000000 = (*(u32*)0x4000000 & ~0x1f00) | ((u32)data_0209d45c << 8);
    *(u32*)0x4001000 = (*(u32*)0x4001000 & ~0x1f00) | ((u32)data_0209d454 << 8);
}

/* ---- func_ov004_020af094, slot 29 -------------------------------------- */
namespace mg94 {
struct Base {
    virtual void d0();    virtual void d1();    virtual void fn2();
    virtual void fn3();   virtual void fn4();   virtual void fn5();
    virtual void fn6();   virtual void fn7();   virtual void fn8();
    virtual void fn9();   virtual void fn10();  virtual void fn11();
    virtual void fn12();  virtual void fn13();  virtual void fn14();
    virtual void fn15();  virtual void fn16();  virtual void fn17();
    virtual void fn18();  virtual void fn19();  virtual void fn20();
    virtual void fn21();  virtual void fn22();  virtual void fn23();
    virtual void fn24();  virtual void fn25();  virtual int  Init();
};
struct Obj : Base {
    virtual int Init();
    char pad[0x4700];
};
}  /* namespace mg94 */

extern "C" void func_ov004_020af094(void *cv)
{
    mg94::Obj *self = (mg94::Obj *)cv;          /* the one added line */
    char *c = (char *)self;

    if (self->Init() != 0)
        func_02019028();

    *(int *)(c + 0x224) = (*(volatile u16 *)0x4000304 & 0x8000) >> 15;

    *(int *)(c + 0x21c) = data_0209d45c;
    *(int *)(c + 0x220) = data_0209d454;

    *(volatile u16 *)0x4000304 |= 0x8000;
    data_0209d45c = 0;
    data_0209d454 = 0;

    *(volatile int *)0x4000000 &= ~0x1f00;
    *(volatile int *)0x4001000 &= ~0x1f00;
    *(volatile int *)0x4000000 &= ~0xe000;
    *(volatile int *)0x4001000 &= ~0xe000;

    MultiCopy_Int((int *)0x5000020, (int *)(c + 0x4228), 0x1e0);
    {
        int *ip4228 = (int *)(c + 0x4228);
        MultiCopy_Int((int *)0x5000400, ip4228 + 0x80, 0x80);
    }
    _ZN4CP1527FlushAndInvalidateDataCacheEjj((void *)(c + 0x4228), 0x400);

    _ZN2GX10LoadBGPlttEPKvjj((const void *)data_ov004_020bea28, 0x20, 0xa0);
    _ZN3GXS10LoadBGPlttEPKvjj((const void *)data_ov004_020beac8, 0, 0x80);

    {
        volatile unsigned short tmp = 0;
        MultiStore16(tmp, (char *)0x5000000, 2);
    }
    {
        volatile unsigned short tmp = 0;
        MultiStore16(tmp, (char *)0x5000400, 2);
    }

    func_0201f32c(*(s16 *)(c + 0x465e));

    data_0209d45c = 0x12;
    data_0209d454 = 0x10;

    if (self->Init() == 2)
        return;

    int *vbase = (int *)0x6600000;
    int *vram = vbase + 0x1800;
    MultiCopy_Int(vram, (int *)(c + 0x2228), 0x2000);
    int idx = GetGameLanguage();
    DecompressLZ16(data_ov004_020bbf94[idx], vram);
    _ZN4CP1527FlushAndInvalidateDataCacheEjj((void *)(c + 0x2228), 0x2000);
}
