//cpp
// @symbol _ZN14MovingBarSmall16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "MovingBarSmall.h"
#include "SharedFilePtr.h"
#include "MeshColliderBase.h"
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the bar model BMD (data_ov015_02114a64) then the
   KCL (data_ov015_02114a5c), in load order. The generic role names G0/G1 bind,
   through the host alias table, to SignPost's model and KCL SharedFilePtrs --
   the CannonHatch disease (see that TU). This body is the one the id-52
   TOWER_STEP actors dispatch (their spawn installs config _ZTV14MovingBarSmall,
   the ov015 name shift), and eight of them die at level boot: eight sign-file
   refs drained, the last drain freed the sign model under nine live signs, and
   the first sign to enter the camera frustum rendered a freed BMD and faulted.
   Byte-identical on the ROM, where the alias names collapse to the same
   addresses. Declarations come from decl_common.h. */

int MovingBarSmall::CleanupResources()
{
    if (((MeshColliderBase *)((char *)&mMeshCollider))->IsEnabled()) {
        ((MeshColliderBase *)((char *)&mMeshCollider))->Disable();
    }
    ((SharedFilePtr *)&data_ov015_02114a64)->Release();
    ((SharedFilePtr *)&data_ov015_02114a5c)->Release();
    return 1;
}
