// Gate-10 linkage aliases: C++-mangled references -> C-named definitions.
//
// Slice .cpp TUs that declare externs without extern "C" emit MSVC
// manglings for what are C-named symbols everywhere else. The linker
// alias closes the gap without touching src/. Decorated names are
// lifted verbatim from the link log; each maps to its cdecl C name.
#include <string.h>

#pragma comment(linker, "/alternatename:?FUN_02029a68@@YAXXZ=_FUN_02029a68")
#pragma comment(linker, "/alternatename:?_ZN6Player11ChangeStateERNS_5StateE@@YAXPAUPlayer@@PAUState@@@Z=__ZN6Player11ChangeStateERNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZN6Player11ChangeStateERNS_5StateE@@YAXPAXPAUState@@@Z=__ZN6Player11ChangeStateERNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZN6Player4HealEi@@YAXPAUPlayer@@H@Z=__ZN6Player4HealEi")
#pragma comment(linker, "/alternatename:?_ZN6Player7IsStateERNS_5StateE@@YAHPAXPAUState@@@Z=__ZN6Player7IsStateERNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZN9Animation8GetFlagsEv@@YAHPAX@Z=__ZN9Animation8GetFlagsEv")
#pragma comment(linker, "/alternatename:?_ZNK9Animation12WillHitFrameEi@@YAHPAXH@Z=__ZNK9Animation12WillHitFrameEi")
#pragma comment(linker, "/alternatename:?data_0208e6ec@@3PAHA=_data_0208e6ec")
#pragma comment(linker, "/alternatename:?data_02092144@@3PAFA=_data_02092144")
#pragma comment(linker, "/alternatename:?data_020992a4@@3PAXA=_data_020992a4")
#pragma comment(linker, "/alternatename:?data_020992b4@@3PAXA=_data_020992b4")
#pragma comment(linker, "/alternatename:?data_0209b0c8@@3HA=_data_0209b0c8")
#pragma comment(linker, "/alternatename:?data_0209f49c@@3PADA=_data_0209f49c")
#pragma comment(linker, "/alternatename:?data_0209f49e@@3PAEA=_data_0209f49e")
#pragma comment(linker, "/alternatename:?data_0209f4ab@@3PAEA=_data_0209f4ab")
#pragma comment(linker, "/alternatename:?data_ov002_0211073c@@3PAHA=_data_ov002_0211073c")
#pragma comment(linker, "/alternatename:?data_ov002_021103f4@@3DA=_data_ov002_021103f4")
#pragma comment(linker, "/alternatename:?data_ov002_021101fc@@3PAHA=_data_ov002_021101fc")
#pragma comment(linker, "/alternatename:?func_ov002_020dba0c@@YAXPAX@Z=_func_ov002_020dba0c")
#pragma comment(linker, "/alternatename:?data_020a0e40@@3EA=_data_020a0e40")
#pragma comment(linker, "/alternatename:?data_020a0e40@@3PAEA=_data_020a0e40")
#pragma comment(linker, "/alternatename:?data_ov002_020ff1b0@@3PAHA=_data_ov002_020ff1b0")
#pragma comment(linker, "/alternatename:?data_ov002_020ff480@@3PAHA=_data_ov002_020ff480")
#pragma comment(linker, "/alternatename:?data_ov002_02110034@@3UState@@A=_data_ov002_02110034")
#pragma comment(linker, "/alternatename:?data_ov002_0211010c@@3UState@@A=_data_ov002_0211010c")
#pragma comment(linker, "/alternatename:?data_ov002_02110124@@3UState@@A=_data_ov002_02110124")
#pragma comment(linker, "/alternatename:?data_ov002_0211019c@@3PAHA=_data_ov002_0211019c")
#pragma comment(linker, "/alternatename:?data_ov002_0211019c@@3UState@@A=_data_ov002_0211019c")
#pragma comment(linker, "/alternatename:?data_ov002_021101e4@@3UState@@A=_data_ov002_021101e4")
#pragma comment(linker, "/alternatename:?data_ov002_0211022c@@3UState@@A=_data_ov002_0211022c")
#pragma comment(linker, "/alternatename:?data_ov002_0211049c@@3PAHA=_data_ov002_0211049c")
#pragma comment(linker, "/alternatename:?data_ov002_0211052c@@3UState@@A=_data_ov002_0211052c")
#pragma comment(linker, "/alternatename:?data_ov002_0211055c@@3UState@@A=_data_ov002_0211055c")
#pragma comment(linker, "/alternatename:?data_ov002_02110574@@3UState@@A=_data_ov002_02110574")
#pragma comment(linker, "/alternatename:?data_ov002_0211058c@@3HA=_data_ov002_0211058c")
#pragma comment(linker, "/alternatename:?data_ov002_021105bc@@3UState@@A=_data_ov002_021105bc")
#pragma comment(linker, "/alternatename:?data_ov002_021105d4@@3UState@@A=_data_ov002_021105d4")
#pragma comment(linker, "/alternatename:?func_020089f8@@YAXPAX@Z=_func_020089f8")
#pragma comment(linker, "/alternatename:?func_0200cae4@@YAHPAX@Z=_func_0200cae4")
#pragma comment(linker, "/alternatename:?func_ov002_020bcdf0@@YAXPAX@Z=_func_ov002_020bcdf0")
#pragma comment(linker, "/alternatename:?func_ov002_020c6adc@@YAHPAX@Z=_func_ov002_020c6adc")
#pragma comment(linker, "/alternatename:?func_ov002_020d2da0@@YAHPAX@Z=_func_ov002_020d2da0")
#pragma comment(linker, "/alternatename:?func_ov002_020d2e74@@YAXPAX@Z=_func_ov002_020d2e74")
#pragma comment(linker, "/alternatename:?func_ov002_020d2f24@@YAXPAX@Z=_func_ov002_020d2f24")
#pragma comment(linker, "/alternatename:?func_ov002_020d2fdc@@YAHPAX@Z=_func_ov002_020d2fdc")
#pragma comment(linker, "/alternatename:?func_ov002_020d3b9c@@YAHPAX@Z=_func_ov002_020d3b9c")
#pragma comment(linker, "/alternatename:?func_ov002_020d413c@@YAXPAXF@Z=_func_ov002_020d413c")
#pragma comment(linker, "/alternatename:?func_ov002_020d45c0@@YAXPAX@Z=_func_ov002_020d45c0")
#pragma comment(linker, "/alternatename:?func_ov002_020d4748@@YAXPAX@Z=_func_ov002_020d4748")
#pragma comment(linker, "/alternatename:?func_ov002_020e6780@@YAXPAD@Z=_func_ov002_020e6780")
#pragma comment(linker, "/alternatename:?Disable@IRQ@@SAIXZ=__ZN3IRQ7DisableEv")
#pragma comment(linker, "/alternatename:?Restore@IRQ@@SAXI@Z=__ZN3IRQ7RestoreEj")

extern "C" {
/* BSS the aliased data references land on (non-ov002 ring) */
int data_0208e6ec[4]; short data_02092144[4];
void *data_020992a4[4], *data_020992b4[4];
/* data_0209b0c8 moved to hal/camera_states.cpp (camera State object 12) */

}

namespace cstd { int strcmp(const char *a, const char *b);
                char *strchr(const char *s, char ch); }
