/* HOST COPY of SNUFIT's (actor 236, ov065, daYurei_Mucho_c) Behavior --
 * vtable slot 6, ROM body 0x02116b84.
 *
 * Transcribed line for line from src/_ZN6Snufit8BehaviorEv.cpp; the ONE
 * change is the state dispatch. The matched TU forms its PMF type over
 * `struct Enemy;` while Enemy is still forward-declared, which MSVC widens
 * to the general 16-byte representation: `Holder { char pad[8]; PMF fn; }`
 * then reads 16 bytes at +8 of an 8-byte-stride pair and dispatches through
 * garbage adjustment fields (the gate-173 measurement). The ROM reads ONE
 * word at table+8 -- the pair's per-frame fn -- and calls it with self.
 * The copy reads that word as a plain function pointer; the words are host
 * bodies because port_ov065_states_seat rewrote the SOURCE statics before
 * Snufit's sinit copied them (port/unmatched/Ov065_StateDispatch.cpp).
 *
 * C-named (extern "C" _ZN6Snufit8BehaviorEv) so the vtable thunk in
 * hal/actor_classes_ov065.cpp calls it directly; the matched TU stays out of
 * slice_w5b.txt with the reason recorded there (the honesty rule -- a
 * shadowed TU would sit in the slice looking seated).
 */
extern "C" {
int _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(void *thiz, void *c);
void _ZN12CylinderClsn5ClearEv(void *thiz);
void _ZN12CylinderClsn6UpdateEv(void *thiz);
int func_ov065_0211691c(char *c, unsigned *table);
int _ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
    void *thiz, void *wm, void *ma, unsigned j);
int ApproachAngle(short *target, short from, short start, short speed,
                  short max);
void _ZN5Enemy11UpdateDeathER12WithMeshClsn(void *thiz, void *wm);
unsigned short DecIfAbove0_Short(unsigned short *p);
void _Z14ApproachLinearRiii(int *x, int target, int step);
void _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(void *thiz,
                                                       void *clsn);
void _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *thiz, void *wm,
                                              unsigned j);
char *_ZN5Actor13ClosestPlayerEv(void *thiz);
void _ZN9Animation7AdvanceEv(void *thiz);
int func_ov065_0211696c(char *c);
void func_ov065_02115ff0(char *c);

extern short data_02082214[];
extern int data_ov065_0211d670[];
extern int data_ov065_0211d650[];
extern int data_ov065_0211d660[];
}

/* PORT_HOST_ABI: mwcc pointer-to-member read at table+8 over the
   incomplete Enemy -- the gate-173 widening, measured. */
extern "C" int _ZN6Snufit8BehaviorEv(void *self)
{
    char *c = (char *)self;
    if (_ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(self, c + 0x144) != 0) {
        _ZN12CylinderClsn5ClearEv(c + 0x110);
        if (*(unsigned char *)(c + 0x107) != 0) {
            if (*(unsigned short *)(c + 0x104) == 0) {
                _ZN12CylinderClsn6UpdateEv(c + 0x110);
            }
        }
        func_ov065_0211696c(c);
        *(int *)(c + 0x3cc) = *(int *)(c + 0x5c);
        *(int *)(c + 0x3d0) = *(int *)(c + 0x60);
        *(int *)(c + 0x3d4) = *(int *)(c + 0x64);
        func_ov065_0211691c(c, (unsigned *)data_ov065_0211d670);
        return 1;
    }
    if (_ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
            self, c + 0x144, c + 0x300, 3) != 0) {
        return 1;
    }
    if (*(int *)(c + 0x10c) != 0) {
        ApproachAngle((short *)(c + 0x8c), -0x4000, 0xa, 0x200, 0x100);
        _ZN5Enemy11UpdateDeathER12WithMeshClsn(self, c + 0x144);
        func_ov065_0211696c(c);
        return 1;
    }
    DecIfAbove0_Short((unsigned short *)(c + 0x100));
    {
        /* the ROM's dispatch: one word at table+8, the per-frame fn */
        unsigned *q = *(unsigned **)(c + 0x3bc);
        if (q[2] != 0) {
            ((int (*)(char *))q[2])(c);
        }
    }
    {
        int v = *(int *)(c + 0xa8) + *(int *)(c + 0x9c);
        int hi = *(int *)(c + 0xa0);
        if (v >= hi) {
            hi = v;
        }
        int tmp = *(int *)(c + 0xac);
        *(int *)(c + 0xa8) = hi;
        *(int *)(c + 0xac) = tmp;
    }
    if (*(int **)(c + 0x3bc) != data_ov065_0211d650) {
        int *p3d8;
        int ang;
        int idx;
        short tbl;
        int result;
        p3d8 = (int *)(c + 0x3d8);
        *p3d8 += 0x200;
        ang = *(int *)(c + 0x3d8);
        idx = ((unsigned short)(short)ang >> 4) * 2;
        tbl = data_02082214[idx];
        result = (int)(((long long)tbl * 0x46000 + 0x800) >> 12);
        _Z14ApproachLinearRiii((int *)(void *)(c + 0x60),
                               *(int *)(c + 0x3d0) + (result + 0xb4000),
                               0x3000);
    }
    _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(self, c + 0x110);
    _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(self, c + 0x144, 0);
    func_ov065_0211696c(c);
    if (*(int **)(c + 0x3bc) != data_ov065_0211d660) {
        *(short *)(c + 0x8c) = *(short *)(c + 0x92);
        *(short *)(c + 0x8e) = *(short *)(c + 0x94);
        *(short *)(c + 0x90) = *(short *)(c + 0x96);
        func_ov065_02115ff0(c);
    }
    _ZN12CylinderClsn5ClearEv(c + 0x110);
    {
        char *p = _ZN5Actor13ClosestPlayerEv(self);
        if (p != 0 && *(unsigned char *)(p + 0x6fb) == 0) {
            _ZN12CylinderClsn6UpdateEv(c + 0x110);
        }
    }
    _ZN9Animation7AdvanceEv(c + 0x350);
    return 1;
}
