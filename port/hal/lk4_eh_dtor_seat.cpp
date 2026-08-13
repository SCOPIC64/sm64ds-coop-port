// Seat ExpandingHeap's dtor chain and its two remaining allocator forwarders,
// closing the class: slots 0/1/2/5/12 of _ZTV13ExpandingHeap were the last
// traps while src/ carried the matched bodies. The audited closure
// (Heap::D2, HeapAllocator::Destroy/Remove, ExpandingHeapAllocator::
// DeallocateAll/InvokeDeallocate) is enrolled alongside; every deeper callee
// (Heap::Deallocate, the NestedHeapIterator pair, Memory::operator_delete2,
// data_02099d90) was already linked.
//
//   [ 0] ~ExpandingHeap D1   0x0203c9f0  flat .c, ecx->arg adapter
//   [ 1] ~ExpandingHeap D0   0x0203c9c0  flat .c, ecx->arg adapter
//   [ 2] VDestroy            0x0203c72c  flat .c, ecx->arg adapter
//   [ 5] VDeallocateAll      0x0203c4b0  MSVC method, qualified dispatch
//   [12] VMemoryLeft         0x0203c5ac  MSVC method, qualified dispatch
//
// TWO NAME SPACES, the batch-2 mechanism: VDeallocateAll and VMemoryLeft are
// real MSVC methods whose callee references are MSVC-mangled, while the
// allocator bodies they reach are flat-C definitions
// (_ZN22ExpandingHeapAllocator13DeallocateAllEPPFvPvPS_jEj,
// _ZN22ExpandingHeapAllocator16InvokeDeallocateEPvPS_j, and the
// already-linked _ZN22ExpandingHeapAllocator10MemoryLeftEv). The
// /alternatename bridges below carry the decorated spellings verbatim from
// the link errors, the heap_globals law (the linker is the authority on
// decoration).
//
// The slots are written at boot (level_boot's port_stage_a2_seat) for the
// walk_window family only, the lk1_eh_vmax_seat treatment: the D0/D1 closure
// needs Memory::operator_delete2, which rides slice_gate16, and the minimal
// root-heap targets do not link it. Those targets keep the traps, which they
// never dispatch (nothing there destroys the root heap).
//
// LATER (run linkw, lane l2): the same file now also closes the BASE class,
// which is the other half of the same object. _ZTV4Heap (data_02099d90) gets
// its two matched destructors and the pure-virtual trap for the fourteen
// slots the ROM leaves null, and the flat face gate 4b calls for
// Heap::Allocate(unsigned) now forwards into that method's matched body
// instead of a host copy. Both blocks carry their own evidence below.

#include <stdio.h>
#include <stdlib.h>
#include "types.h"

// Flat C name -> the MSVC-mangled reference the matched method TUs emit.
#pragma comment(linker, "/alternatename:?DeallocateAll@ExpandingHeapAllocator@@QAEXPAP6AXPAXPAV1@I@ZI@Z=__ZN22ExpandingHeapAllocator13DeallocateAllEPPFvPvPS_jEj")
#pragma comment(linker, "/alternatename:?InvokeDeallocate@ExpandingHeapAllocator@@SAXPAXPAV1@I@Z=__ZN22ExpandingHeapAllocator16InvokeDeallocateEPvPS_j")
#pragma comment(linker, "/alternatename:?MemoryLeft@ExpandingHeapAllocator@@QAEIXZ=__ZN22ExpandingHeapAllocator10MemoryLeftEv")
// ...and the reverse direction once: HeapAllocator::Destroy is flat C and
// calls Remove by its flat name, while the matched Remove TU is a real MSVC
// method against include/HeapAllocator.h.
#pragma comment(linker, "/alternatename:__ZN13HeapAllocator6RemoveEv=?Remove@HeapAllocator@@QAEXXZ")

/* the matched methods, dispatched qualified through a local shadow (the
   heap_vtable slot_intact mechanism; both sides are MSVC C++ on the same
   class name, so the references resolve with no alias machinery) */
class ExpandingHeap
{
public:
    void VDeallocateAll();
    u32 VMemoryLeft();
};

