// Seat the whole solid-heap face: ActorBase::Virtual34/Virtual38 into slots
// 13/14 of _ZTV5Actor (data_0208e3a4), and SolidHeap's OWN sixteen-slot
// vtable, hosted here and filled with the class's matched bodies. This is
// the audited batch slice_gate16.txt's header deferred by name ("pull the
// whole Heap:: allocator face in"): the audit found every member of the
// closure either already linked or a matched TU, no host-ABI floor, no
// hardware poke, no inferred body, so the face comes in whole.
//
// THE EDGE. hal_fill_actor_base_vtable (gate 90, STAR_CAMERA) fills
// _ZTV5Actor at registry install and traps 13/14; STAR_CAMERA leaves that
// vptr installed and dispatches all 31 slots, so the table is live. This
// seat runs at the tail of port_stage_a2_seat, after the registry install,
// and replaces the two traps with the real matched bodies. Virtual34/38 are
// the per-instance heap hooks: an actor that wants a dedicated solid heap
// creates it through them (InitializeSolidHeapAsDefault -> CreateSolidHeap
// -> SolidHeap C1 installs _ZTV9SolidHeap), runs its v15 resource init
// inside it, then restores the default heap. Nothing on the castle grounds
// dispatches them today; the ROM bodies replacing return-0 traps is strictly
// more faithful when a level does.
//
// _ZTV9SolidHeap: SIXTEEN slots, the Heap shape (the layout heap_vtable.cpp
// documents for _ZTV13ExpandingHeap; both classes derive Heap and override
// the same list, and every SolidHeap V-method TU names its slot in its
// header comment). The storage lives HERE, not in heap_vtable.cpp, because
// the only TU that installs it (SolidHeap C1, matched .c) rides
// slice_gate16, so no other target ever names it.
//
// data_020a0ea8 is Memory::tmpHeapPtr, the saved-default slot
// RestoreFromTemporary reads and SetupSolidHeapAsDefault writes; its only
// referencing TUs ride this gate, so the storage lives here beside the
// table (heap_vtable.cpp hosts its sibling data_020a0ea0, the default-heap
// pointer, for every target).
//
// TWO NAME SPACES again: the .cpp bodies are MSVC methods, the .c bodies
// and several callees are flat C. The /alternatename spellings below came
// verbatim from the link errors (the heap_globals law).

#include "types.h"
#include "dsstate_seg.h"

