/* MSVC-METHOD FACES for three arm9 bodies the BBH cast's matched .cpp TUs
 * call as real C++ methods -- each TU declares a minimal local class and
 * dispatches ?Method@Class@@... by MSVC name, while the matched body compiles
 * with C linkage. Each face here IS the MSVC method, defined against the same
 * minimal local declaration (this file includes NO class headers, so the
 * local shapes cannot collide with include/'s richer ones -- the reason these
 * do not sit in hal/actor_classes_ov063.cpp, which includes Actor.h), and it
 * forwards `this` into the C body. An /alternatename can never do this: the
 * targets are __thiscall (the method_faces.cpp law).
 *
 *   ?TrackStar@Actor@@QAEIII@Z            <- _ZN5Actor9TrackStarEjj (arm9
 *       0x0200ff94, matched .c on slice_w5a.txt); wanted by
 *       _ZN10BigBooIcon13InitResourcesEv.cpp (the boss star marker)
 *   ?JustHitGround@WithMeshClsn@@QBE_NXZ  <- _ZNK12WithMeshClsn
 *       13JustHitGroundEv (arm9 0x0203571c, matched .c); wanted by
 *       _ZN7BooCage8BehaviorEv.cpp (the cage's landing bounce). Const
 *       method, bool return -- the C body returns s32, normalized != 0.
 *   ?JumpIntoBooCage@Player@@QAEXAAUVector3@@@Z <- _ZN6Player
 *       15JumpIntoBooCageER7Vector3 (ov002, matched extern-C .cpp); wanted
 *       by the same Behavior (the player getting pulled into the cage).
 */

struct Vector3;
struct Vector3_16;

extern "C" {
int _ZN5Actor9TrackStarEjj(void *self, unsigned a, unsigned b);
int _ZNK12WithMeshClsn13JustHitGroundEv(const void *self);
int _ZN6Player15JumpIntoBooCageER7Vector3(void *self, Vector3 *v);
}

struct Actor { unsigned TrackStar(unsigned a, unsigned b); };
unsigned Actor::TrackStar(unsigned a, unsigned b)
{ return (unsigned)_ZN5Actor9TrackStarEjj(this, a, b); }

struct WithMeshClsn { bool JustHitGround() const; };
bool WithMeshClsn::JustHitGround() const
{ return _ZNK12WithMeshClsn13JustHitGroundEv(this) != 0; }

struct Player { void JumpIntoBooCage(Vector3 &v); };
void Player::JumpIntoBooCage(Vector3 &v)
{ _ZN6Player15JumpIntoBooCageER7Vector3(this, &v); }