extern "C" {
int _ZN4cstd6strcmpEPKcS1_(const char *a, const char *b)
{ return cstd::strcmp(a, b); }
char *_ZN4cstd6strchrEPKcc(const char *s, char ch)
{ return cstd::strchr(s, ch); }

/* MultiCopyHalf: halfword copy loop, (src, dst, byteCount) in r0-r2 */
void MultiCopyHalf(unsigned short *src, unsigned short *dst, unsigned n)
{
    for (unsigned i = 0; i < n; i += 2)
        *(unsigned short *)((char *)dst + i) = *(unsigned short *)((char *)src + i);
}

/* Thumb matrix builders (asm primitives; semantics from their headers):
   4x3 fx32 rotation matrices from (sin, cos), 4096 = 1.0 */
void func_02052800(int *m, int s, int c)   /* X rotation */
{
    m[0] = 0x1000; m[1] = 0; m[2] = 0;
    m[3] = 0; m[4] = c; m[5] = s;
    m[6] = 0; m[7] = -s; m[8] = c;
    m[9] = 0; m[10] = 0; m[11] = 0;
}
void func_0205283c(int *m, int s, int c)   /* Z rotation */
{
    m[0] = c; m[1] = s; m[2] = 0;
    m[3] = -s; m[4] = c; m[5] = 0;
    m[6] = 0; m[7] = 0; m[8] = 0x1000;
    m[9] = 0; m[10] = 0; m[11] = 0;
}

/* asm veneer func_02059824 just tail-calls its C body */
void func_02059834(void);
void func_02059824(void) { func_02059834(); }

/* ITCM soft-float library, explicit call sites only (implicit double ops
   compile to native FP on host). Semantics fixed by the callers:
   the double formatter zero-tests with 9d40, negates via 8e10(0.0, v),
   and extracts digits through 859c. */
int func_01ff9d40(double x, double y) { return x == y; }
double func_01ff8708(double x, double y) { return x * y; }  /* dmul (frexp) */
/* single-precision ITCM pair cstd::atan2 leans on: i2f then float-compare */
int func_01ffa4bc(int a) { float f = (float)a; int b; memcpy(&b, &f, 4); return b; }
int func_01ff98f4(int a, int b)
{ float x, y; memcpy(&x, &a, 4); memcpy(&y, &b, 4); return x < y; }
/* float greater-than: the slide friction gate (i2f(speed) > 48.0f) */
int func_01ff99a4(int a, int b)
{ float x, y; memcpy(&x, &a, 4); memcpy(&y, &b, 4); return x > y; }
/* f2i truncation; one caller's two-arg decl is an r1 ride-through */
int func_01ffa344(int a, int b)
{ float x; memcpy(&x, &a, 4); (void)b; return (int)x; }
/* ITCM signed divide (walk-speed scaling) */
int func_01ffabe4(int a, int b) { return b ? a / b : 0; }
/* atan table the boot builds at runtime; zeros = heading 0 */
unsigned short data_020994e0[0x408];
double func_01ff8e10(double x, double y) { return x - y; }
unsigned long long func_01ff859c(double x) { return (unsigned long long)x; }

/* ITCM soft-float compare: double(a:b) < double(c:d), EABI r0=low word */
int func_01ff9e2c(unsigned a, unsigned b, unsigned c, unsigned d)
{
    double x, y;
    unsigned long long xb = ((unsigned long long)b << 32) | a;
    unsigned long long yb = ((unsigned long long)d << 32) | c;
    memcpy(&x, &xb, 8);
    memcpy(&y, &yb, 8);
    return x < y;
}

/* ARMProcessorMode reads CPSR & 0x1f; host always reports system mode */
int ARMProcessorMode(void) { return 0x1f; }

/* DS thread scheduler context ops. The port runs the game on ONE fiber (the
   ntr rt loop owns real scheduling), so a save reports "already resumed"
   (setjmp-nonzero) and the reschedule path backs out without switching.
   A restore reaching the host would mean a second DS thread went live. */
int ARMSaveContext(void *ctx) { (void)ctx; return 1; }
void ARMRestoreContext(void *ctx) { (void)ctx; __debugbreak(); }

/* func_02071644 (hand-asm): backward digit-carry increment over the decimal
   buffer at obj+5; overflow at the first digit writes 1 and bumps the s16
   exponent at obj+2. */
void func_02071644(unsigned char *obj, int len)
{
    unsigned char *first = obj + 5;
    unsigned char *p = first + len - 1;
    for (;;) {
        if (*p < 9) { *p += 1; return; }
        if (p != first) { *p = 0; --p; continue; }
        *p = 1;
        *(short *)(obj + 2) += 1;
        return;
    }
}

/* C-linkage face of Animation::WillHitFrame (C++ face lives in
   player_bridges; one TU cannot name both linkages) */
int hal_anim_willhit(void *self, int f);
int _ZNK9Animation12WillHitFrameEi(void *self, int f)
{ return hal_anim_willhit(self, f); }

/* SharedFilePtr construct veneers: on the DS these pass fileID in r1
   through a tail call the C decl never names (the ride-through catalog).
   Host spells out both args and routes to the HAL Construct. */
void *_ZN13SharedFilePtr9ConstructEj(void *self, unsigned id);
int func_02017acc(void *self, unsigned id)
{ _ZN13SharedFilePtr9ConstructEj(self, id); return (int)self; }
int SharedFilePtr_Construct_TexSeq(void *self, unsigned id)
{ _ZN13SharedFilePtr9ConstructEj(self, id); return (int)self; }
int func_02017ab4(int x) { return x; }   /* static-dtor veneer: no-op */
int func_02017b4c(void *self, unsigned id)
{ _ZN13SharedFilePtr9ConstructEj(self, id); return (int)self; }
int func_02017e34(int x) { return x; }   /* fileptr dtor body: host no-op */
void SharedFilePtr_Destruct_TexSeq(void) {}
void SharedFilePtr_Destruct_Anim(void) {}
void *data_020aa3f0;                     /* MSL global-dtor chain head */

/* crash-screen-only ITCM entry; trap keeps it honest if ever reached */
void func_01ffdd98(int a) { (void)a; __debugbreak(); }

int data_0209cdcc, data_0209cde4[4], data_0209cde8[4];
int data_020a4b4c, data_020a4b50;
int data_020a7fc0[8];
/* GX bank-state u16 band 0x020a608a..0x020a60a0. On the DS these alias one
   register-cache block (data_020a6088's GXState overlaps 608a/608c); host
   keeps them as separate objects -- fine while nothing in the slice mixes
   the struct view with the field view on the same bit of state. */
unsigned short data_020a608a, data_020a608c, data_020a608e, data_020a6090;
unsigned short data_020a6092, data_020a6094, data_020a6096, data_020a6098;
unsigned short data_020a609a, data_020a609c, data_020a609e, data_020a60a0;
int data_0209a5e4, data_0209a5e8, data_020a612c[4], data_0209a438[8];
/* OS scheduler/thread BSS band 0x0209a628..0x0209a6f8 */
int data_0209a628[12], data_0209a658[10], data_0209a680[6], data_0209a698[4];
int data_0209a6a8[2], data_0209a6b0[2], data_0209a6b8[2], data_0209a6c0[2];
int data_0209a6c8, data_0209a6cc, data_0209a6d0, data_0209a6d4, data_0209a6d8;
int data_0209a6dc, data_0209a6e0, data_0209a6e4, data_0209a6e8, data_0209a6ec;
int data_0209a6f0, data_0209a6f4, data_0209a6f8;
unsigned char data_020a0e98; int data_020a4d6c;
/* ov006 (cutscene overlay) fileptrs St_LevelEnter_Main releases; zeroed
   stand-ins -- Release guards on numRefs 0 */
unsigned char data_ov006_02140330[8], data_ov006_02140338[8];
unsigned char data_ov089_02132894[16], data_ov089_021328b4[16];
int data_0209b48c, data_0209b4a0[4], data_0209b4ac;
int data_020a4c48, data_020a4c4c, data_020a4c54[2], data_020a4c5c;
/* particle system tracker block (refs reach +0x750) */
__declspec(align(8)) unsigned char PARTICLE_SYS_TRACKER[0x1000];
int data_0209ee74[4], data_0209f32c[4], data_0209b4a4[4];
/* camera + player-list globals (gate-9 scoping notes) */
int data_0209d4b0[8];
int data_0209f274[8];
/* data_0209f324 (WIPES) moved to hal/fader_wipes.cpp: it is a POINTER to a
   staged array now, not blank storage, or the death path faults on it. */
int data_0209a5ec, data_0209a5f4[2], data_0209a5fc, data_0209a600;
int data_0209a604, data_0209a60c[2], data_0209a614, data_0209a618;
int data_0209a61c[4], data_020a6128, data_020a6134[4];
}

#pragma comment(linker, "/alternatename:__ZN2GX12SetBankForBGEt=?SetBankForBG@GX@@YAXG@Z")

/* Scene::ResetHardwareRegisters is defined against this exact local shadow
   in its own TU; mirror it so the manglings agree. */
struct Scene { void ResetHardwareRegisters(); };
extern "C" void _ZN5Scene22ResetHardwareRegistersEv(void *s)
{ ((Scene *)s)->Scene::ResetHardwareRegisters(); }

#pragma comment(linker, "/alternatename:?data_020a0e98@@3EA=_data_020a0e98")
#pragma comment(linker, "/alternatename:?data_020a4d6c@@3PAEA=_data_020a4d6c")

/* A slice TU sees NestedHeapIterator::Next through a local shadow returning
   unsigned char*; the real definition returns int against the shared header.
   Mirror the shadow and hop through the C-named helper in player_bridges. */
