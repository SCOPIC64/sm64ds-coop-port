// Gate-10/11 smoke: Mario comes to his feet and walks.
//
// The Player ctor chains everything the earlier gates hosted -- Actor,
// two ModelAnims (body and head), TextureSequence/MaterialChanger arrays,
// ShadowModel (deferred), cylinder and mesh collision clients. This first
// cut proves the CONSTRUCTION chain and grows toward InitResources +
// St_Wait as the link closure lands (the gate-9 method).
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ntr/gx.h"
#include "ntr/mmio.h"

#include "fault_probe.h"

typedef unsigned int u32;

extern "C" {
void *_ZN6PlayerC1Ev(void *self);
void *_ZN4Heap13SetupRootHeapEv(void);
void *_ZN9ActorBasenwEj(unsigned size);
extern int data_0209b3ec[12];
extern unsigned short data_020a4b54;
extern void **data_020a4bb8;
extern void *data_020a0eac_c;
extern void *data_020a0ea0;
void hal_fill_model_vtable(void);
void hal_fill_shadow_vtable(void);
void hal_fill_mmc_vtable(void);
void hal_fill_modelanim2_vtable(void);
int hal_player_init_resources(void *p);
int hal_player_st_wait_init(void *p);
int hal_player_st_wait_main(void *p);
int hal_player_st_walk_main(void *p);
int hal_player_behavior(void *p);
extern char data_0209f4a0[];                 /* per-player pad blocks, 0x18 */
extern unsigned char data_020a0e40[];        /* current player index */
extern unsigned char data_ov002_0211013c[];  /* St_Walk state object */
extern short data_02092144[];                /* per-player health words */
extern unsigned char data_ov002_02110424[];  /* St_Fall state object */
void port_ov002_patch(void);         /* rehome DS-baked data pointers */
/* ov002 static ctors: SharedFilePtr IDs, state tables */
void __sinit_ov002_02100560(void);
void __sinit_ov002_02100938(void);
void __sinit_ov002_02100adc(void);
void __sinit_ov002_02100c50(void);
void __sinit_ov002_02100d44(void);
void __sinit_ov002_02100e50(void);
void __sinit_ov002_02100ec4(void);
void __sinit_ov002_02100f84(void);
void __sinit_ov002_02101064(void);
void __sinit_ov002_02101478(void);
void __sinit_ov002_021014e4(void);
void __sinit_ov002_02101588(void);
void __sinit_ov002_02101738(void);
void __sinit_ov002_02101894(void);
void __sinit_ov002_02101900(void);
void __sinit_ov002_02101968(void);
void __sinit_ov002_021019d0(void);
void __sinit_ov002_02106e40(void);
void __sinit_ov002_02107118(void);
void __sinit_ov002_021071f4(void);
void __sinit_ov002_02107298(void);
void __sinit_ov002_02107304(void);
void __sinit_ov002_02107370(void);
void __sinit_ov002_02107f88(void);
void __sinit_ov002_0210804c(void);
void __sinit_ov002_02108094(void);
}

