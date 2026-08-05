/* Host implementation of the cstd fixed-point divide.
 *
 * On the DS, cstd::fdiv feeds the hardware divider (fdiv_async writes the
 * DIV registers, fdiv_result spins on DIV_BUSY) -- src/_ZN4cstd4fdivEii.c is
 * a thin wrapper over MMIO and cannot run on a host. The operation itself is
 * just a 20.12 divide: (a / b) in Fix12 is (a << 12) / b.
 *
 * DS divider edge cases, preserved deliberately:
 *   b == 0  -> the hardware yields +/-1 with the sign of the numerator (and
 *              sets DIV_DIV0); callers in this codebase never rely on it, but
 *              matching the hardware costs one branch and removes a class of
 *              "host differs on garbage input" surprises.
 */
typedef int s32;

s32 _ZN4cstd4fdivEii(s32 a, s32 b)
{
    if (b == 0)
        return a < 0 ? -1 : 1;
    return (s32)(((long long)a << 12) / b);
}

/* ---- DS INTEGER DIVISION SEMANTICS ----------------------------------------
 *
 * These are NOT cstd::div. They are what mwccarm's `/` and `%` OPERATORS
 * compile to: a call to __aeabi_idiv / _s32_div_f, the CodeWarrior helper the
 * ROM keeps in ITCM at 0x01ffabe4, which returns the quotient in r0 and the
 * remainder in r1.
 *
 * The zero-divisor path is the whole reason this exists, and it is read out
 * of the ROM rather than assumed. 0x01ffabe4 saves the operand signs, then:
 *
 *     0x01ffabf8  cmp   r1, #0
 *     0x01ffac00  beq   0x01ffaddc
 *     ...
 *     0x01ffaddc  ands  r3, r12, #0x80000000
 *     0x01ffade0  rsbne r0, r0, #0        ; restore the numerator's sign
 *     0x01ffade4  ands  r3, r12, #1
 *     0x01ffade8  rsbne r1, r1, #0        ; r1 is still 0
 *     0x01ffadec  bx    lr
 *
 * so on a zero divisor the ROM returns QUOTIENT = NUMERATOR and REMAINDER = 0.
 * It does not fault, and x86's idiv does -- which is a real behavioural
 * difference, not a corner case: 181 of the 321 particle definitions ship a
 * ZERO emission interval, and func_0204a730 gates emission on
 * `counter % interval == 0`. On the ROM that is always true, so a zero
 * interval means "emit every frame". Hosted with a raw x86 idiv it is an
 * immediate crash the first time the Player kicks up dust.
 *
 * hostgen.py routes the affected expressions here (DS_DIV in its patch
 * table); see the note there for why the sites are named one by one.
 */
int ds_idiv(int a, int b) { return b ? a / b : a; }
int ds_imod(int a, int b) { return b ? a % b : 0; }
unsigned ds_udiv(unsigned a, unsigned b) { return b ? a / b : a; }
unsigned ds_umod(unsigned a, unsigned b) { return b ? a % b : 0u; }
