// Actor-hierarchy vtables (gate 9), per the vtable law (clsn_vtable.cpp):
// MSVC slot order. include/ActorBase.h declares the dtor LAST, so MSVC and
// the ROM agree on slots 0..15 and diverge only at the tail -- the header
// was built for exactly this. Lifecycle slots forward to the class's own
// overrides where they exist and to the ActorBase/Actor defaults where
// they do not; the tail traps.
//
// Base-class vtable symbols the ctor chain installs and then overwrites
// (never dispatched between installs) are plain storage.
#include <stdio.h>
#include <stdlib.h>

#include "ActorBase.h"
#include "ArrowSignRight.h"

// The lifecycle definitions are MSVC methods (ArrowSignRight.h/ActorBase.h
// real classes); InitResources alone is a C-named free function. Every shim
// calls QUALIFIED -- never virtual.
extern "C" int _ZN14ArrowSignRight13InitResourcesEv(char *self);

static int __fastcall sl_init(void *self, void *)
{ return _ZN14ArrowSignRight13InitResourcesEv((char *)self); }
static int __fastcall sl_cleanup(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::CleanupResources(); }
static int __fastcall sl_behavior(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::Behavior(); }
static int __fastcall sl_render(void *self, void *)
{ return ((ArrowSignRight *)self)->ArrowSignRight::Render(); }
static int __fastcall sl_binit(void *self, void *)
{ return ((ActorBase *)self)->ActorBase::BeforeInitResources(); }
static void __fastcall sl_ainit(void *self, void *, u32 a)
{ ((ActorBase *)self)->ActorBase::AfterInitResources(a); }
static int __fastcall sl_bclean(void *self, void *)
{ return ((ActorBase *)self)->ActorBase::BeforeCleanupResources(); }
static void __fastcall sl_aclean(void *self, void *, u32 a)
{ ((ActorBase *)self)->ActorBase::AfterCleanupResources(a); }
static int __fastcall sl_bbeh(void *self, void *)
{ return ((ActorBase *)self)->ActorBase::BeforeBehavior(); }
static void __fastcall sl_abeh(void *self, void *, u32 a)
{ ((ActorBase *)self)->ActorBase::AfterBehavior(a); }
static int __fastcall sl_bren(void *self, void *)
{ return ((ActorBase *)self)->ActorBase::BeforeRender(); }
static void __fastcall sl_aren(void *self, void *, u32 a)
{ ((ActorBase *)self)->ActorBase::AfterRender(a); }
static int __fastcall sl_pdes(void *self, void *)
{ ((ActorBase *)self)->ActorBase::OnPendingDestroy(); return 0; }
static int __fastcall sl_heap(void *self, void *)
{ return ((ActorBase *)self)->ActorBase::OnHeapCreated(); }

#define ATRAP(n) \
    static void __fastcall a_trap##n(void *, void *) { \
        fprintf(stderr, "FATAL: ArrowSignRight vtable slot %d trap\n", n); \
        abort(); }
ATRAP(13) ATRAP(14) ATRAP(16) ATRAP(17)

extern "C" void *_ZTV14ArrowSignRight[20] = {
    (void *)sl_init,     /* 0  InitResources */
    (void *)sl_binit,    /* 1  BeforeInitResources */
    (void *)sl_ainit,    /* 2  AfterInitResources */
    (void *)sl_cleanup,  /* 3  CleanupResources */
    (void *)sl_bclean,   /* 4  BeforeCleanupResources */
    (void *)sl_aclean,   /* 5  AfterCleanupResources */
    (void *)sl_behavior, /* 6  Behavior */
    (void *)sl_bbeh,     /* 7  BeforeBehavior */
    (void *)sl_abeh,     /* 8  AfterBehavior */
    (void *)sl_render,   /* 9  Render */
    (void *)sl_bren,     /* 10 BeforeRender */
    (void *)sl_aren,     /* 11 AfterRender */
    (void *)sl_pdes,     /* 12 OnPendingDestroy */
    (void *)a_trap13, (void *)a_trap14,
    (void *)sl_heap,     /* 15 OnHeapCreated */
    (void *)a_trap16, (void *)a_trap17,
    0, 0,
};

// Base vtables the ctor chain installs transiently: storage only.
extern "C" {
void *_ZTV17ExclamationSwitch[20];
int data_0208e4b8[20];   /* ActorBase-era vtable-ish install in Actor ctor */
int data_0208e3a4[20];
}

