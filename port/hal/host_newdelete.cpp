// Host global operator delete: CRT-backed.
//
// The matched ROM TU src/_ZdlPv.cpp defines C++ `operator delete`, which
// MSVC mangles to the GLOBAL ??3@YAXPAX@Z. Gating that TU into the host
// build hijacked EVERY host delete (std::string, filesystem::path,
// ifstream, Lua, lobby) into Memory::defaultHeapPtr -- which is null until
// Heap::SetupRootHeap runs, so the program AV'd inside
// Heap::Deallocate+3 before main() finished ROM location.
//
// The ROM's own deallocation path stays explicit and untouched:
// Memory::operator_delete2 (hal/mem_delete2.cpp) is what game code calls.
// This file only owns the host-global delete symbols.
#include <cstdlib>

void operator delete(void *p) throw() { std::free(p); }
void operator delete(void *p, unsigned) throw() { std::free(p); }
void operator delete[](void *p) throw() { std::free(p); }
void operator delete[](void *p, unsigned) throw() { std::free(p); }
