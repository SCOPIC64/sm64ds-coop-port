//cpp
// @symbol _ZN15ChainChompFence16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "ChainChompFence.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the fence model BMD (data_ov014_021149c0) then the
   KCL (data_ov014_021149b8), in load order. The generic role names G0/G1 bind,
   through the host alias table, to SignPost's model and KCL SharedFilePtrs --
   the CannonHatch disease (127b4e1d8): each cleanup drained a sign-file
   refcount instead of its own. This is the class /OPT:ICF folded the four
   platform cleanups onto, so its name is the one their backtraces all showed.
   Byte-identical on the ROM, where the alias names collapse to the same
   addresses. The port hosts this ov014 storage under the plain C names (the
   per-symbol ov014 mount), so these are declared extern "C" to reach it
   directly -- InitResources spells the same two objects as int[], the "one
   definition, several names" shape the bridges already carry. */
extern "C" {
extern SharedFilePtr data_ov014_021149c0;   /* fence model BMD */
extern SharedFilePtr data_ov014_021149b8;   /* fence KCL */
}

int ChainChompFence::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMovingMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMovingMeshCollider))->Disable();
    }
    ((SharedFilePtr *)&data_ov014_021149c0)->Release();
    ((SharedFilePtr *)&data_ov014_021149b8)->Release();
    return 1;
}
