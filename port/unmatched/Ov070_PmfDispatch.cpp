/* RUN LINKW WAVE 17 (lane w17): the two ov070 STATE-TICK dispatchers, hosted.
 *
 * WHY THIS FILE EXISTS. Mounting level 27 (Tick Tock Clock) put the first
 * AMP (266) and FLAME_CHOMP (270) instances in front of a mounted level's
 * tick loop, and both took an access violation on frame 0 -- the fault run
 * linkw wave 8 recorded against levels 16/21/25/27/28 and filed as
 * "DATA-dependent, not a dead vtable slot". It is neither: it is the mwcc
 * POINTER-TO-MEMBER width bug, the Painting/Bbh/Fish class, and it fires on
 * every instance of both classes on every level.
 *
 * MEASURED, not reasoned. With SM64DS_LEVEL=27 SM64DS_FAULTS_FATAL=1:
 *
 *   FAULT c0000005 at +0xffc00000 (EIP = 0) accessing 00000000
 *     actor id 0x10a (AMP), eax=00000000 ecx=<this> ebx=_ZTV3Amp
 *     stack[00] = Amp::Behavior +0xa
 *
 * and the same shape one class over with actor id 0x10e (FLAME_CHOMP),
 * stack[00] = FlameChomp::Behavior +0x9. Both call sites are the FIRST call
 * in their Behavior, and both callees tail-jump through a null.
 *
 * THE BUG. src/func_ov070_02120d34.cpp is
 *
 *     struct C; typedef void (C::*PMF)();
 *     struct C { char pad[0x41c]; PMF *pp; };
 *     extern "C" void func_ov070_02120d34(C *c) { PMF *p = c->pp + 1; (c->**p)(); }
 *
 * and the ROM body it decompiles (overlay_0070.bin at 0x02120d34) is
 *
 *     ldr  r1, [r0, #0x41c]      pp, the current State object
 *     add  r3, r1, #8            &pp[1]  -- the mwcc PMF stride is EIGHT
 *     ldr  r1, [r3, #4]          adj
 *     add  r0, r0, r1, asr #1    this += adj >> 1
 *     ands r1, r1, #1            odd adj = virtual PMF
 *     ldrne r2, [r0] / ldrne r1, [r3] / ldrne r1, [r2, r1]
 *     ldreq r1, [r3]             even adj = the function itself
 *     blx  r1
 *
 * mwcc's pointer-to-member over an incomplete class is the Itanium 8-byte
 * {fn, adj} record. MSVC compiles the same declaration to its 4-byte
 * single-inheritance form, so `c->pp + 1` strides FOUR and lands on the adj
 * word of record 0 -- which every one of these records holds as ZERO. The
 * call is therefore literally `call 0`, which is exactly the EIP the fault
 * reports.
 *
 * THE STRIDE IS PINNED BY THE SETTER, not inferred. func_ov070_02120da8 (Amp)
 * and func_ov070_02121880 (FlameChomp) both compute the State pointer as
 * `table + (state << 4)` -- sixteen bytes per state, i.e. TWO 8-byte PMFs per
 * State: pp[0] is the enter handler and pp[1] is the per-frame tick. That is
 * also why the record counts line up: Amp seats SIX source records (three
 * states), FlameChomp EIGHT (four states), in ov70_seat_state_pmfs.
 *
 * WHY ONLY THESE TWO OF THE FOUR. ov070 has four of these dispatchers, and
 * the other two -- func_ov070_02120d70 and func_ov070_02121848 -- read pp[0]
 * rather than pp[1]. At index 0 the wrong stride cannot bite: MSVC's 4-byte
 * PMF at offset 0 IS the fn word, and every seated record carries adj 0, so
 * the compiled `call [p]` with the unadjusted `this` is the ROM's own
 * even-adj branch. They are correct BY LUCK, not by design; they are left
 * alone here because they work and because touching them is not this lane's
 * to do, but the luck is worth writing down.
 *
 * THE ROUTE. The two matched sources stay in src/ and stay on
 * port/slice_w5c.txt, untouched -- this lane owns neither. Their two CALLERS
 * get a per-source -D in port/CMakeLists.txt pointing the name at the copy
 * below (the R1..R7 remedy). The consequence is honest and should be read as
 * such: src/func_ov070_02120d34.cpp and src/func_ov070_0212180c.cpp still
 * link and still count as linked TUs, and they are now dead code. Retiring
 * them belongs with the wider fix.
 *
 * THE WIDER FIX, which is NOT this lane's. `grep -rl "c->pp + 1" src/` finds
 * TWENTY-THREE TUs with this exact shape across ov018 ov019 ov027 ov030 ov070
 * ov071 ov072 ov077 ov080 ov081 ov085 ov096 plus KingBobOmb, MrBlizzard and
 * ChiefChilly. Every one of them is the same `call 0` the moment its class is
 * both registered and ticked. Two of the twenty-three are fixed here because
 * they are what stands between level 27 and a green battery; the other
 * twenty-one are a lane of their own and are named in the wave-17 status doc.
 *
 * AND A SECOND, INDEPENDENT BUG BEHIND IT. With the PMF dispatch fixed, both
 * classes then faulted in RENDER instead:
 *
 *   FAULT c0000005 accessing 00000000
 *     Model::Virtual10 +0xc  <- ModelAnim::Virtual10 +0x25
 *                            <- ModelAnim::Virtual18 +0xe
 *                            <- Amp::Render +0x14
 *
 * which is the ModelAnim SLOT-5 collision, signature for signature the one
 * port/unmatched/ModelAnim_Renders.cpp measured on Butterfly. Both Renders
 * dispatch through a LOCAL SIX-VIRTUAL SHADOW over the ModelAnim at +0xd4, so
 * their slot 5 is the ROM's ModelAnim::Render while the host _ZTV9ModelAnim's
 * slot 5 is ModelAnim::Virtual18 -- a two-argument method reading its scale
 * off the stack. Both Renders are hosted below too, from the ROM listings.
 * (This is also why wave 8's SNUFIT note reads Model::Virtual10 +0xc: it is
 * the same collision on a third class, still open.)
 */

