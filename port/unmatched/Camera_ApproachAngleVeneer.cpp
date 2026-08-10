/* HOST COPY of src/func_020078c4.c -- a camera helper that eases an angle field
 * toward a target with ApproachAngle.
 *
 * THE CALLING-CONVENTION SEAM:
 *
 * ApproachAngle takes FIVE arguments:
 *     int ApproachAngle(short *cur, short target, int divisor, int band, int maxStep)
 * (see src/ApproachAngle.c). r0..r3 carry the first four; the fifth, maxStep,
 * rides the stack. func_020078c4 wants maxStep == 0, and the ROM stages that
 * zero on the stack before the call:
 *
 *     020078c8: sub  sp, sp, #4
 *     020078dc: mov  r1, #0
 *     020078e0: str  r1, [sp]         ; stack[0] = 0  == the fifth arg, maxStep
 *     ...
 *     020078f4: mov  r3, #0x4000      ; r3 = band
 *     020078f8: bl   ApproachAngle    ; r0=cur, r1=target, r2=divisor, r3=band, [sp]=0
 *
 * The matched C declares ApproachAngle with only FOUR parameters and passes
 * four arguments; it is byte-identical on ARM because the compiler still emits
 * the `volatile int dummy = 0` store that lands the zero in the outgoing
 * stack-arg slot the ABI reserves. (That is exactly what the `volatile int
 * dummy = 0` line and its comment are for.)
 *
 * On the host cdecl, a four-argument call pushes four slots; ApproachAngle's
 * host object then reads maxStep from a fifth stack slot nothing wrote -- so the
 * step clamp compares against stack garbage and the eased value jumps. THE FIX
 * passes the fifth argument explicitly as 0, exactly the value the ROM stores on
 * the stack. The `volatile int dummy` is no longer needed to place it and is
 * dropped.
 *
 * src/func_020078c4.c is dropped from slice_ptrsweep.txt in favour of this file;
 * the byte-locked source is unchanged. The data_02086d88 pointer table
 * (hal/ptr_tables.cpp) already names func_020078c4 and now resolves to this copy.
 */

struct Camera;

extern "C" short ReadUnalignedShort(unsigned char *p);
/* Same declaration convention the byte-locked source used (void return, int
 * params, C linkage) -- now with the fifth argument the real definition takes. */
extern "C" void ApproachAngle(int a, int b, int c, int d, int e);

// PORT_HOST_ABI: implicit-register-arg (ApproachAngle takes 5 args; the matched decl passes 4, the ARM [sp] slot carried 0).
extern "C" int func_020078c4(struct Camera *self, const unsigned char *data)
{
    short v = ReadUnalignedShort((unsigned char *)data);
    unsigned char b = data[2];
    ApproachAngle((int)self + 0x17a, (int)v, b, 0x4000, 0);   /* <-- maxStep = 0 */
    return 1;
}
