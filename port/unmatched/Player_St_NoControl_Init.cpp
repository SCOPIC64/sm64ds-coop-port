// HOST COPY of src/_ZN6Player17St_NoControl_InitEv.cpp -- the per-kind
// entry-function PMF dispatch (rows sinit-copied from ROM tables into
// data_ov002_02110884) replaced with hal_call_state_fn on the DS code
// word, same treatment as Player_ChangeState.cpp / Player_St_Jump_Main.cpp.
// Everything else is the matched source verbatim.
//
// Unlike St_Jump_Main's rows, the virtual branch goes through
// hal_call_state_fn too: every ptr word in this table is zero, so the
// branch is dead in the ROM's own data, and m->adj would be a DS vtable
// BYTE OFFSET -- meaningless against a host vtable. Routing it keeps an
// unhosted kind at the port's loud no-op instead of a jump into ROM.
#include "Player.h"
extern "C" {
extern int Player_DisableInteraction(void *c);
extern int Player_ReleaseHeldActor(void *c);
extern int hal_call_state_fn(void *self, unsigned ds_addr);
struct PMF { int adj; int ptr; };
extern struct PMF data_ov002_02110884[];
}

int Player::St_NoControl_Init()
{
  mIsControlDisabled=1;
  unk_6e6=0;
  Player_DisableInteraction(((char*)this));
  Player_ReleaseHeldActor(((char*)this));
  mRidingShell=0;
  mHeldObj=0;
  unk_35c=0;
  mIsOpeningBigDoor=0;
  unsigned idx = mNoCtrlKind;
  struct PMF* m=&data_ov002_02110884[idx];
  char* obj=((char*)this) + (m->ptr >> 1);
  int fn=m->ptr;
  if(fn & 1){
    hal_call_state_fn(obj, (unsigned)(*(int*)obj + m->adj));
  } else {
    hal_call_state_fn(obj, (unsigned)m->adj);
  }
  return 1;
}
