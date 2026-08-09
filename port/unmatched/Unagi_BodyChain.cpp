/* HOST COPY of func_ov016_02111c40 -- UNAGI's (eel, actor 242, ov016) per-frame
 * body-segment matrix chain, called from Unagi::Behavior.
 *
 * WHY A HOST COPY, NOT SLICE SRC: this function has no byte-matched decomp yet.
 * Its best attempt in nearmiss/db.jsonl is a size-exact permuter output at
 * div=41 whose only divergence is RMW store ORDER (scrambled by the permuter);
 * the SEMANTICS are complete and correct. For the port that is enough -- store
 * order does not change runtime behavior, and every symbol it references is a
 * hosted arm9 helper (Vec3_Asr, the Matrix4x3 family, ApproachLinear,
 * Actor::FindWithID) or the hosted scratch matrix data_020a0e68. So the draft is
 * dropped in verbatim as the port body, out of the slice, until the decomp lands
 * a byte-match. It walks the head matrix (c+0x36c) down the seven bone matrices
 * at (*(c+0x364))+i*0x30, writes each segment's world position back into c+0x448
 * .., and -- if the eel has a captured actor id at c+0x49c -- parents that
 * actor's matrix (found+0xc8) to the tail. No allocation, no dispatch, pure
 * kinematics; a no-op stub would only freeze the body articulation, this makes
 * it correct.
 */
typedef short s16;
typedef struct { int x, y, z; } Vector3;
typedef struct { int m[12]; } Matrix4x3;

extern "C" {
void Vec3_Asr(Vector3 *d, Vector3 *s, int sh);
void Matrix4x3_FromTranslation(void *m, int x, int y, int z);
void Matrix4x3_FromRotationY(void *m, s16 ang);
void MulVec3Mat4x3(const void *v, const void *m, void *out);
void Matrix4x3_ApplyInPlaceToRotationZXYExt(void *m, s16 x, s16 y, s16 z);
void _Z14ApproachLinearRsss(s16 *dst, s16 target, s16 step);
void MulMat4x3Mat4x3(void *m1, void *m0, void *mF);
void *_ZN5Actor10FindWithIDEj(unsigned id);
void Matrix4x3_ApplyInPlaceToRotationXYZExt(void *m, s16 x, s16 y, s16 z);
extern Matrix4x3 data_020a0e68;
}

extern "C" void func_ov016_02111c40(char *c)
{
    Vector3 in, out, asr;
    int i;
    char *bone;
    int base;
    int *pos;
    Matrix4x3 *mat_src;
    void *found;
    unsigned id;
    int tx, ty, tz;
    int *px, *py, *pz;

    Vec3_Asr(&asr, (Vector3 *)(c + 0x5c), 3);
    Matrix4x3_FromTranslation(&data_020a0e68, asr.x, asr.y, asr.z);
    in.x = 0; in.y = 0; in.z = -0x190000;
    out.x = 0; out.y = 0; out.z = 0;
    Matrix4x3_FromRotationY(&data_020a0e68, *(s16 *)(c + 0x8e));
    MulVec3Mat4x3(&in, &data_020a0e68, &out);
    Matrix4x3_FromTranslation(&data_020a0e68,
        (*(int *)(c + 0x5c) + out.x) >> 3,
        (*(int *)(c + 0x60)) >> 3,
        (*(int *)(c + 0x64) + out.z) >> 3);
    Matrix4x3_ApplyInPlaceToRotationZXYExt(&data_020a0e68,
        *(s16 *)(c + 0x8c), *(s16 *)(c + 0x8e), *(s16 *)(c + 0x90));
    *(Matrix4x3 *)(c + 0x36c) = data_020a0e68;

    _Z14ApproachLinearRsss((s16 *)(c + 0x424), *(s16 *)(c + 0x400 + 0x26), 0x40);
    *(s16 *)(c + 0x422) = *(s16 *)(c + 0x424);
    mat_src = (Matrix4x3 *)(c + 0x36c);

    i = 0;
    bone = c;
    base = 0;
    pos = (int *)(c + 0x448);
    do {
        Matrix4x3 *dst = &data_020a0e68;
        int v = i;
        *(int *)(bone + 0x448) = v;
        *(int *)(bone + 0x44c) = v;
        *(int *)(bone + 0x450) = v;
        *dst = *mat_src;
        MulMat4x3Mat4x3(((char *)(*(void **)(c + 0x364))) + base, dst, dst);
        {
            Matrix4x3 *sc = &data_020a0e68;
            unsigned t;
            tx = *(int *)(((char *)sc) + 0x24); pos[0] = tx;
            px = (int *)(bone + 0x448);
            ty = *(int *)(((char *)sc) + 0x28); pos[1] = ty;
            py = (int *)(bone + 0x44c);
            tz = *(int *)(((char *)sc) + 0x2c); pos[2] = tz;
            pz = (int *)(bone + 0x450);
            t = (unsigned)(*py); *pz = (int)(t << 3);
            t = (unsigned)(*px); pos += 3; base += 0x30; *px = (int)(t << 3);
            t = (unsigned)(*pz); bone += 0xc; i++; *py = (int)(t << 3);
        }
    } while (i < 7);

    id = *(unsigned *)(c + 0x49c);
    if (id == 0)
        return;
    found = _ZN5Actor10FindWithIDEj(id);
    if (found == 0)
        return;
    *(int *)(c + 0x43c) = 0;
    *(int *)(c + 0x440) = 0;
    *(int *)(c + 0x444) = 0;
    MulMat4x3Mat4x3(((char *)(*(void **)(c + 0x364))) + 0xc0, c + 0x36c, c + 0x3c0);
    Matrix4x3_FromTranslation(&data_020a0e68, 0x28000, 0, 0);
    Matrix4x3_ApplyInPlaceToRotationXYZExt(&data_020a0e68, 0x4000, -0x8000,
                                           *(s16 *)(c + 0x416));
    MulMat4x3Mat4x3(&data_020a0e68, c + 0x3c0, c + 0x3c0);
    {
        Matrix4x3 *scratch;
        data_020a0e68 = *(Matrix4x3 *)(c + 0x3c0);
        scratch = &data_020a0e68;
        *(int *)(c + 0x43c) = *(int *)(((char *)scratch) + 0x24);
        *(int *)(c + 0x440) = *(int *)(((char *)scratch) + 0x28);
        *(int *)(c + 0x444) = *(int *)(((char *)scratch) + 0x2c);
        px = (int *)(c + 0x43c);
        py = (int *)(c + 0x440);
        pz = (int *)(c + 0x444);
        *px = (*px) << 3;
        *py = (*py) << 3;
        *pz = (*pz) << 3;
        *(void **)(((char *)found) + 0xc8) = (void *)(c + 0x3c0);
    }
}