static int g_failures;
#define CHECK(cond) \
    do { if (!(cond)) { ++g_failures; \
        fprintf(stderr, "FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); } } while (0)

static void ident_fx(void *m)
{
    memset(m, 0, 48);
    ((int *)m)[0] = ((int *)m)[4] = ((int *)m)[8] = 0x1000;
}

int main(void)
{
    PORT_INSTALL_FAULT_PROBE();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }
    CHECK(_ZN4Heap13SetupRootHeapEv() != NULL);
    ident_fx(data_0209b3ec);
    hal_fill_model_vtable();
    hal_fill_shadow_vtable();
    hal_fill_mmc_vtable();
    hal_fill_modelanim2_vtable();

    port_ov002_patch();
    /* the overlay's static ctors give every static SharedFilePtr its ID */
    __sinit_ov002_02100560();
    __sinit_ov002_02100938();
    __sinit_ov002_02100adc();
    __sinit_ov002_02100c50();
    __sinit_ov002_02100d44();
    __sinit_ov002_02100e50();
    __sinit_ov002_02100ec4();
    __sinit_ov002_02100f84();
    __sinit_ov002_02101064();
    __sinit_ov002_02101478();
    __sinit_ov002_021014e4();
    __sinit_ov002_02101588();
    __sinit_ov002_02101738();
    __sinit_ov002_02101894();
    __sinit_ov002_02101900();
    __sinit_ov002_02101968();
    __sinit_ov002_021019d0();
    __sinit_ov002_02106e40();
    __sinit_ov002_02107118();
    __sinit_ov002_021071f4();
    __sinit_ov002_02107298();
    __sinit_ov002_02107304();
    __sinit_ov002_02107370();
    __sinit_ov002_02107f88();
    __sinit_ov002_0210804c();
    __sinit_ov002_02108094();

    data_02092144[0] = 8 << 8;               /* full health, high byte */

    /* spawn context: Player is actor 0 in the spawn table */
    data_020a4b54 = 0;
    static unsigned short spawn_info[4] = { 0, 0, 100, 100 };
    data_020a4bb8[0] = spawn_info;
    data_020a0eac_c = data_020a0ea0;

    void *player = _ZN9ActorBasenwEj(0x800);
    CHECK(player != NULL);
    _ZN6PlayerC1Ev(player);
    printf("  player constructed at %p, vtable %p\n", player, *(void **)player);
    CHECK(*(void **)player != NULL);

    /* phase 2: resources + standing. The state machine is driven directly
       (ChangeState's PMF dispatch reads DS-baked bytes; see player_bridges) */
    int ir = hal_player_init_resources(player);
    printf("  InitResources -> %d\n", ir);
    CHECK(ir == 1);

    int wi = hal_player_st_wait_init(player);
    printf("  St_Wait_Init -> %d\n", wi);
    CHECK(wi == 1);
    for (int f = 0; f < 4; ++f) {
        int wm = hal_player_st_wait_main(player);
        printf("  St_Wait_Main frame %d -> %d\n", f, wm);
    }

    /* gate 11: a real floor first -- the castle grounds KCL through the
       gate-8 recipe, enabled in the global collision registry so the
       Player's ground probes find it */
    {
        static struct { unsigned short id; unsigned char refs; void *p; } kcl_ptr;
        extern void *_ZN13SharedFilePtr9ConstructEj(void *, unsigned);
        extern void _ZN12MeshColliderC1Ev(void *);
        extern void *_ZN12MeshCollider8LoadFileER13SharedFilePtr(void *);
        extern void _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(
            void *, void *, void *);
        extern int _ZN16MeshColliderBase6EnableEP5Actor(void *, void *);
        _ZN13SharedFilePtr9ConstructEj(&kcl_ptr, 1941);
        static char mc_storage[0x60];
        _ZN12MeshColliderC1Ev(mc_storage);
        char *kcl = (char *)_ZN12MeshCollider8LoadFileER13SharedFilePtr(&kcl_ptr);
        CHECK(kcl != NULL);
        static char clps_storage[0x100];
        _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(mc_storage, kcl,
                                                          clps_storage);
        _ZN16MeshColliderBase6EnableEP5Actor(mc_storage, player);
        /* stand Mario inside the octree box, above the floor plane */
        int ox = *(int *)(kcl + 0), oy = *(int *)(kcl + 4), oz = *(int *)(kcl + 8);
        unsigned xm = *(unsigned *)(kcl + 0x10), zm = *(unsigned *)(kcl + 0x18);
        int ex = (int)(~xm + 1) << 6, ez = (int)(~zm + 1) << 6;
        char *pc = (char *)player;
        *(int *)(pc + 0x5c) = ox + ex / 2;
        *(int *)(pc + 0x60) = oy + 0x8000;    /* 8 units up; he lands */
        *(int *)(pc + 0x64) = oz + ez / 2;
        printf("  floor staged: kcl origin (%d,%d,%d), mario at (%d,%d,%d)\n",
               ox, oy, oz, *(int *)(pc + 0x5c), *(int *)(pc + 0x60),
               *(int *)(pc + 0x64));
    }

    /* gate 11: stick forward, wait ticks into walk, position moves */
    char *c = (char *)player;
    data_020a0e40[0] = 0;
    *(short *)(data_0209f4a0 + 0) = 0x1000;      /* full stick magnitude */
    *(short *)(c + 0x69c) = 0x0000;              /* desired heading */
    int x0 = *(int *)(c + 0x5c), z0 = *(int *)(c + 0x64);
    hal_player_st_wait_main(player);             /* the tick that transitions */
    void *st = *(void **)(c + 0x370);
    printf("  after stick: state=%p (walk=%p)\n", st,
           (void *)data_ov002_0211013c);
    CHECK(st == (void *)data_ov002_0211013c);
    for (int f = 0; f < 30; ++f) {
        int r = hal_player_behavior(player);   /* the real frame tick */
        if (f < 3 || f == 29)
            printf("  Behavior frame %d -> %d pos=(%d, %d)\n", f, r,
                   *(int *)(c + 0x5c), *(int *)(c + 0x64));
    }
    int x1 = *(int *)(c + 0x5c), z1 = *(int *)(c + 0x64);
    printf("  walked: dx=%d dz=%d  y=%d  final state=%p (walk=%p fall=%p)\n",
           x1 - x0, z1 - z0, *(int *)(c + 0x60), *(void **)(c + 0x370),
           (void *)data_ov002_0211013c, (void *)data_ov002_02110424);
    CHECK(x1 != x0 || z1 != z0);

    if (g_failures) {
        fprintf(stderr, "smoke_player: %d FAILURE(S)\n", g_failures);
        return 1;
    }
    printf("smoke_player: Mario walks on the castle grounds "
           "(gates 10+11 GREEN)\n");
    return 0;
}
