/* HOST COPY of _ZN12MansionSteps13InitResourcesEv (ov063 0x0211cf00, 0x370
 * bytes), vtable slot 0 of _ZTV12MansionSteps -- the ONE table all four
 * furniture ids share (65 MansionSteps / 158 Bookshelf / 159 MerryGoRound /
 * 160 TrapDoor; each Spawn stores this table, mode picked off the actor id
 * at +0xc). NOT matched in src/ (the other six slots all are).
 *
 * PROVENANCE: the near-miss DB draft for 0x0211cf00, verified UNCHANGED with
 * tools/abverify.py: equal size (220/220 words), 6 divergences, all one
 * hoisted pool load reordered against the same stores -- instruction
 * scheduling, zero semantic surface. Verified source + diff in
 * runs/linkw/out/w5a/evidence/hostcopy_verified/.
 *
 * TWO RUNTIME SPAWNS THE REGISTRY MUST COVER, read off this body:
 *   - id 0x41 (65) mode 0 spawns TWO more id-65 actors (the staircase
 *     segments) and writes sp+0x128 / sp+0x156 through the result UNCHECKED;
 *   - mode 2 (the Bookshelf, id 0x9e) spawns THREE id 0xd5 (213, BookSwitch,
 *     ov020 SpawnInfo 0x02114898 -- the library-puzzle books) and writes
 *     sp+0x418 UNCHECKED.
 * Registering 65/158 without 65-self and 213 in the table is a null+offset
 * write on level 12's boot; both rows ride this lane.
 */
typedef int s32;
typedef unsigned int u32;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned char u8;
typedef signed char s8;
enum { false, true };

struct Vec3 { int x, y, z; };
struct Vec16 { s16 x, y, z; };

extern void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(u32 a, u32 b, const void *v, const void *v16, int e, int f);
extern void *_ZN12MeshCollider8LoadFileER13SharedFilePtr(void *fp);
extern void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp);
extern int _ZN9ModelBase7SetFileEP8BMD_Fileii(void *self, void *f, int a, int b);
extern void func_ov063_0211c6f8(char *c);
extern void _ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
    void *self, void *f, void *m, int fx, short s, void *b);
extern void func_020393d4(int *p, int v);
extern void func_020393c4(int *p, int v);
extern void _ZN16MeshColliderBase6EnableEP5Actor(void *self, void *actor);

extern void _ZN16MeshColliderBase16UpdatePosAndAngsERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_(void);
extern void func_ov063_0211d28c(void);
extern void *data_ov063_0211e28c[];
extern void *data_ov063_0211e27c[];
extern int data_ov063_0211e9e8[];
extern int data_ov063_0211e9f8[];

