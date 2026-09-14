// Lane INT4, run link100 wave 9c (the fold): the model family's twelve
// Destructor1 / Destructor0 rows.
//
// WHAT THESE NAMES ARE. include/ModelBase.h's own paragraph at line 110 says
// it: mwccarm puts a class's D1 and D0 in vtable slots 0 and 1, and MSVC puts
// one destructor entry there, so every slot from index 1 on would be one word
// early. The decomp headers therefore spell TWO ORDINARY VIRTUALS in those two
// places under the host-invented names Destructor1 and Destructor0, guarded by
// _MSC_VER so the ARM side never sees them and no ROM byte moves. The header
// also says "neither name is ever called; they hold the two slots the ROM's
// table holds."
//
// WHY THEY ARE ON THE WALL ANYWAY. Holding a slot still needs a definition. A
// constructor emits its class's MSVC vftable, and a vftable references every
// slot, so src/_ZN5ModelC1Ev.cpp and its siblings ask the linker for
// ?Destructor1@Model@@UAEXXZ and ?Destructor0@Model@@UAEXXZ. Six classes, two
// slots each, twelve rows of walk_window's unresolved wall, and the FACES4
// residue files put every one of them in the "no ROM name for that class and
// method" bucket, which is true of the NAME and not of the BODY.
//
// AN /alternatename IS NOT ADMISSIBLE HERE and that is why this is a face and
// not a row in cxx_aliases.cpp. The standing test (out/HALROWS/settled.txt
// section B) is that an alias is a name bridge and never an ABI bridge. These
// two sides do not agree about the call: ?Destructor1@Model@@UAEXXZ is
// __thiscall with the receiver in ecx, and _ZN5ModelD1Ev is the ROM body under
// C linkage taking the receiver as an ordinary first argument. A face moves it;
// an alias would leave it in the wrong place.
//
// WHAT EACH BODY IS. The ROM's own function for that slot, called with the
// receiver the host ABI put in ecx. Nothing invented, nothing stubbed: every
// one of the twelve is a real cartridge body with an address in
// config/arm9/symbols.txt, and every one is already DEFINED in this link, which
// is why none of the twelve flat names is on the wall.
//
//   slot 0, Destructor1              slot 1, Destructor0
//   _ZN9ModelBaseD1Ev      0x02017120   _ZN9ModelBaseD0Ev      0x020170e8
//   _ZN5ModelD1Ev          0x02016d20   _ZN5ModelD0Ev          0x02016ce0
//   _ZN11CommonModelD1Ev   0x020161e0   _ZN11CommonModelD0Ev   0x020161b4
//   _ZN10ModelAnim2D1Ev    0x02016364   _ZN10ModelAnim2D0Ev    0x02016320
//   _ZN14BlendModelAnimD1Ev 0x02016690  _ZN14BlendModelAnimD0Ev 0x02016644
//   _ZN11ShadowModelD1Ev   0x02015ff8   _ZN11ShadowModelD0Ev   0x02015f80
//
// THE TIDY VERSION, for whoever owns hal/model_host.cpp next: these twelve
// belong beside the rest of that family's host bodies. They are here because
// no lane owns that file this wave and the house law is that a lane stays
// inside its own files, so they go in additively through port/slice_int4.txt
// and the one CMake block that reads it.

#include "ModelBase.h"
#include "Model.h"
#include "CommonModel.h"
#include "ModelAnim2.h"
#include "BlendModelAnim.h"
#include "ShadowModel.h"

extern "C" {
void _ZN9ModelBaseD1Ev(void *self);
void _ZN9ModelBaseD0Ev(void *self);
void _ZN5ModelD1Ev(void *self);
void _ZN5ModelD0Ev(void *self);
void _ZN11CommonModelD1Ev(void *self);
void _ZN11CommonModelD0Ev(void *self);
void _ZN10ModelAnim2D1Ev(void *self);
void _ZN10ModelAnim2D0Ev(void *self);
void _ZN14BlendModelAnimD1Ev(void *self);
void _ZN14BlendModelAnimD0Ev(void *self);
void _ZN11ShadowModelD1Ev(void *self);
void _ZN11ShadowModelD0Ev(void *self);
}

void ModelBase::Destructor1()      { _ZN9ModelBaseD1Ev(this); }
void ModelBase::Destructor0()      { _ZN9ModelBaseD0Ev(this); }
void Model::Destructor1()          { _ZN5ModelD1Ev(this); }
void Model::Destructor0()          { _ZN5ModelD0Ev(this); }
void CommonModel::Destructor1()    { _ZN11CommonModelD1Ev(this); }
void CommonModel::Destructor0()    { _ZN11CommonModelD0Ev(this); }
void ModelAnim2::Destructor1()     { _ZN10ModelAnim2D1Ev(this); }
void ModelAnim2::Destructor0()     { _ZN10ModelAnim2D0Ev(this); }
void BlendModelAnim::Destructor1() { _ZN14BlendModelAnimD1Ev(this); }
void BlendModelAnim::Destructor0() { _ZN14BlendModelAnimD0Ev(this); }
void ShadowModel::Destructor1()    { _ZN11ShadowModelD1Ev(this); }
void ShadowModel::Destructor0()    { _ZN11ShadowModelD0Ev(this); }
