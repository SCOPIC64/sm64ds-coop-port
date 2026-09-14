// Lane INT4, run link100 wave 9c (the fold): the model family's twelve
// Destructor1 / Destructor0 rows.
//
// WHAT THESE NAMES ARE. include/ModelBase.h's own paragraph at line 110 says
// it: mwccarm puts a class's D1 and D0 in vtable slots 0 and 1, and MSVC puts
// one destructor entry there, so every slot from index 1 on would be one word
// early. The decomp headers therefore spell TWO ORDINARY VIRTUALS in those two
// places under the host-invented names Destructor1 and Destructor0, guarded by
// _MSC_VER so the ARM side never sees them and no ROM byte moves. The header
// also says "neither name is ever called; they hold the two slots the ROM's
// table holds."
//
// WHY THEY ARE ON THE WALL ANYWAY. Holding a slot still needs a definition. A
// constructor emits its class's MSVC vftable, and a vftable references every
// slot, so src/_ZN5ModelC1Ev.cpp and its siblings ask the linker for
// ?Destructor1@Model@@UAEXXZ and ?Destructor0@Model@@UAEXXZ. Six classes, two
// slots each, twelve rows of walk_window's unresolved wall, and the FACES4
// residue files put every one of them in the "no ROM name for that class and
// method" bucket, which is true of the NAME and not of the BODY.
//
// AN /alternatename IS NOT ADMISSIBLE HERE and that is why this is a face and
// not a row in cxx_aliases.cpp. The standing test (out/HALROWS/settled.txt
// section B) is that an alias is a name bridge and never an ABI bridge. These
// two sides do not agree about the call: ?Destructor1@Model@@UAEXXZ is
// __thiscall with the receiver in ecx, and _ZN5ModelD1Ev is the ROM body under
// C linkage taking the receiver as an ordinary first argument. A face moves it;
// an alias would leave it in the wrong place.
//
// WHAT EACH BODY IS. The ROM's own function for that slot, called with the
// receiver the host ABI put in ecx. Nothing invented, nothing stubbed: every
// one of the twelve is a real cartridge body with an address in
// config/arm9/symbols.txt, and every one is already DEFINED in this link, which
// is why none of the twelve flat names is on the wall.
//
//   slot 0, Destructor1              slot 1, Destructor0
//   _ZN9ModelBaseD1Ev      0x02017120   _ZN9ModelBaseD0Ev      0x020170e8
//   _ZN5ModelD1Ev          0x02016d20   _ZN5ModelD0Ev          0x02016ce0
//   _ZN11CommonModelD1Ev   0x020161e0   _ZN11CommonModelD0Ev   0x020161b4
//   _ZN10ModelAnim2D1Ev    0x02016364   _ZN10ModelAnim2D0Ev    0x02016320
//   _ZN14BlendModelAnimD1Ev 0x02016690  _ZN14BlendModelAnimD0Ev 0x02016644
//   _ZN11ShadowModelD1Ev   0x02015ff8   _ZN11ShadowModelD0Ev   0x02015f80
//
// THE TIDY VERSION, for whoever owns hal/model_host.cpp next: these twelve
// belong beside the rest of that family's host bodies. They are here because
// no lane owns that file this wave and the house law is that a lane stays
// inside its own files, so they go in additively through port/slice_int4.txt
// and the one CMake block that reads it.

#include "ModelBase.h"
#include "Model.h"
#include "CommonModel.h"
#include "ModelAnim2.h"
#include "BlendModelAnim.h"
#include "ShadowModel.h"

extern "C" {
void _ZN9ModelBaseD1Ev(void *self);
void _ZN9ModelBaseD0Ev(void *self);
void _ZN5ModelD1Ev(void *self);
void _ZN5ModelD0Ev(void *self);
void _ZN11CommonModelD1Ev(void *self);
void _ZN11CommonModelD0Ev(void *self);
void _ZN10ModelAnim2D1Ev(void *self);
void _ZN10ModelAnim2D0Ev(void *self);
void _ZN14BlendModelAnimD1Ev(void *self);
void _ZN14BlendModelAnimD0Ev(void *self);
void _ZN11ShadowModelD1Ev(void *self);
void _ZN11ShadowModelD0Ev(void *self);
}

