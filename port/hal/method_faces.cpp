// C-linkage faces for METHOD-form definitions (gate 10/11 Behavior ring).
//
// The defining src files compile real methods against the shared headers;
// their .c-file callers reference Itanium C names. Each face forwards with
// a qualified call, the player_bridges pattern, batched here because the
// include surface spans most of the actor stack.
#include "Actor.h"
#include "ActorBase.h"
#include "BgCh.h"
#include "Camera.h"
#include "ClsnResult.h"
#include "CylinderClsn.h"
#include "CylinderClsnWithPos.h"
#include "Heap.h"
#include "Message.h"
#include "ModelBase.h"
#include "Model.h"
#include "ModelAnim2.h"
#include "OAM.h"
#include "PathPtr.h"
#include "Player.h"
#include "RaycastLine.h"
#include "SphereClsn.h"
#include "TextureSequence.h"
#include "Timer.h"
#include "WithMeshClsn.h"

extern "C++" int ApproachLinear2(short &x, short target, short step);

extern "C" {

int _Z15ApproachLinear2Rsss(short *x, short target, short step)
{ return ApproachLinear2(*x, target, step); }

void _ZN10ModelAnim24CopyERKS_Pcj(void *self, const void *src, char *nf,
                                  unsigned nof)
{ ((ModelAnim2 *)self)->ModelAnim2::Copy(*(const ModelAnim2 *)src, nf, nof); }


void _ZN12CylinderClsn5ClearEv(void *self)
{ ((CylinderClsn *)self)->CylinderClsn::Clear(); }
void _ZN12CylinderClsn6UpdateEv(void *self)
{ ((CylinderClsn *)self)->CylinderClsn::Update(); }

void _ZN12WithMeshClsn13SetGroundFlagEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::SetGroundFlag(); }
void _ZN12WithMeshClsn13SetLimMovFlagEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::SetLimMovFlag(); }
void _ZN12WithMeshClsn15ClearGroundFlagEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::ClearGroundFlag(); }
void _ZN12WithMeshClsn15ClearLimMovFlagEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::ClearLimMovFlag(); }
void _ZN12WithMeshClsn18StopDetectingWaterEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::StopDetectingWater(); }
void _ZN12WithMeshClsn19ClearAllGroundFlagsEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::ClearAllGroundFlags(); }
void _ZN12WithMeshClsn19StartDetectingWaterEv(void *self)
{ ((WithMeshClsn *)self)->WithMeshClsn::StartDetectingWater(); }

void _ZN15TextureSequence6UpdateER15ModelComponents(void *self, void *mc)
{ ((TextureSequence *)self)->TextureSequence::Update(
      *(ModelComponents *)mc); }

void _ZN4BgCh19StartDetectingToxicEv(void *self)
{ ((BgCh *)self)->BgCh::StartDetectingToxic(); }
void _ZN4BgCh21StopDetectingOrdinaryEv(void *self)
{ ((BgCh *)self)->BgCh::StopDetectingOrdinary(); }

void _ZN5Model14SetPolygonModeEi(void *self, int mode)
{ ((Model *)self)->Model::SetPolygonMode(mode); }

void _ZN5Timer10ResetTimerEv(void *self)
{ ((Timer *)self)->Timer::ResetTimer(); }
void _ZN5Timer10StartTimerEv(void *self)
{ ((Timer *)self)->Timer::StartTimer(); }
long long _ZN5Timer7GetTimeEv(void *self)
{ return ((Timer *)self)->Timer::GetTime(); }
void _ZN5Timer9StopTimerEv(void *self)
{ ((Timer *)self)->Timer::StopTimer(); }

int _ZN6Player12Unk_020c9e5cEh(void *self, unsigned char h)
{ return ((Player *)self)->Player::Unk_020c9e5c(h); }
int _ZN6Player16St_Shell_CleanupEv(void *self)
{ return ((Player *)self)->Player::St_Shell_Cleanup(); }
void _ZN6Player18SetNewHatCharacterEjjb(void *self, unsigned a, unsigned b,
                                        unsigned char c)
{ ((Player *)self)->Player::SetNewHatCharacter(a, b, c != 0); }
void _ZN6Player18TurnOffToonShadingEj(void *self, unsigned j)
{ ((Player *)self)->Player::TurnOffToonShading(j); }
int _ZN6Player22IsBeingShotOutOfCannonEv(void *self)
{ return ((Player *)self)->Player::IsBeingShotOutOfCannon(); }
int _ZN6Player7IsInAirEv(void *self)
{ return ((Player *)self)->Player::IsInAir(); }
void _ZN6Player4HealEi(void *self, int amt)
{ ((Player *)self)->Player::Heal(amt); }


