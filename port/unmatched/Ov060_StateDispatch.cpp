/* THE BOWSER FIGHT'S STATE MACHINES -- ov060's six pointer-to-member dispatch
 * tables, seated with host bodies, and host copies of the five dispatchers
 * whose pointer-to-member STRIDE does not match the ROM record.
 *
 * run linkw wave 6, lane w6-A.
 *
 * ==== THE SIX TABLES, RE-DERIVED FROM THE OVERLAY IMAGE =====================
 *
 * Every one is bss, filled by an ov060 sinit out of 8-byte {code, adj} source
 * statics in .data, and every one of the 50 source records reads adj == 0 (the
 * non-virtual complete-class Itanium form) -- read from
 * extracted/overlays/overlay_0060.bin and cross-checked word for word against
 * config/arm9/overlays/ov060/relocs.txt.  The three arm9-side widths are the
 * next-symbol landings.
 *
 *   bss table            slots  built by                dispatched by
 *   0x0211aeb4 (0x20)      4    __sinit_ov060_021195dc  func_ov060_02112434
 *                                                       (BOWSER, idx +0x410)
 *   0x0211aed4 (0xa0)     20    __sinit_ov060_021195dc  func_ov060_021128c0
 *                                                       (BOWSER, idx +0x40c)
 *   0x0211ae9c (0x18)      3    __sinit_ov060_021195dc  func_ov060_02115b84
 *                                                       (BOWSER TAIL, idx +0x110)
 *   0x0211af74 (0x40)      8    __sinit_ov060_02119df0  BowserFire::InitResources
 *                                                       (idx +0x35c)
 *   0x0211afb4 (0x40)      8    __sinit_ov060_02119df0  BowserFire::Behavior
 *                                                       (idx +0x35c)
 *   0x0211b1d8 (0x20)      4    __sinit_ov060_0211a388  SpikeBomb::Behavior
 *                                                       (16-byte records, idx +0x170)
 *   0x0211b1ac (0x18)      3    __sinit_ov060_0211a000  func_ov060_02118254
 *                                                       (SKY PLATFORM, idx +0x328)
 *
 * The lane brief carried a banked reading that "Bowser's two state-pair tables"
 * are 0x0211a4e0.. and 0x0211a734..  Corrected here from the readers: the
 * 0x0211a734 run is BOWSER FIRE's (both of its halves), and 0x0211a4e0 itself
 * is not a dispatch record at all -- it is the four-halfword message-id table
 * func_ov060_02115518 indexes with the actor's +0x414 variant byte.  Bowser's
 * own dispatch starts one record later, at 0x0211a4e8.  There are SIX tables,
 * not two, and one of them (SpikeBomb's) was not in the bank at all.
 *
 * ==== WHY FIVE DISPATCHERS ARE HOST COPIES AND TWO ARE NOT ==================
 *
 * The ROM record is 8 bytes.  MSVC's pointer-to-member size depends on what it
 * knows about the class at the point the type is formed, and none of the five
 * shapes in this pack land on 8.  MEASURED with this build's compiler
 * (MSVC 19.44 x86, the same switches the port builds with):
 *
 *   forward-declared C, DEFINED later in the same TU  ->  4
 *     (func_ov060_02112434, func_ov060_02115b84, func_ov060_02118254)
 *   complete empty `struct Actor { }`                 ->  4
 *     (_ZN10BowserFire13InitResourcesEv)
 *   forward-declared, never defined                   ->  16
 *     (_ZN10BowserFire8BehaviorEv)
 *   plain `struct { int a, b; }` + manual decode      ->  8   <-- correct
 *     (func_ov060_021128c0, _ZN17BowserSkyPlatform8BehaviorEv)
 *
 * A 4-byte stride reads record 0 correctly and then walks the adj words, so
 * state 0 runs and every other state calls through a zero -- the fault only
 * appears once the fight leaves its first state, which is exactly the shape
 * that survives a short census and dies in play.  On top of the stride, MSVC
 * emits a __thiscall through a pointer-to-member (receiver in ecx) while every
 * ov060 state body is a cdecl `func_ov060_xxxxxxxx(char *)` -- so even a
 * correctly strided read would hand the body a garbage argument.  Both faults
 * are the Crate / Chuckya / MontyMole case; the fix is the same one
 * port/unmatched/Crate_StateDispatch.cpp writes down: read the record as a
 * plain {function, adj} pair and call the function with `this`.
 *
 * func_ov060_021128c0 and _ZN17BowserSkyPlatform8BehaviorEv already decode the
 * record by hand into a two-int struct and call through a cdecl function
 * pointer, so they are RIGHT as matched src and stay in the slice.  They still
 * need the seat: the words they call are DS code addresses.
 *
 * ==== TWO NAMED HOLES =======================================================
 *
 * func_ov060_021140c0 (0x1f4 bytes, BOWSER state 9) and func_ov060_02116d78
 * (0x1fc bytes, BOWSER FIRE behaviour state 5) are UNMATCHED -- no TU of
 * either name exists in src/ on this tree.  Both are seated with an
 * abort-and-name stub, the Koopa-0x02117724 / Rabbit-0x0212b8dc precedent: the
 * link is satisfied, the seat's ROM check still runs, and if the fight ever
 * reaches those two states the port says which one instead of jumping into the
 * overlay image.
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

/* The ROM's record, as the overlay image lays it out: {code address, adj}. */
struct PortPmf { unsigned fn; int adj; };