struct HeapAllocator;
struct NestedHeapIterator { unsigned char *Next(HeapAllocator *h); };
extern "C" int hal_nhi_next(void *self, void *h);
unsigned char *NestedHeapIterator::Next(HeapAllocator *h)
{ return (unsigned char *)(size_t)hal_nhi_next(this, h); }
#pragma comment(linker, "/alternatename:?GiveHealth@@YAHHH@Z=_GiveHealth")
#pragma comment(linker, "/alternatename:?data_0209caa0@@3PAHA=_data_0209caa0")
#pragma comment(linker, "/alternatename:?data_0209f2d8@@3EA=_data_0209f2d8")
#pragma comment(linker, "/alternatename:?data_0209f49c@@3GA=_data_0209f49c")
#pragma comment(linker, "/alternatename:?data_0209f4a0@@3FA=_data_0209f4a0")
#pragma comment(linker, "/alternatename:?data_0209f4ac@@3EA=_data_0209f4ac")
#pragma comment(linker, "/alternatename:?data_0209f4ae@@3EA=_data_0209f4ae")
#pragma comment(linker, "/alternatename:?data_ov002_0211010c@@3PAHA=_data_ov002_0211010c")
#pragma comment(linker, "/alternatename:?data_ov002_02110124@@3PAHA=_data_ov002_02110124")
#pragma comment(linker, "/alternatename:?data_0208e430@@3HA=_data_0208e430")
#pragma comment(linker, "/alternatename:?data_0209f32c@@3HA=_data_0209f32c")
#pragma comment(linker, "/alternatename:?func_02022d00@@YAIIIHHHPAX@Z=_func_02022d00")
#pragma comment(linker, "/alternatename:?NewSimple@System@Particle@@SAXIHHH@Z=__ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_")
#pragma comment(linker, "/alternatename:?PlayBank0@Sound@@SAXIABUVector3@@@Z=__ZN5Sound9PlayBank0EjRK7Vector3")
#pragma comment(linker, "/alternatename:?data_02082214@@3PAFA=_data_02082214")
#pragma comment(linker, "/alternatename:?data_0209f264@@3EA=_data_0209f264")
#pragma comment(linker, "/alternatename:?data_0209f2f8@@3CA=_data_0209f2f8")
#pragma comment(linker, "/alternatename:?data_0209f2fc@@3EA=_data_0209f2fc")
#pragma comment(linker, "/alternatename:?data_ov002_0210a7e8@@3PAIA=_data_ov002_0210a7e8")
#pragma comment(linker, "/alternatename:?func_ov002_020bdd2c@@YAXPAX@Z=_func_ov002_020bdd2c")

/* Sound sequence-info lookup: sound system deferred, no entry found */
struct SeqEntry;
struct Sound {
    struct InfoSequenceEntry { static SeqEntry *GetWithID(unsigned id); };
};
SeqEntry *Sound::InfoSequenceEntry::GetWithID(unsigned) { return 0; }

/* Heap::_Deallocate is a DS tail-call veneer to Deallocate; operator delete
   dispatches it as a method. Same-shadow definition forwarding to the HAL
   dealloc keeps the mangling the reference expects. */
struct Heap { void _Deallocate(void *ptr); };
extern "C" void _ZN4Heap10DeallocateEPv(void *self, void *ptr);
void Heap::_Deallocate(void *ptr) { _ZN4Heap10DeallocateEPv(this, ptr); }

/* RaycastGround::DetectClsn is defined against a local shadow in its own
   TU; mirror the shadow (no real header here) so the manglings agree. */
