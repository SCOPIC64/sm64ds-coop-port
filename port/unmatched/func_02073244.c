/* HOST COPIES of func_02073244 and func_0203cbc0 -- the MSL array-delete pair.
 *
 * Both are ARGUMENT RIDE-THROUGHS on the ARM, and neither survives the trip to
 * a host calling convention.
 *
 *   func_0203cbc0 is a two-instruction veneer onto _ZdlPv. src spells it
 *   `void func_0203cbc0(void)` calling `_ZdlPv()` with no argument, which is
 *   exactly right for ARM -- the pointer is already in r0 and the branch keeps
 *   it there -- and exactly wrong for cdecl, where the callee would read the
 *   stack for an argument nobody declared.
 *
 *   func_02073244 hands func_02073300 THREE arguments and lets the fourth (the
 *   element destructor) ride through in r3, which is where its callee expects
 *   it. func_02073300 itself is an asm hatch -- a reverse destructor loop with
 *   an MSL exception frame -- so there is no C to compile for it either.
 *
 * The host version is the same operation without the register trick: walk the
 * array backwards calling the destructor, then free the raw block. The COOKIE
 * LAYOUT is the one hal/actor_vtables.cpp's func_02073470 writes -- the host's
 * own array-new-with-ctor, which mirrors the DS's: the raw block starts with
 * {element size, count} and the array pointer the caller holds is raw + cookie.
 * So the count is at base[-1] for the cookie of 8 that every caller passes,
 * which is what the ROM's own func_02073244 reads (`*(int *)(a - 4)`).
 *
 * Reached from Player::CleanupResources (the 0x32-element queue at +0x578,
 * built by Player::InitResources through the matching func_02073470) and from
 * Stage::CleanupResources, which the port does not run.
 */
extern void _ZdlPv(void *p);

void func_0203cbc0(void *p)
{
    _ZdlPv(p);
}

void func_02073244(void *base, int stride, int cookie, void (*dtor)(void *))
{
    char *b = (char *)base;
    if (!b)
        return;
    if (dtor) {
        int count = ((const int *)b)[-1];
        int i;
        for (i = count - 1; i >= 0; --i)
            dtor(b + (long)i * stride);
    }
    _ZdlPv(b - cookie);
}
