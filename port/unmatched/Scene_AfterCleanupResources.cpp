// HOST COPY of src/_ZN5Scene21AfterCleanupResourcesEj.cpp -- vtable slot 5 of
// every Scene-derived class, and the one place data_02092660 is cleared.
//
// The body is the matched source verbatim. The ONE difference is the storage
// class of data_02092660: the matched TU spells it
//
//     extern "C" { unsigned char data_02092660; ... }
//
// which in C++ is a DEFINITION (there are no tentative definitions), so the
// matched TU owns the byte. That byte is mutable DS state -- Scene::
// SpawnIfNecessary's "a scene has already spawned" latch, set when a scene
// spawns and cleared here when one tears down with vfSuccess 2 -- and
// port/tools/dsstate_guard refuses a hosted DS global that lands outside the
// .dsstate section, because a save state would not capture it and a restore
// would come back into a scene that the latch says is still up.
//
// The port cannot put DSSTATE_BEGIN/END around a definition inside src/: that
// tree is byte-verified against the ROM and the port does not edit it. So the
// definition moves to hal/scene_boot.cpp, inside the bracket, and this copy
// declares it extern. Nothing else changes -- same test, same constant, same
// chain call, same argument order.
//
// PORT_HOST_ABI is NOT the right tag for this and it is deliberately absent:
// no ABI is being worked around. It is a storage-placement copy, the smallest
// class of host copy in the tree.
#include "types.h"

extern "C" {
extern unsigned char data_02092660;
void _ZN9ActorBase21AfterCleanupResourcesEj(void *thiz, u32 x);
}

extern "C" void _ZN5Scene21AfterCleanupResourcesEj(void *thiz, u32 x)
{
    if (x == 2)
        data_02092660 = 0;
    _ZN9ActorBase21AfterCleanupResourcesEj(thiz, x);
}
