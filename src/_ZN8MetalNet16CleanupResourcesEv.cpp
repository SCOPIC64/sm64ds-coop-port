//cpp
// @symbol _ZN8MetalNet16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "MetalNet.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the metal-net model BMD (data_ov009_02113e90) then
   the KCL (data_ov009_02113e88), in load order. The decompiler spelled them by
   the generic role names G0/G1, and the host alias table binds the C++-mangled
   G0/G1 to SignPost's model and KCL SharedFilePtrs (0210e064/0210e05c) -- the
   class those aliases were recovered FOR (see cxx_aliases.cpp). So every
   MetalNet cleanup released the SIGN's files instead of its own. METAL_NET
   (class 339) is an ov009 actor that does not spawn on the castle grounds, so
   the collision cost nothing there, but it is the CannonHatch disease
   (127b4e1d8) all the same. Byte-identical on the ROM, where the alias names
   collapse to the same addresses. Spelled the same way its InitResources
   declares them, inside extern "C" (data_ov009_* are not in decl_common.h,
   and the port hosts this ov009 storage under the plain C name). */
extern "C" {
extern SharedFilePtr data_ov009_02113e90;   /* metal-net model BMD */
extern SharedFilePtr data_ov009_02113e88;   /* metal-net KCL */
}

int MetalNet::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMovingMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMovingMeshCollider))->Disable();
    }
    ((SharedFilePtr *)&data_ov009_02113e90)->Release();
    ((SharedFilePtr *)&data_ov009_02113e88)->Release();
    return 1;
}