/* ---- BOWSER: 0x0211aeb4, four records (sinit 021195dc copy order) -------- */
extern PortPmf data_ov060_0211a578[], data_ov060_0211a510[],
    data_ov060_0211a518[], data_ov060_0211a520[];
/* ---- BOWSER: 0x0211aed4, twenty records --------------------------------- */
extern PortPmf data_ov060_0211a568[], data_ov060_0211a560[],
    data_ov060_0211a538[], data_ov060_0211a4e8[], data_ov060_0211a4f0[],
    data_ov060_0211a500[], data_ov060_0211a540[], data_ov060_0211a5b8[],
    data_ov060_0211a5b0[], data_ov060_0211a5a0[], data_ov060_0211a5a8[],
    data_ov060_0211a530[], data_ov060_0211a508[], data_ov060_0211a528[],
    data_ov060_0211a598[], data_ov060_0211a590[], data_ov060_0211a588[],
    data_ov060_0211a580[], data_ov060_0211a4f8[], data_ov060_0211a570[];
/* ---- BOWSER TAIL: 0x0211ae9c, three records ----------------------------- */
extern PortPmf data_ov060_0211a558[], data_ov060_0211a550[],
    data_ov060_0211a548[];
/* ---- BOWSER FIRE: 0x0211afb4 (behaviour) and 0x0211af74 (init) ---------- */
extern PortPmf data_ov060_0211a794[], data_ov060_0211a78c[],
    data_ov060_0211a76c[], data_ov060_0211a77c[], data_ov060_0211a764[],
    data_ov060_0211a774[], data_ov060_0211a744[], data_ov060_0211a784[];
extern PortPmf data_ov060_0211a75c[], data_ov060_0211a754[],
    data_ov060_0211a73c[], data_ov060_0211a74c[], data_ov060_0211a734[],
    data_ov060_0211a7ac[], data_ov060_0211a7a4[], data_ov060_0211a79c[];
/* ---- SPIKE BOMB: 0x0211b1d8, four records ------------------------------- */
extern PortPmf data_ov060_0211aa30[], data_ov060_0211aa48[],
    data_ov060_0211aa40[], data_ov060_0211aa38[];
/* ---- SKY PLATFORM: 0x0211b1ac, three records ---------------------------- */
extern PortPmf data_ov060_0211a938[], data_ov060_0211a940[],
    data_ov060_0211a930[];