// Flat C references -> the MSVC-method definitions the matched TUs emit
// (decorated names read from the compiled objs with dumpbin /SYMBOLS):
// the .c face TUs and Virtual34 call SetDefault/Destroy/ResizeToFit by
// their flat Itanium names while the defining TUs are real Heap methods.
//
// SetDefault USED TO BE THE FIRST LINE OF THIS BLOCK AND IT WAS WRONG. An
// /alternatename is a NAME bridge and never an ABI bridge, and this class of
// directive (flat C LHS, __thiscall (QAE) RHS) silently drops the receiver:
// the C caller pushes it, the method reads ECX. That is hal/method_faces.cpp's
// failure class 3 spelled as a linker directive. It cost a measured fault. The
// receiver-bridging face further down this file replaces it, and the evidence
// is in the header there.
//
// AND THE TWO SURVIVORS BELOW ARE THE SAME SHAPE, so do not read this block as
// cleaned. They are left standing because this lane had a reproduction for
// SetDefault and has none for them, not because they were checked and cleared:
//   Destroy      src/func_0201a428.c:9 pushes data_0209d524, and both names
//                are in the map, so the alias is live. Heap::Destroy
//                dereferences the receiver (a virtual call, then three stores),
//                so this one faults rather than publishing a wrong word. Its
//                only caller is func_ov007_020cc4c0, and scene 1 is the
//                battery's one BLOCKED skip, which is why nothing has hit it.
//                THE _Destroy VENEER IS WORSE THAN LUCK, and an earlier draft
//                of this note got it wrong by saying the receiver survives.
//                Read from the disassembly instead: _ZN4Heap8_DestroyEv is a
//                bare five-byte jmp (0049FDC0: E9 .. jmp 0049FDD0) that sets
//                no ECX and reads no stack slot, and Heap::Destroy opens
//                `mov esi,ecx` then `call [eax+8]` through it, so the receiver
//                comes from ECX and is dereferenced at once. Its callers are
//                ActorBase::Virtual34 at :50, :78, :100, :106 and :111, a
//                thiscall member: at the jump ECX still holds the ActorBase
//                pointer from the virtual call before it (`mov ecx,ebx`), and
//                the heap the caller pushed is DISCARDED. So on the host the
//                receiver does not survive at all. It survives on ARM only
//                because bx ip preserves r0, which is a register convention
//                and not a property of the C, and a C rewrite of the veneer
//                destroys it. Broken on the host, undriven today (slot 13
//                never dispatches on any battery path), ARM-correct by
//                register convention.
//                AND THE FIX CONSEQUENCE, because it constrains whoever takes
//                this: a plain receiver-taking face for Destroy would fix the
//                func_0201a428 caller and BREAK this veneer, which reaches the
//                same name with the receiver only in ECX. The veneer TU has to
//                change in the same commit as any Destroy fix.
//   ResizeToFit  src/_ZN9ActorBase9Virtual34Ejj.cpp pushes the heap at :57,
//                :116 and :123, and Virtual34 is slot 13 of _ZTV5Actor, which
//                THIS FILE seats. Nothing dispatches it on the castle grounds
//                today (see the header), so it is latent the same way
//                SetDefault was: linked, wrong, and off every path the battery
//                covers.
// The SolidHeapAllocator pair further down runs the other way (thiscall LHS,
// flat C RHS whose first stack argument is the receiver), so both arguments
// shift by one; those reach through slots 3 and 8 of the table seated below.
// None of that is fixed here.
//
// THE SIBLING-LANE RULING, from the review of this lane, so the file carries
// it rather than a status note nobody reads:
//   * A DRIVE IS NOT REQUIRED to settle any of the three families. Probe and
//     disassembly evidence is decisive on its own, and the precedent is the
//     seven bytes of this very body: ?SetDefault@Heap@@QAEHXZ compiles to
//     `mov eax,[data_020a0ea0]; mov [data_020a0ea0],ecx; ret`, which reads ECX
//     and never touches a stack slot. That settles the ABI question without
//     reaching the code at run time, and the same reading settles the rest.
//   * SolidHeapAllocator GOES FIRST. It is the only SILENT corrupter of the
//     three: size lands in the receiver slot and align lands in size, so the
//     call returns a plausible pointer out of the wrong arena instead of
//     faulting, and it is reached through slots 3 and 8 of the table seated
//     below. A wrong pointer that looks right outranks a crash.
//   * Destroy IS SEQUENCING-GATED and must land BEFORE the scene 1 unblock
//     lane, by the same rule stage_lifecycle_map.txt section 11g states for
//     the unlinked SetDefault carriers: unblocking scene 1 is what puts
//     func_ov007_020cc4c0 on a path, and a lane that does it first inherits
//     the fault with none of this evidence.
#pragma comment(linker, "/alternatename:__ZN4Heap7DestroyEv=?Destroy@Heap@@QAEXXZ")
#pragma comment(linker, "/alternatename:__ZN4Heap11ResizeToFitEv=?ResizeToFit@Heap@@QAEIXZ")
// G is the ROM's shorthand for the default-heap pointer (decl_common.h
// `extern int G`), the same 0x020a0ea0 word heap_vtable.cpp hosts as
// data_020a0ea0 / Memory::defaultHeapPtr. One storage, one more name.
#pragma comment(linker, "/alternatename:_G=_data_020a0ea0")
// The remaining cross-namespace edges, spellings verbatim from the link
// errors: Virtual38 and the SolidHeap V-methods reference these as MSVC
// statics/methods while the definitions are flat C (or, for Rescue, a
// method whose matched TU returns int where the hostgen caller spelled
// void; same __thiscall, r0/EAX ignored, the ROM's own shape).
#pragma comment(linker, "/alternatename:?InitializeSolidHeapAsDefault@Heap@@SAPAU1@IPAU1@H@Z=__ZN4Heap28InitializeSolidHeapAsDefaultEjPS_i")
#pragma comment(linker, "/alternatename:?Allocate@Memory@@SAPAXIHPAUHeap@@@Z=__ZN6Memory8AllocateEjiP4Heap")
#pragma comment(linker, "/alternatename:?RestoreFromTemporary@Heap@@SAXXZ=__ZN4Heap20RestoreFromTemporaryEv")
#pragma comment(linker, "/alternatename:?_Destroy@Heap@@QAEXXZ=__ZN4Heap8_DestroyEv")
#pragma comment(linker, "/alternatename:?Rescue@Heap@@QAEXXZ=?Rescue@Heap@@QAEHXZ")
#pragma comment(linker, "/alternatename:?Allocate@SolidHeapAllocator@@QAEPAXIH@Z=__ZN18SolidHeapAllocator8AllocateEji")
#pragma comment(linker, "/alternatename:?Reallocate@SolidHeapAllocator@@QAEPAXPAXI@Z=__ZN18SolidHeapAllocator10ReallocateEPvj")