void ModelBase::Destructor1()      { _ZN9ModelBaseD1Ev(this); }
void ModelBase::Destructor0()      { _ZN9ModelBaseD0Ev(this); }
void Model::Destructor1()          { _ZN5ModelD1Ev(this); }
void Model::Destructor0()          { _ZN5ModelD0Ev(this); }
void CommonModel::Destructor1()    { _ZN11CommonModelD1Ev(this); }
void CommonModel::Destructor0()    { _ZN11CommonModelD0Ev(this); }
void ModelAnim2::Destructor1()     { _ZN10ModelAnim2D1Ev(this); }
void ModelAnim2::Destructor0()     { _ZN10ModelAnim2D0Ev(this); }
void BlendModelAnim::Destructor1() { _ZN14BlendModelAnimD1Ev(this); }
void BlendModelAnim::Destructor0() { _ZN14BlendModelAnimD0Ev(this); }
void ShadowModel::Destructor1()    { _ZN11ShadowModelD1Ev(this); }
void ShadowModel::Destructor0()    { _ZN11ShadowModelD0Ev(this); }

// =========================================================================
// FOUR MORE ROWS THE LEDGER COULD NOT SPELL, one hand face each
// =========================================================================
//
// Every one of these is a row facegen REFUSED for a reason about the TYPE
// SPELLING and not about the binding: a function-pointer parameter, a
// by-value template parameter, a class-spelled parameter, and a ROM free
// function whose body is a __thiscall member. In every case the member IS
// ALREADY DEFINED in this link and the flat ROM name is the one on the wall,
// so a hand face is the whole of the answer and nothing invents a body.
//
// THE ARITY OF EACH FACE WAS READ OFF ITS CALLER, never assumed. A face with
// the wrong arity is not a compile error, it smashes the stack on the first
// call, which is the standing hazard the port records against raw casts.
//
//   flat name                                        caller, and its own declaration
//   _ZN22ExpandingHeapAllocator13DeallocateAll...    port/hal/lk4_eh_dtor_seat.cpp:285
//       void *(void *thiz, void (*fn)(void*,void*,void*), void *ctx)   3 args
//   _ZN12dEnemyBase_c20KillByInvincibleChar...       include/decl_Enemy.h:24
//       void (void*, Vector3_16*, void*, int)                          4 args
//   _ZN8Particle10SysTracker8Contents6Create...      src/_ZN8Particle6System3New...c:8
//       void *(void*, unsigned, void*, const void*, void*)             5 args
//   func_ov006_020e39e0                              src/func_ov006_020e5450.c:57
//       void (char *c, int a, int b)                                   3 args
//
// THE SHADOW RULE APPLIES TO THE FIRST ONE and is why it binds the spelling it
// binds. port/hal/lk4_eh_dtor_seat.cpp declares its own private
// `struct ExpandingHeapAllocator` with `void DeallocateAll(Visitor *, u32)`,
// which decorates ?DeallocateAll@ExpandingHeapAllocator@@QAEXPAP6AXPAXPAV1@I@ZI@Z
// (V for class, and a pointer to the function pointer). That is a SHADOW. The
// owning translation unit emits
// ?DeallocateAll@ExpandingHeapAllocator@@QAEPAXP6AXPAXPAU1@I@ZI@Z, which is
// what out/HALROWS/settled.txt binds this row to and what this face calls.
// The shadow's own method keeps calling the flat name, so the chain is
// shadow method -> this face -> the real member, with no cycle: the two
// decorated names differ.

#include "ExpandingHeapAllocator.h"
#include "dEnemyBase_c.h"
#include "Particle__SysTracker.h"
#include "dScMgCurling2_c.h"

extern "C" void *_ZN22ExpandingHeapAllocator13DeallocateAllEPFvPvPS_jEj(
    void *thiz, void (*fn)(void *, void *, void *), void *ctx)
{
    return ((ExpandingHeapAllocator *)thiz)->ExpandingHeapAllocator::DeallocateAll(
        (ExpandingHeapAllocator::DeallocationFunction)fn, (u32)(size_t)ctx);
}

