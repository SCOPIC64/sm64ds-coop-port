//cpp
// @symbol _ZN11CannonHatch16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "CannonHatch.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the hatch model BMD (data_ov002_0210e12c) then the
   KCL (data_ov002_0210e124), in load order. The decompiler spelled them by the
   generic role names G0/G1, and the host alias table binds the C++-mangled
   G0/G1 to SignPost's model and KCL SharedFilePtrs (0210e064/0210e05c) -- the
   class those aliases were recovered FOR. So every CannonHatch cleanup released
   the SIGN's files: on Whomp's Fortress the hatch dies at boot (the cannon is
   open), the sign model's refcount drained to zero under nine live signs, the
   file was freed, and the first sign to enter the camera frustum rendered a
   freed BMD and faulted. Byte-identical on the ROM, where the alias names
   collapse to the same addresses. */
extern SharedFilePtr data_ov002_0210e12c;   /* hatch model BMD */
extern SharedFilePtr data_ov002_0210e124;   /* hatch KCL */

int CannonHatch::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMeshCollider))->Disable();
    }
    data_ov002_0210e12c.Release();
    data_ov002_0210e124.Release();
    return 1;
}
