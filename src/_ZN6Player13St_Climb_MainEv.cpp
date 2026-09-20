//cpp
typedef int s32;
typedef short s16;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long long s64;
typedef s32 Fix12;

struct Obj {
    virtual s32 v0();
    virtual s32 v1();
    virtual s32* v2();
};

extern "C" {
extern void Vec3_RotateYAndTranslate(s32* out, s32* in, s16 angle, s32* src);
extern void func_0200d5c0(void* thiz, u8 playerID);
extern void func_0200d5fc(void* thiz, s32 playerID);
extern void ApproachAngle(s16* cur, s16 target, s32 divisor, s32 band, s32 maxStep);
extern s32 _ZN6Player12FinishedAnimEv(void* c);
extern void _ZN6Player7SetAnimEji5Fix12IiEj(void* c, u32 anim, s32 a, Fix12 b, u32 d);
extern void _ZN6Player11ChangeStateERNS_5StateE(void* c, void* s);
extern void func_ov002_020cb354(void* c);
extern s16 GetAngleToCamera(u8 i);
extern void _Z14ApproachLinearRsss(s16* cur, s16 target, s16 step);
extern void func_ov002_020bf340(void* c, s32* p, s32 delta, s32 limit);
extern s32 _ZNK6Player14GetBodyModelIDEjb(void* c, u32 a, s32 b);
extern void func_ov002_020cb400(void* c);
extern s32 _ZN6Player6IsAnimEj(void* c, u32 anim);
extern void func_ov002_020cc05c(void* c, u16 flags);
extern void func_ov002_020cb474(void* c);
extern s32 func_ov002_020cbea8(void* c);
extern void Player_AdvanceAnims(void* c);

extern void* data_0209f318;
extern u8 data_020a0e40;
extern u16 data_0209f49e[];
extern u16 data_0209f49c[];
extern s16 data_02082214[];
extern int data_ov002_021101b4[];
extern s16 data_0209f4a4[];
extern int data_ov002_021106f4[];
extern u8 data_0209f4ac[];
extern int data_ov002_0211013c[];
extern int data_ov002_0211070c[];
}

