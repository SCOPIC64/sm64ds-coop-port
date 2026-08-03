// Reverse bridges: MSVC-method references -> C-named definitions.
//
// Slice .cpp TUs compiled against local shadows call these as real methods
// (?Name@Class@@QAE...); the definitions live under Itanium C names in
// their own files. Shadow classes here (NO game headers -- the owner name
// alone fixes the mangling) define each member forwarding to the C name.
// Sound/Scene statics that have no host backend yet are quiet stubs; the
// port grows real ones with the audio/fader gates.

struct Vector3 { int x, y, z; };
struct Actor;
struct OamAttr;
struct Matrix2x2;
struct FaderBrightness;
struct Vector3_16;
struct State;

extern "C" {
void _ZN13RaycastGroundC1Ev(void *self);
void _ZN13RaycastGroundD1Ev(void *self);
void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(void *self,
                                                        const void *v,
                                                        void *a);
void _ZN11RaycastLineC1Ev(void *self);
void _ZN11RaycastLineD1Ev(void *self);
void _ZN11RaycastLine13SetObjAndLineERK7Vector3S2_P5Actor(void *self,
                                                          const void *a,
                                                          const void *b,
                                                          void *actor);
void _ZN11RaycastLine10GetClsnPosEv(void *res, void *self);
int _ZN6Player11ChangeStateERNS_5StateE(void *self, void *st);
int _ZN6Player7IsStateERNS_5StateE(void *self, void *st);
struct Actor *_ZN5Actor10FindWithIDEj(unsigned id);
int _ZNK10ClsnResult9GetClsnIDEv(const void *self);
int _ZNK12WithMeshClsn10IsOnGroundEv(const void *self);
unsigned char _ZN3OAM11GetObjWidthEii(int a, int b);
unsigned char _ZN3OAM12GetObjHeightEii(int a, int b);
int _ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2(void *attr, int *p,
                                                     void *m);
int _ZN8SaveData19IsCharacterUnlockedEj(unsigned ch);
int _ZN4cstd4fdivEii(int a, int b);
}

struct RaycastGround {
    RaycastGround();
    ~RaycastGround();
    void SetObjAndPos(const Vector3 &v, Actor *a);
};
RaycastGround::RaycastGround() { _ZN13RaycastGroundC1Ev(this); }
RaycastGround::~RaycastGround() { _ZN13RaycastGroundD1Ev(this); }
void RaycastGround::SetObjAndPos(const Vector3 &v, Actor *a)
{ _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(this, &v, a); }

struct RaycastLine {
    RaycastLine();
    ~RaycastLine();
    int DetectClsn();
    void SetObjAndLine(const Vector3 &a, const Vector3 &b, Actor *actor);
    Vector3 GetClsnPos();
};
RaycastLine::RaycastLine() { _ZN11RaycastLineC1Ev(this); }
RaycastLine::~RaycastLine() { _ZN11RaycastLineD1Ev(this); }
void RaycastLine::SetObjAndLine(const Vector3 &a, const Vector3 &b,
                                Actor *actor)
{ _ZN11RaycastLine13SetObjAndLineERK7Vector3S2_P5Actor(this, &a, &b, actor); }
Vector3 RaycastLine::GetClsnPos()
{
    Vector3 tmp;
    _ZN11RaycastLine10GetClsnPosEv(&tmp, this);
    return tmp;
}

struct Player {
    struct State;
    int ChangeState(::State &s);
    void ChangeState(State &s);
    int IsState(::State &s);
    int IsState(State &s);
    void SetAnim(unsigned a, int b, int f, unsigned d);
};
int Player::ChangeState(::State &s)
{ return _ZN6Player11ChangeStateERNS_5StateE(this, &s); }
void Player::ChangeState(State &s)
{ _ZN6Player11ChangeStateERNS_5StateE(this, &s); }
int Player::IsState(::State &s)
{ return _ZN6Player7IsStateERNS_5StateE(this, &s); }
int Player::IsState(State &s)
{ return _ZN6Player7IsStateERNS_5StateE(this, &s); }
extern "C" int _ZN6Player7SetAnimEji5Fix12IiEj(void *, unsigned, int, int,
                                               unsigned);
void Player::SetAnim(unsigned a, int b, int f, unsigned d)
{ _ZN6Player7SetAnimEji5Fix12IiEj(this, a, b, f, d); }

struct ClsnResult { int GetClsnID() const; };
int ClsnResult::GetClsnID() const { return _ZNK10ClsnResult9GetClsnIDEv(this); }

struct WithMeshClsn {
    int IsOnGround() const;
    int GetWallResult() const;
    int GetFloorResult() const;
};
int WithMeshClsn::IsOnGround() const
{ return _ZNK12WithMeshClsn10IsOnGroundEv(this); }

struct Actor_statics_shadow;   /* Actor is only a return type here */
struct Actor2 { static Actor *FindWithID(unsigned id); };
/* FindWithID's owner must literally be "Actor" for the mangling: */
struct CylinderClsn;
struct Actor {
    static Actor *FindWithID(unsigned id);
    void UpdatePosWithOnlySpeed(CylinderClsn *c);
};
Actor *Actor::FindWithID(unsigned id)
{ return (Actor *)_ZN5Actor10FindWithIDEj(id); }