// ---- ActorBase::ActorBase() transcription ---------------------------------
// The ROM ctor is a hand-asm block (src/_ZN9ActorBaseC1Ev.cpp); this is its
// C transcription, field for field against the disassembly there. The spawn
// CONTEXT globals it reads (pending actor ID, area byte, the spawn-info
// pointer table for the two processing-list priorities) are storage here;
// the smoke seeds them the way func_02010e78/ActorDerived::Spawn would.
extern "C" {
void _ZN9ActorBase9SceneNodeC1Ev(void *node);
int func_0203b438(void *a, void *b, void *c);
int func_02043810(void *p);

int data_02099edc[8];           /* the transient ActorBase vtable install */
int data_02099e70[1];           /* next unique actor id */
int data_020a4b60[1];
unsigned short data_020a4b54;   /* PENDING ACTOR ID (the spawn context) */
unsigned char data_020a4b48;    /* pending area byte */
int data_020a4b64[1];
int data_020a4b6c[8];           /* the scene tree root the ctor links into */
void *data_020a4bb8_storage[512];
void **data_020a4bb8 = data_020a4bb8_storage;  /* actorID -> SpawnInfo* */

void *_ZN9ActorBaseC1Ev(char *self)
{
    *(void **)self = data_02099edc;
    _ZN9ActorBase9SceneNodeC1Ev(self + 0x14);
    *(void **)(self + 0x24) = self;             /* sceneNode.actor */
    for (int off = 0x28; off <= 0x38; off += 0x10) {
        *(void **)(self + off) = 0;
        *(void **)(self + off + 4) = 0;
        *(void **)(self + off + 8) = self;
        *(unsigned short *)(self + off + 0xc) = 0;
        *(unsigned short *)(self + off + 0xe) = 0;
    }
    int id = data_02099e70[0];
    *(int *)(self + 4) = id;
    data_02099e70[0] = id + 1;
    *(int *)(self + 8) = data_020a4b60[0];
    *(unsigned short *)(self + 0xc) = data_020a4b54;
    *(unsigned char *)(self + 0x12) = data_020a4b48;
    func_0203b438(data_020a4b6c, self + 0x14, (void *)(size_t)data_020a4b64[0]);
    {
        unsigned short *info = (unsigned short *)data_020a4bb8[
            *(unsigned short *)(self + 0xc)];
        *(unsigned short *)(self + 0x28 + 0xc) = info[2];   /* prio at +4 */
        *(unsigned short *)(self + 0x28 + 0xe) = info[2];
        *(unsigned short *)(self + 0x38 + 0xc) = info[3];   /* prio at +6 */
        *(unsigned short *)(self + 0x38 + 0xe) = info[3];
    }
    {
        char *parent = (char *)(size_t)func_02043810(data_020a4b6c);
        if (parent) {
            unsigned char pf = *(unsigned char *)(parent + 0x13);
            if (pf & 3)
                *(unsigned char *)(self + 0x13) |= 2;
            if (pf & 0xC)
                *(unsigned char *)(self + 0x13) |= 8;
        }
    }
    return self;
}
} /* extern "C" */

// ---- gate-9 storage and bridges -------------------------------------------
extern "C" {
void *_ZTV11ShadowModel[8];

// ov098's SharedFilePtr entry table for the arrow signs: three-pointer
// entries {model, kcl, ?}. The smoke seeds the pointers with its own
// SharedFilePtr objects (on DS these live prebuilt in the overlay's data).
void *data_ov098_0213c380[6];
} /* extern "C" */

// The extern "C" side of the cxxname bridges (see hal/cxxname_bridge.cpp).
extern "C" {
void _ZN13SharedFilePtr7ReleaseEv(void *self);
void hal_fileptr_release(void *self) { _ZN13SharedFilePtr7ReleaseEv(self); }
}



extern "C" {
/* asm primitive: plain 48-byte block copy (with writeback, unlike the
   FIFO-fixed variant) */
void Copy48Bytes(int *src, int *dst) { for (int i = 0; i < 12; ++i) dst[i] = src[i]; }
int data_020a0e68[12];     /* Matrix4x3 scratch (0x30 on DS) -- was [4],
                              and FromTranslation's overrun stomped the
                              actor list head 0x20 bytes downstream */
short data_0208e378;
short *data_0209b45c;      /* spawn default rotation ptr (null = none) */
short *data_0209b460;      /* spawn default position ptr */
signed char data_0209b44c_c;
int data_0209b468[4];      /* actor list head the ctor links into */
}
#pragma comment(linker, "/alternatename:?data_0209b44c@@3CA=_data_0209b44c_c")

extern "C" {
unsigned char data_0209f2d8_c;   /* mega-char state byte: none */
int data_0209ceec[4];            /* shadow scale globals */
int data_0209cef4[4];
}
#pragma comment(linker, "/alternatename:_data_0209f2d8=_data_0209f2d8_c")
#pragma comment(linker, "/alternatename:?data_020a0e68@@3UMatrix4x3@@A=_data_020a0e68")

extern "C" {
void Matrix4x3_FromRotationY(void *m, int a);
void hal_m43_roty(void *m, int a) { Matrix4x3_FromRotationY(m, a); }
}

extern "C" {
/* asm primitive: 4x3 fx32 Y-rotation from (sin, cos):
   rows {c,0,-s},{0,1,0},{s,0,c},{0,0,0} */
void func_02052820(int *m, int s, int c)
{
    m[0] = c;      m[1] = 0; m[2] = -s;
    m[3] = 0;      m[4] = 0x1000; m[5] = 0;
    m[6] = s;      m[7] = 0; m[8] = c;
    m[9] = 0;      m[10] = 0; m[11] = 0;
}
}

