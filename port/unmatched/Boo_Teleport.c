/* HOST COPY of func_ov063_02117650 (ov063 0x02117650, 0x11c bytes), the Boo
 * re-placement helper: rerolls a random position until it is at least
 * 0x320000 from the closest player, then faces the player. The matched TU
 * (src/unnamed/ov063/func_ov063_02117650.c) is byte-locked and calls
 * Actor::ClosestPlayer() with NO argument -- the r0-passthrough seam
 * unmatched/Actor_ClosestPlayer_OverlayReaders.cpp documents, and its LATENT
 * registry names this exact TU as the ov063 entry ("BEFORE hosting the
 * overlay that carries one of these, host copy the reader to PASS the
 * receiver first"). This is that copy: the receiver rides the call, exactly
 * the value the ROM leaves in r0; nothing else changes. The src line stays
 * out of slice_w5a.txt in favour of this file.
 */
#include "common.h"
/* PORT_HOST_ABI: r0-passthrough seam -- the matched TU drops the
   ClosestPlayer receiver that rides ARM r0; passed explicitly here. */
extern void *_ZN5Actor13ClosestPlayerEv(void *self);
extern int RandomIntInternal(int *seed);
extern int Vec3_HorzDist(struct Vector3 *a, struct Vector3 *b);
extern short Vec3_HorzAngle(struct Vector3 *a, struct Vector3 *b);
extern int data_0209e650;

void func_ov063_02117650(char *self)
{
    struct Vector3 ppos;
    struct Vector3 npos;
    char *p;
    int neg1 = (int)(-1LL);

    p = (char *)_ZN5Actor13ClosestPlayerEv(self);
    if (p == 0) {
        return;
    }

    {
        int *pp = (int *)(((long long)(int)(p + 0x5c)));
        ppos.x = pp[0];
        ppos.y = pp[1];
        ppos.z = pp[2];
    }
    npos.y = *(int *)(self + 0x60);

    do {
        npos.x = ((int)(((unsigned int)RandomIntInternal(&data_0209e650) >> 0x10) % 0x578) * neg1 - 0x190) << 0xc;
        npos.z = ((int)(((unsigned int)RandomIntInternal(&data_0209e650) >> 0x10) % 0xa28) - 0x514) << 0xc;
    } while (Vec3_HorzDist(&ppos, &npos) < 0x320000);

    *(int *)(self + 0x5c) = npos.x;
    *(int *)(self + 0x60) = npos.y;
    *(int *)(self + 0x64) = npos.z;
    *(short *)(self + 0x8e) = Vec3_HorzAngle((struct Vector3 *)(self + 0x5c), &ppos);
    *(short *)(self + 0x94) = *(short *)(self + 0x8e);
}
