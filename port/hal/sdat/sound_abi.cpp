// ARM argument ride-throughs on the sound path.
//
// Five functions in the sound stack are declared in src/ with fewer
// parameters than their callers pass. That is not a decomp error -- on ARM
// the extra arguments sit in r1..r3 across a call that never names them, and
// the callee's own call passes them straight on. mwccarm reproduces the ROM
// bytes exactly that way, so the src stays as it is and the host spells the
// arguments out instead. Same pattern as the SharedFilePtr::Construct
// veneers in hal/cxx_aliases.cpp and the gate-14 ride-throughs in
// port/unmatched/.
//
// Each of these src files is filtered out of SLICE10_CAM_SOURCES in
// port/CMakeLists.txt; the bodies below are the same code with the riders
// named.
//
//   func_0204f600  (1 -> 4)  START: the sequence pointer, entry offset and
//                            bank all ride into Snd_SendCommand(0, ...).
//   func_0204f89c  (1 -> 2)  volume rides into Snd_SendCommand(3, ...).
//   func_0204f7cc  (1 -> 3)  pan mode + value ride into Snd_SendCommand(4).
//   func_0204f86c  (1 -> 3)  two riders into Snd_SendCommand(5, ...).
//   func_0204fa2c  (1 -> 2)  the fade length rides into func_0204f5a0.
#include "sdat.h"

typedef unsigned char u8;

extern "C" {

void func_0205adc4(void *a, int b, int c, int d);
void func_0205ad24(int a, int b);
int  func_0205acac(int a, int b, int c);
void func_0205aaf4(void *a, int b, int c);
void func_0204f4bc(void *obj);
void *func_0205afb4(void);
void func_0204f5a0(u8 *thiz, int arg1);

// func_0204f600(thiz) on ARM; r1 = sequence data, r2 = entry offset,
// r3 = resident bank, all of which ride into func_0205adc4.
int func_0204f600(void *thiz, int seqData, int entryOff, int bank)
{
    func_0205adc4((void *)(size_t)(unsigned)*(u8 *)((char *)thiz + 0x3c),
                  seqData, entryOff, bank);
    func_0204f4bc(thiz);
    *(void **)((char *)thiz + 0x30) = func_0205afb4();
    *(u8 *)((char *)thiz + 0x2c) = 1;
    return 1;
}

// func_0204f89c(c) on ARM; r1 = volume, riding into func_0205ad24's second
// parameter (which the src file's own declaration does not name either).
void func_0204f89c(char **c, int volume)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205ad24(p[0x3c], volume);
}

// func_0204f7cc(c) on ARM; r1/r2 = the pan mode and value.
void func_0204f7cc(char **c, int mode, int pan)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205acac(p[0x3c], mode, pan);
}

// func_0204f86c(c) on ARM; r1/r2 ride into func_0205aaf4.
void func_0204f86c(char **c, int b, int d)
{
    u8 *p = (u8 *)*c;
    if (p) func_0205aaf4((void *)(size_t)(unsigned)p[0x3c], b, d);
}

// func_0204fa2c(p) on ARM; r1 = the fade length in frames, riding into
// func_0204f5a0's second parameter. 0 means stop now, non-zero ramps down.
int func_0204fa2c(int *p, int fade)
{
    func_0204f5a0((u8 *)(size_t)(unsigned)*p, fade);
    return 0;
}

}  // extern "C"
