// @symbol _ZN14BlueCoinSwitch16CleanupResourcesEv
// recovered name: daObjBC_Switch_c_CleanupResources
/* recovered: renamed to Class_Method, declarations from a shared header */
#include "decl_common.h"
/* recovered: renamed to Class_Method */
/* daObjBC_Switch_c::CleanupResources - recovered from vtable slot identity */
extern void _ZN13SharedFilePtr7ReleaseEv(void *);
/* The two SharedFilePtrs this releases are the class's OWN files, the same two
   its InitResources loads: the model BMD (data_ov002_02110acc) then the KCL
   (data_ov002_02110ac4), in load order. The decompiler spelled them by the
   generic role names G0/G1; on the host, _G0 is aliased to data_020a0eac
   (Memory::gameHeapPtr) for the Platform-D0 family, so Release(G0) wrote into
   the gameHeapPtr pointer itself (its +2 byte), walking it 0x10000 down and
   faulting the next heap query. Spelled by their real symbols the release hits
   the right SharedFilePtrs; byte-identical on the ROM, where both live at the
   same overlapping addresses the alias table collapses. */
extern char data_ov002_02110acc;   /* model BMD SharedFilePtr */
extern char data_ov002_02110ac4;   /* KCL SharedFilePtr */
int _ZN14BlueCoinSwitch16CleanupResourcesEv(void *t)
{
    if (_ZN16MeshColliderBase9IsEnabledEv((char *)t + 0x124)) {
        _ZN16MeshColliderBase7DisableEv((char *)t + 0x124);
    }
    _ZN13SharedFilePtr7ReleaseEv(&data_ov002_02110acc);
    _ZN13SharedFilePtr7ReleaseEv(&data_ov002_02110ac4);
    return 1;
}
