/* HOST COPIES of src/_ZN5Actor13DistToCPlayerEv.cpp and
 * src/_ZN5Actor14FarthestPlayerEv.cpp -- the two thin wrappers that call
 * Actor::ClosestPlayer() for its side effect (it caches the closest distance
 * and the farthest-player pointer in file-scope globals) and then return one
 * of those globals.
 *
 * THE CALLING-CONVENTION SEAM:
 *
 * Both callers are Actor member functions; each receives its own `this` in r0.
 * Actor::ClosestPlayer() is ALSO a member -- it reads `this` (r0) to measure
 * distance from `this` to every player: Vec3_Dist((char*)this + 0x5c, ...).
 * On the DS the two wrappers call it without touching r0, so ClosestPlayer's
 * `this` arrives for free as the wrapper's own `this`:
 *
 *     DistToCPlayer (0x020109e4):   stmdb sp!,{lr}; sub sp,#4
 *                                   bl   ClosestPlayer   ; r0 still = this
 *                                   ldr  r0,[pc]; ldr r0,[r0]   ; = data_0208e380
 *     FarthestPlayer (0x02010958):  same shape, returns data_0209b450
 *
 * The matched C spells the call as ClosestPlayer() with no argument (the src
 * extern is `_ZN5Actor13ClosestPlayerEv(void)`); byte-identical on ARM because
 * r0 already holds `this`.
 *
 * On the host the ClosestPlayer bridge (hal/reverse_bridges.cpp) is
 * `_ZN5Actor13ClosestPlayerEv(void *self)` -- cdecl, `self` off the stack. The
 * zero-argument call pushes nothing, so `self` is stack garbage and
 * ClosestPlayer measures distance from garbage+0x5c, faulting or returning a
 * meaningless cache. THE FIX passes `this` explicitly, exactly the value the
 * ROM leaves in r0.
 *
 * src/_ZN5Actor13DistToCPlayerEv.c and src/_ZN5Actor14FarthestPlayerEv.c are
 * dropped from their slice files (gate 16, gate 89) in favour of this file; the
 * byte-locked sources are unchanged.
 */

extern "C" {

void *_ZN5Actor13ClosestPlayerEv(void *self);   /* the real one-arg (this) shape */

extern int   data_0208e380;   /* closest-player distance, set by ClosestPlayer */
extern void *data_0209b450;   /* farthest-player pointer,  set by ClosestPlayer */

/* Actor::DistToCPlayer() -> s32 */
// PORT_HOST_ABI: implicit-register-arg (ClosestPlayer's this rode r0 from the enclosing member; the host passes it).
int _ZN5Actor13DistToCPlayerEv(void *self)
{
    _ZN5Actor13ClosestPlayerEv(self);   /* <-- this, the ROM's r0 */
    return data_0208e380;
}

/* Actor::FarthestPlayer() -> Player* */
// PORT_HOST_ABI: implicit-register-arg (FarthestPlayer: same shape, ClosestPlayer's this rode r0; the host passes it).
void *_ZN5Actor14FarthestPlayerEv(void *self)
{
    _ZN5Actor13ClosestPlayerEv(self);   /* <-- this, the ROM's r0 */
    return data_0209b450;
}

}  /* extern "C" */
