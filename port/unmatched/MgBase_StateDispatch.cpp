// PORT_HOST_ABI. The mwcc POINTER-TO-MEMBER WALL, framework half: dScMgBase_c's
// seven dispatching TUs, host-copied against an address switch. Run link60,
// lane MG2.
//
// This is the file port/mg_fanout_costs.txt section 4 says is "worth more than
// any single minigame", and it is PAID ONCE for all thirty. The per-class half
// is unmatched/MgCurling_StateDispatch.cpp, the same shape at one class's size.
//
// ---- 1. WHAT THE ROM DOES, READ OUT OF THE ROM ----------------------------
//
// An mwcc pointer-to-member is EIGHT bytes, {code, adjustment}, and the call
// through one is five instructions. Disassembled from
// extracted/overlays/overlay_0004.bin, func_ov004_020b31b4 verbatim:
//
//     020b31dc  add   r3, r0, #8          ; &self->pmf
//     020b31e0  ldr   r1, [r3, #4]        ; adjustment
//     020b31e4  add   r0, r0, r1, asr #1  ; this += adj >> 1  (arithmetic)
//     020b31e8  ands  r1, r1, #1          ; virtual bit is the adjustment's LSB
//     020b31ec  ldrne r2, [r0]            ;   virtual: r2 = *this  (the vtable)
//     020b31f0  ldrne r1, [r3]            ;            r1 = code, a BYTE OFFSET
//     020b31f4  ldrne r1, [r2, r1]        ;            fn = vtable[code]
//     020b31f8  ldreq r1, [r3]            ;   direct:  fn = code, an ADDRESS
//     020b31fc  blx   r1
//
// MSVC's single-inheritance member pointer is FOUR bytes and its call is its
// own incompatible shape, so none of that survives a recompile. Three separate
// failures, any one fatal: the stride, the content (the word is a DS code
// address), and the dispatch sequence.
//
// EVERY ADJUSTMENT WORD IN THIS SEAT'S CLOSURE READS ZERO. Verified word for
// word out of the two ROM images: dScMgBase_c's four framework pairs at
// 0x020bbf4c / 0x020bbf54 / 0x020bbf5c / 0x020bbf64, and dScMgCurling_c's
// twenty-five at 0x0213c1e4..0x0213c2bc. So every dispatch this seat can
// actually reach is the DIRECT case: no this-adjustment, no vtable indirection.
// port_mg_call0 and port_mg_call1 below implement exactly that case and REPORT
// any other, rather than implementing a shape no measurement supports. A run
// that prints the report is a measurement; a run that guesses is a wild jump.
//
// ---- 2. THE DATA IS ALREADY RIGHT, AND ONLY THE CONSUMERS ARE WRONG --------
//
// This is the finding that makes the fix small, and it was not known when
// port/mg_fanout_costs.txt was written. The .bss dispatch tables are filled by
// the overlay constructors, and EVERY ONE OF THOSE CONSTRUCTORS IS A PLAIN .c
// FILE THAT SPELLS THE PAIR AS TWO INTS. src/__sinit_ov004_020b948c.c:
//
//     struct B8 { int a, b; };  struct B16 { struct B8 p, q; };
//     data_ov004_020beb88.p = data_ov004_020bbf4c;
//
// Eight bytes on MSVC, eight in the ROM, C linkage, and the mount already
// defines the storage. So the constructors copy the ROM's pairs into the
// mount's tables BYTE-FAITHFULLY AT THE RIGHT STRIDE, and they have been doing
// it correctly all along. The only broken thing in the chain is the consumer
// that re-declares the same table as an array of MSVC member pointers and
// strides it by four. That is why these host copies change ONE declaration and
// ONE call site each and leave everything else verbatim.
//
// A COROLLARY WORTH HAVING IN FRONT OF THE FAN-OUT: a pair that is COPIED as
// data stays correct and only a pair that is CALLED needs a host copy.
// src/func_ov004_020b7cd0.cpp and src/func_ov004_020b72d4.cpp both read an
// eight-byte pair out of .data and store it into the object's own state field;
// they are NOT here, they are two ordinary aliases in hal/scene_mg_faces.cpp
// section 2b, and an alias is right because a byte copy of eight bytes is a
// byte copy of eight bytes.
//
// ---- 3. THE WALL IS TWELVE TUs AND THE LINK ONLY NAMES SIX -----------------
//
// READ THIS BEFORE COSTING ANY OF THE OTHER TWENTY-NINE. The first link of this
// slice named 36 unresolved externals, of which SEVEN are pointer-to-member
// globals in SIX TUs, and port/mg_fanout_costs.txt costs the fan-out on that
// number. It is half the real figure. A sweep of the slice's TUs for a
// pointer-to-member dispatch finds TWELVE:
//
//   NAMED BY THE LINK, because the PMF global is declared at C++ linkage and
//   MSVC bakes the member-pointer type into the symbol:
//     func_ov004_020add88  _020adf2c  _020b3278
//     func_ov006_020e0d84  _020e12d0  _020e3528
//
//   SILENT, because the PMF global is declared inside extern "C" and therefore
//   mangles as the plain C name the mount already defines. These LINK CLEAN
//   AND MISCOMPILE:
//     func_ov004_020b31b4  _020b321c  _020b8714  _020b8778
//     func_ov006_020e1214  _020e3078
//
// So A LINK IS NOT A COMPLETE DETECTOR FOR THIS WALL, and a lane that works to
// a clean link has six wild dispatches left. The four ov004 silent ones are the
// worst of the set, because they dispatch the OBJECT'S OWN pmf field rather
// than a table, so MSVC's four-byte member pointer moves every field after it:
// src/func_ov004_020b31b4.cpp puts `state` at 0x1c where the ROM reads
// [r0,#0x20]. Their layouts are re-derived from the disassembly below, one
// offset at a time, rather than from the src structs.
//
// ---- 4. WHAT IS NOT HERE ---------------------------------------------------
//
// func_ov004_020b87e0, dScMgBase_c's state SETTER, stays excluded from the
// slice and trapped in hal/scene_mg_faces.cpp. It is a different problem from
// these seven: it does not dispatch a table the mount holds, it BUILDS a
// twenty-entry static table out of twenty ov004 globals whose MSVC symbol names
// carry the member-pointer type, so there is nothing for an alias or a stride
// fix to attach to. It needs its twenty addresses routed the way these are, and
// that is a lane of its own. Until then the trap reports and sets no state.