class RaycastGround { public: int DetectClsn(); };
extern "C" int _ZN13RaycastGround10DetectClsnEv(void *self)
{ return ((RaycastGround *)self)->DetectClsn(); }
#pragma comment(linker, "/alternatename:?data_0209f254@@3EA=_data_0209f254")
#pragma comment(linker, "/alternatename:?data_0209f4a6@@3FA=_data_0209f4a6")
#pragma comment(linker, "/alternatename:?func_ov002_020bdd9c@@YAXPAX@Z=_func_ov002_020bdd9c")
#pragma comment(linker, "/alternatename:?func_ov002_020bdef0@@YAXPAX@Z=_func_ov002_020bdef0")
#pragma comment(linker, "/alternatename:?func_ov002_020bf13c@@YAXPAX@Z=_func_ov002_020bf13c")
#pragma comment(linker, "/alternatename:?func_ov002_020bf36c@@YAXPAX0@Z=_func_ov002_020bf36c")
#pragma comment(linker, "/alternatename:?func_ov002_020c2db8@@YAXPAX@Z=_func_ov002_020c2db8")
#pragma comment(linker, "/alternatename:?func_ov002_020c2e78@@YAXPAX@Z=_func_ov002_020c2e78")
#pragma comment(linker, "/alternatename:?func_ov002_020c4188@@YAHPAX@Z=_func_ov002_020c4188")
#pragma comment(linker, "/alternatename:?func_ov002_020ca940@@YAXPAX@Z=_func_ov002_020ca940")
#pragma comment(linker, "/alternatename:?func_ov002_020d8158@@YAXPAX@Z=_func_ov002_020d8158")
#pragma comment(linker, "/alternatename:?func_ov002_020d869c@@YAXPAX@Z=_func_ov002_020d869c")
#pragma comment(linker, "/alternatename:?func_ov002_020db704@@YAXPAX@Z=_func_ov002_020db704")
#pragma comment(linker, "/alternatename:?func_ov002_020e032c@@YAXPAX@Z=_func_ov002_020e032c")
#pragma comment(linker, "/alternatename:?func_ov002_020e4bb8@@YAXPAX@Z=_func_ov002_020e4bb8")
#pragma comment(linker, "/alternatename:?data_0209cee8@@3PAXA=_data_0209cee8")
#pragma comment(linker, "/alternatename:?data_0209b49c@@3HA=_data_0209b49c")
#pragma comment(linker, "/alternatename:?data_ov002_02110094@@3DA=_data_ov002_02110094")
#pragma comment(linker, "/alternatename:?data_ov002_0211013c@@3UState@@A=_data_ov002_0211013c")
#pragma comment(linker, "/alternatename:?data_ov002_021101b4@@3UState@@A=_data_ov002_021101b4")
#pragma comment(linker, "/alternatename:?data_ov002_0211034c@@3DA=_data_ov002_0211034c")
#pragma comment(linker, "/alternatename:?AngleDiff@@YAHHH@Z=_AngleDiff")
#pragma comment(linker, "/alternatename:?IsButtonInputValid@@YAHXZ=_IsButtonInputValid")
#pragma comment(linker, "/alternatename:?_ZN3OAM6RenderEbP7OamAttriiii5Fix12IiES3_ii@@YAPAXHPAXHHHHHHHH@Z=__ZN3OAM6RenderEbP7OamAttriiii5Fix12IiES3_ii")
#pragma comment(linker, "/alternatename:?_ZN5Model14SetPolygonModeEi@@YAHPAXH@Z=__ZN5Model14SetPolygonModeEi")
#pragma comment(linker, "/alternatename:?_ZNK6Player14GetBodyModelIDEjb@@YAHPAXIH@Z=__ZNK6Player14GetBodyModelIDEjb")
#pragma comment(linker, "/alternatename:?data_0208ee44@@3HA=_data_0208ee44")
#pragma comment(linker, "/alternatename:?data_0209d650@@3EA=_data_0209d650")
#pragma comment(linker, "/alternatename:?data_0209d65c@@3CA=_data_0209d65c")
#pragma comment(linker, "/alternatename:?data_0209d66c@@3EA=_data_0209d66c")
#pragma comment(linker, "/alternatename:?data_0209d670@@3EA=_data_0209d670")
#pragma comment(linker, "/alternatename:?data_0209d684@@3EA=_data_0209d684")
#pragma comment(linker, "/alternatename:?data_0209d688@@3EA=_data_0209d688")
#pragma comment(linker, "/alternatename:?data_0209d68c@@3EA=_data_0209d68c")
#pragma comment(linker, "/alternatename:?data_0209d69c@@3EA=_data_0209d69c")
#pragma comment(linker, "/alternatename:?data_0209d6a0@@3EA=_data_0209d6a0")
#pragma comment(linker, "/alternatename:?data_0209d6a8@@3EA=_data_0209d6a8")
#pragma comment(linker, "/alternatename:?data_0209d6b0@@3EA=_data_0209d6b0")
#pragma comment(linker, "/alternatename:?data_0209d6b4@@3EA=_data_0209d6b4")
#pragma comment(linker, "/alternatename:?data_0209d6bc@@3EA=_data_0209d6bc")
#pragma comment(linker, "/alternatename:?data_0209d6c4@@3EA=_data_0209d6c4")
#pragma comment(linker, "/alternatename:?data_0209d6c8@@3EA=_data_0209d6c8")
#pragma comment(linker, "/alternatename:?data_0209d6cc@@3EA=_data_0209d6cc")
#pragma comment(linker, "/alternatename:?data_0209d6d0@@3EA=_data_0209d6d0")
#pragma comment(linker, "/alternatename:?data_0209d6d8@@3FA=_data_0209d6d8")
#pragma comment(linker, "/alternatename:?data_0209d6dc@@3FA=_data_0209d6dc")
#pragma comment(linker, "/alternatename:?data_0209d6e4@@3PAEA=_data_0209d6e4")
#pragma comment(linker, "/alternatename:?data_0209d6f0@@3PAUStruct6f0@@A=_data_0209d6f0")
#pragma comment(linker, "/alternatename:?data_0209d6f4@@3HA=_data_0209d6f4")
#pragma comment(linker, "/alternatename:?data_0209f4a2@@3EA=_data_0209f4a2")
#pragma comment(linker, "/alternatename:?data_0209f4a4@@3EA=_data_0209f4a4")
#pragma comment(linker, "/alternatename:?data_020a0c80@@3PAPAXA=_data_020a0c80")
#pragma comment(linker, "/alternatename:?data_020a0db0@@3HA=_data_020a0db0")
#pragma comment(linker, "/alternatename:?data_020a0de8@@3PAEA=_data_020a0de8")
#pragma comment(linker, "/alternatename:?data_020a0de9@@3PAEA=_data_020a0de9")
#pragma comment(linker, "/alternatename:?data_020a0deb@@3PAEA=_data_020a0deb")
#pragma comment(linker, "/alternatename:?data_020a0e5a@@3EA=_data_020a0e5a")
#pragma comment(linker, "/alternatename:?data_ov002_020ff128@@3PAGA=_data_ov002_020ff128")
#pragma comment(linker, "/alternatename:?data_ov002_0210c390@@3EA=_data_ov002_0210c390")
#pragma comment(linker, "/alternatename:?data_ov002_0210c398@@3EA=_data_ov002_0210c398")
#pragma comment(linker, "/alternatename:?data_ov002_0210c3a0@@3EA=_data_ov002_0210c3a0")
#pragma comment(linker, "/alternatename:?data_ov002_0210c3a8@@3EA=_data_ov002_0210c3a8")
#pragma comment(linker, "/alternatename:?data_ov002_0210ffec@@3UState@@A=_data_ov002_0210ffec")
#pragma comment(linker, "/alternatename:?data_ov002_0211001c@@3UState@@A=_data_ov002_0211001c")
#pragma comment(linker, "/alternatename:?data_ov002_021100ac@@3HA=_data_ov002_021100ac")
#pragma comment(linker, "/alternatename:?data_ov002_02110574@@3DA=_data_ov002_02110574")
#pragma comment(linker, "/alternatename:?data_ov002_021105a4@@3UState@@A=_data_ov002_021105a4")
#pragma comment(linker, "/alternatename:?data_ov002_021105bc@@3DA=_data_ov002_021105bc")
#pragma comment(linker, "/alternatename:?data_ov002_0211067c@@3DA=_data_ov002_0211067c")
#pragma comment(linker, "/alternatename:?func_02012790@@YAXH@Z=_func_02012790")
#pragma comment(linker, "/alternatename:?func_02014fa4@@YAXPAD@Z=_func_02014fa4")
#pragma comment(linker, "/alternatename:?func_0201adfc@@YAXXZ=_func_0201adfc")
#pragma comment(linker, "/alternatename:?func_0201b388@@YAXH@Z=_func_0201b388")
#pragma comment(linker, "/alternatename:?func_0201b6f8@@YAXH@Z=_func_0201b6f8")
#pragma comment(linker, "/alternatename:?func_0201b7cc@@YAPAXXZ=_func_0201b7cc")
#pragma comment(linker, "/alternatename:?func_02059650@@YA_JXZ=_func_02059650")
#pragma comment(linker, "/alternatename:?func_ov002_020d82f0@@YAHPAX@Z=_func_ov002_020d82f0")
#pragma comment(linker, "/alternatename:?func_ov002_020d91b8@@YAXPADH@Z=_func_ov002_020d91b8")
#pragma comment(linker, "/alternatename:?func_ov002_020e6b74@@YAHPAXH@Z=_func_ov002_020e6b74")
#pragma comment(linker, "/alternatename:?data_ov002_02110064@@3PAHA=_data_ov002_02110064")
#pragma comment(linker, "/alternatename:?data_ov002_02110094@@3UState@Player@@A=_data_ov002_02110094")
#pragma comment(linker, "/alternatename:?data_ov002_021101cc@@3UState@@A=_data_ov002_021101cc")
#pragma comment(linker, "/alternatename:?data_ov002_02110604@@3UState@@A=_data_ov002_02110604")
#pragma comment(linker, "/alternatename:?GetSoundMode@@YAHXZ=_GetSoundMode")
#pragma comment(linker, "/alternatename:?SetSoundMode@@YAXH@Z=_SetSoundMode")
#pragma comment(linker, "/alternatename:?data_02088fb8@@3HA=_data_02088fb8")
#pragma comment(linker, "/alternatename:?data_020890a0@@3HA=_data_020890a0")
#pragma comment(linker, "/alternatename:?data_0209b270@@3EA=_data_0209b270")
#pragma comment(linker, "/alternatename:?data_0209b284@@3PAIA=_data_0209b284")
#pragma comment(linker, "/alternatename:?data_0209b2a4@@3PAIA=_data_0209b2a4")
#pragma comment(linker, "/alternatename:?data_0209b454@@3HA=_data_0209b454")
#pragma comment(linker, "/alternatename:?data_0209f21c@@3EA=_data_0209f21c")
#pragma comment(linker, "/alternatename:?data_0209fc48@@3HA=_data_0209fc48")
#pragma comment(linker, "/alternatename:?data_0209fc4c@@3HA=_data_0209fc4c")
#pragma comment(linker, "/alternatename:?func_02011c8c@@YAXXZ=_func_02011c8c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a054@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a054")
#pragma comment(linker, "/alternatename:?data_ov002_0210a064@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a064")
#pragma comment(linker, "/alternatename:?data_ov002_0210a094@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a094")
#pragma comment(linker, "/alternatename:?data_ov002_0210a0b4@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a0b4")
#pragma comment(linker, "/alternatename:?data_ov002_0210a0dc@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a0dc")
#pragma comment(linker, "/alternatename:?data_ov002_0210a124@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a124")
#pragma comment(linker, "/alternatename:?data_ov002_0210a14c@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a14c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a36c@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a36c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a3c4@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a3c4")
#pragma comment(linker, "/alternatename:?data_ov002_0210a3fc@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a3fc")
#pragma comment(linker, "/alternatename:?data_ov002_0210a40c@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a40c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a44c@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a44c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a474@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a474")
#pragma comment(linker, "/alternatename:?data_ov002_0210a534@@3P8C@@AEXPAEHH@ZQ1@=_data_ov002_0210a534")
#pragma comment(linker, "/alternatename:?Player_AdvanceAnims@@YAHPAX@Z=_Player_AdvanceAnims")
#pragma comment(linker, "/alternatename:?Player_ScaleByCharFactor@@YAHPAXH@Z=_Player_ScaleByCharFactor")
#pragma comment(linker, "/alternatename:?_ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block@@YAXPAX00@Z=__ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block")
#pragma comment(linker, "/alternatename:?_ZN12MeshCollider8LoadFileER13SharedFilePtr@@YAPAXPAX@Z=__ZN12MeshCollider8LoadFileER13SharedFilePtr")
#pragma comment(linker, "/alternatename:?_ZN12MeshColliderC1Ev@@YAXPAX@Z=__ZN12MeshColliderC1Ev")
#pragma comment(linker, "/alternatename:?_ZN13SharedFilePtr9ConstructEj@@YAPAXPAXI@Z=__ZN13SharedFilePtr9ConstructEj")
#pragma comment(linker, "/alternatename:?_ZN16MeshColliderBase6EnableEP5Actor@@YAHPAX0@Z=__ZN16MeshColliderBase6EnableEP5Actor")
#pragma comment(linker, "/alternatename:?_ZN6Player11ChangeStateERNS_5StateE@@YAHPAX0@Z=__ZN6Player11ChangeStateERNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZN6Player6IsAnimEj@@YAHPAXI@Z=__ZN6Player6IsAnimEj")
#pragma comment(linker, "/alternatename:?_ZN6Player7SetAnimEji5Fix12IiEj@@YAHPAXIHHI@Z=__ZN6Player7SetAnimEji5Fix12IiEj")
#pragma comment(linker, "/alternatename:?data_0209f4a0@@3PADA=_data_0209f4a0")
#pragma comment(linker, "/alternatename:?data_ov002_0211007c@@3HA=_data_ov002_0211007c")
#pragma comment(linker, "/alternatename:?data_ov002_0211019c@@3HA=_data_ov002_0211019c")
#pragma comment(linker, "/alternatename:?data_ov002_021101b4@@3HA=_data_ov002_021101b4")
#pragma comment(linker, "/alternatename:?data_ov002_021101e4@@3HA=_data_ov002_021101e4")
#pragma comment(linker, "/alternatename:?data_ov002_02110424@@3PADA=_data_ov002_02110424")
#pragma comment(linker, "/alternatename:?data_ov002_02110454@@3HA=_data_ov002_02110454")
#pragma comment(linker, "/alternatename:?data_ov002_0211052c@@3HA=_data_ov002_0211052c")
#pragma comment(linker, "/alternatename:?data_ov002_0211055c@@3HA=_data_ov002_0211055c")
#pragma comment(linker, "/alternatename:?data_ov002_0211055c@@3PAHA=_data_ov002_0211055c")
#pragma comment(linker, "/alternatename:?data_ov002_02110724@@3HA=_data_ov002_02110724")
#pragma comment(linker, "/alternatename:?func_ov002_020d5c6c@@YAHPAX@Z=_func_ov002_020d5c6c")
#pragma comment(linker, "/alternatename:?func_ov002_020dde74@@YAHPAX@Z=_func_ov002_020dde74")
#pragma comment(linker, "/alternatename:?func_ov002_020e04a4@@YAXPAX@Z=_func_ov002_020e04a4")
#pragma comment(linker, "/alternatename:?func_ov002_020e2664@@YAHPAX@Z=_func_ov002_020e2664")
#pragma comment(linker, "/alternatename:?func_ov002_020e28d4@@YAHPAXHH@Z=_func_ov002_020e28d4")
#pragma comment(linker, "/alternatename:?data_ov002_021101e4@@3UState@Player@@A=_data_ov002_021101e4")
#pragma comment(linker, "/alternatename:?data_ov002_0211040c@@3UState@Player@@A=_data_ov002_0211040c")
#pragma comment(linker, "/alternatename:?data_ov002_021105a4@@3UState@Player@@A=_data_ov002_021105a4")
#pragma comment(linker, "/alternatename:?data_ov002_021105bc@@3UState@Player@@A=_data_ov002_021105bc")

