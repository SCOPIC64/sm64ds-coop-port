//cpp
// @symbol _ZN13PoleBillboard16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "PoleBillboard.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the pole model BMD (data_ov015_0211497c) then the
   KCL (data_ov015_02114974), in load order. The generic role names G0/G1 bind,
   through the host alias table, to SignPost's model and KCL SharedFilePtrs --
   the CannonHatch disease (see that TU): each cleanup drained a sign-file
   refcount instead of its own. Byte-identical on the ROM, where the alias
   names collapse to the same addresses. Declarations come from decl_common.h. */

int PoleBillboard::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMeshCollider))->Disable();
    }
    ((SharedFilePtr *)&data_ov015_0211497c)->Release();
    ((SharedFilePtr *)&data_ov015_02114974)->Release();
    return 1;
}