/* the matched MSVC-method bodies, dispatched qualified through local
   shadows; the signatures are copied from each TU's own declaration */
class SolidHeap
{
public:
    void *VAllocate(u32 size, u32 align);           /* slot 3 */
    void VDeallocate(void *p);                      /* slot 4 */
    void VDeallocateAll();                          /* slot 5 */
    bool VIntact();                                 /* slot 6 */
    void VRescue();                                 /* slot 7 */
    void *VReallocate(void *p, u32 size);           /* slot 8 */
    int VSizeof(void *p);                           /* slot 9 */
    unsigned VMaxAllocationUnitSize();              /* slot 10 */
    unsigned VMaxAllocatableSize();                 /* slot 11 */
    unsigned VMemoryLeft();                         /* slot 12 */
    u32 VSetNodeID(unsigned id);                    /* slot 13 */
    u32 VGetNodeID();                               /* slot 14 */
};

/* the matched Virtual34/38 are real ActorBase methods; this local decl
   produces the same ?Virtual34@ActorBase@@QAEHII@Z the TUs define */
struct ActorBase
{
    int Virtual34(u32 a, u32 b);
    int Virtual38(u32 a, u32 b);
};

/* Declared locally rather than by including Heap.h, for the reason
   hal/stage_slot0.cpp gives beside its own Stage decl: a wrong signature here
   is an LNK2019 on a name that does not exist, never a quiet call to a
   sibling. `int` return and no arguments is what src/_ZN4Heap10SetDefaultEv.cpp
   defines and what ?SetDefault@Heap@@QAEHXZ encodes (QAE = public __thiscall,
   H = int, XZ = no arguments). */
struct Heap
{
    int SetDefault();
};

extern "C" {

extern void *data_0208e3a4[31];       /* _ZTV5Actor, hal/actor_vtables.cpp */

void *_ZTV9SolidHeap[16];             /* installed by SolidHeap C1 */
DSSTATE_BEGIN
void *data_020a0ea8;                  /* Memory::tmpHeapPtr */
DSSTATE_END

/* the flat .c bodies */
void *_ZN9SolidHeapD1Ev(void *self);              /* slot 0 */
void *_ZN9SolidHeapD0Ev(void *self);              /* slot 1 */
void _ZN9SolidHeap8VDestroyEv(void *self);        /* slot 2 */
u32 _ZN9SolidHeap12VResizeToFitEv(void *self);    /* slot 15 */

}