int _ZN12MansionSteps13InitResourcesEv(char *c)
{
    int is41;
    int t;
    u16 actorType;
    s32 param;
    s32 dx;
    int i;
    int bit;
    struct Vec3 pos;
    void *sp;

    param = *(s32 *)(c + 8);
    *(u8 *)(c + 0x151) = ((u32)param >> 8) & 3;
    *(s32 *)(c + 0x128) = 0;
    is41 = 1;
    *(u8 *)(c + 0x156) = is41;
    actorType = *(u16 *)(c + 0xc);
    if (actorType != 0x41)
        is41 = 0;
    if (is41 != false) {
        *(s32 *)(c + 0x140) = 0;
        if (*(u8 *)(c + 0x151) == 0) {
            bit = *(s32 *)(c + 8) & 1;
            dx = (bit ^ 1) * 0xc8000;
            if (bit != false)
                *(u8 *)(c + 0x156) = 0;
            *(s32 *)(c + 0x60) = *(s32 *)(c + 0x60) - 0x258000 + dx;
            pos.x = *(s32 *)(c + 0x5c);
            pos.y = *(s32 *)(c + 0x60);
            pos.z = *(s32 *)(c + 0x64);
            for (i = 1; i <= 2; i++) {
                pos.z -= 0xc8000;
                pos.y += dx;
                sp = _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(0x41, 0x100 * i,
                    &pos, 0, *(s8 *)(c + 0xcc), -1);
                *(s32 *)((char *)sp + 0x128) = *(s32 *)(c + 4);
                *(u8 *)((char *)sp + 0x156) = *(u8 *)(c + 0x156);
            }
        }
    } else {
        t = (int)(actorType == 0xa0);
        if (t != false) {
            *(s32 *)(c + 0x140) = 1;
        } else {
            t = (int)(actorType == 0x9e);
            if (t != false) {
                *(s32 *)(c + 0x140) = 2;
            } else {
                *(s32 *)(c + 0x140) = 3;
            }
        }
    }

    *(s32 *)(c + 0x12c) = *(s32 *)(c + 0x5c);
    *(s32 *)(c + 0x130) = *(s32 *)(c + 0x60);
    *(s32 *)(c + 0x134) = *(s32 *)(c + 0x64);
    *(u8 *)(c + 0x150) = 0;
    *(s32 *)(c + 0x124) = 0;
    *(u8 *)(c + 0x152) = 0;
    *(u8 *)(c + 0x153) = 0;
    *(u8 *)(c + 0x154) = 0;
    *(u8 *)(c + 0x155) = 0;
    *(s32 *)(c + 0x148) = 0;
    *(s16 *)(c + 0x14e) = 0;

    _ZN12MeshCollider8LoadFileER13SharedFilePtr(data_ov063_0211e28c[*(s32 *)(c + 0x140)]);
    if (_ZN9ModelBase7SetFileEP8BMD_Fileii(c + 0xd4,
            _ZN5Model8LoadFileER13SharedFilePtr(data_ov063_0211e27c[*(s32 *)(c + 0x140)]), 1, -1) == 0)
        return 0;

    func_ov063_0211c6f8(c);

    {
        int m = *(s32 *)(c + 0x140);
        if (m == 3) {
            _ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
                c + 0x15c, *(void **)((char *)data_ov063_0211e28c[m] + 4), c + 0x324,
                0x1000, *(s16 *)(c + 0x8e), (void *)data_ov063_0211e9e8[m]);
        } else {
            _ZN18MovingMeshCollider7SetFileEP8KCL_FileRK9Matrix4x35Fix12IiEsR10CLPS_Block(
                c + 0x15c, *(void **)((char *)data_ov063_0211e28c[m] + 4), c + 0x324,
                0x199, *(s16 *)(c + 0x8e), (void *)data_ov063_0211e9e8[m]);
        }
    }

    func_020393d4((int *)(c + 0x15c),
        (int)&_ZN16MeshColliderBase16UpdatePosAndAngsERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_);
    func_020393c4((int *)(c + 0x15c), (int)&func_ov063_0211d28c);
    _ZN16MeshColliderBase6EnableEP5Actor(c + 0x15c, c);

    if (*(s32 *)(c + 0x140) == 2) {
        struct Vec3 base; struct Vec16 v16;
        int j, even, odd, actorId, neg1;
        volatile int *vt;
        base.x = *(s32 *)(c + 0x12c);
        base.y = *(s32 *)(c + 0x130);
        j = 0;
        base.z = *(s32 *)(c + 0x134);
        v16.y = -0x8000;
        even = j;
        v16.x = 0;
        v16.z = 0;
        odd = 1;
        actorId = 0xd5;
        neg1 = -1;
        vt = data_ov063_0211e9f8;
        for (; j < 3; j++) {
            base.x = *(s32 *)(c + 0x12c) - vt[even];
            base.y = *(s32 *)(c + 0x130) + vt[odd];
            base.z = *(s32 *)(c + 0x134) - 0xb4000;
            sp = _ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(actorId, j, &base, &v16, *(s8 *)(c + 0xcc), neg1);
            *(s32 *)((char *)sp + 0x418) = *(s32 *)(c + 4);
            even += 2; odd += 2;
        }
        *(u8 *)(c + 0x158) = *(s8 *)(c + 0xcc);
    }
    *(u8 *)(c + 0x157) = 0;
    *(s16 *)(c + 0x138) = 0;
    *(s16 *)(c + 0x13a) = 0;
    *(s16 *)(c + 0x13c) = 0;
    return 1;
}
