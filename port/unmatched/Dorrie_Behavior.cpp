/* HOST COPY of DORRIE's (actor 168, ov065, daDossy_c) Behavior -- vtable
 * slot 6, ROM body 0x02118df0 -- and of the neck-chain helper the matched
 * tree never banked, func_ov065_02118838.
 *
 * Behavior is transcribed line for line from src/_ZN6Dorrie8BehaviorEv.cpp;
 * the one change is the state dispatch. The matched TU indexes
 * `data_ov065_0211d7fc[state]` as an array OF pointer-to-member over a
 * forward-declared class, which MSVC widens to the 16-byte general
 * representation: stride 16 over a table the ROM lays out as 8-byte {fn, 0}
 * records (three of them, copied by __sinit_ov065_0211c440 from
 * 0x0211cd2c/cd1c/cd24 -- state order 0/1/2). The copy indexes 8-byte
 * records and calls fn(self); the fn words are host bodies because
 * port_ov065_states_seat rewrote the sources first.
 *
 * func_ov065_02118838 (ROM 0x02118838, 0x414 bytes) is the one HMC-cast
 * body src/ does not carry: Dorrie's per-frame neck/segment placement, called
 * three times per frame (Behavior + twice from InitResources). The body
 * below is the near-miss DB's structurally-aligned draft (same 1044-byte
 * size as the ROM body; divergences are register/schedule shaped), audited
 * against the ROM's own anchors before adoption: the literal pool at
 * 0x02118bf0 carries 0x020a0e68 / 0x0211c078 / 0x02082214 / offsets
 * 0x1184-0x1188, the loop closes on `cmp r5, #7` (seven segments, matching
 * the seven-pointer table at 0x0211cd68), and the row stride is +0x200 per
 * segment. HOST COPY, not matched src: when the decomp banks the real TU,
 * this body retires for the slice line.
 */
extern "C" {
void _Z14ApproachLinearRiii(int *a, int b, int c);
int Vec3_HorzDist(void *a, void *b);
short Vec3_HorzAngle(void *a, void *b);
int AngleDiff(int a, int b);
void _ZN5Actor9UpdatePosEP12CylinderClsn(void *c, void *p);
void WithMeshClsn_UpdateContinuous_Veneer(void *p);
void _ZN9Animation7AdvanceEv(void *p);
int func_ov065_02118cc4(char *c);
int func_ov065_02118248(char *c);

void Matrix4x3_FromTranslation(void *m, int x, int y, int z);
void Matrix4x3_ApplyInPlaceToRotationY(void *m, short ang);
void Matrix4x3_ApplyInPlaceToRotationX(void *m, short ang);
void Matrix4x3_ApplyInPlaceToRotationZ(void *m, short ang);
void MulMat4x3Mat4x3(void *a, void *b, void *out);
void SubVec3(const void *a, const void *b, void *out);
void Vec3_LslInPlace(void *v, int sh);
void AddVec3(const void *a, const void *b, void *out);
void _ZN18MovingMeshCollider9TransformERK9Matrix4x3s(void *self, void *m,
                                                     short ang);

extern short data_02082214[];
extern unsigned data_ov065_0211d7fc[];
extern unsigned char data_ov065_0211c078[];
extern int data_020a0e68[12];   /* the arm9 scratch matrix, romdata-hosted */
}

namespace {
struct Vec3 { int x, y, z; };
struct Mtx { int m[12]; };
}

