//cpp
// @symbol _ZN19FirePiranhaPlantBig16CleanupResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "FirePiranhaPlantBig.h"
#include "SharedFilePtr.h"
extern "C" {
void UnloadBlueCoinModel(void*);
extern int data_ov084_02130dfc;
extern int data_ov002_0210da38;
/* The six animation SharedFilePtrs this class RELEASES are the same six its
   own InitResources LOADS -- data_ov084_021302f4[0..5], a 0x18-byte table in
   FirePiranhaPlantBig's OWN overlay (ov084). The symbol was recovered as
   data_ov085_021302f4, ov085's LakituBro state PMF table, which sits at the
   identical VA because ov084 and ov085 overlap the same overlay window and are
   never co-resident on the DS. Byte-identical on the ROM; on the host, where
   both overlays are mapped, ov085 was the wrong bytes and its non-pointer PMF
   words faulted SharedFilePtr::Release. */
extern SharedFilePtr *data_ov084_021302f4[];
}

int FirePiranhaPlantBig::CleanupResources()
{
  ((SharedFilePtr *)(&data_ov084_02130dfc))->Release();
  for (int i = 0; i < 6; i++) {
    data_ov084_021302f4[i]->Release();
  }
  ((SharedFilePtr *)(&data_ov002_0210da38))->Release();
  UnloadBlueCoinModel(((void *)this));
  return 1;
}