/* the runtime tables the dispatchers read (bss, filled by the sinits) */
extern PortPmf data_ov060_0211aeb4[];
extern PortPmf data_ov060_0211ae9c[];
extern PortPmf data_ov060_0211b1ac[];
extern PortPmf data_ov060_0211af74[];
extern PortPmf data_ov060_0211afb4[];

/* ---- every state body, all matched src (cdecl, `this` by argument) ------- */
void func_ov060_021128c0(char *c);
void func_ov060_02112724(char *c);
void func_ov060_021125f0(char *c);

void func_ov060_02114f88(char *c);
void func_ov060_02113b5c(char *c);
void func_ov060_02113740(char *c);
void func_ov060_02113710(char *c);
void func_ov060_02112ddc(char *c);
void func_ov060_021154e8(char *c);
void func_ov060_021153f8(char *c);
void func_ov060_02113d8c(char *c);
void func_ov060_02114858(char *c);
void func_ov060_021142b4(char *c);
void func_ov060_02113fcc(char *c);
void func_ov060_021146d0(char *c);
void func_ov060_021143b8(char *c);
void func_ov060_02114d08(char *c);
void func_ov060_02114e9c(char *c);
void func_ov060_02114b60(char *c);
void func_ov060_02114300(char *c);
void func_ov060_02114ff8(char *c);
void func_ov060_02112bfc(char *c);

void func_ov060_02115d68(char *c);
void func_ov060_02115d50(char *c);
void func_ov060_02115c1c(char *c);

void func_ov060_0211747c(char *c);
void func_ov060_021169f8(char *c);
void func_ov060_02116b68(char *c);
void func_ov060_021167ec(char *c);
void func_ov060_021168c4(char *c);
void func_ov060_02116f90(char *c);
void func_ov060_021167c8(char *c);
void func_ov060_02116b18(char *c);
void func_ov060_02116c68(char *c);
void func_ov060_021169b0(char *c);
void func_ov060_02116f74(char *c);
void func_ov060_0211722c(char *c);
void func_ov060_021171e8(char *c);

void func_ov060_02118970(char *c);
void func_ov060_021188e8(char *c);
void func_ov060_02118834(char *c);
void func_ov060_02118728(char *c);

void func_ov060_021181b4(char *c);
void func_ov060_021180e0(char *c);
void func_ov060_02117db8(char *c);

/* what the five host copies below call */
int Vec3_HorzDist(const void *a, const void *b);
short Vec3_HorzAngle(const void *a, const void *b);
int _ZN5Actor14GetSubtractionEss(void *self, short a, short b);
char *_ZN5Actor10FindWithIDEj(unsigned id);
void _ZN12CylinderClsn5ClearEv(void *cc);
void _ZN12CylinderClsn6UpdateEv(void *cc);
void _ZN8Platform21UpdateModelPosAndRotYEv(void *p);
void _ZN8Platform19UpdateClsnPosAndRotEv(void *p);
int _ZN11ShadowModel12InitCylinderEv(void *self);
void _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(
    void *self, void *actor, int a, int b, unsigned c, unsigned d);
void _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(
    void *self, void *actor, int a, int b, void *v, int c);
void _ZN13RaycastGroundC1Ev(void *self);
void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(
    void *self, const void *v, void *actor);
int _ZN13RaycastGround10DetectClsnEv(void *self);
void _ZN13RaycastGroundD1Ev(void *self);
void WithMeshClsn_UpdateDiscreteNoLava_veneer(void *p);
int _ZNK12WithMeshClsn10IsOnGroundEv(void *c);
void func_ov060_02116740(char *c);
void func_ov060_02117624(char *c);

}  /* extern "C" */

/* ============ THE TWO NAMED HOLES ========================================= */
static void ov60_hole(const char *what, unsigned rom)
{
    std::fprintf(stderr, "FATAL: ov060 state body %s (ROM 0x%08x) is NOT "
                 "decompiled -- the Bowser fight reached a state this port "
                 "cannot run\n", what, rom);
    std::abort();
}
extern "C" void func_ov060_021140c0(char *c)
{ (void)c; ov60_hole("func_ov060_021140c0 (BOWSER state 9)", 0x021140c0); }
extern "C" void func_ov060_02116d78(char *c)
{ (void)c; ov60_hole("func_ov060_02116d78 (BOWSER FIRE state 5)", 0x02116d78); }