/* stale caller names -> renamed callees (the #973 class, host side) */
#pragma comment(linker, "/alternatename:_func_02037670=__ZN11RaycastLine13SetObjAndLineERK7Vector3S2_P5Actor")
#pragma comment(linker, "/alternatename:_func_02037764=__ZN11RaycastLineD1Ev")
#pragma comment(linker, "/alternatename:_func_020377b0=__ZN11RaycastLineC1Ev")
#pragma comment(linker, "/alternatename:_func_02038638=__ZN11RaycastLine10DetectClsnEv")
#pragma comment(linker, "/alternatename:_func_0203b0e8=_AngleDiff")
#pragma comment(linker, "/alternatename:_func_0203b4dc=__ZN4cstd5atan2E5Fix12IiES1_")
#pragma comment(linker, "/alternatename:_func_0203cf78=_Vec3_HorzLen")

/* gate-13 state-family ring (sprint/crouch/punch/slope-jump wave):
   C++-mangled refs from the new St_ TUs -> C-named defs/storage */
#pragma comment(linker, "/alternatename:?data_ov002_020ff130@@3PAHA=_data_ov002_020ff130")
#pragma comment(linker, "/alternatename:?data_ov002_020ff164@@3PAHA=_data_ov002_020ff164")
#pragma comment(linker, "/alternatename:?data_ov002_021101e4@@3PAHA=_data_ov002_021101e4")
#pragma comment(linker, "/alternatename:?data_ov002_02110694@@3PAHA=_data_ov002_02110694")
#pragma comment(linker, "/alternatename:?data_ov002_021106dc@@3PAHA=_data_ov002_021106dc")
#pragma comment(linker, "/alternatename:?data_ov002_02110724@@3PAHA=_data_ov002_02110724")
#pragma comment(linker, "/alternatename:?data_ov002_0210e160@@3HA=_data_ov002_0210e160")
#pragma comment(linker, "/alternatename:?data_ov002_0211013c@@3HA=_data_ov002_0211013c")
#pragma comment(linker, "/alternatename:?data_0209f4a4@@3PAFA=_data_0209f4a4")
#pragma comment(linker, "/alternatename:?data_0209f318@@3PAUCamera@@A=_data_0209f318")
#pragma comment(linker, "/alternatename:?func_ov002_020d1164@@YAHPAX@Z=_func_ov002_020d1164")
#pragma comment(linker, "/alternatename:?func_ov002_020d1204@@YAHPAX@Z=_func_ov002_020d1204")
#pragma comment(linker, "/alternatename:?func_ov002_020d12b0@@YAHPAX@Z=_func_ov002_020d12b0")
#pragma comment(linker, "/alternatename:?func_ov002_020dc020@@YAHPAX@Z=_func_ov002_020dc020")
#pragma comment(linker, "/alternatename:?func_ov002_020e25f0@@YAHPAXH@Z=_func_ov002_020e25f0")
#pragma comment(linker, "/alternatename:?func_ov002_020e2ad0@@YAHPAX@Z=_func_ov002_020e2ad0")
#pragma comment(linker, "/alternatename:?func_ov002_020e2b6c@@YAHPAX@Z=_func_ov002_020e2b6c")
#pragma comment(linker, "/alternatename:?func_ov002_020e2ba8@@YAHPAX@Z=_func_ov002_020e2ba8")
#pragma comment(linker, "/alternatename:?func_ov002_020e2be4@@YAHPAX@Z=_func_ov002_020e2be4")
#pragma comment(linker, "/alternatename:?func_ov002_020e2c84@@YAHPAD@Z=_func_ov002_020e2c84")
#pragma comment(linker, "/alternatename:?Player_ReleaseHeldActor@@YAHPAX@Z=_Player_ReleaseHeldActor")
#pragma comment(linker, "/alternatename:?_ZN5Sound9PlayBank0EjRK7Vector3@@YAHIPAX@Z=__ZN5Sound9PlayBank0EjRK7Vector3")
#pragma comment(linker, "/alternatename:?data_ov002_0211061c@@3UState@@A=_data_ov002_0211061c")
#pragma comment(linker, "/alternatename:?data_ov002_02110634@@3UState@@A=_data_ov002_02110634")
#pragma comment(linker, "/alternatename:?data_0209f318@@3PAXA=_data_0209f318")
#pragma comment(linker, "/alternatename:?func_0200d580@@YAXPAUCamera@@H@Z=_func_0200d580")
#pragma comment(linker, "/alternatename:?func_ov002_020cc05c@@YAXPAXG@Z=_func_ov002_020cc05c")
#pragma comment(linker, "/alternatename:?func_ov002_020dbaec@@YAXPAX@Z=_func_ov002_020dbaec")
#pragma comment(linker, "/alternatename:?func_ov002_020dd5ec@@YAXPAX@Z=_func_ov002_020dd5ec")
#pragma comment(linker, "/alternatename:?func_ov002_020eee3c@@YAXPAD0@Z=_func_ov002_020eee3c")

/* gate-10 tier-2 state wave: C++-mangled refs from the new St_ TUs
   resolved onto the C-named defs and storage they actually link to. */
