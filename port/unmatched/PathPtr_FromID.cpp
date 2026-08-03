/* HOST COPY of src/_ZN7PathPtr6FromIDEj.cpp -- the structure return given
 * its hidden argument explicitly.
 *
 * PathPtr is two words, and mwcc returns it through a caller-supplied slot in
 * r0 with the id in r1. FromID's whole body is a call to func_0203ad6c, whose
 * matched source is `p[0] = v` -- the slot in r0, the value in r1 -- and
 * FromID names only the value, letting the return slot ride through in r0.
 * Every CALLER in src spells the pair out (`FromID(PathPtr *thiz, u32 id)`),
 * which is the shape the host needs.
 *
 * MSVC returns an eight-byte struct in EAX:EDX instead, so the matched pair
 * compiled for the host reads the callers' `thiz` as the id and writes the
 * record address over whatever the first stack argument happened to be. It
 * has been dormant only because the harness pinned the player's path binding
 * to 0xff; the real level boot seats data_020a0d84 and the ground tracking
 * starts calling this on the first contact frame.
 */
extern "C" {

extern int data_020a0d84;   /* the path table base the level boot seats */

void _ZN7PathPtr6FromIDEj(void *self, unsigned id)
{
    *(int *)self = data_020a0d84 + (int)id * 6;
}

}