extern "C" void _ZN12dEnemyBase_c20KillByInvincibleCharERK10Vector3_16R6Player5Fix12IiE(
    void *thiz, Vector3_16 *vel, void *player, int unused)
{
    /* Fix12<int> is an aggregate holding the raw 20.12 bits and has no
       converting constructor (include/math/Fix12.h). The parameter is the
       one the header names unused_, and the caller's declaration in
       include/decl_Enemy.h passes it as a plain int, so the raw bits go
       through unchanged. */
    Fix12<int> unused_bits;
    unused_bits.val = unused;
    ((dEnemyBase_c *)thiz)->dEnemyBase_c::KillByInvincibleChar(
        *vel, *(Player *)player, unused_bits);
}

extern "C" void *_ZN8Particle10SysTracker8Contents6CreateEjR7Vector3PK11Vector3_16fPN5dPa_c7level_c10callback_cE(
    void *contents, unsigned int definitionID, void *position,
    const void *direction, void *callback)
{
    return (void *)(size_t)((Particle::SysTracker::Contents *)contents)
        ->Particle::SysTracker::Contents::Create(
            definitionID, *(Vector3 *)position,
            (const Vector3_16f *)direction,
            (dPa_c::level_c::callback_c *)callback);
}

extern "C" void func_ov006_020e39e0(char *c, int a, int b)
{
    ((dScMgCurling2_c *)c)->dScMgCurling2_c::SpawnValue(a, b);
}

// =========================================================================
// THE TWO STRUCT-RETURN ROWS, which are mechanical once the order is read
// =========================================================================
//
// Both members return a Vector3 BY VALUE, and both ROM bodies are in this link
// already under their flat C names. facegen refused them because it will not
// emit an indirect return: the host compiler builds its own hidden return slot
// and the ROM body takes that slot as an explicit first argument, so the face
// has to move one and not the other. Nothing here is guessed -- the argument
// ORDER is read off each body's own definition, which is the only thing that
// could be got wrong:
//
//   src/_ZN8dActor_c25OnAimedAtWithEggReturnVecEv.cpp:45
//       extern "C" void _ZN8dActor_c25OnAimedAtWithEggReturnVecEv(Vector3 *ret,
//                                                                 dActor_c *self)
//   src/_ZN9dBgCh_Lin10GetClsnPosEv.cpp:15
//       extern "C" void _ZN9dBgCh_Lin10GetClsnPosEv(Vector3 *res, dBgCh_Lin *self)
//
// Return slot first, receiver second, which is AAPCS indirect return with this
// displaced into r1, and which the first file's own header paragraph spells out
// from the ROM: r0 is written and never read, r1 supplies every field load.
// That file also explains why the ROM-side definition stays a free function
// rather than becoming a method -- mwcc does not apply the named return value
// optimisation there and the method spelling costs 0x10 bytes. That reasoning
// is about the ARM build and this face does not touch it: the definition below
// is host-only, the src file is unchanged, and no ROM byte moves.
//
// dActor_c::OnAimedAtWithEggReturnVec is slot 30 and 226 objects in this link
// reference it, every actor's vftable among them, so it is the single most
// referenced row left on the wall even though it is only one row.

#include "dActor_c.h"
#include "dBgCh_Lin.h"

extern "C" {
void _ZN8dActor_c25OnAimedAtWithEggReturnVecEv(Vector3 *ret, void *self);
void _ZN9dBgCh_Lin10GetClsnPosEv(Vector3 *ret, void *self);
}

Vector3 dActor_c::OnAimedAtWithEggReturnVec()
{
    Vector3 out;
    _ZN8dActor_c25OnAimedAtWithEggReturnVecEv(&out, this);
    return out;
}

Vector3 dBgCh_Lin::GetClsnPos()
{
    Vector3 out;
    _ZN9dBgCh_Lin10GetClsnPosEv(&out, this);
    return out;
}