// Cleanup-path heap wrappers: unnamed on the DS side here; the semantics
// are the gate-3a Memory layer's. Destroy is never reached in the gate
// (the smoke does not tear its heap-owning actor down), so it traps.
extern "C" {
void Memory_Deallocate(void *p);
void Heap_Destroy(void *h)
{
    (void)h;
    fprintf(stderr, "FATAL: Heap_Destroy reached (unwired)\n");
    abort();
}
}

extern "C" {
int func_0204424c(char *c);
int hal_f0204424c(char *c) { return func_0204424c(c); }
}

extern "C" {
/* MSL runtime array construction (asm on the DS, with EH frames the host
   does not need): apply the ctor forward across n elements. */
void func_020733a8(void *base, int n, int stride,
                   void (*ctor)(void *), void (*dtor)(void *))
{
    (void)dtor;
    char *p = (char *)base;
    for (int i = 0; i < n; ++i, p += stride)
        ctor(p);
}
}

// ---- gate-10 storage and stubs --------------------------------------------
extern "C" {
/* SOUND: stubbed silent for the walking campaign (the SPU is its own
   project phase). The calls are fire-and-forget on the DS. */
void _ZN5Sound13PlayCharVoiceEjjRK7Vector3(unsigned, unsigned, const void *) {}
void _ZN5Sound9PlayBank0EjRK7Vector3(unsigned, const void *) {}

void *_ZTV15MaterialChanger[8];
void *_ZTV15TextureSequence[8];
void *_ZTV25MovingCylinderClsnWithPos[12];
int VT0[20];    /* an unresolved shared-header vtable alias in ov002 TUs */

unsigned char data_02092128[0x40];
/* 0x0209f318 is a Camera POINTER, not storage -- every declaration of it
   in src is `Camera *` / `char *` / `short *` / `void *`, never an array.
   It was 32 zero bytes here, which reads as a null camera, and the states
   that poke the camera through it were faulting on the write rather than
   skipping: St_DeadHit_Init calls func_0200d89c(data_0209f318), whose
   whole body is `*(short *)(p + 0x18c) = 384`, so a null landed on
   address 0x18c. Point it at a zeroed block instead. Generous size, same
   convention as auto_bss: Camera.h evidences fields to 0x1a6 and the
   class is an Actor subclass, so 0x400 covers it with room.
   Verified regression-free: the eight walk probes come back
   byte-identical with this non-null. */
__declspec(align(8)) static unsigned char HAL_CAMERA[0x400];
void *data_0209f318 = HAL_CAMERA;
unsigned short data_0209f49c, data_0209f49e, data_0209f4a0;
unsigned char data_0209f4ab;
unsigned short data_0209f4ac, data_0209f4ae;
int data_020a0e40[8];
}

extern "C" {
/* MSL array new-with-ctor (asm on the DS, EH machinery the host skips):
   allocate count*size + an 8-byte cookie, record the count, construct
   forward. Layout mirrors the DS cookie so array-delete agrees. */
void *func_0203cc0c(unsigned size);
void *func_02073470(int count, int size, int cookie,
                    void (*ctor)(void *), void (*dtor)(void *))
{
    (void)dtor;
    char *raw = (char *)func_0203cc0c(count * size + cookie);
    if (!raw) return 0;
    ((int *)raw)[0] = size;
    ((int *)raw)[1] = count;
    char *base = raw + cookie;
    for (int i = 0; i < count; ++i)
        if (ctor) ctor(base + i * size);
    return base;
}
}

extern "C" {
/* __aeabi_idiv: the EABI signed-divide helper; host has native idiv */
int __aeabi_idiv(int n, int d) { return d ? n / d : 0; }
void *_ZTV18MovingCylinderClsn[12];

/* gate-10 BSS ring (spawn/camera/collision-config globals; zeros are the
   pre-scene defaults) */
short SUBLEVEL_LEVEL_TABLE[64];
int data_020991d8[8], data_02099264[8], data_02099274[8];
int data_02099338[8], data_02099348[8], data_02099358[8], data_02099368[8];
int data_020994cc[8], data_02099fa4[4], data_02099fa8[4], data_02099fac[4];
/* data_0209b008 moved to hal/camera_states.cpp: it is the first of the 19
   camera State objects, and the whole run has to carry the DS fn addresses */
int data_0209b478[8], data_0209b484[4], data_0209b488[4];
int data_0209b498[8], data_0209b53c[8], data_0209d574[8];
int data_0209f310[8], data_020a0f10[8], data_020a4bec[8];
/* data_0209f340 is a POINTER to the camera-preferences block, set by the
   camera boot; host parks it on a zeroed block so flag reads see 0 */
static unsigned char hal_campref_blk[0x40];
unsigned char *data_0209f340 = hal_campref_blk;
}

extern "C" {
/* ModelAnim2's two tables: the ctor/dtors only STORE these pointers; any
   virtual dispatch through them lands on a null slot and the fault probe
   names it. Filled at runtime if a gate ever dispatches ModelAnim2. */
void *_ZTV10ModelAnim2[12];
void *VTable_Animation_ModelAnim2Thunk[12];
void *data_020a5bb8;            /* table root pointer (func_02050xxx family) */
int data_0209f5c0[8], data_020ad560[8];
}