static void __fastcall sh_d1(void *s, void *)       { _ZN9SolidHeapD1Ev(s); }
static void __fastcall sh_d0(void *s, void *)       { _ZN9SolidHeapD0Ev(s); }
static void __fastcall sh_vdestroy(void *s, void *) { _ZN9SolidHeap8VDestroyEv(s); }
static void *__fastcall sh_alloc(void *s, void *, u32 size, u32 align)
{ return ((SolidHeap *)s)->VAllocate(size, align); }
static void __fastcall sh_dealloc(void *s, void *, void *p)
{ ((SolidHeap *)s)->VDeallocate(p); }
static void __fastcall sh_dealloc_all(void *s, void *)
{ ((SolidHeap *)s)->VDeallocateAll(); }
static int __fastcall sh_intact(void *s, void *)
{ return ((SolidHeap *)s)->VIntact(); }   /* int widens bool the ARM r0 way */
static void __fastcall sh_rescue(void *s, void *)
{ ((SolidHeap *)s)->VRescue(); }
static void *__fastcall sh_realloc(void *s, void *, void *p, u32 size)
{ return ((SolidHeap *)s)->VReallocate(p, size); }
static int __fastcall sh_sizeof(void *s, void *, void *p)
{ return ((SolidHeap *)s)->VSizeof(p); }
static u32 __fastcall sh_maxunit(void *s, void *)
{ return ((SolidHeap *)s)->VMaxAllocationUnitSize(); }
static u32 __fastcall sh_maxalloc(void *s, void *)
{ return ((SolidHeap *)s)->VMaxAllocatableSize(); }
static u32 __fastcall sh_memleft(void *s, void *)
{ return ((SolidHeap *)s)->VMemoryLeft(); }
static void __fastcall sh_setnodeid(void *s, void *, u32 id)
{ ((SolidHeap *)s)->VSetNodeID(id); }
static u32 __fastcall sh_getnodeid(void *s, void *)
{ return ((SolidHeap *)s)->VGetNodeID(); }
static u32 __fastcall sh_resizetofit(void *s, void *)
{ return _ZN9SolidHeap12VResizeToFitEv(s); }

static int __fastcall ab_v34(void *s, void *, u32 a, u32 b)
{ return ((ActorBase *)s)->ActorBase::Virtual34(a, b); }
static int __fastcall ab_v38(void *s, void *, u32 a, u32 b)
{ return ((ActorBase *)s)->ActorBase::Virtual38(a, b); }