/* ============ HOST COPY 1: func_ov060_02112434 ============================
 * BOWSER's per-frame target/flag pass, called from Bowser::Behavior.  Line for
 * line with src/func_ov060_02112434.cpp; only the dispatch is respelled.
 * PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
extern "C" void func_ov060_02112434(unsigned char *thiz)
{
    int zero[3];
    zero[0] = 0;
    zero[1] = 0;
    zero[2] = 0;
    *(int *)(thiz + 0x3f4) = Vec3_HorzDist(thiz + 0x5c, zero);
    *(short *)(thiz + 0x408) = Vec3_HorzAngle(thiz + 0x5c, zero);

    int s0 = _ZN5Actor14GetSubtractionEss(thiz, *(short *)(thiz + 0x8e),
                                          *(short *)(thiz + 0x406));
    int s1 = _ZN5Actor14GetSubtractionEss(thiz, *(short *)(thiz + 0x8e),
                                          *(short *)(thiz + 0x408));

    *(int *)(thiz + 0x418) &= ~0xff;
    if (s0 < 0x2000)
        *(int *)(thiz + 0x418) |= 2;
    if (s1 < 0x3800)
        *(int *)(thiz + 0x418) |= 4;
    if (*(int *)(thiz + 0x3f4) < 0x3e8000)
        *(int *)(thiz + 0x418) |= 0x10;
    if (*(int *)(thiz + 0x3ec) < 0x352000)
        *(int *)(thiz + 0x418) |= 8;

    {
        PortPmf *e = &data_ov060_0211aeb4[*(int *)(thiz + 0x410)];
        ((void (*)(char *))(size_t)e->fn)((char *)thiz + (e->adj >> 1));
    }

    if (*(int *)(thiz + 0x40c) == 4)
        return;

    unsigned char lo = thiz[0x41c];
    unsigned char hi = thiz[0x41d];
    if (hi == lo)
        return;
    if (hi > lo) {
        int v = lo + 0x14;
        if (v >= 0xff) {
            thiz[0x41c] = 0xff;
            return;
        }
        thiz[0x41c] = (unsigned char)(thiz[0x41c] + 0x14);
        return;
    }
    {
        int v = lo - 0x14;
        if (v <= 0)
            thiz[0x41c] = 0;
        else
            thiz[0x41c] = (unsigned char)(thiz[0x41c] - 0x14);
    }
}

/* ============ HOST COPY 2: func_ov060_02115b84 ============================
 * BOWSER TAIL's per-frame pass: it dispatches its own three-state table, then
 * reads the OWNER Bowser (its +0x108 actor id, the handle Bowser's init wrote)
 * to learn whether the fight is over.  Line for line with
 * src/func_ov060_02115b84.cpp.
 * PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
extern "C" void func_ov060_02115b84(char *c)
{
    char *r5 = _ZN5Actor10FindWithIDEj(*(unsigned *)(c + 0x108));
    int idx = *(int *)(c + 0x110);
    {
        PortPmf *e = &data_ov060_0211ae9c[idx];
        ((void (*)(char *))(size_t)e->fn)(c + (e->adj >> 1));
    }
    if (*(int *)(r5 + 0x40c) == 4)
        *(int *)(c + 0xec) |= 1;
    *(unsigned short *)(c + 0x114) = (unsigned short)
        (*(unsigned short *)(c + 0x114) + 1);
    if (idx != *(int *)(c + 0x110))
        *(short *)(c + 0x114) = 0;
    _ZN12CylinderClsn5ClearEv(c + 0xd4);
    _ZN12CylinderClsn6UpdateEv(c + 0xd4);
}

/* ============ HOST COPY 3: func_ov060_02118254 ============================
 * BOWSER SKY PLATFORM's Behavior (vtable slot 6 of data_ov060_0211a9b0).
 * Line for line with src/func_ov060_02118254.cpp.
 * PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
extern "C" int func_ov060_02118254(char *c)
{
    {
        PortPmf *e = &data_ov060_0211b1ac[*(unsigned char *)(c + 0x328)];
        ((void (*)(char *))(size_t)e->fn)(c + (e->adj >> 1));
    }
    _ZN8Platform21UpdateModelPosAndRotYEv(c);
    _ZN8Platform19UpdateClsnPosAndRotEv(c);
    *(unsigned char *)(c + 0x32b) = 0;
    return 1;
}

/* ============ HOST COPY 4: _ZN10BowserFire13InitResourcesEv ===============
 * Line for line with src/_ZN10BowserFire13InitResourcesEv.cpp.
 * PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
extern "C" int _ZN10BowserFire13InitResourcesEv(char *c)
{
    unsigned char rc[0x50];
    int pos[3];
    if (_ZN11ShadowModel12InitCylinderEv(c + 0x304) == 0)
        return 0;
    _ZN18MovingCylinderClsn4InitEP5Actor5Fix12IiES3_jj(
        c + 0x2d0, c, 0x28000, 0x50000, 0x200002, 0);
    _ZN12WithMeshClsn4InitEP5Actor5Fix12IiES3_P10Vector3_16S5_(
        c + 0x110, c, 0x32000, 0x32000, 0, 0);
    *(int *)(c + 0x9c) = -0x4000;
    *(int *)(c + 0xa0) = -0x1e000;
    *(int *)(c + 0x35c) = *(int *)(c + 8) & 7;
    *(short *)(c + 0x374) = 0;
    if (*(int *)(c + 0x35c) == 0)
        *(unsigned char *)(c + 0x379) = 0;
    else
        *(unsigned char *)(c + 0x379) = 1;
    *(int *)(c + 0x36c) = 0;
    *(unsigned char *)(c + 0x378) =
        (unsigned char)((*(unsigned *)(c + 8) >> 4) & 3);
    if (*(int *)(c + 0x35c) == 0)
        *(int *)(c + 0x2e8) |= 1;
    *(int *)(c + 0x360) = 0x2000;
    *(int *)(c + 0x380) = 0;
    *(int *)(c + 0x37c) = *(int *)(c + 0x380);
    *(int *)(c + 0x2cc) = 0;
    _ZN13RaycastGroundC1Ev(rc);
    pos[0] = *(int *)(c + 0x5c);
    pos[1] = *(int *)(c + 0x60) + 0x32000;
    pos[2] = *(int *)(c + 0x64);
    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(rc, pos, 0);
    if (_ZN13RaycastGround10DetectClsnEv(rc))
        *(int *)(c + 0x364) = *(int *)(rc + 0x14 + 12 * 4);
    else
        *(int *)(c + 0x364) = *(int *)(c + 0x60);
    {
        PortPmf *e = &data_ov060_0211af74[*(int *)(c + 0x35c)];
        ((void (*)(char *))(size_t)e->fn)(c + (e->adj >> 1));
    }
    *(int *)(c + 0x384) = 0;
    *(int *)(c + 0x388) = 0;
    _ZN13RaycastGroundD1Ev(rc);
    return 1;
}

/* ============ HOST COPY 5: _ZN10BowserFire8BehaviorEv =====================
 * Line for line with src/_ZN10BowserFire8BehaviorEv.cpp.
 * PORT_HOST_ABI: mwcc pointer-to-member stride/receiver, the Crate case. */