#include "Model.h"
#include "ModelAnim.h"

extern "C" {

/* One mwcc {fn, adj} pointer-to-member call, layout spelled out. adj is 0 on
   every record ov70_seat_state_pmfs writes and on every ROM source record it
   copies, so the virtual branch is unreachable in practice; it is spelled
   anyway so a record that ever carries an odd adj declines loudly instead of
   dispatching through a host vtable with ROM byte offsets. */
void port_actor_slot_decline(const char *what);

static void ov070_pmf_call(unsigned char *rec, char *self)
{
    int adj = *(int *)(rec + 4);
    char *adjusted = self + (adj >> 1);
    if (adj & 1) {
        port_actor_slot_decline("ov070 state PMF carries a VIRTUAL adj; the "
                                "host vtables do not use ROM byte offsets");
        return;
    }
    ((void (*)(char *)) * (void **)rec)(adjusted);
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch (8-byte {fn,adj} vs MSVC's
   4-byte single-inheritance form); src/func_ov070_02120d34.cpp strides
   c->pp + 1 by four under MSVC and calls the zero adj word. */
void port_ov070_amp_state_tick(void *selfv)
{
    char *c = (char *)selfv;
    unsigned char *pp = *(unsigned char **)(c + 0x41c);
    ov070_pmf_call(pp + 8, c);
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch, the same ruling one class
   over -- src/func_ov070_0212180c.cpp over the State at this+0x39c. */
void port_ov070_flamechomp_state_tick(void *selfv)
{
    char *c = (char *)selfv;
    unsigned char *pp = *(unsigned char **)(c + 0x39c);
    ov070_pmf_call(pp + 8, c);
}

int _ZN15TextureSequence6UpdateER15ModelComponents(void *seq, void *comp);
int _ZN18TextureTransformer6UpdateER15ModelComponents(void *tr, void *comp);

/* PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the Whomp/Butterfly
   case -- the host _ZTV9ModelAnim slot 5 is Virtual18. Amp::Render, ROM
   0x02120e24 line for line:
       add r0,r4,#0xd4 / ldr r2,[r0] / mov r1,#0 / ldr r2,[r2,#0x14] / blx r2
       ldr r0,[r4,#0x420] ; return 1 if it is 0 or 2
       TextureSequence::Update(this+0x188, this+0x140)
       TextureTransformer::Update(this+0x19c, this+0x140)
       add r0,r4,#0x138 / ldr r2,[r0] / add r1,r4,#0x80 / ldr r2,[r2,#0x14]
   The second dispatch is Model's slot 5, which cxxname_bridge DOES dual-fill,
   so only the first one was ever broken; it is spelled qualified anyway so the
   file reads the same way twice. */
int port_ov070_amp_render(void *selfv)
{
    char *c = (char *)selfv;
    ((ModelAnim *)(c + 0xd4))->ModelAnim::Render(0);
    int s = *(int *)(c + 0x420);
    if (s != 0 && s != 2) {
        _ZN15TextureSequence6UpdateER15ModelComponents(c + 0x188, c + 0x140);
        _ZN18TextureTransformer6UpdateER15ModelComponents(c + 0x19c, c + 0x140);
        ((Model *)(c + 0x138))->Model::Render((const Vector3 *)(c + 0x80));
    }
    return 1;
}

/* PORT_HOST_ABI: ROM-order ModelAnim slot-5 dispatch, the same ruling.
   FlameChomp::Render, ROM 0x021218c4, four instructions of body:
       add r0,r1,#0xd4 / ldr r2,[r0] / add r1,r1,#0x80 / ldr r2,[r2,#0x14]
   -- one draw at the actor's own scale Vector3, then return 1. */
int port_ov070_flamechomp_render(void *selfv)
{
    char *c = (char *)selfv;
    ((ModelAnim *)(c + 0xd4))->ModelAnim::Render((const Vector3 *)(c + 0x80));
    return 1;
}

}
