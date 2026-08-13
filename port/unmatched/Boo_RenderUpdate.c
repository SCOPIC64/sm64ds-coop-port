/* HOST COPY of func_ov063_021166ac (ov063 0x021166ac, 0x318 bytes), the Boo
 * pack's post-behavior visual update: render matrix + opacity from the fade
 * byte (+0x5c8 >> 3), the bone-driven hat angles in the material-anim states,
 * the drop shadow (radius 0x12c000 for BigBoo id 0xd2, 0x64000 otherwise),
 * and CapEnemy::UpdateCapPos on the cap slot. Called only by Boo::Behavior
 * (Boo_Behavior.c) and by nothing else in the image (measured: no other src
 * reference). NOT matched in src/.
 *
 * PROVENANCE: written fresh against the annotated capstone disassembly of the
 * raw image bytes (extracted/overlays/overlay_0063.bin; ov063 is compressed
 * so the dsd export is noise) with every BL resolved through config relocs.
 * Verified with tools/abverify.py: EQUAL SIZE (198/198 words), 10
 * divergences -- register renames on the flag reads, an argument-setup order
 * swap before UpdateCapPos, and ldrh-vs-ldrsh on three halfwords that are
 * stored straight back to halfword slots (sign never observed). Verified
 * source + diff in runs/linkw/out/w5a/evidence/hostcopy_verified/.
 *
 * data_020a0e68 is the arm9 shared scratch Matrix4x3 every actor render path
 * uses; already hosted (walk_window.map). The 48-byte struct copies are the
 * ROM's ldm/stm triples.
 */
typedef int s32;
typedef short s16;
typedef long long s64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef struct { s32 x, y, z; } Vector3;
typedef struct { s32 m[12]; } Mtx43;

extern Mtx43 data_020a0e68;

extern void func_ov063_0211640c(void *c);
extern void Matrix4x3_FromRotationY(void *m, s16 ry);
extern void Vec3_Asr(Vector3 *dst, const Vector3 *src, s32 shift);
extern void Matrix4x3_FromTranslation(Mtx43 *m, s32 x, s32 y, s32 z);
extern void Matrix4x3_ApplyInPlaceToRotationY(Mtx43 *m, s16 ry);
extern void _ZN9ModelBase12ApplyOpacityEj(void *mb, u32 op, u32 b);
extern void func_020167a4(void *mb);
extern void _ZN15ModelComponents21UpdateVertsUsingBonesEv(void *mc);
extern void _ZN5Actor19DropShadowRadHeightER11ShadowModelR9Matrix4x35Fix12IiES5_j(
    void *c, void *shadow, void *m, s32 rad, s32 height, u32 op);
extern void _ZN8CapEnemy12UpdateCapPosERK7Vector3RK10Vector3_16(
    void *c, const Vector3 *pos, const void *ang);
extern void func_ov063_021160d4(void *c);

void func_ov063_021166ac(char *c)
{
    Vector3 t1;
    Vector3 t2;
    s16 hat[3];
    Vector3 cap;

    if ((*((u8 *) (c + 0x5cf))) == 0xf)
    {
        func_ov063_0211640c(c);
        return;
    }
    if ((((u32) ((*((u16 *) (c + 0x5d4))) << 0x1c)) >> 0x1f) == 0)
    {
        return;
    }
    if ((((u32) ((*((u16 *) (c + 0x5d4))) << 0x1e)) >> 0x1f) != 0)
    {
        if ((*((u16 *) (c + 0x4a0))) != 0xd4)
        {
            s16 *a = (s16 *) (c + 0x5ba);
            *a = (*a) + 0xc00;
        }
        Matrix4x3_FromRotationY(c + 0x400, *((s16 *) (c + 0x5ba)));
        *((s32 *) (c + 0x424)) = (*((s32 *) (c + 0x504))) >> 3;
        *((s32 *) (c + 0x428)) = (*((s32 *) (c + 0x508))) >> 3;
        *((s32 *) (c + 0x42c)) = (*((s32 *) (c + 0x50c))) >> 3;
    }
    if (((((*((u8 *) (c + 0x5cc))) == 3) || ((*((u8 *) (c + 0x5cc))) == 3)) || ((*((u8 *) (c + 0x5cc))) == 3)) || ((*((u8 *) (c + 0x5cc))) == 3))
    {
        char *bones;
        Vec3_Asr(&t1, (Vector3 *) (c + 0x5c), 3);
        Matrix4x3_FromTranslation(&data_020a0e68, t1.x, t1.y, t1.z);
        *((Mtx43 *) (c + 0x39c)) = data_020a0e68;
        _ZN9ModelBase12ApplyOpacityEj(c + 0x380, (u32) (((s32) (*((u8 *) (c + 0x5c8)))) >> 3) & 0xff, 1);
        func_020167a4(c + 0x380);
        bones = *((char **) (c + 0x390));
        *((s16 *) (bones + 0x1a)) = *((s16 *) (c + 0x8c));
        *((s16 *) (bones + 0x1c)) = (*((s16 *) (c + 0x8e))) - 0x4000;
        *((s16 *) (bones + 0x1e)) = *((s16 *) (c + 0x90));
        _ZN15ModelComponents21UpdateVertsUsingBonesEv(c + 0x388);
    }
    else
    {
        Vec3_Asr(&t2, (Vector3 *) (c + 0x5c), 3);
        Matrix4x3_FromTranslation(&data_020a0e68, t2.x, t2.y, t2.z);
        Matrix4x3_ApplyInPlaceToRotationY(&data_020a0e68, *((s16 *) (c + 0x8e)));
        *((Mtx43 *) (c + 0x39c)) = data_020a0e68;
        _ZN9ModelBase12ApplyOpacityEj(c + 0x380, (u32) (((s32) (*((u8 *) (c + 0x5c8)))) >> 3) & 0xff, 1);
    }
    Matrix4x3_FromTranslation(&data_020a0e68,
                              (*((s32 *) (c + 0x5c))) >> 3,
                              (*((s32 *) (c + 0x60))) >> 3,
                              (*((s32 *) (c + 0x64))) >> 3);
    *((Mtx43 *) (c + 0x4a4)) = data_020a0e68;
    if (((s32) ((*((u16 *) (c + 0xc))) == 0xd2)) != 0)
    {
        _ZN5Actor19DropShadowRadHeightER11ShadowModelR9Matrix4x35Fix12IiES5_j(
            c, c + 0x434, c + 0x4a4, 0x12c000, 0xc8000, 0xf);
    }
    else
    {
        _ZN5Actor19DropShadowRadHeightER11ShadowModelR9Matrix4x35Fix12IiES5_j(
            c, c + 0x434, c + 0x4a4, 0x64000, 0xc8000, 0xf);
    }
    *((s32 *) (c + 0x568)) = (*((s32 *) (c + 0x538)))
        + ((s32) ((((s64) (*((s32 *) (c + 0x80)))) * 0x60000 + 0x800) >> 0xc));
    cap.x = *((s32 *) (c + 0x564));
    cap.y = *((s32 *) (c + 0x568));
    cap.z = *((s32 *) (c + 0x56c));
    hat[0] = *((s16 *) (c + 0x8c));
    hat[1] = *((s16 *) (c + 0x8e));
    hat[2] = *((s16 *) (c + 0x90));
    _ZN8CapEnemy12UpdateCapPosERK7Vector3RK10Vector3_16(c, &cap, hat);
    func_ov063_021160d4(c);
}
