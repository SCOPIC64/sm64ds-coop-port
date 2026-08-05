// GATE 32: the linkage seam for Bob-omb Battlefield's cast.
//
// Same two jobs hal/cxx_aliases.cpp and hal/actor_class_faces.cpp already do,
// kept in a file of this gate's own so the merge with the other actor stream
// is a file add rather than a diff through two shared files.
//
//   * a C++-MANGLED REFERENCE onto a C-named definition gets a linker alias.
//     Both sides are cdecl and describe the same ROM function; the mangling
//     differs only because the referencing TU declared the symbol inside a
//     namespace or a shadow class instead of extern "C".
//   * a __thiscall METHOD reference gets a real definition against a shadow
//     class of the same name, because an alias cannot bridge ecx-vs-stack.
//
// Nothing here invents behaviour. The two exceptions are called out where they
// are, and both are the ROM's own answer rather than a guess.
#include <cstdio>
#include <cstdlib>

extern "C" {
/* the C-named definitions the aliases below land on */
void *_ZN5Model8LoadFileER13SharedFilePtr(void *fp);      /* cxxname_bridge */
char *_ZN9Animation8LoadFileER13SharedFilePtr(void *fp);  /* player_bridges */
void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(unsigned, unsigned,
                                                   const void *, const void *,
                                                   int, int);
unsigned _ZN5Sound8PlayLongEjjjRK7Vector3j(unsigned, unsigned, unsigned,
                                           const void *, unsigned);
void *_ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
    unsigned, unsigned, int, int, int, const void *, void *);
int _ZNK12WithMeshClsn13JustHitGroundEv(void *self);
int _ZNK12WithMeshClsn10IsOnGroundEv(const void *self);
void _ZN5Actor8PoofDustEv(void *self);
int func_ov002_020ada40(void *self, void *v, void *other, unsigned r);
extern int data_020ad560[];        /* the cylinder template BMD (stub bytes) */
}

/* ---- static/namespace functions: same ABI, different spelling -------------
   BobOmb::InitResources declares Animation::LoadFile returning void and
   Model::LoadFile returning BMD_File*; the definitions return char* and void*.
   One register either way. Enemy::SpawnCoin's Actor::Spawn and
   func_ov102_0214b248's Sound::PlayLong are the same shape, and
   Enemy::SpawnMegaCharParticles declares Particle::System::New by its ITANIUM
   name inside C++ without extern "C", so MSVC mangles the mangled name. */
#pragma comment(linker, "/alternatename:?LoadFile@Animation@@SAXAAUSharedFilePtr@@@Z=__ZN9Animation8LoadFileER13SharedFilePtr")
#pragma comment(linker, "/alternatename:?LoadFile@Model@@SAPAUBMD_File@@AAUSharedFilePtr@@@Z=__ZN5Model8LoadFileER13SharedFilePtr")
#pragma comment(linker, "/alternatename:?Spawn@Actor@@SAPAU1@IIABUVector3@@PBUVector3_16@@HH@Z=__ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii")
#pragma comment(linker, "/alternatename:?PlayLong@Sound@@YAIIIIABUVector3@@I@Z=__ZN5Sound8PlayLongEjjjRK7Vector3j")
#pragma comment(linker, "/alternatename:?_ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE@@YAPAXIIHHHPBXPAX@Z=__ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE")

/* ---- the ov002 name without its overlay tag -------------------------------
   BobOmb::Behavior (and four other levels' classes) spell ov002 0x020ada40 as
   `func_020ada40`, with no overlay in the name. The reloc settles which one it
   is: ov102 0x0214c328 targets 0x020ada40 in module overlays(2,4), and ov002
   is the overlay this port mounts. */
#pragma comment(linker, "/alternatename:_func_020ada40=_func_ov002_020ada40")

/* ---- data: the same object under a different declared type ---------------- */
#pragma comment(linker, "/alternatename:?data_ov102_0214e9c0@@3USharedFilePtr@@A=_data_ov102_0214e9c0")
#pragma comment(linker, "/alternatename:?data_ov102_0214e9c8@@3USharedFilePtr@@A=_data_ov102_0214e9c8")
#pragma comment(linker, "/alternatename:?data_ov102_0214e9c8@@3UG2@@A=_data_ov102_0214e9c8")
#pragma comment(linker, "/alternatename:?data_ov002_0210d9e0@@3USharedFilePtr@@A=_data_ov002_0210d9e0")
#pragma comment(linker, "/alternatename:?data_02082128@@3US48@@A=_data_02082128")
#pragma comment(linker, "/alternatename:?data_ov002_020ff014@@3GA=_data_ov002_020ff014")
#pragma comment(linker, "/alternatename:?data_ov002_0210dbc0@@3PAP8Enemy@@AEHAAUWithMeshClsn@@@ZA=_data_ov002_0210dbc0")

