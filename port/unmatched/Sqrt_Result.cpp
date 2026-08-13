/* HOST BODY of func_02053130 (arm9 0x02053130): the fixed-point
 * reciprocal-square-root tail that pairs the DIVIDER's async reciprocal with
 * the SQRT unit's result -- 1/sqrt(x) in the game's 20.12/long forms.
 *
 * This closes the sqrt half of the gap hal/actor_classes_ov045.cpp's id-141
 * section named and slice_w1l4.txt's block (G) prescribed: "host the sqrt
 * pair the way hostgen gates 8/13 host the divide unit". func_02053008 (the
 * parameter writer) IS hostgen-generated in this wave -- its MMIO is literal
 * pointer casts, which tools/hostgen.py rewrites to NTR_MMIO so the final
 * SQRT_PARAM word-write runs run_sqrt() (port/ntr/io.cpp; the high-word
 * dispatch is this wave's io.cpp amendment). THIS body cannot be hostgen'd:
 * the matched src/func_02053130.c reaches the unit through NAMED externs
 * (`extern volatile u16 SQRTCNT;`), and hostgen only rewrites literal casts
 * -- the generated file would just re-export the undefined names. The copy
 * below is the same statements with the two named externs spelled as the
 * NTR_MMIO accesses they are (0x040002b0 / 0x040002b4); the busy-bit spin is
 * kept verbatim -- the host model computes synchronously, so it falls
 * through on the first read, which is the run_divide precedent.
 *
 * func_ov021_02112024 (WorkElevator's rider-push callback) is what pulls
 * this chain into level 13: Quaternion_FromVector3 -> func_020531a4 ->
 * func_02052fdc (both hostgen'd, literal MMIO) and Quaternion_Normalize ->
 * func_02053130 (this body). Re-enabling ov045's TILTING_PLATFORM_BFS (141)
 * on the back of this pair is the follow-up its section already describes.
 */
#include "ntr/mmio.h"

extern "C" {
void _ZN4cstd16reciprocal_asyncE5Fix12IiE(int x);
void func_02053008(int x);
long long _ZN4cstd11ldiv_resultEv(void);
}

/* PORT_HOST_ABI: unmodelled DS hardware reached through NAMED externs
   (SQRTCNT/SQRT_RESULT) that hostgen's literal-cast rewriter cannot touch;
   this body reads the modeled unit through NTR_MMIO instead. */
extern "C" int func_02053130(int r0)
{
    int r4 = r0;
    long long divres;

    if (r4 <= 0)
        return 0;

    _ZN4cstd16reciprocal_asyncE5Fix12IiE(r4);
    func_02053008(r4);
    divres = _ZN4cstd11ldiv_resultEv();

    while (static_cast<unsigned short>(NTR_MMIO(unsigned short, 0x040002b0)) &
           0x8000)
        ;

    return (int)((divres * (long long)(int)NTR_MMIO(int, 0x040002b4) +
                  ((long long)0x200 << 32)) >>
                 (32 + 10));
}
