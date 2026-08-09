//cpp
// @symbol _ZN9TowerStep16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "TowerStep.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the step model BMD (data_ov015_02114a8c) then the
   KCL (data_ov015_02114a84), in load order. The generic role names G0/G1 bind,
   through the host alias table, to SignPost's model and KCL SharedFilePtrs --
   the CannonHatch disease (see that TU): each tower-step cleanup drained a
   sign-file refcount instead of its own. Byte-identical on the ROM, where the
   alias names collapse to the same addresses. Declarations come from
   decl_common.h. */

int TowerStep::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMeshCollider))->Disable();
    }
    ((SharedFilePtr *)&data_ov015_02114a8c)->Release();
    ((SharedFilePtr *)&data_ov015_02114a84)->Release();
    return 1;
}
