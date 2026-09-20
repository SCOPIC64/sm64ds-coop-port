//cpp
// @symbol _ZN6Player16St_WallJump_MainEv
/* recovered: named members + shared header, real C++ method, declarations from a shared header */
#include "decl_common.h"
/* recovered: named members + shared header, real C++ method */
#include "Player.h"
#include <cstdio>
extern "C" {
typedef int Fix12i;
extern int func_ov002_020eeca8(void*, void*);
extern int func_ov002_020e28d4(void*, int, int);
extern int _ZN6Player11ChangeStateERNS_5StateE(void*, void*);
extern int _ZN6Player7IsStateERNS_5StateE(void*, void*);
extern int Player_AdvanceAnims(void*);
extern char data_ov002_02110424[];
extern unsigned char data_020a0e40[];
extern unsigned short data_0209f49e[];
extern char data_ov002_0211052c[];
extern int hal_call_state_fn(void *self, unsigned ds_addr);
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
      /* PORT: jump table holds rows 0..10; clamp wild kinds to default. */
      if (idx < 0 || idx > 10) {
        std::printf("[jumpidx] WallJump_Main kind %d out of range, clamped\n",
                    idx);
        idx = 0;
      }
      int* row = &data_ov002_0211073c[idx*2];
      int v = row[1];
      void* p = (char*)((void*)this) + (v>>1);
      int (*f)(void*);
      if (v & 1) {
        f = *(int(**)(void*))((char*)(*(int**)p) + row[0]);
        /* PORT: unfilled vtable slots still hold ROM addresses (op=8 DEP
           fault when called). Host code never lives in DS address space. */
        if ((unsigned)f >= 0x02000000u && (unsigned)f < 0x03000000u) {
          std::printf("[jumpidx] WallJump_Main vtable slot holds ROM %08x, "
                      "routed\n", (unsigned)f);
          hal_call_state_fn(p, (unsigned)f);
          f = 0;
        }
      } else {
        /* PORT DIVERGENCE (DEP crash): same raw-DS-helper hazard as
           St_Jump_Main -- route through the state dispatcher. */
        hal_call_state_fn(p, (unsigned)row[0]);
        f = 0;
      }
      if (f) f(p);
    }
  }
  Player_AdvanceAnims(((void*)this));
  return 1;
}
