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
int data_0209b0c8;

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
int data_0209f274[8], data_0209f324[8];
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