#pragma comment(linker, "/alternatename:?FUN_02029934@@YAXXZ=_FUN_02029934")
#pragma comment(linker, "/alternatename:?FUN_02029980@@YAXXZ=_FUN_02029980")
#pragma comment(linker, "/alternatename:?Player_AdvanceAnims@@YAXPAD@Z=_Player_AdvanceAnims")
#pragma comment(linker, "/alternatename:?Player_AdvanceAnims@@YAXPAX@Z=_Player_AdvanceAnims")
#pragma comment(linker, "/alternatename:?Player_DisableInteraction@@YAHPAX@Z=_Player_DisableInteraction")
#pragma comment(linker, "/alternatename:?Player_DisableInteraction@@YAXPAX@Z=_Player_DisableInteraction")
#pragma comment(linker, "/alternatename:?Player_ReleaseHeldActor@@YAXPAX@Z=_Player_ReleaseHeldActor")
#pragma comment(linker, "/alternatename:?Player_ScaleByCharFactor@@YAHPADH@Z=_Player_ScaleByCharFactor")
#pragma comment(linker, "/alternatename:?_Z14ApproachLinearRiii@@YAXPAHHH@Z=__Z14ApproachLinearRiii")
#pragma comment(linker, "/alternatename:?_ZN4cstd5atan2E5Fix12IiES1_@@YAHHH@Z=__ZN4cstd5atan2E5Fix12IiES1_")
#pragma comment(linker, "/alternatename:?_ZN5Actor10SpawnCoinsERK7Vector3j5Fix12IiEs@@YAXPAXABUVector3@@IHF@Z=__ZN5Actor10SpawnCoinsERK7Vector3j5Fix12IiEs")
#pragma comment(linker, "/alternatename:?_ZN5Sound13PlayCharVoiceEjjRK7Vector3@@YAXIIPAX@Z=__ZN5Sound13PlayCharVoiceEjjRK7Vector3")
#pragma comment(linker, "/alternatename:?_ZN6Player11ChangeStateERNS_5StateE@@YAXPAX0@Z=__ZN6Player11ChangeStateERNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZN6Player12FinishedAnimEv@@YAHPAX@Z=__ZN6Player12FinishedAnimEv")
#pragma comment(linker, "/alternatename:?_ZN6Player7SetAnimEji5Fix12IiEj@@YAXPADIHHI@Z=__ZN6Player7SetAnimEji5Fix12IiEj")
#pragma comment(linker, "/alternatename:?_ZN6Player7SetAnimEji5Fix12IiEj@@YAXPAXHHHI@Z=__ZN6Player7SetAnimEji5Fix12IiEj")
#pragma comment(linker, "/alternatename:?_ZN6Player7SetAnimEji5Fix12IiEj@@YAXPAXIHHI@Z=__ZN6Player7SetAnimEji5Fix12IiEj")
#pragma comment(linker, "/alternatename:?_ZN8Particle20RunningSlidingDustAtE5Fix12IiES1_S1_@@YAXHHH@Z=__ZN8Particle20RunningSlidingDustAtE5Fix12IiES1_S1_")
#pragma comment(linker, "/alternatename:?_ZN9ActorBase18MarkForDestructionEv@@YAXPAX@Z=__ZN9ActorBase18MarkForDestructionEv")
#pragma comment(linker, "/alternatename:?_ZNK10ClsnResult9GetClsnIDEv@@YAHPAX@Z=__ZNK10ClsnResult9GetClsnIDEv")
#pragma comment(linker, "/alternatename:?_ZNK12WithMeshClsn13GetWallResultEv@@YAPAXPAX@Z=__ZNK12WithMeshClsn13GetWallResultEv")
#pragma comment(linker, "/alternatename:?_ZNK6Player14GetBodyModelIDEjb@@YAHPADIH@Z=__ZNK6Player14GetBodyModelIDEjb")
#pragma comment(linker, "/alternatename:?data_02092110@@3CA=_data_02092110")
#pragma comment(linker, "/alternatename:?data_0209f250@@3EA=_data_0209f250")
#pragma comment(linker, "/alternatename:?data_0209f28c@@3EA=_data_0209f28c")
#pragma comment(linker, "/alternatename:?data_0209f318@@3PADA=_data_0209f318")
#pragma comment(linker, "/alternatename:?data_0209f49e@@3GA=_data_0209f49e")
#pragma comment(linker, "/alternatename:?data_0209f49e@@3PAGA=_data_0209f49e")
#pragma comment(linker, "/alternatename:?data_0209f4a0@@3PAFA=_data_0209f4a0")
#pragma comment(linker, "/alternatename:?data_020a0e5a@@3PADA=_data_020a0e5a")
#pragma comment(linker, "/alternatename:?data_ov002_020ff0ec@@3PAEA=_data_ov002_020ff0ec")
#pragma comment(linker, "/alternatename:?data_ov002_020ff1c0@@3PAHA=_data_ov002_020ff1c0")
#pragma comment(linker, "/alternatename:?data_ov002_020ff1d0@@3PAHA=_data_ov002_020ff1d0")
#pragma comment(linker, "/alternatename:?data_ov002_020ff254@@3PAHA=_data_ov002_020ff254")
#pragma comment(linker, "/alternatename:?data_ov002_02109fe4@@3PAHA=_data_ov002_02109fe4")
#pragma comment(linker, "/alternatename:?data_ov002_0210a560@@3PAHA=_data_ov002_0210a560")
#pragma comment(linker, "/alternatename:?data_ov002_0210a578@@3PAIA=_data_ov002_0210a578")
#pragma comment(linker, "/alternatename:?data_ov002_0210a584@@3PAIA=_data_ov002_0210a584")
#pragma comment(linker, "/alternatename:?data_ov002_0210a60c@@3PAIA=_data_ov002_0210a60c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a6d4@@3PAIA=_data_ov002_0210a6d4")
#pragma comment(linker, "/alternatename:?data_ov002_0211004c@@3UState@@A=_data_ov002_0211004c")
#pragma comment(linker, "/alternatename:?data_ov002_0211007c@@3PAHA=_data_ov002_0211007c")
#pragma comment(linker, "/alternatename:?data_ov002_0211013c@@3DA=_data_ov002_0211013c")
#pragma comment(linker, "/alternatename:?data_ov002_0211013c@@3PADA=_data_ov002_0211013c")
#pragma comment(linker, "/alternatename:?data_ov002_021101b4@@3DA=_data_ov002_021101b4")
#pragma comment(linker, "/alternatename:?data_ov002_0211031c@@3DA=_data_ov002_0211031c")
#pragma comment(linker, "/alternatename:?data_ov002_021103dc@@3DA=_data_ov002_021103dc")
#pragma comment(linker, "/alternatename:?data_ov002_02110424@@3DA=_data_ov002_02110424")
#pragma comment(linker, "/alternatename:?data_ov002_021106c4@@3PAHA=_data_ov002_021106c4")
#pragma comment(linker, "/alternatename:?func_0200d10c@@YAXPAXE@Z=_func_0200d10c")
#pragma comment(linker, "/alternatename:?func_0200d1e4@@YAXPAD@Z=_func_0200d1e4")
#pragma comment(linker, "/alternatename:?func_0200d63c@@YAXPAXE@Z=_func_0200d63c")
#pragma comment(linker, "/alternatename:?func_0200d6b4@@YAXPAXE@Z=_func_0200d6b4")
#pragma comment(linker, "/alternatename:?func_0200d768@@YAXPAXE@Z=_func_0200d768")
#pragma comment(linker, "/alternatename:?func_0200d7a4@@YAXPAXE@Z=_func_0200d7a4")
#pragma comment(linker, "/alternatename:?func_0200d89c@@YAXPAD@Z=_func_0200d89c")
#pragma comment(linker, "/alternatename:?func_0201226c@@YAHHHHHHF@Z=_func_0201226c")
#pragma comment(linker, "/alternatename:?func_0201fc88@@YAXF@Z=_func_0201fc88")
#pragma comment(linker, "/alternatename:?func_02020388@@YAXH@Z=_func_02020388")
#pragma comment(linker, "/alternatename:?func_02022b04@@YAXHHH@Z=_func_02022b04")
#pragma comment(linker, "/alternatename:?func_02035638@@YAHPAE@Z=_func_02035638")
#pragma comment(linker, "/alternatename:?func_0203564c@@YAHH@Z=_func_0203564c")
#pragma comment(linker, "/alternatename:?func_ov002_020bdb50@@YAXPADH@Z=_func_ov002_020bdb50")
#pragma comment(linker, "/alternatename:?func_ov002_020beb38@@YAHPAD@Z=_func_ov002_020beb38")
#pragma comment(linker, "/alternatename:?func_ov002_020bf56c@@YAHPAXH@Z=_func_ov002_020bf56c")
#pragma comment(linker, "/alternatename:?func_ov002_020bf5e0@@YAXPAX@Z=_func_ov002_020bf5e0")
#pragma comment(linker, "/alternatename:?func_ov002_020bf88c@@YAXPAX@Z=_func_ov002_020bf88c")
#pragma comment(linker, "/alternatename:?func_ov002_020c04e0@@YAHPAD@Z=_func_ov002_020c04e0")
#pragma comment(linker, "/alternatename:?func_ov002_020c1eb4@@YAXPAXH@Z=_func_ov002_020c1eb4")
#pragma comment(linker, "/alternatename:?func_ov002_020c2f64@@YAXPAX@Z=_func_ov002_020c2f64")
#pragma comment(linker, "/alternatename:?func_ov002_020c47f4@@YAHPAD@Z=_func_ov002_020c47f4")
#pragma comment(linker, "/alternatename:?func_ov002_020cc660@@YAXPADH@Z=_func_ov002_020cc660")
#pragma comment(linker, "/alternatename:?func_ov002_020cd190@@YAXPAX@Z=_func_ov002_020cd190")
#pragma comment(linker, "/alternatename:?func_ov002_020cf20c@@YAHPAD@Z=_func_ov002_020cf20c")
#pragma comment(linker, "/alternatename:?func_ov002_020cf2f8@@YAXPAD@Z=_func_ov002_020cf2f8")
#pragma comment(linker, "/alternatename:?func_ov002_020cf384@@YAXPAD@Z=_func_ov002_020cf384")
#pragma comment(linker, "/alternatename:?func_ov002_020d1f78@@YAXPAXI@Z=_func_ov002_020d1f78")
#pragma comment(linker, "/alternatename:?func_ov002_020d3498@@YAXPAX@Z=_func_ov002_020d3498")
#pragma comment(linker, "/alternatename:?func_ov002_020d5ab4@@YAHPAX@Z=_func_ov002_020d5ab4")
#pragma comment(linker, "/alternatename:?func_ov002_020d6368@@YAXPAD@Z=_func_ov002_020d6368")
#pragma comment(linker, "/alternatename:?func_ov002_020d674c@@YAHPAD@Z=_func_ov002_020d674c")
#pragma comment(linker, "/alternatename:?func_ov002_020d718c@@YAXPAX@Z=_func_ov002_020d718c")
#pragma comment(linker, "/alternatename:?func_ov002_020d71a0@@YAXPAD@Z=_func_ov002_020d71a0")
#pragma comment(linker, "/alternatename:?func_ov002_020d93ac@@YAXPAD@Z=_func_ov002_020d93ac")
#pragma comment(linker, "/alternatename:?func_ov002_020d9dcc@@YAHPAX@Z=_func_ov002_020d9dcc")
#pragma comment(linker, "/alternatename:?func_ov002_020daa74@@YAXPAX@Z=_func_ov002_020daa74")
#pragma comment(linker, "/alternatename:?func_ov002_020db54c@@YAXPADHHH@Z=_func_ov002_020db54c")
#pragma comment(linker, "/alternatename:?func_ov002_020dcafc@@YAXPAD@Z=_func_ov002_020dcafc")
#pragma comment(linker, "/alternatename:?func_ov002_020de968@@YAXPAX@Z=_func_ov002_020de968")
#pragma comment(linker, "/alternatename:?func_ov002_020e0f38@@YAXPAXE@Z=_func_ov002_020e0f38")
#pragma comment(linker, "/alternatename:?func_ov002_020e0f38@@YAXPAXH@Z=_func_ov002_020e0f38")
#pragma comment(linker, "/alternatename:?func_ov002_020e28d4@@YAXPAXHH@Z=_func_ov002_020e28d4")
#pragma comment(linker, "/alternatename:?func_ov100_02144fcc@@YAHXZ=_func_ov100_02144fcc")

