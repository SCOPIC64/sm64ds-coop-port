/* HOST COPY of src/_ZN3G3i13PerspectiveW_E5Fix12IiES1_S1_S1_S1_S1_bP9Matrix4x3.c
 *
 * Why a copy and not hostgen: the matrix payload leaves through a POINTER
 * BOUND TO A REGISTER --
 *
 *     port = (volatile int *)0x4000458;    // MTX_LOAD_4x4
 *     ...
 *     *port = xx;                          // sixteen of these
 *
 * hostgen.py rewrites `*(volatile T *)0xLITERAL` and nothing else, so every
 * `*port` store lands in the plain memory the host maps at the DS I/O window.
 * It latches, the geometry engine never sees it, and the projection matrix is
 * silently dropped -- the failure looks like "the camera does nothing" rather
 * than a crash. Each store here goes through NTR_MMIO, which dispatches into
 * ntr::gx_write_port.
 *
 * One deliberate divergence, marked below: DIVCNT. On the DS the function
 * inherits 64/32 mode from the cstd::fdiv call it opens with (fdiv_async
 * writes DIVCNT itself). The port's cstd::fdiv is a pure C divide in
 * port/hal/cstd_div.c and touches no register, so the mode has to be set
 * here or the two divisions run as 32/32 and the near/far row comes out
 * garbage. Everything else is the matched source, statement for statement.
 */
#include "ntr/mmio.h"

typedef long long s64;
typedef unsigned long long u64;

extern "C" {
int _ZN4cstd4fdivEii(int, int);
int _ZN4cstd11fdiv_resultEv(void);
s64 _ZN4cstd11ldiv_resultEv(void);

void _ZN3G3i13PerspectiveW_E5Fix12IiES1_S1_S1_S1_S1_bP9Matrix4x3(
    int sinF, int cosF, int aspect, int n, int f, int scaleW, int flag,
    int *mtx)
{
    int tmp;
    int xx;
    s64 q;
    int zz;
    int v;
    int wz;

    tmp = _ZN4cstd4fdivEii(cosF, sinF);
    if (scaleW != 0x1000)
        tmp = tmp * scaleW / 0x1000;

    /* HOST ONLY: the DS inherits 64/32 from fdiv_async (see header). */
    NTR_MMIO(unsigned short, 0x4000280) = 1;

    NTR_MMIO(u64, 0x4000290) = (u64)(unsigned)tmp << 32;
    NTR_MMIO(u64, 0x4000298) = (unsigned)aspect;
    if (flag)
        NTR_MMIO(int, 0x4000440) = 0;          /* MTX_MODE projection */
    if (mtx) {
        mtx[1] = 0;
        mtx[2] = 0;
        mtx[3] = 0;
        mtx[4] = 0;
        mtx[6] = 0;
        mtx[7] = 0;
        mtx[8] = 0;
        mtx[9] = 0;
        mtx[11] = -scaleW;
        mtx[12] = 0;
        mtx[13] = 0;
        mtx[15] = 0;
    }
    xx = _ZN4cstd11fdiv_resultEv();
    NTR_MMIO(u64, 0x4000290) = (u64)0x1000 << 32;
    NTR_MMIO(u64, 0x4000298) = (unsigned)(n - f);
    if (flag) {
        /* MTX_LOAD_4x4: sixteen parameter words */
        NTR_MMIO(int, 0x4000458) = xx;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = tmp;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
    }
    if (mtx) {
        mtx[0] = xx;
        mtx[5] = tmp;
    }
    q = _ZN4cstd11ldiv_resultEv();
    if (scaleW != 0x1000)
        q = q * scaleW / 0x1000;
    zz = (int)(((f + n) * q + 0x80000000LL) >> 32);
    v = (int)(((s64)(n << 1) * f + 0x800) >> 12);
    wz = (int)((q * v + 0x80000000LL) >> 32);
    if (flag) {
        NTR_MMIO(int, 0x4000458) = zz;
        NTR_MMIO(int, 0x4000458) = -scaleW;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = 0;
        NTR_MMIO(int, 0x4000458) = wz;
        NTR_MMIO(int, 0x4000458) = 0;
    }
    if (mtx) {
        mtx[10] = zz;
        mtx[14] = wz;
    }
}
}  /* extern "C" */
