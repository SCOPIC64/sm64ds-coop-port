/* HOST COPY of src/_ZN9OneUpLogo13InitResourcesEv.cpp -- the calling-
 * convention seam for TextureSequence::Prepare (the SHORT-1 argsweep row).
 *
 * WHY A HOST COPY (the calling-convention seam):
 *
 * TextureSequence::Prepare is a real non-static C++ method,
 * Prepare(BMD_File &model, BTP_File &animFile), which at the ARM ABI level
 * consumes THREE registers: r0=this, r1=&model, r2=&animFile. Its ROM body
 * is a 0xc-byte tail-call veneer (ldr r12,[pc]; bx r12 -> func_02046d50) that
 * does not touch r0-r2 at all, so whatever the caller already has loaded in
 * those three registers rides straight through. func_02046d50's own matched
 * C source (src/func_02046d50.c) only reads r0/r1 in its prologue, but that
 * is irrelevant to the call site's own ABI obligations -- the compiler at
 * OneUpLogo::InitResources's call site still had to set up r0=this before
 * loading r1/r2, because Prepare is invoked as a real method call, not a
 * free function.
 *
 * The matched src/_ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File.cpp
 * spells this correctly (func_02046d50(this, &model, &animFile), 3 args) and
 * the port's real bridge in hal/player_bridges.cpp mirrors it exactly:
 *
 *     void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(void *self,
 *         void *bmd, void *btp)
 *     { ((TextureSequence *)self)->TextureSequence::Prepare(
 *           *(BMD_File *)bmd, *(BTP_File *)btp); }
 *
 * src/_ZN9OneUpLogo13InitResourcesEv.cpp, however, locally re-declares the
 * symbol as a two-argument free function:
 *
 *     extern void _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(
 *         void* bmd, void* btp);
 *     ...
 *     _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(
 *         (void*)data_ov002_02110aa4.file, (void*)data_ov002_02110a9c.file);
 *
 * two arguments, decl=2/push=2 in the argsweep table (FINAL_TABLE.csv row
 * __ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File, consumed=3,
 * SHORT-1). On the DS this is byte-identical because Prepare is called as a
 * true method (this already lives in r0 from the enclosing InitResources's
 * own `this`, model in r1, animFile in r2) -- the ARM call site itself
 * supplies the third register regardless of the C declaration's arity. On
 * the host, cdecl passes only the two declared arguments on the stack; the
 * bridge's `self` parameter then reads whatever `bmd` (the BMD_File
 * pointer) happens to be, and its own `bmd`/`btp` parameters shift one slot
 * short, landing on stack garbage for `btp`.
 *
 * THE FIX is to pass OneUpLogo's own `this` explicitly as the bridge's
 * first argument, exactly the value the ROM already carries in r0 at this
 * call site (OneUpLogo::InitResources's own `this`, since Prepare is called
 * as `this->TextureSequence::Prepare(...)` on the object's embedded
 * TextureSequence sub-object at +0x124 -- see the matched CleanupResources/
 * Render files for the same +0x124 sub-object cast shape). This host copy
 * is the matched source line for line otherwise; only the Prepare call
 * gains its real first argument.
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
  _ZN15TextureSequence7PrepareER8BMD_FileR8BTP_File(((char*)this)+0x124, (void*)data_ov002_02110aa4.file, (void*)data_ov002_02110a9c.file);
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