/* Player::ST_WAIT is an ov006 FUNCTION name; at that address ov002 holds
   the Wait State object, which is what St_WaitQuicksand_Main wants. */
#pragma comment(linker, "/alternatename:__ZN6Player7ST_WAITE=_data_ov002_02110154")

/* Return-type-only variants of methods the port already faces. __thiscall,
   same argument list, result in EAX -- the existing face is ABI-identical
   and every call site here discards or byte-truncates the result. */
#pragma comment(linker, "/alternatename:?ChangeState@Player@@QAEXAAUState@@@Z=?ChangeState@Player@@QAEHAAUState@@@Z")
#pragma comment(linker, "/alternatename:?SetAnim@Player@@QAEHIHHI@Z=?SetAnim@Player@@QAEXIHHI@Z")
#pragma comment(linker, "/alternatename:?SetAnim@Player@@QAEIIHHI@Z=?SetAnim@Player@@QAEXIHHI@Z")
#pragma comment(linker, "/alternatename:?GetBodyModelID@Player@@QBEEI_N@Z=?GetBodyModelID@Player@@QBEII_N@Z")

/* tier-2 round 2: the rest of the ring the new state TUs reach. */
#pragma comment(linker, "/alternatename:?_ZN5Actor13SpawnSoundObjEj@@YAPADPADI@Z=__ZN5Actor13SpawnSoundObjEj")
#pragma comment(linker, "/alternatename:?_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii@@YAPADIIABUVector3@@PBXHH@Z=__ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii")
#pragma comment(linker, "/alternatename:?_ZN5Sound7PlaySubEjjj5Fix12IiEb@@YAHIIIHH@Z=__ZN5Sound7PlaySubEjjj5Fix12IiEb")
#pragma comment(linker, "/alternatename:?_ZN6Player18SetNewHatCharacterEjjb@@YAXPADII_N@Z=__ZN6Player18SetNewHatCharacterEjjb")
#pragma comment(linker, "/alternatename:?_ZN6Player8HasNoCapEv@@YA_NPAD@Z=__ZN6Player8HasNoCapEv")
#pragma comment(linker, "/alternatename:?data_0209212c@@3HA=_data_0209212c")
#pragma comment(linker, "/alternatename:?data_0209f310@@3CA=_data_0209f310")
#pragma comment(linker, "/alternatename:?data_ov002_02110364@@3UState@@A=_data_ov002_02110364")
#pragma comment(linker, "/alternatename:?data_ov002_02110394@@3UState@@A=_data_ov002_02110394")
#pragma comment(linker, "/alternatename:?data_ov002_02110a5c@@3PAUEntry@@A=_data_ov002_02110a5c")
#pragma comment(linker, "/alternatename:?func_ov100_02145014@@YAHXZ=_func_ov100_02145014")

/* Same ADDRESS, different overlay label: 0x020e3078 and 0x020c5dec carry
   an ov002 function too, and ov002 is the overlay the port runs. The src
   files naming them ov006/ov007 describe a different overlay's bytes. */
#pragma comment(linker, "/alternatename:_func_ov006_020e3078=_func_ov002_020e3078")
#pragma comment(linker, "/alternatename:_func_ov007_020c5dec=_func_ov002_020c5dec")
#pragma comment(linker, "/alternatename:?data_0208e42c@@3CA=_data_0208e42c")
#pragma comment(linker, "/alternatename:?data_0209b470@@3CA=_data_0209b470")
#pragma comment(linker, "/alternatename:?data_0209b490@@3HA=_data_0209b490")
#pragma comment(linker, "/alternatename:?data_0209b494@@3HA=_data_0209b494")
#pragma comment(linker, "/alternatename:?data_0209b4b0@@3PAIA=_data_0209b4b0")
#pragma comment(linker, "/alternatename:?data_0209f4a4@@3FA=_data_0209f4a4")
#pragma comment(linker, "/alternatename:?func_ov002_020d5f34@@YAHPADPAX@Z=_func_ov002_020d5f34")
#pragma comment(linker, "/alternatename:?func_ov002_020d708c@@YAXPAD@Z=_func_ov002_020d708c")
#pragma comment(linker, "/alternatename:?func_ov002_020d708c@@YAXPAD@Z=_func_ov002_020d708c")

/* shared-body state Inits (Dive, BackFlip, HeadstandJump, SlideKickRecover,
   WaterJump): the C++-mangled refs their TUs emit -> the C-named storage */
#pragma comment(linker, "/alternatename:?data_ov002_020ff100@@3PAHA=_data_ov002_020ff100")

/* death-state ring (DeadHit, DeadPit, Squish, BurnLava + the KillPlayer
   chain): C++-mangled refs -> the C-named defs and ov002 storage */
#pragma comment(linker, "/alternatename:?KillPlayer@@YAXXZ=_KillPlayer")
#pragma comment(linker, "/alternatename:?func_ov002_020c0108@@YAXPAXH@Z=_func_ov002_020c0108")
#pragma comment(linker, "/alternatename:?func_ov002_020c647c@@YAHPADH@Z=_func_ov002_020c647c")
#pragma comment(linker, "/alternatename:?func_ov002_020c6538@@YAHPAD@Z=_func_ov002_020c6538")
#pragma comment(linker, "/alternatename:?func_ov002_020c6908@@YAHPAD@Z=_func_ov002_020c6908")
#pragma comment(linker, "/alternatename:?func_ov006_020e3078@@YAHPAXPAH@Z=_func_ov002_020e3078")
#pragma comment(linker, "/alternatename:?data_ov002_02109db8@@3PAEA=_data_ov002_02109db8")
#pragma comment(linker, "/alternatename:?data_ov002_0210a07c@@3PAIA=_data_ov002_0210a07c")
#pragma comment(linker, "/alternatename:?data_ov002_0210a424@@3PAHA=_data_ov002_0210a424")
#pragma comment(linker, "/alternatename:?data_ov002_021100f4@@3PAHA=_data_ov002_021100f4")
#pragma comment(linker, "/alternatename:?data_ov002_0211117c@@3EA=_data_ov002_0211117c")

/* NoControl ring (the cutscene/door/pipe state and its 19 per-step
   helpers): C++-mangled refs -> the C-named defs and ov002 storage */
#pragma comment(linker, "/alternatename:?func_ov002_020c84b0@@YAHPAD@Z=_func_ov002_020c84b0")
#pragma comment(linker, "/alternatename:?func_ov002_020c8540@@YAHPAD@Z=_func_ov002_020c8540")
#pragma comment(linker, "/alternatename:?func_ov002_020c8714@@YAXPAD@Z=_func_ov002_020c8714")
#pragma comment(linker, "/alternatename:?func_ov002_020c897c@@YAXPAD@Z=_func_ov002_020c897c")
#pragma comment(linker, "/alternatename:?func_ov002_020c8b54@@YAXPAD@Z=_func_ov002_020c8b54")
#pragma comment(linker, "/alternatename:?func_ov002_020c8b78@@YAXPAD@Z=_func_ov002_020c8b78")
#pragma comment(linker, "/alternatename:?func_ov002_020c8cb0@@YAXPAD@Z=_func_ov002_020c8cb0")
#pragma comment(linker, "/alternatename:?func_ov002_020c8d14@@YAXPAD@Z=_func_ov002_020c8d14")
#pragma comment(linker, "/alternatename:?func_ov002_020c8f0c@@YAXPAD@Z=_func_ov002_020c8f0c")
#pragma comment(linker, "/alternatename:?func_ov002_020c8f80@@YAXPAD@Z=_func_ov002_020c8f80")
#pragma comment(linker, "/alternatename:?func_ov002_020c904c@@YAXPAD@Z=_func_ov002_020c904c")
#pragma comment(linker, "/alternatename:?func_ov002_020c9128@@YAHPAD@Z=_func_ov002_020c9128")
#pragma comment(linker, "/alternatename:?func_ov002_020c91bc@@YAHPAD@Z=_func_ov002_020c91bc")
#pragma comment(linker, "/alternatename:?func_ov002_020c924c@@YAXPAD@Z=_func_ov002_020c924c")
#pragma comment(linker, "/alternatename:?func_ov002_020c9288@@YAXPAD@Z=_func_ov002_020c9288")
#pragma comment(linker, "/alternatename:?func_ov002_020c92fc@@YAXPAD@Z=_func_ov002_020c92fc")
#pragma comment(linker, "/alternatename:?func_ov002_020c94a4@@YAHPAD@Z=_func_ov002_020c94a4")
#pragma comment(linker, "/alternatename:?func_ov002_020c965c@@YAXPAD@Z=_func_ov002_020c965c")
#pragma comment(linker, "/alternatename:?func_ov002_020ca108@@YAXPAD@Z=_func_ov002_020ca108")
#pragma comment(linker, "/alternatename:?func_0200ee68@@YAHXZ=_func_0200ee68")
#pragma comment(linker, "/alternatename:?func_020072c0@@YAXXZ=_func_020072c0")
#pragma comment(linker, "/alternatename:?func_02053274@@YAHPBUVector3@@0@Z=_func_02053274")
#pragma comment(linker, "/alternatename:?Vec3_RotateYAndTranslate@@YAXPAUVector3@@PAXH1@Z=_Vec3_RotateYAndTranslate")
#pragma comment(linker, "/alternatename:?PlayBank0@Sound@@YAXIABUVector3@@@Z=__ZN5Sound9PlayBank0EjRK7Vector3")
#pragma comment(linker, "/alternatename:?_ZN6Player11ShowMessageER9ActorBasejPK7Vector3jj@@YAXPAUPlayer@@AAUActorBase@@IPBUVector3@@II@Z=__ZN6Player11ShowMessageER9ActorBasejPK7Vector3jj")
#pragma comment(linker, "/alternatename:?_ZN6Player12FinishedAnimEv@@YAHPAUPlayer@@@Z=__ZN6Player12FinishedAnimEv")
#pragma comment(linker, "/alternatename:?_ZNK6Player14GetBodyModelIDEjb@@YAHPBUPlayer@@I_N@Z=__ZNK6Player14GetBodyModelIDEjb")
#pragma comment(linker, "/alternatename:?_ZNK9Animation12WillHitFrameEi@@YAHPBUAnimation@@H@Z=__ZNK9Animation12WillHitFrameEi")
#pragma comment(linker, "/alternatename:?data_ov002_0210e150@@3HA=_data_ov002_0210e150")
#pragma comment(linker, "/alternatename:?data_ov002_0210f89c@@3HA=_data_ov002_0210f89c")
#pragma comment(linker, "/alternatename:?data_ov002_0210f8cc@@3PAHA=_data_ov002_0210f8cc")
/* return-type-only variant of the Animation::WillHitFrame face the port
   already has (bool vs int; same __thiscall shape, result in EAX) */
