// Gate-10 smoke: Mario comes to his feet.
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

    if (g_failures) {
        fprintf(stderr, "smoke_player: %d FAILURE(S)\n", g_failures);
        return 1;
    }
    printf("smoke_player: construction chain complete (gate 10 phase 1)\n");
    return 0;
}