extern "C" int _ZN10BowserFire8BehaviorEv(char *c)
{
    *(int *)(c + 0x370) += 1;
    {
        PortPmf *e = &data_ov060_0211afb4[*(int *)(c + 0x35c)];
        ((void (*)(char *))(size_t)e->fn)(c + (e->adj >> 1));
    }
    *(unsigned short *)(c + 0x374) = (unsigned short)
        (*(unsigned short *)(c + 0x374) + 1);
    if (*(int *)(c + 0x9c) != 0) {
        WithMeshClsn_UpdateDiscreteNoLava_veneer(c + 0x110);
        if (*(int *)(c + 0x35c) != 4) {
            if (_ZNK12WithMeshClsn10IsOnGroundEv(c + 0x110) != 0) {
                *(int *)(c + 0xa8) = 0;
                *(int *)(c + 0x9c) = 0;
            }
        }
    }
    func_ov060_02116740(c);
    func_ov060_02117624(c);
    _ZN12CylinderClsn5ClearEv(c + 0x2d0);
    _ZN12CylinderClsn6UpdateEv(c + 0x2d0);
    return 1;
}

/* ============ THE SEAT ====================================================
 * Rewrites each SOURCE static's code word with its host body BEFORE the sinits
 * copy it into the runtime table (the WaterBomb / LakituBro / Crate reading:
 * one less mapping to get wrong), each checked against the ROM address the
 * body was compiled from, so a mount pointing at the wrong bytes says so
 * instead of calling into the overlay image.  All 50 adj halves are 0. */
