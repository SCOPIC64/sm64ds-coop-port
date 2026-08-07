// Gate 82: the one method face SHUTTER_BOB's InitResources needs that the rest
// of hal/actor_classes_bob_world.cpp cannot supply.
//
// src/_ZN10ShutterBob13InitResourcesEv.cpp declares its own local
//   class Actor {};
//   class MeshColliderBase { public: void Enable(Actor *a); };
// and calls Enable as a thiscall method. MSVC mangles that reference with the
// CLASS tag on Actor -> ?Enable@MeshColliderBase@@QAEXPAVActor@@@Z (PAV = "ptr
// to class Actor"). The real MeshColliderBase::Enable body returns int and, in
// every TU that includes Actor.h, Actor is a STRUCT, so a thunk compiled there
// mangles the parameter as PAU (struct) and does not match. This TU deliberately
// does NOT include Actor.h: here `class Actor` stays a class, so the symbol this
// file emits is byte-for-byte the one the source references. It forwards to the
// C-linkage host wrapper cxxname_bridge.cpp routes every other Enable through.
extern "C" int _ZN16MeshColliderBase6EnableEP5Actor(void *self, void *actor);

class Actor;
class MeshColliderBase {
public:
    void Enable(Actor *a);
};
void MeshColliderBase::Enable(Actor *a)
{ _ZN16MeshColliderBase6EnableEP5Actor(this, a); }