#include <cstdio>

/* The eight-byte mwcc member pointer, in the only spelling that is true on
   both machines: two words, no member-pointer type anywhere. */
struct MgPmf { unsigned code; int adj; };

extern "C" {

/* ---- the ov004 state bodies these tables and fields hold ---------------- */
/* Reached ONLY through the switch below. None of the four had a caller in the
   build before this file: the pair words are mounted DATA holding DS
   addresses, so /OPT:REF had dropped all four. They join port/slice_mg1.txt in
   the same commit as this file, which is what gives them one. */
void func_ov004_020adc80(int *c);
void func_ov004_020adcc8(short *obj);
void func_ov004_020addcc(char *r5);
void func_ov004_020adeb0(char *c);

/* the per-class half's switch, tried after this one; the header of
   unmatched/MgCurling_StateDispatch.cpp says why the chain runs this way */
int port_mg_try_ov006_0(void *self, unsigned code);
int port_mg_try_ov006_1(void *self, unsigned code, int a);

/* the bodies the host copies below call, unchanged from their src */
void DecompressLZ16(int src, int dst);
int  func_ov004_020af5e0(int a, void *b, int c, int d);
void func_ov004_020b42c0(void);
void _Z14ApproachLinearRiii(int &, int, int);

}  /* extern "C" */

// ---- the address switch ----------------------------------------------------

static unsigned g_mg_dispatch_calls;
static unsigned g_mg_dispatch_unknown;

/* One line per distinct unhandled address, so a per-frame loop cannot flood
   the log and a single occurrence cannot hide in one. */
static void mg_unhandled(const char *what, unsigned code, int adj)
{
    static unsigned said[16];
    static int nsaid;
    ++g_mg_dispatch_unknown;
    for (int i = 0; i < nsaid; ++i)
        if (said[i] == code)
            return;
    if (nsaid < 16)
        said[nsaid++] = code;
    std::fprintf(stderr, "  [scene] MINIGAME STATE DISPATCH %s: DS address "
                 "0x%08x (adjustment 0x%08x). Nothing was called. "
                 "port/unmatched/MgBase_StateDispatch.cpp\n", what, code,
                 (unsigned)adj);
    std::fflush(stderr);
}