namespace {
struct Seat { PortPmf *slot; unsigned rom; void (*host)(char *); const char *tab; };
const Seat g_ov060_states[] = {
    /* 0x0211aeb4 -- BOWSER, four */
    {data_ov060_0211a578, 0x021128c0, func_ov060_021128c0, "aeb4[0]"},
    {data_ov060_0211a510, 0x02112724, func_ov060_02112724, "aeb4[1]"},
    {data_ov060_0211a518, 0x021125f0, func_ov060_021125f0, "aeb4[2]"},
    {data_ov060_0211a520, 0x021125f0, func_ov060_021125f0, "aeb4[3]"},
    /* 0x0211aed4 -- BOWSER, twenty */
    {data_ov060_0211a568, 0x02114f88, func_ov060_02114f88, "aed4[0]"},
    {data_ov060_0211a560, 0x02113b5c, func_ov060_02113b5c, "aed4[1]"},
    {data_ov060_0211a538, 0x02113740, func_ov060_02113740, "aed4[2]"},
    {data_ov060_0211a4e8, 0x02113710, func_ov060_02113710, "aed4[3]"},
    {data_ov060_0211a4f0, 0x02112ddc, func_ov060_02112ddc, "aed4[4]"},
    {data_ov060_0211a500, 0x021154e8, func_ov060_021154e8, "aed4[5]"},
    {data_ov060_0211a540, 0x021153f8, func_ov060_021153f8, "aed4[6]"},
    {data_ov060_0211a5b8, 0x02113d8c, func_ov060_02113d8c, "aed4[7]"},
    {data_ov060_0211a5b0, 0x02114858, func_ov060_02114858, "aed4[8]"},
    {data_ov060_0211a5a0, 0x021140c0, func_ov060_021140c0, "aed4[9] HOLE"},
    {data_ov060_0211a5a8, 0x021142b4, func_ov060_021142b4, "aed4[10]"},
    {data_ov060_0211a530, 0x02113fcc, func_ov060_02113fcc, "aed4[11]"},
    {data_ov060_0211a508, 0x021146d0, func_ov060_021146d0, "aed4[12]"},
    {data_ov060_0211a528, 0x021143b8, func_ov060_021143b8, "aed4[13]"},
    {data_ov060_0211a598, 0x02114d08, func_ov060_02114d08, "aed4[14]"},
    {data_ov060_0211a590, 0x02114e9c, func_ov060_02114e9c, "aed4[15]"},
    {data_ov060_0211a588, 0x02114b60, func_ov060_02114b60, "aed4[16]"},
    {data_ov060_0211a580, 0x02114300, func_ov060_02114300, "aed4[17]"},
    {data_ov060_0211a4f8, 0x02114ff8, func_ov060_02114ff8, "aed4[18]"},
    {data_ov060_0211a570, 0x02112bfc, func_ov060_02112bfc, "aed4[19]"},
    /* 0x0211ae9c -- BOWSER TAIL, three */
    {data_ov060_0211a558, 0x02115d68, func_ov060_02115d68, "ae9c[0]"},
    {data_ov060_0211a550, 0x02115d50, func_ov060_02115d50, "ae9c[1]"},
    {data_ov060_0211a548, 0x02115c1c, func_ov060_02115c1c, "ae9c[2]"},
    /* 0x0211afb4 -- BOWSER FIRE behaviour, eight */
    {data_ov060_0211a794, 0x0211747c, func_ov060_0211747c, "afb4[0]"},
    {data_ov060_0211a78c, 0x021169f8, func_ov060_021169f8, "afb4[1]"},
    {data_ov060_0211a76c, 0x02116b68, func_ov060_02116b68, "afb4[2]"},
    {data_ov060_0211a77c, 0x021167ec, func_ov060_021167ec, "afb4[3]"},
    {data_ov060_0211a764, 0x021168c4, func_ov060_021168c4, "afb4[4]"},
    {data_ov060_0211a774, 0x02116d78, func_ov060_02116d78, "afb4[5] HOLE"},
    {data_ov060_0211a744, 0x02116f90, func_ov060_02116f90, "afb4[6]"},
    {data_ov060_0211a784, 0x02116f90, func_ov060_02116f90, "afb4[7]"},
    /* 0x0211af74 -- BOWSER FIRE init, eight */
    {data_ov060_0211a75c, 0x021167c8, func_ov060_021167c8, "af74[0]"},
    {data_ov060_0211a754, 0x02116b18, func_ov060_02116b18, "af74[1]"},
    {data_ov060_0211a73c, 0x02116c68, func_ov060_02116c68, "af74[2]"},
    {data_ov060_0211a74c, 0x021167c8, func_ov060_021167c8, "af74[3]"},
    {data_ov060_0211a734, 0x021169b0, func_ov060_021169b0, "af74[4]"},
    {data_ov060_0211a7ac, 0x02116f74, func_ov060_02116f74, "af74[5]"},
    {data_ov060_0211a7a4, 0x0211722c, func_ov060_0211722c, "af74[6]"},
    {data_ov060_0211a79c, 0x021171e8, func_ov060_021171e8, "af74[7]"},
    /* 0x0211b1d8 -- SPIKE BOMB, four */
    {data_ov060_0211aa30, 0x02118970, func_ov060_02118970, "b1d8[0]"},
    {data_ov060_0211aa48, 0x021188e8, func_ov060_021188e8, "b1d8[1]"},
    {data_ov060_0211aa40, 0x02118834, func_ov060_02118834, "b1d8[2]"},
    {data_ov060_0211aa38, 0x02118728, func_ov060_02118728, "b1d8[3]"},
    /* 0x0211b1ac -- SKY PLATFORM, three (sinit copy order) */
    {data_ov060_0211a938, 0x021181b4, func_ov060_021181b4, "b1ac[0]"},
    {data_ov060_0211a940, 0x021180e0, func_ov060_021180e0, "b1ac[1]"},
    {data_ov060_0211a930, 0x02117db8, func_ov060_02117db8, "b1ac[2]"},
};
}  /* namespace */

extern "C" void port_ov060_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    for (unsigned i = 0; i < sizeof g_ov060_states / sizeof g_ov060_states[0];
         ++i) {
        PortPmf *p = g_ov060_states[i].slot;
        if (p->fn != g_ov060_states[i].rom || p->adj != 0) {
            std::fprintf(stderr, "FATAL: ov060 state %s: the mount holds "
                         "%08x/%d, the ROM's own record says %08x/0 -- WRONG "
                         "BYTES\n", g_ov060_states[i].tab, p->fn, p->adj,
                         g_ov060_states[i].rom);
            std::abort();
        }
        p->fn = (unsigned)(size_t)g_ov060_states[i].host;
    }
}
