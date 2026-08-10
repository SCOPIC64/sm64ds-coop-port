/* HOST COPY of src/func_ov072_021217ac.cpp -- the SAME func_ov072_02121d50
 * SHORT-1 seam port/unmatched/BabyPenguin_InitResources.cpp fixes,
 * applied to this landing-transition helper.
 *
 * The matched src's `func_ov072_02121d50(thiz);` (one argument, real
 * signature `(char *c, int i)`) sits right after `*(int*)(c+0x360) = 0`
 * -- the identical "reset to idle, state 0" pairing InitResources' own
 * call carries. See BabyPenguin_InitResources.cpp's header for the full
 * derivation. This host copy is the matched src line for line otherwise;
 * only the func_ov072_02121d50 call gains its real second argument (0).
 *
 * src/func_ov072_021217ac.cpp is dropped from slice_gate193.txt in
 * favour of this file; the byte-locked source is unchanged.
 */
#include "common.h"

class Actor;
extern "C" {
void func_ov072_02121d50(Actor *a, int i);
int func_0201267c(int id, void *p);
bool _ZN5Actor17DetectRaycastClsnER7Vector3S1_b(Actor *thiz, Vector3 &a, Vector3 &b, bool c);
void _ZN9Animation7AdvanceEv(void *anim);
void _ZN12CylinderClsn5ClearEv(void *clsn);

int func_ov072_021217ac(Actor *thiz)
{
    char *c = (char *)thiz;
    short h = *(short *)(*(char **)(c + 0x360) + 0x8e);
    *(short *)(c + 0x8e) = h;
    *(short *)(c + 0x94) = *(short *)(c + 0x8e);
    int flags = *(int *)(c + 0xb0);
    int b1 = (int)((flags & 0x100) != 0);
    if (b1 != 0) {
        int b2 = (int)((flags & 0x2000) != 0);
        if (b2 == 0) goto after_ray;
    }
    {
        char *q = *(char **)(c + 0x360);
        Vector3 v;
        int y = *(int *)(q + 0x60);
        int z = *(int *)(q + 0x64);
        int x = *(int *)(q + 0x5c);
        int y2 = y + 0x32000;
        v.x = x;
        v.y = y2;
        v.z = z;
        _ZN5Actor17DetectRaycastClsnER7Vector3S1_b(thiz, v, *(Vector3 *)(c + 0x5c), true);
        {
            int z0 = 0;
            *(int *)(c + 0x360) = z0;
            func_ov072_02121d50(thiz, 0);
        }
    }
after_ray:
    _ZN9Animation7AdvanceEv(c + 0x124);
    _ZN12CylinderClsn5ClearEv(c + 0x160);
    if (*(int *)(c + 8) == 0) {
        unsigned int t = (unsigned int)(*(int *)(c + 0x12c) << 4) >> 16;
        if (t == 0x10 || t == 0x25) {
            func_0201267c(0xf2, c + 0x74);
        }
    }
    return 1;
}
}