/* dScMgBase_c's own four. The framework half of the switch, and the reason
   this file is paid once: all thirty minigames dispatch these same four
   addresses out of the same two tables. */
static int mg_try_ov004_0(void *self, unsigned code)
{
    switch (code) {
    case 0x020adc80u: func_ov004_020adc80((int *)self);   return 1;
    case 0x020adcc8u: func_ov004_020adcc8((short *)self); return 1;
    case 0x020addccu: func_ov004_020addcc((char *)self);  return 1;
    case 0x020adeb0u: func_ov004_020adeb0((char *)self);  return 1;
    default:                                              return 0;
    }
}

/* THE ONE ENTRY POINT for a zero-argument state call. Both host-copy files
   route through it, so there is exactly one place that decides what an
   adjustment word means. */
extern "C" void port_mg_call0(void *self, unsigned code, int adj)
{
    ++g_mg_dispatch_calls;
    if (code == 0)
        return;                       /* the ROM's own null-pmf guard */
    if (adj != 0) {
        /* No pair in this seat's closure carries one, so there is no case to
           verify an implementation against. The this-adjustment is one line
           and the virtual branch is a vtable read at a byte offset, but a
           dispatch shape nobody has measured is exactly the plausible body
           port/tools/inferred_stub_guard exists to refuse. */
        mg_unhandled("with a NONZERO ADJUSTMENT, which no measured pair in "
                     "this closure has", code, adj);
        return;
    }
    if (mg_try_ov004_0(self, code))
        return;
    if (port_mg_try_ov006_0(self, code))
        return;
    mg_unhandled("UNHANDLED", code, adj);
}

extern "C" void port_mg_call1(void *self, unsigned code, int adj, int a)
{
    ++g_mg_dispatch_calls;
    if (code == 0)
        return;
    if (adj != 0) {
        mg_unhandled("with a NONZERO ADJUSTMENT, which no measured pair in "
                     "this closure has", code, adj);
        return;
    }
    /* ov004 contributes no one-argument state table to this closure: both of
       dScMgBase_c's are zero-argument. The chain still runs through here so a
       derived class's table holding a framework address finds it. */
    if (port_mg_try_ov006_1(self, code, a))
        return;
    mg_unhandled("UNHANDLED", code, adj);
}

extern "C" void port_mg_dispatch_counts(unsigned *calls, unsigned *unknown)
{
    if (calls)   *calls   = g_mg_dispatch_calls;
    if (unknown) *unknown = g_mg_dispatch_unknown;
}

// ---- the seven host copies -------------------------------------------------
//
// Each is its src TU with the pointer-to-member declaration replaced by MgPmf
// and the dispatch replaced by port_mg_call0. Everything else is verbatim,
// INCLUDING the other extern declarations: those keep their C++ spellings so
// the nineteen generated aliases in hal/scene_mg_faces_gen.cpp still have the
// references they were generated for. The twelve src lines are commented out
// of port/slice_mg1.txt with a pointer back here.

extern "C" {
/* the two framework tables, re-typed. The mount defines the storage and
   __sinit_ov004_020b948c fills it; see section 2. */
extern MgPmf data_ov004_020beb88[];
extern MgPmf data_ov004_020beb98[];
}

/* src/func_ov004_020add88.cpp. The src's struct C { char pad[0x1e]; short idx; }
   is right and is kept: idx is a short at 0x1e and no member pointer sits
   inside this object, so nothing shifts. */
namespace { struct CIdx1e { char pad[0x1e]; short idx; }; }

extern "C" void func_ov004_020add88(void *p)
{
    CIdx1e *c = (CIdx1e *)p;
    const MgPmf *e = &data_ov004_020beb98[c->idx];
    port_mg_call0(c, e->code, e->adj);
}

/* src/func_ov004_020adf2c.cpp, the same shape on the other table. */
extern "C" void func_ov004_020adf2c(void *p)
{
    CIdx1e *c = (CIdx1e *)p;
    const MgPmf *e = &data_ov004_020beb88[c->idx];
    port_mg_call0(c, e->code, e->adj);
}