void _ZN9ActorBase18MarkForDestructionEv(void *self)
{ ((ActorBase *)self)->ActorBase::MarkForDestruction(); }

/* Gate 15: Actor::BeforeBehavior is a .c-style TU that calls its base by
   Itanium name, while the definition is a real __thiscall method. */
int _ZN9ActorBase14BeforeBehaviorEv(void *self)
{ return ((ActorBase *)self)->ActorBase::BeforeBehavior() ? 1 : 0; }

unsigned _ZNK7PathPtr8NumNodesEv(const void *self)
{ return ((const PathPtr *)self)->PathPtr::NumNodes(); }


}  /* extern "C" */



/* REVERSE faces: these St_ files define the ITANIUM C name; the state
   dispatcher references the MSVC method. Forward method -> C def. */
extern "C" int _ZN6Player19St_GroundPound_MainEv(void *self);
extern "C" int _ZN6Player16St_LongJump_InitEv(void *self);
int Player::St_GroundPound_Main()
{ return _ZN6Player19St_GroundPound_MainEv(this); }
int Player::St_LongJump_Init()
{ return _ZN6Player16St_LongJump_InitEv(this); }

/* C-name-at-C++-linkage state Init refs (Main TUs call their own Init
   by Itanium name) plus round-3 small faces */
void _ZN6Player14St_OnWall_InitEv(char *self)
{ ((Player *)self)->Player::St_OnWall_Init(); }
void _ZN6Player17St_PunchKick_InitEv(void *self)
{ ((Player *)self)->Player::St_PunchKick_Init(); }
extern "C" int _Z14ApproachLinearRsss(short *x, short target, short step)
{ return ApproachLinear2(*x, target, step); }
extern "C" int _ZN6Player15IsCollectingCapEv(char *self)
{ return ((Player *)self)->Player::IsCollectingCap(); }

/* gate-10 tier-2 wave: these St_ files define the ITANIUM C name, the
   state dispatcher calls the MSVC method. Forward method -> C def only;
   never the other way round for the same function. */
extern "C" int _ZN6Player15St_Balloon_MainEv(void *self);
extern "C" int _ZN6Player16St_BurnFire_InitEv(void *self);
extern "C" int _ZN6Player16St_BurnFire_MainEv(void *self);
extern "C" int _ZN6Player18St_CameraZoom_MainEv(void *self);
extern "C" int _ZN6Player18St_DizzyStars_MainEv(void *self);
extern "C" int _ZN6Player19St_Electrocute_MainEv(void *self);
extern "C" int _ZN6Player18St_Grabbed_CleanupEv(void *self);
extern "C" int _ZN6Player12St_Hurt_MainEv(void *self);
extern "C" int _ZN6Player23St_MetalWaterWater_MainEv(void *self);
extern "C" int _ZN6Player15St_Respawn_InitEv(void *self);
extern "C" int _ZN6Player12St_Spin_MainEv(void *self);
extern "C" int _ZN6Player17St_SweepKick_InitEv(void *self);
extern "C" int _ZN6Player12St_Swim_MainEv(void *self);
extern "C" int _ZN6Player15St_Talk_CleanupEv(void *self);
extern "C" int _ZN6Player13St_Throw_InitEv(void *self);
extern "C" int _ZN6Player14St_Thrown_InitEv(void *self);
extern "C" int _ZN6Player19St_TornadoSpin_MainEv(void *self);
int Player::St_Balloon_Main()
{ return _ZN6Player15St_Balloon_MainEv(this); }
int Player::St_BurnFire_Init()
{ return _ZN6Player16St_BurnFire_InitEv(this); }
int Player::St_BurnFire_Main()
{ return _ZN6Player16St_BurnFire_MainEv(this); }
int Player::St_CameraZoom_Main()
{ return _ZN6Player18St_CameraZoom_MainEv(this); }
int Player::St_DizzyStars_Main()
{ return _ZN6Player18St_DizzyStars_MainEv(this); }
int Player::St_Electrocute_Main()
{ return _ZN6Player19St_Electrocute_MainEv(this); }
int Player::St_Grabbed_Cleanup()
{ return _ZN6Player18St_Grabbed_CleanupEv(this); }
int Player::St_Hurt_Main()
{ return _ZN6Player12St_Hurt_MainEv(this); }
int Player::St_MetalWaterWater_Main()
{ return _ZN6Player23St_MetalWaterWater_MainEv(this); }
int Player::St_Respawn_Init()
{ return _ZN6Player15St_Respawn_InitEv(this); }
int Player::St_Spin_Main()
{ return _ZN6Player12St_Spin_MainEv(this); }
int Player::St_SweepKick_Init()
{ return _ZN6Player17St_SweepKick_InitEv(this); }
int Player::St_Swim_Main()
{ return _ZN6Player12St_Swim_MainEv(this); }
int Player::St_Talk_Cleanup()
{ return _ZN6Player15St_Talk_CleanupEv(this); }
int Player::St_Throw_Init()
{ return _ZN6Player13St_Throw_InitEv(this); }
int Player::St_Thrown_Init()
{ return _ZN6Player14St_Thrown_InitEv(this); }
int Player::St_TornadoSpin_Main()
{ return _ZN6Player19St_TornadoSpin_MainEv(this); }
extern "C" int _ZN6Player18St_YoshiPower_MainEv(void *self);
int Player::St_YoshiPower_Main()
{ return _ZN6Player18St_YoshiPower_MainEv(this); }
/* St_Grabbed_Main calls DropActor by its Itanium name; the definition is a
   real method. Forward C name -> method (no face the other way). */
