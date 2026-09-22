/* missing_stubs.cpp — provides remaining undefined symbols.
   MSVC x86: extern "C" void foo() -> linker symbol _foo.
   All _ZN* symbols are Itanium ABI names used as C-linkage throughout the ROM.
   They MUST be inside extern "C" to produce the right linker symbol. */

#include "SharedFilePtr.h"
#include "common.h"

extern "C" {

/* ---- C++ Itanium-ABI names (C linkage on MSVC) ---- */
void _ZN8dActor_cD2Ev(void *s) { (void)s; }
void *_ZN9ActorBasenwEj(unsigned size) { (void)size; return (void*)0; }
void _ZN13RaycastGroundD1Ev(void *) {}
void _ZNK5dBgPi6CopyToERS_(void *, void *) {}
void _ZN8dActor_cC2Ev(void *s) { *(void**)s = (void*)0; }
void _ZN10dBgActor_cC2Ev(void *s) { _ZN8dActor_cC2Ev(s); }
void _ZN7dCcAc_cC1Ev(void *) {}
void _ZN7fBase_c9SceneNodeC1Ev(void *) {}

/* ---- hal_fill_modelanim2_vtable ---- */
void *_ZTV10ModelAnim2[];
void _hal_fill_modelanim2_vtable(void) {
    for (int i = 0; i < 12; ++i)
        _ZTV10ModelAnim2[i] = (void *)0;
}

/* ---- gPFlower model file handles ---- */
int gPFlowerCloseModelFile[4];
int gPFlowerOpenModelFile[4];

/* ---- ACTOR_DEBUG_NAMES ---- */
int ACTOR_DEBUG_NAMES[512];

/* ---- ov002 data symbols ---- */
int data_ov002_020ff220[8];
int data_ov002_0210a8b8[8];
int data_ov002_0210e164[8];
int data_ov002_0210f308[8];
int data_ov002_0210f344[8];

/* ---- ov077 (daJgm_c) data symbols ---- */
int data_ov077_02127230[8];
int data_ov077_02127238[8];
SharedFilePtr data_ov077_02127b20;
SharedFilePtr data_ov077_02127b28;
SharedFilePtr data_ov077_02127b30;
SharedFilePtr data_ov077_02127b38;
SharedFilePtr data_ov077_02127b40;
SharedFilePtr data_ov077_02127b48;
SharedFilePtr data_ov077_02127b50;
int data_ov077_02127b88[3]; /* Vector3 */
struct DaJgmStateHandler { void (*enter)(void*); void (*update)(void*); };
struct DaJgmStateHandlers { DaJgmStateHandler enter; DaJgmStateHandler update; };
DaJgmStateHandlers data_ov077_02127bc4[10];

/* ---- arm9 data symbols ---- */
int data_02088610[8];
int data_020a0edc[8];

/* ---- sinit_ov100_02147a70 stub ---- */
void __sinit_ov100_02147a70(void) {}

/* ---- func stubs (unrecovered single-function TUs) ---- */
void func_020072c0(void) {}
void func_02073534(void) {}
void func_ov002_020cac30(void) {}
void func_ov002_020d6084(void) {}
void func_ov002_020e17f8(void) {}
void func_ov002_020f0438(void) {}
void func_ov009_02111bd4(void) {}
void func_ov009_02111c18(void) {}
void func_ov009_02111c4c(void) {}
void func_ov009_02111c74(void) {}
void func_ov085_0212a904(void) {}
void func_ov085_0212aaa4(void) {}
void func_ov085_0212aaec(void) {}
void func_ov085_0212ac3c(void) {}
void func_ov085_0212ac4c(void) {}
void func_ov085_0212ad8c(void) {}
void func_ov085_0212ae08(void) {}
void func_ov085_0212b3fc(void) {}
void func_ov085_0212b444(void) {}
void func_ov085_0212b478(void) {}
void func_ov085_0212b4b4(void) {}
void func_ov085_0212b75c(void) {}
void func_ov085_0212b86c(void) {}
void func_ov085_0212b8a0(void) {}
void func_ov085_0212bc14(void) {}
void func_ov085_0212c150(void) {}
void func_ov085_0212cc18(void) {}
void func_ov100_02140e44(void) {}
void func_ov100_0214109c(void) {}
void func_ov100_0214117c(void) {}
void func_ov100_021412d8(void) {}
void func_ov100_02141470(void) {}
void func_ov100_021415bc(void) {}
void func_ov100_02141800(void) {}
void func_ov100_02141848(void) {}
void func_ov100_0214542c(void) {}
void func_ov100_021454c4(void) {}
void func_ov100_021455a0(void) {}

}  /* extern "C" */