// ---- the four self-field dispatchers ---------------------------------------
//
// These read the pmf out of the OBJECT, so the four-byte MSVC member pointer
// in their src structs moves every later field. The offsets below are the
// ROM's, read off the disassembly rather than off the struct:
//
//   func_ov004_020b31b4   state [r0,#0x20]   pmf r0+0x08
//   func_ov004_020b321c   state [r0,#0x20]   pmf r0+0x00
//   func_ov004_020b8714   field [r0,#0x18]   pmf r0+0x10
//   func_ov004_020b8778   field [r4,#0x18]   pmf r4+0x08, ApproachLinear on +0x1c
//
// Two of them agree on state at 0x20 with the pmf at different offsets, which
// is two different classes and not a transcription slip: 020b31b4 opens the
// pair with `add r3, r0, #8` and 020b321c with `ldr r2, [r0]`.

static inline const MgPmf *mg_self_pmf(void *self, unsigned off)
{
    return (const MgPmf *)((char *)self + off);
}

/* src/func_ov004_020b31b4.cpp */
extern "C" void func_ov004_020b31b4(void *self)
{
    char *c = (char *)self;
    if (*(int *)(c + 0x20) == 0x1d)
        return;
    const MgPmf *p = mg_self_pmf(c, 8);
    if (p->code != 0) {
        port_mg_call0(c, p->code, p->adj);
        return;
    }
    func_ov004_020b42c0();
}

/* src/func_ov004_020b321c.cpp. The ROM moves `this` to the adjusted pointer
   before the call (mov r0, r3); with every measured adjustment zero that is
   the object itself, and port_mg_call0 refuses any nonzero one. */
extern "C" void func_ov004_020b321c(void *self)
{
    char *c = (char *)self;
    if (*(int *)(c + 0x20) == 0x1d)
        return;
    const MgPmf *p = mg_self_pmf(c, 0);
    if (p->code == 0)
        return;
    port_mg_call0(c, p->code, p->adj);
}

/* src/func_ov004_020b8714.cpp */
extern "C" void func_ov004_020b8714(void *self)
{
    char *c = (char *)self;
    if (*(int *)(c + 0x18) == -1)
        return;
    const MgPmf *p = mg_self_pmf(c, 0x10);
    if (p->code == 0)
        return;
    port_mg_call0(c, p->code, p->adj);
}

/* src/func_ov004_020b8778.cpp */
extern "C" void func_ov004_020b8778(void *self)
{
    char *c = (char *)self;
    if (*(int *)(c + 0x18) == -1)
        return;
    _Z14ApproachLinearRiii(*(int *)(c + 0x1c), 0, 1);
    const MgPmf *p = mg_self_pmf(c, 8);
    if (p->code == 0)
        return;
    port_mg_call0(c, p->code, p->adj);
}

// ---- func_ov004_020b3278 ---------------------------------------------------
//
// The one big body in the set, and the only one whose OTHER declarations
// matter: it names seven globals at C++ linkage and seven of the nineteen
// generated aliases exist for exactly those references. They are transcribed
// unchanged for that reason. Only `PMF data_ov004_020bf490[]` becomes MgPmf.
//
// data_ov004_020bf490 IS ov004 .bss PAST THE END OF THE OVERLAY IMAGE
// (overlay_0004.bin covers 0x020ad660..0x020beb60) and no sinit in src/ writes
// it: the constructor that would is __sinit_ov004_020b955c, one of the two with
// a config symbol, no delink block and no source. So on the port the table
// reads zero, the ROM's own `if (data_ov004_020bf490[st])` guard declines, and
// nothing is dispatched. That is a decomp hole showing through honestly rather
// than a port bug, and it is the same shape for data_ov004_020bf428 and
// _020bf4f8 two blocks below it.

struct Obj {
    virtual int m00(); virtual int m01(); virtual int m02(); virtual int m03();
    virtual int m04(); virtual int m05(); virtual int m06(); virtual int m07();
    virtual int m08(); virtual int m09(); virtual int m10(); virtual int m11();
    virtual int m12(); virtual int m13(); virtual int m14(); virtual int m15();
    virtual int m16(); virtual int m17(); virtual int m18(); virtual int m19();
    virtual int m20(); virtual int m21(); virtual int m22(); virtual int m23();
    virtual int m24(); virtual int m25(); virtual int m26();
};

struct Pair { int w0; int w1; };
struct S3 { int v[3]; };

extern Obj *data_ov004_020beb68;
extern unsigned char data_ov004_020bf3e8[];
extern int data_ov004_020bf560[];
extern S3 data_ov004_020bc27c;
extern int data_ov004_020bf5d4[];
extern Pair data_ov004_020bf428[];
extern Pair data_ov004_020bf4f8[];
extern "C" MgPmf data_ov004_020bf490[];