#pragma comment(linker, "/alternatename:?WillHitFrame@Animation@@QBEHH@Z=?WillHitFrame@Animation@@QBE_NH@Z")

/* gate 13, the real Camera: the C++-mangled references its TUs emit (they
   declare their externs outside extern "C") -> the C-named definitions, plus
   the two community names that sit on top of matched symbols. */
#pragma comment(linker, "/alternatename:?CAM_SPACE_CAM_POS_ASR_3@@3DA=_CAM_SPACE_CAM_POS_ASR_3")
#pragma comment(linker, "/alternatename:?data_0209b008@@3DA=_data_0209b008")
#pragma comment(linker, "/alternatename:?data_0209b008@@3PAUCamera_State@@A=_data_0209b008")
#pragma comment(linker, "/alternatename:?data_0209b41c@@3DA=_data_0209b41c")
#pragma comment(linker, "/alternatename:?data_0208733c@@3IA=_data_0208733c")
#pragma comment(linker, "/alternatename:?data_0209f20c@@3EA=_data_0209f20c")
#pragma comment(linker, "/alternatename:?data_0209f294@@3EA=_data_0209f294")
#pragma comment(linker, "/alternatename:?data_0209f2c4@@3EA=_data_0209f2c4")
#pragma comment(linker, "/alternatename:?_ZN6Camera11ChangeStateEPNS_5StateE@@YAHPAUCamera@@PAUCamera_State@@@Z=__ZN6Camera11ChangeStateEPNS_5StateE")
#pragma comment(linker, "/alternatename:?_ZNK6Camera12IsUnderwaterEv@@YAHPAX@Z=__ZNK6Camera12IsUnderwaterEv")
#pragma comment(linker, "/alternatename:?func_0200ca50@@YAHPAX@Z=_func_0200ca50")
#pragma comment(linker, "/alternatename:?func_0203dafc@@YAXH@Z=_func_0203dafc")
#pragma comment(linker, "/alternatename:?Math_Function_0203b0fc@@YAXPAHHHH@Z=_Math_Function_0203b0fc")
#pragma comment(linker, "/alternatename:?_ZN8Particle6System10NewWeatherEjj5Fix12IiES2_S2_PK11Vector3_16fj@@YAIIIHHHPBXE@Z=__ZN8Particle6System10NewWeatherEjj5Fix12IiES2_S2_PK11Vector3_16fj")
/* the G3i pair are host copies (port/unmatched/); their TU declares them as
   C names, Camera::Render as class statics */
#pragma comment(linker, "/alternatename:?PerspectiveW_@G3i@@SAXHHHHHH_NPAUMatrix4x3@@@Z=__ZN3G3i13PerspectiveW_E5Fix12IiES1_S1_S1_S1_S1_bP9Matrix4x3")
#pragma comment(linker, "/alternatename:?LookAt_@G3i@@SAXPBUVector3@@00_NPAUMatrix4x3@@@Z=__ZN3G3i7LookAt_EPK7Vector3S2_S2_bP9Matrix4x3")
#pragma comment(linker, "/alternatename:?SetBlendAlpha@G2x@@SAXPCGGGGG@Z=__ZN3G2x13SetBlendAlphaEPVttttt")
#pragma comment(linker, "/alternatename:?Render@OAM@@SAX_NPAUOamAttr@@HHHHHHHH@Z=__ZN3OAM6RenderEbP7OamAttriiii5Fix12IiES3_ii")
/* community names for matched symbols: Vec3_DistSq IS func_0203cf94,
   STAR_MARKERS is the bss array at 0x0209f40c, and func_0200cc5c is
   Camera::SaveCameraStateBeforeTalk (both callers spell it argless) */
#pragma comment(linker, "/alternatename:_Vec3_DistSq=_func_0203cf94")
#pragma comment(linker, "/alternatename:_STAR_MARKERS=_data_0209f40c")
#pragma comment(linker, "/alternatename:_func_0200cc5c=__ZN6Camera25SaveCameraStateBeforeTalkEv")
/* ActorBase's own vtable symbol: the transient install the ctor/dtor chain
   writes, already storage in hal/actor_vtables.cpp under its DS name */
#pragma comment(linker, "/alternatename:__ZTV9ActorBase=_data_02099edc")
/* Camera::~Camera (D0) frees by C name; Memory::Deallocate's own TU declares
   Heap as a `class` (PAV) while every header spells it `struct` (PAU), so the
   alias carries the decorated name from the link log rather than a face */
#pragma comment(linker, "/alternatename:__ZN6Memory10DeallocateEPvP4Heap=?Deallocate@Memory@@YAXPAXPAVHeap@@@Z")

/* ---- gate 14, the level boot -------------------------------------------
   Stage::LoadClsnAndObjects and the sub-loaders pick their externs out of
   include/decl_common.h, which is generated without extern "C", so a .cpp
   TU emits MSVC manglings for symbols the .c definitions publish as plain C
   names. Same closure as the state waves; nothing in src/ changes. */
#pragma comment(linker, "/alternatename:?ContinueKuppaScriptIfNecessary@@YAHXZ=_ContinueKuppaScriptIfNecessary")
#pragma comment(linker, "/alternatename:?StartIntroCutscene@@YAXXZ=_StartIntroCutscene")
#pragma comment(linker, "/alternatename:?func_0202a850@@YAXHH@Z=_func_0202a850")
#pragma comment(linker, "/alternatename:?func_0203aca0@@YAXHH@Z=_func_0203aca0")
#pragma comment(linker, "/alternatename:?func_0203accc@@YAXH@Z=_func_0203accc")
#pragma comment(linker, "/alternatename:?data_02092134@@3HA=_data_02092134")
#pragma comment(linker, "/alternatename:?data_ov002_0210cb70@@3PAEA=_data_ov002_0210cb70")
#pragma comment(linker, "/alternatename:?data_ov002_0210cb88@@3PAGA=_data_ov002_0210cb88")
#pragma comment(linker, "/alternatename:?data_ov002_0210cbf4@@3PAGA=_data_ov002_0210cbf4")
#pragma comment(linker, "/alternatename:?data_ov002_0211118c@@3FA=_data_ov002_0211118c")
/* Sound::LoadInitialGroup is a class static in its TU and a C name to the
   kuppa tail; LoadGroupAndSetBank is the mirror case one call deeper. */
#pragma comment(linker, "/alternatename:__ZN5Sound16LoadInitialGroupEi=?LoadInitialGroup@Sound@@SAXH@Z")
#pragma comment(linker, "/alternatename:?LoadGroupAndSetBank@Sound@@SAXHH@Z=__ZN5Sound19LoadGroupAndSetBankEii")
/* gate 14, stage A2: the entrance step handlers. 020c71e0's own TU spells it
   as a C name while 020c72a4's declares it without extern "C". */
#pragma comment(linker, "/alternatename:?func_ov002_020c71e0@@YAXPAX@Z=_func_ov002_020c71e0")
/* NOT aliases: Actor::GetBitInDeathTable and Actor::AfterInitResources are
   real MSVC methods, so their C-named callers would enter a __thiscall body
   through a cdecl call and read `this` out of ecx garbage. Both get faces in
   hal/level_boot.cpp instead. */