// ---- __thiscall method faces ------------------------------------------------
//
// Each is a real definition against a shadow class with the ROM's own name, so
// the decorated symbol comes out identical to the one the caller emitted. The
// bodies forward to the C-named definition the port already has.

/* WithMeshClsn2: the name func_ov102_0214aa18's own TU gives the collider at
   +0x144. Both methods are the plain WithMeshClsn ones. */
struct WithMeshClsn2 { int JustHitGround() const; int IsOnGround() const; };
int WithMeshClsn2::JustHitGround() const
{ return _ZNK12WithMeshClsn13JustHitGroundEv((void *)this); }
int WithMeshClsn2::IsOnGround() const
{ return _ZNK12WithMeshClsn10IsOnGroundEv((const void *)this); }

/* Actor::PoofDust, reached as a method by Enemy::SpawnCoin. */
struct Actor { void PoofDust(); };
void Actor::PoofDust() { _ZN5Actor8PoofDustEv(this); }

/* Enemy's two methods that the rest of the Enemy tier reaches by their Itanium
   C names. Their own TUs define them as real methods over locally-declared
   classes, so this is the ordinary face direction, not an alias. */
struct Enemy {
    void SpawnCoin();
    void SpawnMegaCharParticles(Actor &a, char *p);
};
extern "C" void _ZN5Enemy9SpawnCoinEv(void *self)
{ ((Enemy *)self)->Enemy::SpawnCoin(); }
extern "C" void _ZN5Enemy22SpawnMegaCharParticlesER5ActorPc(void *self,
                                                            void *a, char *p)
{ ((Enemy *)self)->Enemy::SpawnMegaCharParticles(*(Actor *)a, p); }

/* Player::IncMegaKillCount, which the knockback at ov002 0x020ada40 runs when
   a mega-form Mario is the one that hit the enemy. Its own TU defines it
   against include/Player.h; the caller spells the C name. */
struct Player { void IncMegaKillCount(); };
extern "C" void _ZN6Player16IncMegaKillCountEv(void *self)
{ ((Player *)self)->Player::IncMegaKillCount(); }

// ---- the two RIDE-THROUGH returns ------------------------------------------
//
// ModelBase::SetFile and ShadowModel::InitCylinder both end in a TAIL BRANCH on
// the ROM, so r0 carries the callee's result out even though the decomp
// declares both `void`. Every caller the port had before ignored the value;
// BobOmb::InitResources is the first that tests it, twice:
//
//     if (SetFile(bmd, 1, -1) == 0) return 0;
//     if (InitCylinder() == 0)      return 0;
//
// so a void definition and an eax full of whatever the last call left would
// decide whether the bomb initialises at all.
//
// The int-returning spellings the caller emitted are DEFINED here rather than
// aliased onto the void ones. Both are __thiscall against the ROM's own class
// name, so the decorated symbols match exactly.

/* SetFile is `DoSetFile(file, a, b)` and nothing else -- the ROM branches
   straight into it. DoSetFile is slot 1 of the host tables
   (hal/cxxname_bridge.cpp fills _ZTV5Model and _ZTV9ModelAnim in MSVC order:
   dtor 0, DoSetFile 1, UpdateVerts 2, Virtual10 3, Render 4), and every slot
   there is a __fastcall thunk. Dispatching it here is what returns the real
   value. */
struct BMD_File;
struct ModelBase { int SetFile(BMD_File *file, int a, int b); };

typedef int(__fastcall *PortDoSetFile)(void *, void *, char *, int, int);

int ModelBase::SetFile(BMD_File *file, int a, int b)
{
    void **vt = *(void ***)this;
    if (!vt || !vt[1]) {
        std::fprintf(stderr, "FATAL: ModelBase::SetFile on %p: vtable %p has "
                     "no DoSetFile\n", (void *)this, (void *)vt);
        std::abort();
    }
    return ((PortDoSetFile)vt[1])(this, 0, (char *)file, a, b);
}

/* InitCylinder is `SetFile(&data_020ad560, 1, -1)`, and THE SHADOW SYSTEM IS
   DEFERRED: hal/cxxname_bridge.cpp records the three reasons (ov001 is not
   mounted so the template BMD's bytes are absent, _ZTV11ShadowModel is eight
   nulls, and no per-frame RenderAll caller is hosted), and
   hal/player_bridges.cpp already stubs the C-named spelling of this same
   function to nothing. So this one does not call SetFile either -- it returns
   the ROM's success value, 1, which is what the callers test for. When the
   shadow system lands this becomes the same dispatch as SetFile above. */
struct ShadowModel { int InitCylinder(); };
int ShadowModel::InitCylinder() { return 1; }