/* THE RECEIVER-BRIDGING FACE FOR Heap::SetDefault, replacing the
   /alternatename this file used to carry. Same shape as hal/stage_slot0.cpp's
   LoadFog face: the C name takes the receiver as its first stack argument, the
   qualified call moves it into ECX.

   WHY IT EXISTS, measured rather than argued. The body is one line
   (src/_ZN4Heap10SetDefaultEv.cpp: `int old = G; G = ((int)this); return old;`)
   so a dropped receiver does not fault at the call. It PUBLISHES a register
   nobody set into the default-heap pointer (G is data_020a0ea0, aliased above)
   and the boot carries on with it. Through the old directive, with a probe
   passing a known receiver:

     receiver 001aff1c pushed, data_020a0ea0 became e35ebbce   (plain call)
     receiver 001aff1c pushed, data_020a0ea0 became deadbeef   (ECX seeded
                                                                 DEADBEEF)

   The second line is the whole bug in one reading: the value published is
   whatever ECX held, and the argument the caller pushed is never looked at.
   Through this face both lines read 001aff1c.

   AND THE SAME READING OFF A REAL DRIVE, not only a probe. SM64DS_SLOT0_ROM=3
   (hal/stage_slot0.cpp part 4) puts Stage::InitResources' own pair at :234 and
   :236 through this name on level 1. Before, the run faulted in Heap::Allocate
   +0xd accessing 00000005 under SharedFilePtr::Load -> fs_hand_out ->
   Memory::Allocate, and that 5 IS data_020a0ea0 read back, because
   src/_ZN6Memory8AllocateEjiP4Heap.c substitutes the default-heap pointer for
   a null heap argument. After, the pair reads

     SetDefault(30000060): data_020a0ea0 30000000 -> 30000060
     SetDefault(30000000): data_020a0ea0 30000060 -> 30000000

   which is the game heap the boot reports at 30000060, saved and restored in
   balance. The run then faults further in, at Model::UpdateFileOffsets+0x25
   accessing 600b8ca8. That is SD0's wall 4, which that lane recorded as NOT
   cleanly attributable because the probe runs the ROM body on the host boot.
   So this face resolves wall 3 into wall 4 and claims nothing beyond that.
   Walls 1 and 2 are untouched and the swap stays declined.

   SIX TUs SPELL THE FLAT C NAME AND PUSH THE RECEIVER, three of them linked
   today, so this is a live bug and not one a future seat would introduce:
     src/_ZN4Heap20RestoreFromTemporaryEv.c:11
     src/_ZN4Heap23SetupSolidHeapAsDefaultEjPS_i.c:18
     src/func_ov007_020cc2cc.c:49 and :53
     src/_ZN5Stage13InitResourcesEv.cpp:234 and :236   (linked, env-gated)
     src/func_02034fbc.c:22, :27, :31, :33             (not compiled today)
     src/func_ov075_02118bf8.c:9 and :18               (not compiled today)
   The last two are the sequencing rule port/stage_lifecycle_map.txt section
   11g carries: a slice that enrols either of them without this face in front
   inherits the fault with none of the evidence.

   NOT A SHADOWING DEFINITION ON TOP OF THE DIRECTIVE. The directive is gone.
   Leaving both would strongly define the LHS, which is exactly the arrival
   shape port/tools/alternatename_guard.py fails the build on.

   AND DELETING IT EXPOSED A SCOPE BUG IN THAT GUARD, recorded here for the
   lane that fixes it. The guard reads a bare /alternatename line in ANY
   port/*.txt as a linker input, so section 11d's PROSE QUOTATION of this
   directive counted as one, and the build failed by name the moment this face
   defined the LHS. The quote is annotated "(DELETED by lane LK4)" for that
   reason, and the annotation is load-bearing: remove it and the build stops.
   The guard's own docstring says of txt directives "None exist today", which
   is false both at this lane's base and at its tip. THREE such lines exist:
   stage_lifecycle_map.txt:995 and :1084 and ov007_seat.txt:444. The other two
   pass only because their aliases still fire, so each is armed for whoever
   correctly deletes one. */
extern "C" int _ZN4Heap10SetDefaultEv(void *thiz)
{ return ((Heap *)thiz)->Heap::SetDefault(); }

extern "C" void hal_seat_solidheap(void)
{
    void **vt = _ZTV9SolidHeap;
    vt[0]  = (void *)sh_d1;
    vt[1]  = (void *)sh_d0;
    vt[2]  = (void *)sh_vdestroy;
    vt[3]  = (void *)sh_alloc;
    vt[4]  = (void *)sh_dealloc;
    vt[5]  = (void *)sh_dealloc_all;
    vt[6]  = (void *)sh_intact;
    vt[7]  = (void *)sh_rescue;
    vt[8]  = (void *)sh_realloc;
    vt[9]  = (void *)sh_sizeof;
    vt[10] = (void *)sh_maxunit;
    vt[11] = (void *)sh_maxalloc;
    vt[12] = (void *)sh_memleft;
    vt[13] = (void *)sh_setnodeid;
    vt[14] = (void *)sh_getnodeid;
    vt[15] = (void *)sh_resizetofit;

    /* the per-instance heap hooks, replacing star_trap13/14 after the
       registry install's fill has run */
    data_0208e3a4[13] = (void *)ab_v34;
    data_0208e3a4[14] = (void *)ab_v38;
}