extern "C" int _ZN6Player13St_Climb_MainEv(char* c)
{
    s32 vbuf[6];

    *(s32*)(c + 0x684) = *(s32*)(c + 0x60);

    {
        s32* p = (*(Obj**)(c + 0x37c))->v2();
        vbuf[0] = p[0];
        vbuf[1] = p[1];
        vbuf[2] = p[2];
    }
    if (*(s32*)(c + 8) == 2) {
        vbuf[4] = 0;
        vbuf[3] = 0;
        vbuf[5] = -0x10000;
        Vec3_RotateYAndTranslate(&vbuf[0], (*(Obj**)(c + 0x37c))->v2(), *(s16*)(c + 0x8e), &vbuf[3]);
    }
    *(s32*)(c + 0x5c) = vbuf[0];
    *(s32*)(c + 0x64) = vbuf[2];

    {
        void* cam = data_0209f318;
        if (*(u32*)((char*)(*(Obj**)(c + 0x37c)) + 0x18) & 0x1000000) {
            func_0200d5c0(cam, *(u8*)(c + 0x6d8));
        } else {
            s32 v = (s32)(((s64)(*(s32*)((char*)(*(Obj**)(c + 0x37c)) + 8)) * 0xb33 + 0x800) >> 12);
            if (v <= *(s32*)(c + 0x688)) {
                func_0200d5c0(cam, *(u8*)(c + 0x6d8));
            } else {
                func_0200d5fc(cam, *(u8*)(c + 0x6d8));
            }
        }
    }

    switch (*(u8*)(c + 0x6e3)) {
    case 0:
        ApproachAngle((s16*)(c + 0x69c), 0, 8, 0x2000, 0x200);
        if (_ZN6Player12FinishedAnimEv(c)) {
            _ZN6Player7SetAnimEji5Fix12IiEj(c, 0x26, 0, 0x1000, 0);
            *(u8*)(c + 0x6e3) = 1;
        }
        goto tail;

    case 1: {
        s16 v;
        u32 idx = (u32)data_020a0e40 * 0x18;
        u16 flagsE = *(u16*)((char*)data_0209f49e + idx);
        u16 flagsC = *(u16*)((char*)data_0209f49c + idx);
        if (flagsE & 0x400) {
            s16 ang = (s16)(*(s16*)(c + 0x8e) + 0x8000);
            s32 t = (u16)ang;
            s32 ti = (t >> 4) * 2;
            s16 cosv = data_02082214[ti];
            *(s32*)(((long long)(int)(c + 0x5c)) & 0xFFFFFFFFFFFFFFFFLL) += (s32)(((s64)cosv * 0x64000 + 0x800) >> 12);
            s16 sinv = data_02082214[ti + 1];
            *(s32*)(((long long)(int)(c + 0x64)) & 0xFFFFFFFFFFFFFFFFLL) += (s32)(((s64)sinv * 0x64000 + 0x800) >> 12);
            _ZN6Player11ChangeStateERNS_5StateE(c, data_ov002_021101b4);
            return 1;
        }

        {
            v = *(s16*)((char*)data_0209f4a4 + idx);
            if (v < -0x400) {
                func_ov002_020cb354(c);
                *(s16*)(c + 0x69c) = 0;
                _Z14ApproachLinearRsss((s16*)(c + 0x8e), GetAngleToCamera(*(u8*)(c + 0x6d8)), 0x200);
                if (*(s32*)((char*)(*(Obj**)(c + 0x37c)) + 8) <= *(s32*)(c + 0x688)) {
                    if ((*(u32*)((char*)(*(Obj**)(c + 0x37c)) + 0x18) & 0x2000000) &&
                        *(s32*)(c + 0xa8) == 0 &&
                        (s32)(*(s16*)((char*)data_0209f4a4 + (u32)data_020a0e40 * 0x18)) < -0xc00 &&
                        *(u8*)(c + 0x6e5) == 1) {
                        _ZN6Player11ChangeStateERNS_5StateE(c, data_ov002_021106f4);
                    }
                    *(s32*)(c + 0x688) = *(s32*)((char*)(*(Obj**)(c + 0x37c)) + 8);
                    *(s32*)(c + 0xa8) = 0;
                } else {
                    u32 idx2 = (u32)data_020a0e40 * 0x18;
                    s32 mag;
                    if (*((u8*)data_0209f4ac + idx2) == 0) {
                        mag = (*(u16*)((char*)data_0209f49c + idx2) & 0x800) ? 0xc000 : 0x6660;
                    } else {
                        mag = 0xc000;
                    }
                    s16 av = *(s16*)((char*)data_0209f4a4 + idx2);
                    if (av < 0) av = -av;
                    s32 fx = (s32)(((s64)mag * av + 0x800) >> 12);
                    func_ov002_020bf340(c, (s32*)(c + 0xa8), 0x800, fx);
                    _ZN6Player7SetAnimEji5Fix12IiEj(c, 0x24, 0, 0x1000, 0);
                    s32 fx2 = (s32)(((s64)fx * 0x500 + 0x800) >> 12);
                    if (fx2 >= 0x4000) fx2 = 0x4000;
                    {
                        s32 modelIdx = _ZNK6Player14GetBodyModelIDEjb(c, (u8)(*(s32*)(c + 8)), 0);
                        char* p2 = (char*)(((long long)(int)(*(char**)(c + modelIdx * 4 + 0xdc) + 0x50)) & 0xFFFFFFFFFFFFFFFFLL);
                        *(s32*)(p2 + 0xc) = fx2;
                    }
                    *(u8*)(c + 0x6e5) = 0;
                }
                goto end;
            } else if (v > 0x200) {
                func_ov002_020cb400(c);
                *(s16*)(c + 0x6a4) = 0x1e;
                {
                    u32 idx3 = (u32)data_020a0e40 * 0x18;
                    s32 mag;
                    if (*((u8*)data_0209f4ac + idx3) == 0) {
                        mag = (*(u16*)((char*)data_0209f49c + idx3) & 0x800) ? 0x1e000 : 0xfff0;
                    } else {
                        mag = 0x1e000;
                    }
                    s16 val = *(s16*)((char*)data_0209f4a4 + idx3);
                    s32 fx = (s32)(((s64)mag * val + 0x800) >> 12);
                    func_ov002_020bf340(c, (s32*)(c + 0xa8), -0x800, fx);
                }
                *(s16*)(((long long)(int)(c + 0x69c)) & 0xFFFFFFFFFFFFFFFFLL) -= 0x40;
                {
                    u32 idx4 = (u32)data_020a0e40 * 0x18;
                    s16 av = *(s16*)((char*)data_0209f4a4 + idx4);
                    if (av < 0) av = -av;
                    if (*(s16*)(c + 0x69c) <= av) av = *(s16*)(c + 0x69c);
                    *(s16*)(c + 0x69c) = av;
                }
                if (!_ZN6Player6IsAnimEj(c, 0x25)) {
                    _ZN6Player7SetAnimEji5Fix12IiEj(c, 0x25, 0, 0x1000, 0);
                }
                if (*(s32*)(c + 0x688) < -0x64000) {
                    _ZN6Player11ChangeStateERNS_5StateE(c, data_ov002_021101b4);
                    return 1;
                }
                goto end;
            } else {
                *(s16*)(c + 0x6a4) = 0x1e;
                *(s32*)(c + 0xa8) = 0;
                func_ov002_020cc05c(c, flagsC);
                if (!_ZN6Player6IsAnimEj(c, 0x26)) {
                    _ZN6Player7SetAnimEji5Fix12IiEj(c, 0x26, 0, 0x1000, 0);
                }
                goto end;
            }
        }
    }

    default:
        goto tail;
    }

end:
    if (*(u8*)(c + 0x6de) == 0) {
        _ZN6Player11ChangeStateERNS_5StateE(c, data_ov002_0211013c);
        return 1;
    }
    if (*(u16*)((char*)data_0209f49e + (u32)data_020a0e40 * 0x18) & 2) {
        *(s16*)(c + 0x94) = *(s16*)(c + 0x8e);
        _ZN6Player11ChangeStateERNS_5StateE(c, data_ov002_0211070c);
        return 1;
    }

tail:
    {
        s32* p3 = (*(Obj**)(c + 0x37c))->v2();
        s32 y = p3[1];
        if ((*(s32*)(c + 0x60) - y) < -0x64000) {
            *(s32*)(c + 0x60) = y - 0x64000;
        }
    }
    if (*(s32*)(c + 0x688) < -0x64000) {
        *(s32*)(c + 0x688) = -0x64000;
    }
    {
        s32 t2 = *(s32*)((char*)(*(Obj**)(c + 0x37c)) + 8);
        if (t2 <= *(s32*)(c + 0x688)) {
            *(s32*)(c + 0x688) = t2;
        }
    }
    if (*(u16*)(c + 0x6a8) != 0 || (*(s32*)(c + 0xa8) != 0 && *(u8*)(c + 0x6e3) == 1)) {
        func_ov002_020cb474(c);
    }
    {
        s16* p8e = (s16*)(((long long)(int)(c + 0x8e)) & 0xFFFFFFFFFFFFFFFFLL);
        *p8e = (s16)(*p8e + *(s16*)(c + 0x69c));
    }
    {
        s32* p688 = (s32*)(((long long)(int)(c + 0x688)) & 0xFFFFFFFFFFFFFFFFLL);
        *p688 += *(s32*)(c + 0xa8);
    }
    if (func_ov002_020cbea8(c)) {
        return 1;
    }
    {
        s32* p4 = (*(Obj**)(c + 0x37c))->v2();
        *(s32*)(c + 0x60) = *(s32*)(c + 0x688) + p4[1];
    }
    if (*(s16*)((char*)data_0209f4a4 + (u32)data_020a0e40 * 0x18) == 0) {
        *(u8*)(c + 0x6e5) = 1;
    }
    Player_AdvanceAnims(c);
    return 1;
}
