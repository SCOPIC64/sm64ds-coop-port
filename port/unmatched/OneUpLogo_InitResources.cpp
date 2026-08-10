/* HOST COPY of src/_ZN9OneUpLogo13InitResourcesEv.cpp -- the calling-
 * convention seam for TextureSequence::Prepare (the SHORT-1 argsweep row).
 *
 * WHY A HOST COPY (the calling-convention seam):
 *
 * The Prepare symbol's ROM body is a 0xc-byte tail-call veneer (ldr
 * r12,[pc]; bx r12 -> func_02046d50) that does not touch r0-r2 at all, so
 * whatever the caller has loaded in those registers rides straight through
 * to func_02046d50 -- and func_02046d50's own matched body
 * (src/func_02046d50.c) reads exactly TWO of them: r0 = the BMD model
 * file whose name table gets SEARCHED, r1 = the BTP animation file whose
 * name table gets WALKED (its `struct Tbl` field layout matches BTP_File,
 * not BMD_File -- the gate-191 MotherPenguin derivation, confirmed again
 * for daBgSnwmn_c in gate 193). The matched src here calls the veneer as
 * a two-argument free function --
 *
 *     _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(
 *         (void*)data_ov002_02110aa4.file, (void*)data_ov002_02110a9c.file);
 *
 * -- decl=2/push=2, consumed=3 (the argsweep SHORT-1 row). On the DS this
 * is byte-identical AND semantically right: model lands in r0, animFile in
 * r1, and the third register the veneer nominally carries is never read.
 * On the host, the real bridge (hal/player_bridges.cpp) is a genuine
 * three-argument function (self, bmd, btp) that forwards (self=r0-slot,
 * bmd=r1-slot) into func_02046d50 -- so a two-argument call shifts
 * everything one slot and the walked table reads garbage.
 *
 * THE FIX (the MotherPenguin/Snowman shape, NOT a this-first call): pass
 * the DS-effective register values explicitly --
 * (model.file, anim.file, anim.file). The third argument is padding for
 * the bridge's arity only; func_02046d50 never reads it. An earlier
 * revision of this file passed (this+0x124, model, anim) on a "missing
 * this register" reading -- that predated the gate-191 register-level
 * derivation and was WRONG: it handed the TS sub-object to the slot
 * func_02046d50 searches as the model's name table. This host copy is the
 * matched source line for line otherwise; only the Prepare call differs.
 *
 * src/_ZN9OneUpLogo13InitResourcesEv.cpp is dropped from slice_gate190.txt
 * in favour of this file; the byte-locked source is unchanged.
 */
#include "OneUpLogo.h"

extern "C" {
struct SharedFilePtr { int a, file; };
extern SharedFilePtr data_ov002_02110a9c;
extern SharedFilePtr data_ov002_02110aa4;
extern void _ZN15TextureSequence8LoadFileER13SharedFilePtr(SharedFilePtr&);
extern void* _ZN5Model8LoadFileER13SharedFilePtr(SharedFilePtr&);
extern int _ZN9ModelBase7SetFileEP8BMD_Fileii(void* self, void* f, int a, int b);
/* the real three-argument bridge (hal/player_bridges.cpp): self, model, animFile */
extern void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void* self, void* bmd, void* btp);
extern void _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(void* self, void* btp, int a, int c, unsigned int n);
}

int OneUpLogo::InitResources()
{
  unsigned short n;
  {
    unsigned int v = mParam;
    n = (unsigned short)(v > 8 ? 7 : (v - 1));
  }
  _ZN15TextureSequence8LoadFileER13SharedFilePtr(data_ov002_02110a9c);
  if (_ZN9ModelBase7SetFileEP8BMD_Fileii(((char*)this)+0xd4, _ZN5Model8LoadFileER13SharedFilePtr(data_ov002_02110aa4), 1, -1) == 0)
    return 0;
  _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File((void*)data_ov002_02110aa4.file, (void*)data_ov002_02110a9c.file, (void*)data_ov002_02110a9c.file);
  _ZN15TextureSequence7SetFileER8BTP_Filei5Fix12IiEj(((char*)this)+0x124, (void*)data_ov002_02110a9c.file, 0x40000000, 0, n);
  unk_14e = 0;
  unk_13c = mPosX;
  unk_140 = mPosY;
  unk_144 = mPosZ;
  unk_0a8 = 0x14000;
  unk_09c = -0x2000;
  unk_0a0 = -0x32000;
  unk_14c = 0;
  unk_138 = 0;
  unk_148 = 0;
  return 1;
}