extern "C" void func_ov065_02118838(char *r6)
{
    Mtx *scratch = (Mtx *)data_020a0e68;
    Mtx local;
    char *m150;
    char *m180;
    unsigned char *r4;
    int i;
    Vec3 v;

    Matrix4x3_FromTranslation(scratch, *(int *)(r6 + 0x5c),
                              *(int *)(r6 + 0x60), *(int *)(r6 + 0x64));
    Matrix4x3_ApplyInPlaceToRotationY(scratch, *(short *)(r6 + 0x8e));
    local = *scratch;
    m150 = r6 + 0x150;
    m180 = r6 + 0x180;
    i = 0;
    r4 = data_ov065_0211c078;
    do {
        char *ent = (*r4) * 0x34 + *(char **)(r6 + 0xfc);
        char *row = r6 + (i << 9);
        char *base = row + 0x300;
        *(short *)(base + 0x48) = (short)*(unsigned short *)(ent + 0x1a);
        *(short *)(base + 0x4a) = (short)*(unsigned short *)(ent + 0x1c);
        *(short *)(base + 0x4c) = (short)*(unsigned short *)(ent + 0x1e);
        if (i == 2) {
            short *p0 = (short *)(row + 0x348);
            short *p1 = (short *)(row + 0x34a);
            short *p2 = (short *)(row + 0x34c);
            *p0 = (short)(*p0 + *(short *)(r6 + 0x548));
            *p1 = (short)(*p1 + *(short *)(r6 + 0x54a));
            *p2 = (short)(*p2 + *(short *)(r6 + 0x54c));
        }
        v.x = 0;
        v.y = 0;
        v.z = 0;
        *scratch = local;
        MulMat4x3Mat4x3(*(char **)(r6 + 0x100) + (*r4) * 0x30, scratch,
                        scratch);
        v.x = scratch->m[9];
        v.y = scratch->m[10];
        v.z = scratch->m[11];
        SubVec3(&v, r6 + 0x5c, &v);
        Vec3_LslInPlace(&v, 3);
        AddVec3(&v, r6 + 0x5c, &v);
        if (i == 2) {
            int *base1k = (int *)(r6 + 0x1000);
            int *pd8 = (int *)(r6 + 0xd8);
            int *pdc = (int *)(r6 + 0xdc);
            int *pe0 = (int *)(r6 + 0xe0);
            int *p1180 = (int *)(r6 + 0x1180);
            int *p1184 = (int *)(r6 + 0x1184);
            int *p1188 = (int *)(r6 + 0x1188);
            unsigned short yaw_u = *(unsigned short *)(r6 + 0x8e);
            short ce, cy, sy;
            int scaled;
            *(short *)(r6 + 0xe4) = *(short *)(r6 + 0x748);
            base1k[0x60] = v.x;
            base1k[0x61] = v.y;
            base1k[0x62] = v.z;
            *pd8 = base1k[0x60];
            ce = data_02082214[(*(unsigned short *)(r6 + 0xe4) >> 4) * 2];
            scaled = ce * 0x96;
            *pdc = base1k[0x61];
            *pe0 = base1k[0x62];
            cy = data_02082214[(yaw_u >> 4) * 2];
            *pd8 += (int)(((long long)scaled * cy + 0x800) >> 12);
            ce = data_02082214[(*(unsigned short *)(r6 + 0xe4) >> 4) * 2];
            *pdc += 0x8c000 - ce * 0x1e;
            sy = data_02082214[(yaw_u >> 4) * 2 + 1];
            *pe0 += (int)(((long long)scaled * sy + 0x800) >> 12);
            cy = data_02082214[(yaw_u >> 4) * 2];
            *p1180 += cy * 0x82;
            *p1184 += 0x50000;
            sy = data_02082214[(yaw_u >> 4) * 2 + 1];
            *p1188 += sy * 0x82;
        }
        {
            char *b = r6 + (i << 9) + 0x300;
            Matrix4x3_FromTranslation(scratch, v.x, v.y, v.z);
            Matrix4x3_ApplyInPlaceToRotationY(
                scratch,
                (short)(*(short *)(r6 + 0x8e) + *(short *)(b + 0x4a)));
            Matrix4x3_ApplyInPlaceToRotationX(scratch, *(short *)(b + 0x48));
            Matrix4x3_ApplyInPlaceToRotationZ(scratch, *(short *)(b + 0x4c));
            *(Mtx *)(r6 + (i << 9) + 0x150) = *scratch;
        }
        _ZN18MovingMeshCollider9TransformERK9Matrix4x3s(
            m180, m150, *(short *)(r6 + 0x8e));
        m150 += 0x200;
        m180 += 0x200;
        r4++;
        i++;
    } while (i < 7);
}

extern "C" int _ZN6Dorrie8BehaviorEv(void *self)
{
    char *c = (char *)self;
    int d;
    int a2;
    int thr;

    if (*(unsigned char *)(c + 0x11b5) != 0)
        _Z14ApproachLinearRiii((int *)(c + 0x11ac), 0xa000, 0x1000);
    else
        _Z14ApproachLinearRiii((int *)(c + 0x11ac), 0, 0x1000);

    {
        /* the ROM's dispatch: data_ov065_0211d7fc[state], 8-byte records */
        unsigned fn =
            data_ov065_0211d7fc[*(unsigned char *)(c + 0x11b4) * 2];
        ((int (*)(char *))fn)(c);
    }

    *(int *)(c + 0x11a0) = Vec3_HorzDist(c + 0x5c, c + 0x1194);
    *(short *)(c + 0x11a4) = Vec3_HorzAngle(c + 0x5c, c + 0x1194);

    d = (short)AngleDiff(*(short *)(c + 0x11a4), *(short *)(c + 0x8e));

    {
        short sv = data_02082214[((unsigned short)d >> 4) * 2 + 1];
        short cv = data_02082214[(*(unsigned short *)(c + 0x548) >> 4) * 2];
        int sm = sv * 0x190;
        int t = (int)(((long long)sm * cv + 0x800) >> 12);
        if (d < 0x4000) {
            thr = t + 0x5f8000;
            a2 = 0x97c000;
        } else {
            thr = 0x5f8000;
            a2 = t + 0x97c000;
        }
    }

    if (*(int *)(c + 0x11a0) >= a2) {
        *(int *)(c + 0x5c) =
            *(int *)(c + 0x1194) -
            (int)(((long long)a2 *
                       data_02082214[(*(unsigned short *)(c + 0x11a4) >> 4) *
                                     2] +
                   0x800) >>
                  12);
        *(int *)(c + 0x64) =
            *(int *)(c + 0x119c) -
            (int)(((long long)a2 *
                       data_02082214[(*(unsigned short *)(c + 0x11a4) >> 4) *
                                         2 +
                                     1] +
                   0x800) >>
                  12);
    } else if (*(int *)(c + 0x11a0) <= thr) {
        *(int *)(c + 0x5c) =
            *(int *)(c + 0x1194) -
            (int)(((long long)thr *
                       data_02082214[(*(unsigned short *)(c + 0x11a4) >> 4) *
                                     2] +
                   0x800) >>
                  12);
        *(int *)(c + 0x64) =
            *(int *)(c + 0x119c) -
            (int)(((long long)thr *
                       data_02082214[(*(unsigned short *)(c + 0x11a4) >> 4) *
                                         2 +
                                     1] +
                   0x800) >>
                  12);
    }

    _ZN5Actor9UpdatePosEP12CylinderClsn(c, 0);
    WithMeshClsn_UpdateContinuous_Veneer(c + 0xf50);
    *(int *)(c + 0x60) =
        *(int *)(c + 0x1198) - *(int *)(c + 0x11ac) - *(int *)(c + 0x11a8);
    _ZN9Animation7AdvanceEv(c + 0x13c);
    func_ov065_02118cc4(c);
    func_ov065_02118838(c);
    func_ov065_02118248(c);
    *(unsigned char *)(c + 0x11b5) = 0;
    *(int *)(c + 0x118c) = 0;
    return 1;
}
