#include "ExpandingHeapAllocator.h"
#include <new>

// Preserve the flat constructor's receiver return without depending on EAX
// after a SysV constructor, whose C++ return type is void.
extern "C" ExpandingHeapAllocator *port_native_expanding_heap_ctor(
    ExpandingHeapAllocator *self, void *heap_end, unsigned flags) {
    return ::new (static_cast<void *>(self)) ExpandingHeapAllocator(heap_end, flags);
}