extern "C" void func_ov004_020b3278(char *self, int arg1, short arg2, short arg3, int arg4, int arg5, short arg6)
{
    int a, b;

    if (data_ov004_020beb68->m26() == 2) {
        a = 0x6400000;
        b = 0;
    } else {
        a = 0x6600000;
        b = 0x6400000;
    }

    switch (arg1) {
    case 3: case 4: case 5: case 6:
    case 8: case 9: case 10: case 11: case 12:
    case 14: case 16: case 17: case 18: case 19: case 20: case 21:
        if (data_ov004_020bf3e8[0] != 0)
            return;
        DecompressLZ16(data_ov004_020bf560[arg1], a + 0x7000);
        if (b != 0)
            DecompressLZ16(data_ov004_020bf560[arg1], b + 0x7000);
        *(short *)(self + 0x30) = 0;
        data_ov004_020bf3e8[0] = 1;
        break;
    default:
        if (data_ov004_020bf3e8[1] == 0) {
            DecompressLZ16(data_ov004_020bf560[arg1], a + 0x6000);
            if (b != 0)
                DecompressLZ16(data_ov004_020bf560[arg1], b + 0x6000);
            *(short *)(self + 0x30) = 1;
            data_ov004_020bf3e8[1] = 1;
            break;
        }
        if (data_ov004_020bf3e8[2] == 0) {
            DecompressLZ16(data_ov004_020bf560[arg1], a + 0x6800);
            if (b != 0)
                DecompressLZ16(data_ov004_020bf560[arg1], b + 0x6800);
            *(short *)(self + 0x30) = 2;
            data_ov004_020bf3e8[2] = 1;
            break;
        }
        return;
    }

    *(int *)(self + 0x20) = arg1;
    *(short *)(self + 0x10) = arg2;
    *(short *)(self + 0x12) = arg3;
    *(short *)(self + 0x14) = *(short *)(self + 0x10);
    *(short *)(self + 0x16) = *(short *)(self + 0x12);
    *(int *)(self + 0x1c) = arg4;
    *(int *)(self + 0x18) = arg5;
    *(short *)(self + 0x32) = 0;

    if (arg6 != 0xd) {
        *(short *)(self + 0x2e) = arg6;
    } else {
        switch (arg1) {
        case 11:
            *(short *)(self + 0x2e) = 7;
            break;
        case 3: case 4: case 5: case 6: case 20: case 21:
            *(short *)(self + 0x2e) = 8;
            break;
        case 8: case 14:
            *(short *)(self + 0x2e) = 9;
            break;
        case 0:
            *(short *)(self + 0x2e) = 3;
            break;
        case 1: case 2:
            *(short *)(self + 0x2e) = 4;
            break;
        case 13:
            *(short *)(self + 0x2e) = 0xc;
            break;
        default:
            *(short *)(self + 0x2e) = 0;
            break;
        }
    }

    {
        S3 tmp = data_ov004_020bc27c;
        *(short *)(self + 0x2c) = (short)func_ov004_020af5e0(
            data_ov004_020bf5d4[*(int *)(self + 0x20)],
            self + 0x34,
            tmp.v[*(short *)(self + 0x30)],
            *(int *)(self + 0x20));
    }

    /* THE ONE CHANGED CALL. The src reads
           if (data_ov004_020bf490[st])
               (((Base *)self)->*data_ov004_020bf490[st])();
       and its null test is a test of the code word, which is what it becomes. */
    {
        int st = *(short *)(self + 0x2e);
        const MgPmf *p = &data_ov004_020bf490[st];
        if (p->code != 0)
            port_mg_call0(self, p->code, p->adj);
    }

    {
        short st;
        Pair *e;
        st = *(short *)(self + 0x2e);
        e = &data_ov004_020bf428[st];
        a = data_ov004_020bf428[st].w0;
        b = e->w1;
        *(int *)(self + 0) = b ? a : a;
        *(int *)(self + 4) = b;
        st = *(short *)(self + 0x2e);
        e = &data_ov004_020bf4f8[st];
        a = data_ov004_020bf4f8[st].w0;
        b = e->w1;
        *(int *)(self + 8) = b ? a : a;
        *(int *)(self + 0xc) = b;
    }
}
