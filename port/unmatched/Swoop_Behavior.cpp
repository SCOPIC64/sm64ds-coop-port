/* HOST COPY of SWOOP's (actor 237, ov065, daBasabasa_c) Behavior -- vtable
 * slot 6, ROM body 0x02117b64.
 *
 * Transcribed line for line from src/_ZN5Swoop8BehaviorEv.cpp; the one
 * change is the state dispatch, for the reason Snufit_Behavior.cpp's banner
 * gives (incomplete-class PMF widening; the ROM reads one word at table+8).
 * Swoop keeps its state-pair pointer at +0x420 and its per-frame fn words
 * were seated by port_ov065_states_seat before Swoop's sinit copied them.
 */
extern "C" {
int _ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(void *thiz, void *c);
void _ZN12CylinderClsn5ClearEv(void *thiz);
void _ZN12CylinderClsn6UpdateEv(void *thiz);
int _ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
    void *thiz, void *wm, void *ma, unsigned j);
void _ZN5Enemy11UpdateDeathER12WithMeshClsn(void *thiz, void *wm);
unsigned short DecIfAbove0_Short(unsigned short *p);
void func_02012694(int, void *);
void _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(void *thiz,
                                                       void *clsn);
void _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(void *thiz, void *wm,
                                              unsigned j);
char *_ZN5Actor13ClosestPlayerEv(void *thiz);
void _ZN9Animation7AdvanceEv(void *thiz);
int _ZNK9Animation12WillHitFrameEi(void *thiz, int frame);
int func_ov065_02117994(char *c);
int func_ov065_0211704c(char *c);

extern char data_ov065_0211d6e0[];
extern char data_ov065_0211d6f0[];
}

/* PORT_HOST_ABI: mwcc pointer-to-member read at table+8 over the
   incomplete Enemy -- the gate-173 widening, measured. */
extern "C" int _ZN5Swoop8BehaviorEv(void *self)
{
    char *c = (char *)self;
    if (_ZN5Enemy14UpdateYoshiEatER12WithMeshClsn(self, c + 0x144) != 0) {
        _ZN12CylinderClsn5ClearEv(c + 0x110);
        if (*(unsigned char *)(c + 0x107) != 0) {
            if (*(unsigned short *)(c + 0x104) == 0) {
                _ZN12CylinderClsn6UpdateEv(c + 0x110);
            }
        }
        func_ov065_02117994(c);
        return 1;
    }
    if (_ZN5Enemy26UpdateKillByInvincibleCharER12WithMeshClsnR9ModelAnimj(
            self, c + 0x144, c + 0x300, 3) != 0) {
        return 1;
    }
    if (*(int *)(c + 0x10c) != 0) {
        _ZN5Enemy11UpdateDeathER12WithMeshClsn(self, c + 0x144);
        func_ov065_02117994(c);
        return 1;
    }
    DecIfAbove0_Short((unsigned short *)(c + 0x100));
    {
        /* the ROM's dispatch: one word at table+8, the per-frame fn */
        unsigned *q = *(unsigned **)(c + 0x420);
        if (q[2] != 0)
            ((int (*)(char *))q[2])(c);
    }
    {
        char *m = *(char **)(c + 0x420);
        if (m == data_ov065_0211d6e0 || m == data_ov065_0211d6f0) {
            if (_ZNK9Animation12WillHitFrameEi(c + 0x350, 3) != 0 ||
                _ZNK9Animation12WillHitFrameEi(c + 0x350, 0xf) != 0 ||
                _ZNK9Animation12WillHitFrameEi(c + 0x350, 0x1b) != 0) {
                func_02012694(0xe1, c + 0x74);
            }
        }
    }
    {
        int v = *(int *)(c + 0xa8) + *(int *)(c + 0x9c);
        int hi = *(int *)(c + 0xa0);
        if (v >= hi)
            hi = v;
        int tmp = *(int *)(c + 0xac);
        *(int *)(c + 0xa8) = hi;
        *(int *)(c + 0xac) = tmp;
    }
    _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(self, c + 0x110);
    _ZN5Enemy12UpdateWMClsnER12WithMeshClsnj(self, c + 0x144, 0);
    *(short *)(c + 0x8c) = *(short *)(c + 0x92);
    *(short *)(c + 0x8e) = *(short *)(c + 0x94);
    *(short *)(c + 0x90) = *(short *)(c + 0x96);
    func_ov065_02117994(c);
    if (*(unsigned char *)(c + 0x43c) == 1) {
        func_ov065_0211704c(c);
    }
    _ZN12CylinderClsn5ClearEv(c + 0x110);
    {
        char *p = _ZN5Actor13ClosestPlayerEv(self);
        if (p != 0 && *(unsigned char *)(p + 0x6fb) == 0) {
            _ZN12CylinderClsn6UpdateEv(c + 0x110);
        }
    }
    if (*(unsigned char *)(c + 0x43c) == 1) {
        _ZN9Animation7AdvanceEv(c + 0x350);
    } else {
        _ZN9Animation7AdvanceEv(c + 0x3b4);
    }
    return 1;
}