template <typename T> struct Fix12 { T val; };
struct OAM {
    static unsigned char GetObjWidth(int a, int b);
    static unsigned char GetObjHeight(int a, int b);
    static int LoadAffineParams(OamAttr *attr, int *p, Matrix2x2 *m);
    static void Render(bool sub, OamAttr *attr, int x, int y, int pal,
                       int pri, Fix12<int> sx, Fix12<int> sy, int rot,
                       int mode);
};
unsigned char OAM::GetObjWidth(int a, int b)
{ return _ZN3OAM11GetObjWidthEii(a, b); }
unsigned char OAM::GetObjHeight(int a, int b)
{ return _ZN3OAM12GetObjHeightEii(a, b); }
int OAM::LoadAffineParams(OamAttr *attr, int *p, Matrix2x2 *m)
{ return _ZN3OAM16LoadAffineParamsEP7OamAttrPiP9Matrix2x2(attr, p, m); }

struct SaveData { static int IsCharacterUnlocked(unsigned ch); };
int SaveData::IsCharacterUnlocked(unsigned ch)
{ return _ZN8SaveData19IsCharacterUnlockedEj(ch); }

namespace cstd { int fdiv(int a, int b); }
int cstd::fdiv(int a, int b) { return _ZN4cstd4fdivEii(a, b); }

/* ---- sound / fader / particle statics: no host backend yet ------------- */
struct Sound {
    static int PlaySub(unsigned, unsigned, unsigned, int, bool);
    static void Play2D(unsigned, unsigned);
    static void LoadAndSetMusic_Layer1(int);
    static void StopLoadedMusic_Layer1(unsigned);
};
int Sound::PlaySub(unsigned, unsigned, unsigned, int, bool) { return 0; }
void Sound::Play2D(unsigned, unsigned) {}
void Sound::LoadAndSetMusic_Layer1(int) {}
void Sound::StopLoadedMusic_Layer1(unsigned) {}

struct Scene {
    static void SetAndStopColorFader();
    static void SetFaders(FaderBrightness *);
    static void StartSceneFade(unsigned, unsigned, unsigned short);
};
void Scene::SetAndStopColorFader() {}
void Scene::SetFaders(FaderBrightness *) {}
void Scene::StartSceneFade(unsigned, unsigned, unsigned short) {}

struct Particle {
    struct Callback;
    struct System {
        static void New(unsigned, unsigned, int, int, int,
                        const Vector3_16 *, Particle::Callback *);
    };
};
void Particle::System::New(unsigned, unsigned, int, int, int,
                           const Vector3_16 *, Particle::Callback *) {}

/* C-named particle effect entries (the real src bodies walk a particle
   manager the host never seats -- St_Land's dust NewSimple faulted on
   the null system). No-ops until the particle subsystem is hosted. */
extern "C" {
void *_ZN8Particle6System9NewSimpleEj5Fix12IiES2_S2_(unsigned, int, int, int)
{ return 0; }
void *_ZN8Particle6System3NewEjj5Fix12IiES2_S2_PK11Vector3_16fPNS_8CallbackE(
    unsigned, unsigned, int, int, int, const void *, float, void *)
{ return 0; }
void _ZN8Particle20RunningSlidingDustAtE5Fix12IiES1_S1_(int, int, int) {}
void *_ZN8Particle6System12NewBigSplashE5Fix12IiES2_S2_(int, int, int)
{ return 0; }
}

extern "C" int _ZN11RaycastLine10DetectClsnEv(void *self)
{ return ((RaycastLine *)self)->DetectClsn(); }
extern "C" int _ZNK12WithMeshClsn13GetWallResultEv(const void *self)
{ return ((const WithMeshClsn *)self)->GetWallResult(); }
extern "C" int _ZNK12WithMeshClsn14GetFloorResultEv(const void *self)
{ return ((const WithMeshClsn *)self)->GetFloorResult(); }

struct SphereClsn { int DetectClsn(); };
extern "C" int _ZN10SphereClsn10DetectClsnEv(void *self)
{ return ((SphereClsn *)self)->DetectClsn(); }

struct Message { void Update(); static void AddChar(char c); };
extern "C" void _ZN7Message6UpdateEv(void *self)
{ ((Message *)self)->Update(); }
extern "C" void _ZN7Message7AddCharEc(char ch)
{ Message::AddChar(ch); }


extern "C" void *_ZN3OAM6RenderEbP7OamAttriiii5Fix12IiES3_ii(
    int sub, void *attr, int x, int y, int pal, int pri,
    int sx, int sy, int rot, int mode)
{
    Fix12<int> fsx, fsy;
    fsx.val = sx; fsy.val = sy;
    OAM::Render(sub != 0, (OamAttr *)attr, x, y, pal, pri, fsx, fsy, rot,
                mode);
    return 0;
}

/* shadow-defined in their own TUs (struct CylinderClsn / struct Camera) */
extern "C" void _ZN5Actor22UpdatePosWithOnlySpeedEP12CylinderClsn(void *self,
                                                                  void *cl)
{ ((Actor *)self)->UpdatePosWithOnlySpeed((CylinderClsn *)cl); }
struct Camera { void SetFlag_3(); };
extern "C" void _ZN6Camera9SetFlag_3Ev(void *self)
{ ((Camera *)self)->SetFlag_3(); }

/* REAL BODY here, not a hop back to the method: the method shadow above
   already forwards to this C name, and forwarding back made a mutual
   tail-call -- /O2 turned it into a two-instruction jmp cycle that hung
   the first real-collision frame (WATCHDOG pinned EIP inside it).
   Field per src/_ZNK10ClsnResult9GetClsnIDEv.cpp: objID at +0x1c. */
extern "C" int _ZNK10ClsnResult9GetClsnIDEv(const void *self)
{ return (int)((const unsigned *)self)[7]; }