/* ---- the BASE class, closed the same way --------------------------------
 *
 * _ZTV4Heap is data_02099d90; hal/heap_vtable.cpp hosts it and its comment
 * there carries the reloc evidence for the shape. Two slots, both matched:
 *
 *   [ 0] ~Heap D1  0x0203ca44  src/_ZN4HeapD1Ev.c  flat .c, ecx->arg adapter
 *   [ 1] ~Heap D0  0x0203ca20  src/_ZN4HeapD0Ev.c  flat .c, ecx->arg adapter
 *
 * and fourteen the ROM leaves null, because Heap declares the rest of the
 * list pure virtual. A null slot is nothing the host can usefully reproduce,
 * so those get one shared trap -- which is exactly what __cxa_pure_virtual
 * is, and it keeps the heap_vtable rule that a slot with no body says so
 * loudly instead of jumping to zero.
 *
 * Nothing dispatches through this table today. Heap is abstract, and
 * Heap::C1 (src/_ZN4HeapC1EPvjP4Heap.c) installs the vptr only for the window
 * between the base constructor and the derived one overwriting it -- the same
 * reading hal/lk2_platform_dtor_seat.cpp records for the Platform base table,
 * and it is why the pair could sit unlinked this long. Seating it is the
 * reference edge that pulls the two matched bodies in, and it makes the
 * hosted table say what the ROM's table says. D1's closure is data_02099d90
 * itself; D0's adds Memory::operator_delete2, already linked.
 */

/* Heap::Allocate(unsigned) at 0x0203c28c, src/_ZN4Heap8AllocateEj.cpp, now the
 * body behind the flat name func_0203cc0c (gate 4b) calls. The matched TU is
 * an MSVC method while the caller spells a __cdecl free function with `this`
 * on the stack, so the face below is a converting forwarder, not an alias --
 * the heap_globals law about which edges may be /alternatename'd and which
 * may not. hal/heap_vtable.cpp keeps the old host body as an alternatename
 * fallback for the ten targets that carry gate 4b without this slice.
 *
 * The one alias that IS safe here: the matched body calls Allocate(size, 4),
 * which it declares returning void*, while the matched two-argument TU
 * (src/_ZN4Heap8AllocateEji.cpp, written against include/Heap.h) defines it
 * returning int. Same __thiscall, same arguments, EAX either way -- the same
 * void/int return bridge hal/lk4_solidheap_seat.cpp takes for Heap::Rescue. */
#pragma comment(linker, "/alternatename:?Allocate@Heap@@QAEPAXIH@Z=?Allocate@Heap@@QAEHIH@Z")

class Heap
{
public:
    void *Allocate(u32 size);
};

extern "C" {

extern void *_ZTV13ExpandingHeap[16];   /* storage in hal/heap_vtable.cpp */
extern void *data_02099d90[16];         /* _ZTV4Heap, same storage file */

void *_ZN13ExpandingHeapD1Ev(void *self);   /* slot 0 */
void *_ZN13ExpandingHeapD0Ev(void *self);   /* slot 1 */
void _ZN13ExpandingHeap8VDestroyEv(void *self);   /* slot 2 */

void _ZN4HeapD1Ev(void *self);              /* _ZTV4Heap slot 0 */
void *_ZN4HeapD0Ev(void *self);             /* _ZTV4Heap slot 1 */

/* the flat __cdecl face gate 4b's func_0203cc0c calls */
void *_ZN4Heap8AllocateEj(void *self, u32 size)
{ return ((Heap *)self)->Allocate(size); }

}

static void __fastcall eh_d1(void *s, void *)      { _ZN13ExpandingHeapD1Ev(s); }
static void __fastcall eh_d0(void *s, void *)      { _ZN13ExpandingHeapD0Ev(s); }
static void __fastcall eh_vdestroy(void *s, void *){ _ZN13ExpandingHeap8VDestroyEv(s); }
static void __fastcall eh_vdeallall(void *s, void *)
{ ((ExpandingHeap *)s)->VDeallocateAll(); }
static u32 __fastcall eh_vmemleft(void *s, void *)
{ return ((ExpandingHeap *)s)->VMemoryLeft(); }

static void __fastcall heap_d1(void *s, void *) { _ZN4HeapD1Ev(s); }
static void __fastcall heap_d0(void *s, void *) { _ZN4HeapD0Ev(s); }

/* the fourteen slots the ROM leaves null: Heap's pure virtuals */
static void __fastcall heap_pure_virtual(void *, void *)
{
    fprintf(stderr, "FATAL: _ZTV4Heap pure-virtual slot dispatched -- the base "
                    "table is live and it should not be (see "
                    "hal/lk4_eh_dtor_seat.cpp)\n");
    abort();
}

extern "C" void hal_seat_expandingheap_dtors(void)
{
    _ZTV13ExpandingHeap[0]  = (void *)eh_d1;
    _ZTV13ExpandingHeap[1]  = (void *)eh_d0;
    _ZTV13ExpandingHeap[2]  = (void *)eh_vdestroy;
    _ZTV13ExpandingHeap[5]  = (void *)eh_vdeallall;
    _ZTV13ExpandingHeap[12] = (void *)eh_vmemleft;

    data_02099d90[0] = (void *)heap_d1;
    data_02099d90[1] = (void *)heap_d0;
    for (int i = 2; i < 16; ++i)
        data_02099d90[i] = (void *)heap_pure_virtual;
}