extern "C" int _ZN6Player9DropActorEv(void *self)
{ return ((Player *)self)->Player::DropActor(); }

/* gate 14: the init chain the actor spawn spine dispatches. Both are real
   __thiscall methods, so a linker alias onto the Itanium name their .c
   callers use would enter the body with `this` in whatever ecx held. */
extern "C" int _ZN5Actor18GetBitInDeathTableEv(void *self)
{ return ((Actor *)self)->Actor::GetBitInDeathTable(); }
extern "C" void _ZN5Actor18AfterInitResourcesEj(void *self, unsigned a)
{ ((Actor *)self)->Actor::AfterInitResources(a); }

/* gate 16: Actor::BeforeRender is the same shape -- a .c TU calling its base
   by Itanium name over a real __thiscall definition. Slot 10 of every actor
   class the registry carries goes through it. */
extern "C" int _ZN9ActorBase12BeforeRenderEv(void *self)
{ return ((ActorBase *)self)->ActorBase::BeforeRender(); }

/* gate 16: ModelBase::ApplyOpacity is a real method whose only caller,
   Tree::Render, spells it as an Itanium C name (and passes a third argument
   the ROM's r2 carried into a two-parameter body; cdecl lets the caller keep
   cleaning it). */
extern "C" void _ZN9ModelBase12ApplyOpacityEj(void *self, unsigned a)
{ ((ModelBase *)self)->ModelBase::ApplyOpacity(a); }

/* Model::UpdateFileOffsets is a STATIC member (include/Model.h), which is why
   func_02016ff4 calls it with the file alone and no `this` -- the Itanium name
   is the same either way, so only the header says which. Face, not alias:
   MSVC decorates a static member differently again. */
extern "C" void _ZN5Model17UpdateFileOffsetsER8BMD_File(BMD_File *f)
{ Model::UpdateFileOffsets(*f); }

/* gate 16, the shrink-to-fit tail of Model::LoadAndSetFile. Both are real
   Heap methods reached by Itanium name from func_02017060; _Sizeof is the ARM
   two-instruction veneer onto Sizeof, so the face calls the target directly
   rather than forwarding through a body that would drop both arguments. */
/* gate 16: the actor teardown is a HOST COPY now (see
   port/unmatched/ActorBase_AfterCleanupResources.cpp -- the matched TU defines
   three engine globals rather than declaring them), so the slot-5 thunks that
   call the method need the method to exist. */
extern "C" void _ZN9ActorBase21AfterCleanupResourcesEj(void *self, unsigned a);
void ActorBase::AfterCleanupResources(u32 a)
{ _ZN9ActorBase21AfterCleanupResourcesEj(this, a); }

extern "C" int _ZN4Heap7_SizeofEPv(void *self, void *p)
{ return ((Heap *)self)->Heap::Sizeof(p); }
extern "C" void _ZN4Heap10ReallocateEPvj(void *self, void *p, unsigned n)
{ ((Heap *)self)->Heap::Reallocate(p, n); }

/* gate 16, THE OTHER DIRECTION: CylinderClsnWithPos::Init is defined at C
   linkage in its own TU while Tree::InitResources declares it as a method on
   a local class shape and calls it __thiscall. An /alternatename would enter
   the cdecl body with `this` still in ecx, so this is a face. */
extern "C" void _ZN19CylinderClsnWithPos4InitERK7Vector35Fix12IiES4_jj(
    void *self, const void *pos, int radius, int height, unsigned flags,
    unsigned vulnFlags);
void CylinderClsnWithPos::Init(const Vector3 &pos, Fix12i radius,
                               Fix12i height, u32 flags, u32 vulnFlags)
{
    _ZN19CylinderClsnWithPos4InitERK7Vector35Fix12IiES4_jj(
        this, &pos, radius, height, flags, vulnFlags);
}

