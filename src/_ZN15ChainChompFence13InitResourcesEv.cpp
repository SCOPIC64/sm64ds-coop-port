//cpp
// @symbol _ZN15ChainChompFence13InitResourcesEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "ChainChompFence.h"
extern "C" {
extern int _ZN5Model8LoadFileER13SharedFilePtr(void*);
extern int _ZN9ModelBase7SetFileEP8BMD_Fileii(void*,int,int,int);
extern int _ZN8Platform21UpdateModelPosAndRotYEv(void*);
extern int _ZN8Platform19UpdateClsnPosAndRotEv(void*);
extern int _ZN12MeshCollider8LoadFileER13SharedFilePtr(void*);
extern int _ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(void*,int,void*,int,int,void*);
}
/* These three data references belong to ov014, the fence's own overlay: the
   model BMD (0x021149c0), the KCL (0x021149b8) and the CLPS block
   (0x02114558). The decompiler had spelled them under ov021/ov022, overlays
   that never load co-resident with ov014 (the FIRE_PIRANHA overlay-collision
   pattern); config/arm9/overlays/ov014/symbols.txt proves ov014 owns all
   three addresses. Byte-identical on the ROM -- the addresses are unchanged,
   only the overlay tag on the name. The port hosts this ov014 storage under
   the plain C names (the per-symbol ov014 mount), so these are declared
   extern "C" to reach it directly; on the ROM mwccarm ignores the linkage
   for a data reference and the match is unchanged. */
extern "C" {
extern int data_ov014_021149c0[];
extern int data_ov014_021149b8[];
extern int data_ov014_02114558[];
}

int ChainChompFence::InitResources()
{
  int m = _ZN5Model8LoadFileER13SharedFilePtr(data_ov014_021149c0);
  _ZN9ModelBase7SetFileEP8BMD_Fileii((char*)((char*)this)+0xd4, m, 1, -1);
  _ZN8Platform21UpdateModelPosAndRotYEv(((char*)this));
  _ZN8Platform19UpdateClsnPosAndRotEv(((char*)this));
  int k = _ZN12MeshCollider8LoadFileER13SharedFilePtr(data_ov014_021149b8);
  _ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block((char*)((char*)this)+0x124, k, (char*)((char*)this)+0x2ec, 0x1000, unk_08e, (void*)data_ov014_02114558);
  return 1;
}
