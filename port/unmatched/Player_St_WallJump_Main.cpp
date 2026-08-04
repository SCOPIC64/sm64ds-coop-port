// HOST COPY of src/_ZN6Player16St_WallJump_MainEv.cpp -- the per-character
// airborne-gravity PMF dispatch (rows sinit-copied into the BSS table
// data_ov002_0211073c) replaced with hal_call_state_fn on the DS code word,
// same treatment as Player_St_Jump_Main.cpp / Player_St_NoControl_Init.cpp.
// Everything else is the matched source verbatim.
//
// WHY THIS FILE EXISTS: St_Jump_Main and St_WallJump_Main read the SAME
// four-row table, and Jump has had a host copy since gate 10. WallJump did
// not, so it still did the ROM's `f = (int(*)(void*))row[0]; f(p);` -- and
// row[0] is a DS code address (0x020e200c for Mario). On the DS that is the
// character's gravity function; on the host it is an address inside the
// mounted ov002 DATA image, so the call jumped into mapped data and died
// hard enough that SetUnhandledExceptionFilter never got to report it. That
// is the walljump crash: the playlog just ends mid-frame.
//
// The virtual branch (row word 1 bit 0) goes through hal_call_state_fn too,
// following Player_St_NoControl_Init.cpp rather than Player_St_Jump_Main.cpp.
// Every ptr word in this table is zero in the ROM's own data, so the branch
// is dead either way -- but row[0] would there be a DS vtable BYTE OFFSET,
// and adding that to a host vtable pointer is precisely the wild jump this
// file is fixing. Routing it keeps an unhosted character at the port's loud
// no-op instead.
#include "decl_common.h"
#include "Player.h"
extern "C" {
typedef int Fix12i;
extern int func_ov002_020eeca8(void*, void*);
extern int func_ov002_020e28d4(void*, int, int);
extern int _ZN6Player11ChangeStateERNS_5StateE(void*, void*);
extern int _ZN6Player7IsStateERNS_5StateE(void*, void*);
extern int Player_AdvanceAnims(void*);
extern int hal_call_state_fn(void* self, unsigned ds_addr);
extern char data_ov002_02110424[];
extern unsigned char data_020a0e40[];
extern unsigned short data_0209f49e[];
extern char data_ov002_0211052c[];
}

int Player::St_WallJump_Main()
{
  func_ov002_020eeca8((char*)((void*)this)+0x380, ((void*)this));
  func_ov002_020e28d4(((void*)this), 0x1800, 0x800);
  if (*(unsigned char*)((char*)&mIsAirborne) == 0) {
    _ZN6Player11ChangeStateERNS_5StateE(((void*)this), data_ov002_02110424);
  } else {
    if (*(unsigned short*)((char*)data_0209f49e + data_020a0e40[0]*0x18) & 0x400) {
      if (_ZN6Player7IsStateERNS_5StateE(((void*)this), data_ov002_0211052c)) {
        *(short*)((char*)&mPrevAngleY) = *(short*)((char*)&mAngleY);
      }
    }
    if (func_ov002_020e2664(((void*)this))) return 1;
    {
      int idx = *(int*)((char*)&param1);
      int* row = &data_ov002_0211073c[idx*2];
      int v = row[1];
      void* p = (char*)((void*)this) + (v>>1);
      if (v & 1) {
        /* PORT: row[0] is a DS vtable byte offset; route it, do not walk a
           host vtable with it */
        hal_call_state_fn(p, (unsigned)(*(int*)p + row[0]));
      } else {
        /* PORT: row[0] is a DS code address (mwcc PMF); route through the
           state-fn mapper instead of calling it raw */
        hal_call_state_fn(p, (unsigned)row[0]);
      }
    }
  }
  Player_AdvanceAnims(((void*)this));
  return 1;
}
