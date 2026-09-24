// Itanium-ABI faces: C-linkage definitions for the Itanium-mangled names
// the tree still references, forwarding to the renamed methods.
// Upstream links a ROM (mwcc keeps Itanium names); the host links MSVC
// (?Name@Class@@...), so every Itanium C reference needs one of these.
// Signatures come from the class headers; bodies are one-line forwards.
#include <stddef.h>
#include <cstdio>
#include <new>

#include "dBgCh_Actr.h"
#include "dBgCh_Gnd.h"
#include "dBgCh_Lin.h"
#include "dBgCh_SphCrr.h"
#include "dBgCh.h"
#include "dBgW.h"
#include "dBgW_Kc.h"
#include "dBgW_KcMbg.h"

extern "C" {
/* ---- dBgCh_Actr (was WithMeshClsn) ---- */
void _ZN10dBgCh_Actr13SetGroundFlagEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::SetGroundFlag(); }
void _ZN10dBgCh_Actr13SetLimMovFlagEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::SetLimMovFlag(); }
void _ZN10dBgCh_Actr15ClearGroundFlagEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::ClearGroundFlag(); }
void _ZN10dBgCh_Actr15ClearLimMovFlagEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::ClearLimMovFlag(); }
void _ZN10dBgCh_Actr18StopDetectingWaterEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::StopDetectingWater(); }
void _ZN10dBgCh_Actr19ClearAllGroundFlagsEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::ClearAllGroundFlags(); }
void _ZN10dBgCh_Actr19StartDetectingWaterEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::StartDetectingWater(); }
void _ZN10dBgCh_Actr20UpdateDiscreteNoLavaEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::UpdateDiscreteNoLava(); }
void _ZN10dBgCh_Actr20UpdateExtraContinousEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::UpdateExtraContinous(); }
void _ZN10dBgCh_Actr22UpdateContinuousNoLavaEv(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::UpdateContinuousNoLava(); }
void _ZN10dBgCh_Actr22UpdateDiscreteNoLava_2Ev(void *s)
{ ((dBgCh_Actr *)s)->dBgCh_Actr::UpdateDiscreteNoLava_2(); }
void _ZN10dBgCh_ActrC1Ev(void *s)
{ ::new (s) dBgCh_Actr(); }
void _ZN10dBgCh_ActrD1Ev(void *s)
{ ((dBgCh_Actr *)s)->~dBgCh_Actr(); }
int _ZNK10dBgCh_Actr10IsOnGroundEv(const void *s)
{ return ((const dBgCh_Actr *)s)->dBgCh_Actr::IsOnGround(); }
int _ZNK10dBgCh_Actr12TouchesWaterEv(const void *s)
{
    extern int SurfaceInfo_TestFlag0x20(const void *p);
    return SurfaceInfo_TestFlag0x20((const char *)s + 0x34);
}
int _ZNK10dBgCh_Actr13GetLimMovFlagEv(const void *s)
{ return ((const dBgCh_Actr *)s)->dBgCh_Actr::GetLimMovFlag(); }
int _ZNK10dBgCh_Actr13JustHitGroundEv(const void *s)
{ return ((const dBgCh_Actr *)s)->dBgCh_Actr::JustHitGround(); }
int _ZNK10dBgCh_Actr8IsOnWallEv(const void *s)
{ return ((const dBgCh_Actr *)s)->dBgCh_Actr::IsOnWall(); }
}  // extern "C"

/* ---- dCcAcPos_c / dCcPos_c / dCc_c ---- */
#include "dCcAcPos_c.h"
#include "dCcPos_c.h"
#include "dCc_c.h"
extern "C" {
void _ZN10dCcAcPos_cC1Ev(void *s)
{ ::new (s) dCcAcPos_c(); }
void _ZN10dCcAcPos_c4InitEP8dActor_cRK7Vector35Fix12IiES6_jj(
    void *s, void *actor, const void *off, int radius, int height,
    unsigned flags, unsigned vuln)
{ ((dCcAcPos_c *)s)->dCcAcPos_c::Init((dActor_c *)actor,
                                      *(const Vector3 *)off,
                                      Fix12<int>{radius}, Fix12<int>{height},
                                      flags, vuln); }
void _ZN10dCcAcPos_c21SetPosRelativeToActorERK7Vector3(void *s, const void *v)
{ ((dCcAcPos_c *)s)->dCcAcPos_c::SetPosRelativeToActor(*(const Vector3 *)v); }
void _ZN8dCcPos_cC1Ev(void *s)
{ ::new (s) dCcPos_c(); }
void _ZN8dCcPos_c4InitERK7Vector35Fix12IiES4_jj(void *s, const void *pos,
                                                int radius, int height,
                                                unsigned flags,
                                                unsigned vuln)
{ ((dCcPos_c *)s)->dCcPos_c::Init(*(const Vector3 *)pos,
                                  Fix12<int>{radius}, Fix12<int>{height},
                                  flags, vuln); }
void *_ZN8dCcPos_c6GetPosEv(void *s)
{ return &((dCcPos_c *)s)->dCcPos_c::GetPos(); }
unsigned _ZN8dCcPos_c10GetOwnerIDEv(void *s)
{ return ((dCcPos_c *)s)->dCcPos_c::GetOwnerID(); }
void _ZN8dCcPos_cD1Ev(void *s)
{ ((dCcPos_c *)s)->~dCcPos_c(); }
void _ZN8dCcPos_cD0Ev(void *s)
{ ((dCcPos_c *)s)->~dCcPos_c(); }
}  // extern "C"

/* ---- dBgW_KcMbg / dBgW_Kc / dBgW ---- */
#include "dBgW_KcMbg.h"
#include "fBase_c.h"
extern "C" {
void _ZN10dBgW_KcMbgC1Ev(void *s)
{ ::new (s) dBgW_KcMbg(); }
void _ZN10dBgW_KcMbgD1Ev(void *s)
{ ((dBgW_KcMbg *)s)->~dBgW_KcMbg(); }
int _ZN10dBgW_KcMbg10DetectClsnER12dBgCh_SphCrr(void *s, void *sph)
{ return ((dBgW_KcMbg *)s)->dBgW_KcMbg::DetectClsn(*(dBgCh_SphCrr *)sph); }
int _ZN10dBgW_KcMbg10DetectClsnER9dBgCh_Gnd(void *s, void *g)
{ return ((dBgW_KcMbg *)s)->dBgW_KcMbg::DetectClsn(*(dBgCh_Gnd *)g); }
int _ZN10dBgW_KcMbg10DetectClsnER9dBgCh_Lin(void *s, void *r)
{ return ((dBgW_KcMbg *)s)->dBgW_KcMbg::DetectClsn(*(dBgCh_Lin *)r); }
void _ZN10dBgW_KcMbg9TransformERK9Matrix4x3s(void *s, const void *m, short a)
{ ((dBgW_KcMbg *)s)->dBgW_KcMbg::Transform(*(const Matrix4x3 *)m, a); }
void _ZN7dBgW_Kc17UpdateFileOffsetsER8KCL_File(void *f)
{ dBgW_Kc::UpdateFileOffsets(*(KCL_File *)f); }
int _ZNK7dBgW_Kc16GetOctreeOriginYEv(const void *s)
{ return ((const dBgW_Kc *)s)->dBgW_Kc::GetOctreeOriginY(); }
int _ZNK7dBgW_Kc13GetUnkOctreeYEv(const void *s)
{ return ((const dBgW_Kc *)s)->dBgW_Kc::GetUnkOctreeY(); }
void _ZN4dBgW7DisableEv(void *s)
{ ((dBgW *)s)->dBgW::Disable(); }
int _ZN4dBgW9IsEnabledEv(void *s)
{ return ((dBgW *)s)->dBgW::IsEnabled(); }
void _ZN7fBase_c18MarkForDestructionEv(void *s)
{ ((fBase_c *)s)->fBase_c::MarkForDestruction(); }
}  // extern "C"

/* ---- Sound: music control (no host path yet -- the sdat host plays
   one-shots and BGM, but nothing stops or switches music; these keep the
   link while that work is pending) ---- */
extern "C" {
void _ZN5Sound8EndMusicEjj(unsigned a, unsigned b)
{ ((void)a); ((void)b); }
void _ZN5Sound8SetMusicEjj(unsigned a, unsigned b)
{ ((void)a); ((void)b); }
void _ZN5Sound22LoadAndSetMusic_Layer1Ei(int a)
{ ((void)a); }
}  // extern "C"

/* ---- Event (namespace free functions) ---- */
namespace Event {
unsigned GetBit(unsigned bit);
void SetBit(unsigned bit);
void ClearBit(unsigned bit);
}
extern "C" {
unsigned _ZN5Event6GetBitEj(unsigned b)
{ return Event::GetBit(b); }
void _ZN5Event6SetBitEj(unsigned b)
{ Event::SetBit(b); }
void _ZN5Event8ClearBitEj(unsigned b)
{ Event::ClearBit(b); }
}  // extern "C"

/* ---- dCc_c (cylinder) ---- */
extern "C" {
void _ZN5dCc_c5ClearEv(void *s)
{ ((dCc_c *)s)->dCc_c::Clear(); }
void _ZN5dCc_c6UpdateEv(void *s)
{ ((dCc_c *)s)->dCc_c::Update(); }
void _ZN5dCc_c4InitE5Fix12IiES1_jj(void *s, int r, int h, unsigned f,
                                   unsigned v)
{ ((dCc_c *)s)->dCc_c::Init((Fix12i)r, (Fix12i)h, f, v); }
}  // extern "C"

/* ---- ModelAnim2 / ShadowModel / Vector3 / Clipper ---- */
#include "ModelAnim2.h"
#include "ShadowModel.h"
#include "Clipper.h"
extern "C" {
void _ZN10ModelAnim2C1Ev(void *s)
{ ::new (s) ModelAnim2(); }
void _ZN11ShadowModelC1Ev(void *s)
{ ::new (s) ShadowModel(); }
void _ZN11ShadowModelD1Ev(void *s)
{ ((ShadowModel *)s)->~ShadowModel(); }
int _ZN11ShadowModel12DoSetFileEPci(void *s, void *f, int a, int b)
{ return ((ShadowModel *)s)->ShadowModel::DoSetFile((char *)f, a, b); }
void _ZN7Vector3D1Ev(void *s)
{ ((Vector3 *)s)->~Vector3(); }
void _ZN7Clipper13Func_020156DCEitii(void *s, int a, unsigned short b, int c,
                                      int d)
{ ((Clipper *)s)->Clipper::Func_020156DC(a, b, c, d); }
}  // extern "C"

/* ---- dActor_c ---- */
#include "dActor_c.h"
#include "ShadowModel.h"
extern "C" {
void *_ZN8dActor_c10FindWithIDEj(unsigned id)
{ return dActor_c::FindWithID(id); }
/* init/behavior spine faces: the processing-list callbacks (ac_bbeh in
   actor_classes.cpp, ps_binit/ps_ainit in level_boot.cpp) and the spawn
   init Process reach BeforeInitResources/AfterInitResources/BeforeBehavior
   by Itanium name; the real MSVC methods live in the gated TUs. */
int _ZN8dActor_c14BeforeBehaviorEv(void *s)
{ return ((dActor_c *)s)->dActor_c::BeforeBehavior(); }
int _ZN8dActor_c19BeforeInitResourcesEv(void *s)
{ return ((dActor_c *)s)->dActor_c::BeforeInitResources() ? 1 : 0; }
void _ZN8dActor_c18AfterInitResourcesEj(void *s, unsigned v)
{ ((dActor_c *)s)->dActor_c::AfterInitResources(v); }
void _ZN8dActor_c13SpawnSoundObjEj(void *s, unsigned p)
{ ((dActor_c *)s)->dActor_c::SpawnSoundObj(p); }
void _ZN8dActor_c19DropShadowRadHeightER11ShadowModelR9Matrix4x35Fix12IiES5_j(
    void *s, void *sm, void *m, int rad, int h, unsigned f)
{ ((dActor_c *)s)->dActor_c::DropShadowRadHeight(*(ShadowModel *)sm,
                                                 *(Matrix4x3 *)m,
                                                 Fix12<int>{rad},
                                                 Fix12<int>{h}, f); }
void _ZN8dActor_c9UpdatePosEP5dCc_c(void *s, void *clsn)
{ ((dActor_c *)s)->dActor_c::UpdatePos((dCc_c *)clsn); }
void _ZN8dActor_cD2Ev(void *s)
{ ((dActor_c *)s)->~dActor_c(); }
void _ZN8dActor_c22UpdatePosWithOnlySpeedEP5dCc_c(void *s, void *clsn)
{ ((dActor_c *)s)->dActor_c::UpdatePosWithOnlySpeed((dCc_c *)clsn); }
void _ZN8dActor_c28UpdatePosWithHorzSpeedAndAngEv(void *s)
{ ((dActor_c *)s)->dActor_c::UpdatePosWithHorzSpeedAndAng(); }
void *_ZN8dActor_c11UpdateCarryER6PlayerRK7Vector3(void *s, void *player,
                                                   const void *vec)
{ return ((dActor_c *)s)->dActor_c::UpdateCarry(*(Player *)player,
                                               *(const Vector3 *)vec); }
int _ZN8dActor_c13DistToCPlayerEv(void *s)
{ return ((dActor_c *)s)->dActor_c::DistToCPlayer(); }
short _ZN8dActor_c18HorzAngleToCPlayerEv(void *s)
{ return ((dActor_c *)s)->dActor_c::HorzAngleToCPlayer(); }
int _ZN8dActor_c15IsPlayerInRangeEi(void *s, int d)
{ return ((dActor_c *)s)->dActor_c::IsPlayerInRange(d) ? 1 : 0; }
void *_ZN8dActor_c15FindWithActorIDEjPS_(unsigned id, void *after)
{ return dActor_c::FindWithActorID(id, (dActor_c *)after); }
void *_ZN8dActor_c4NextEPKS_(const void *after)
{ return dActor_c::Next((const dActor_c *)after); }
void *_ZN8dActor_c5SpawnEjjRK7Vector3PK10Vector3_16as(
    unsigned id, unsigned param, const void *pos, const void *rot, short area,
    short death)
{ void *r = dActor_c::Spawn(id, param, *(const Vector3 *)pos,
                            (const Vector3_16 *)rot, area, death);
  std::fprintf(stderr, "[spawn] new id=0x%x param=0x%x area=%d death=%d -> %p\n",
               id, param, area, death, r);
  std::fflush(stderr);
  return r; }
/* pre-rename spelling used by every port/unmatched object loader
   (Door/Exit/Simple/Entrance/Standard/Teleport): same contract, the two
   trailing words are (area, death) = -1/-1 for "none", truncated to the
   short/short the renamed overload takes. */
void *_ZN5Actor5SpawnEjjRK7Vector3PK10Vector3_16ii(
    unsigned id, unsigned param, const void *pos, const void *rot, int area,
    int death)
{ void *r = dActor_c::Spawn(id, param, *(const Vector3 *)pos,
                            (const Vector3_16 *)rot, (short)area, (short)death);
  std::fprintf(stderr, "[spawn] old id=0x%x param=0x%x area=%d death=%d -> %p\n",
               id, param, area, death, r);
  std::fflush(stderr);
  return r; }
void *_ZN8dActor_c7FindEggER5dCc_c(void *s, void *clsn)
{ return ((dActor_c *)s)->dActor_c::FindEgg(*(dCc_c *)clsn); }
int _ZN8dActor_c16JumpedOnByPlayerER5dCc_cR6Player(void *s, void *clsn,
                                                  void *player)
{ return ((dActor_c *)s)->dActor_c::JumpedOnByPlayer(*(dCc_c *)clsn,
                                                    *(Player *)player); }
void _ZN8dActor_c11LandingDustEb(void *s, int b)
{ ((dActor_c *)s)->dActor_c::LandingDust(b ? true : false); }
void _ZN8dActor_c11SpawnNumberERK7Vector3jbtPS_(void *s, const void *pos,
                                                unsigned v, int pack, void *o)
{ ((dActor_c *)s); ((void)pos); ((void)v); ((void)pack); ((void)o); }
void _ZN8dActor_c11UntrackStarERa(void *s, void *id)
{ ((dActor_c *)s)->dActor_c::UntrackStar(*(signed char *)id); }
void _ZN8dActor_c10SpawnCoinsERK7Vector3j5Fix12IiEs(void *s, const void *pos,
                                                    unsigned n, int spread,
                                                    short ang)
{ ((dActor_c *)s)->dActor_c::SpawnCoins(*(const Vector3 *)pos, n,
                                        Fix12<int>{spread}, ang); }
void _ZN8dActor_c19DisappearPoofDustAtERK7Vector3(void *s, const void *pos)
{ ((dActor_c *)s)->dActor_c::DisappearPoofDustAt(*(const Vector3 *)pos); }
int _ZN8dActor_c22IsTooFarAwayFromPlayerE5Fix12IiE(void *s, int d)
{ ((void)s); ((void)d); return 0; }
void _ZN8dActor_c24KillAndTrackInDeathTableEv(void *s)
{ ((dActor_c *)s)->dActor_c::KillAndTrackInDeathTable(); }
int _ZN8dActor_c14KillByMegaCharER6Player(void *s, void *player)
{ ((dActor_c *)s)->dActor_c::MarkForDestruction(); ((void)player); return 0; }
void _ZN8dActor_c10EarthquakeERK7Vector35Fix12IiE(void *s, const void *pos,
                                                  int mag)
{ ((dActor_c *)s); ((void)pos); ((void)mag); }
}  // extern "C"

/* ---- Player ---- */
#include "Player.h"
extern "C" {
int _ZN6Player12St_Hurt_MainEv(void *s)
{ return ((Player *)s)->Player::St_Hurt_Main(); }
int _ZN6Player12St_Jump_InitEv(void *s)
{ return ((Player *)s)->Player::St_Jump_Init(); }
int _ZN6Player12St_Spin_MainEv(void *s)
{ return ((Player *)s)->Player::St_Spin_Main(); }
int _ZN6Player12St_Swim_MainEv(void *s)
{ return ((Player *)s)->Player::St_Swim_Main(); }
int _ZN6Player13St_Throw_InitEv(void *s)
{ return ((Player *)s)->Player::St_Throw_Init(); }
int _ZN6Player14St_OnWall_InitEv(void *s)
{ return ((Player *)s)->Player::St_OnWall_Init(); }
int _ZN6Player14St_Thrown_InitEv(void *s)
{ return ((Player *)s)->Player::St_Thrown_Init(); }
int _ZN6Player15St_Balloon_MainEv(void *s)
{ return ((Player *)s)->Player::St_Balloon_Main(); }
int _ZN6Player15St_Respawn_InitEv(void *s)
{ return ((Player *)s)->Player::St_Respawn_Init(); }
int _ZN6Player15St_Talk_CleanupEv(void *s)
{ return ((Player *)s)->Player::St_Talk_Cleanup(); }
void _ZN6Player16IncMegaKillCountEv(void *s)
{ ((Player *)s)->Player::IncMegaKillCount(); }
int _ZN6Player16IsInsideOfCannonEv(void *s)
{ return ((Player *)s)->Player::IsInsideOfCannon(); }
int _ZN6Player16St_BurnFire_InitEv(void *s)
{ return ((Player *)s)->Player::St_BurnFire_Init(); }
int _ZN6Player16St_BurnFire_MainEv(void *s)
{ return ((Player *)s)->Player::St_BurnFire_Main(); }
int _ZN6Player16St_BurnLava_MainEv(void *s)
{ return ((Player *)s)->Player::St_BurnLava_Main(); }
int _ZN6Player16St_LongJump_InitEv(void *s)
{ return ((Player *)s)->Player::St_LongJump_Init(); }
void _ZN6Player17PlayMammaMiaSoundEv(void *s)
{ ((Player *)s)->Player::PlayMammaMiaSound(); }
int _ZN6Player17St_PunchKick_InitEv(void *s)
{ return ((Player *)s)->Player::St_PunchKick_Init(); }
int _ZN6Player17St_SweepKick_InitEv(void *s)
{ return ((Player *)s)->Player::St_SweepKick_Init(); }
int _ZN6Player18St_CameraZoom_MainEv(void *s)
{ return ((Player *)s)->Player::St_CameraZoom_Main(); }
int _ZN6Player18St_DizzyStars_MainEv(void *s)
{ return ((Player *)s)->Player::St_DizzyStars_Main(); }
int _ZN6Player18St_Grabbed_CleanupEv(void *s)
{ return ((Player *)s)->Player::St_Grabbed_Cleanup(); }
int _ZN6Player18St_YoshiPower_MainEv(void *s)
{ return ((Player *)s)->Player::St_YoshiPower_Main(); }
int _ZN6Player19St_Electrocute_MainEv(void *s)
{ return ((Player *)s)->Player::St_Electrocute_Main(); }
int _ZN6Player19St_GroundPound_MainEv(void *s)
{ return ((Player *)s)->Player::St_GroundPound_Main(); }
int _ZN6Player19St_TornadoSpin_MainEv(void *s)
{ return ((Player *)s)->Player::St_TornadoSpin_Main(); }
int _ZN6Player23St_MetalWaterWater_MainEv(void *s)
{ return ((Player *)s)->Player::St_MetalWaterWater_Main(); }
int _ZN6Player7IsStateERNS_5StateE(void *s, void *st)
{ return ((Player *)s)->Player::IsState(*(Player::State *)st) ? 1 : 0; }
int _ZN6Player7TryGrabER8dActor_c(void *s, void *a)
{ return ((Player *)s)->Player::TryGrab(*(dActor_c *)a); }
int _ZN6Player9StartTalkER7fBase_cb(void *s, void *a, int b)
{ return ((Player *)s)->Player::StartTalk(*(fBase_c *)a, b ? true : false); }
int _ZN6Player9StartTalkER9ActorBaseb(void *s, void *a, int b)
{ return ((Player *)s)->Player::StartTalk(*(fBase_c *)a, b ? true : false); }
unsigned _ZNK6Player14GetBodyModelIDEjb(void *s, unsigned a, int b)
{ return ((const Player *)s)->Player::GetBodyModelID(a, b ? true : false); }
int _ZN6Player11ShowMessageER7fBase_cjPK7Vector3hh(void *s, void *a,
                                                   unsigned b, const void *v,
                                                   int d, int e)
{ return ((Player *)s)->Player::ShowMessage(*(fBase_c *)a, b,
                                            (const Vector3 *)v, (u8)d, (u8)e); }
int _ZN6Player12ShowMessage2ER7fBase_cjPK7Vector3hh(void *s, void *a,
                                                    unsigned b, const void *v,
                                                    int d, int e)
{ return ((Player *)s)->Player::ShowMessage2(*(fBase_c *)a, b,
                                             (const Vector3 *)v, (u8)d,
                                             (u8)e); }
/* Player::C3 as FACTORY (matches the registry: allocates its own object,
   like the ROM's 0x020e6c0c). Placement-constructs through the real C1Ev
   TU (gated), which runs the member-wise init the spawn then continues. */
extern "C" void *_ZN6PlayerC1Ev(void *c);
extern "C" void *func_0203cc0c(unsigned size);
void *_ZN6PlayerC3Ev(void)
{
    void *s = func_0203cc0c((unsigned)sizeof(Player));
    if (!s) return 0;
    return _ZN6PlayerC1Ev(s);
}
}  // extern "C"

/* ---- Camera ---- */
#include "Camera.h"
extern "C" {
/* (LookAtExit's C name is provided by its own TU's HOST-glue wrapper.) */
int _ZN6Camera11ChangeStateEPNS_5StateE(void *s, void *st)
{ return ((Camera *)s)->Camera::ChangeState((Camera::State *)st); }
int _ZN6Camera16CleanupResourcesEv(void *s)
{ return ((Camera *)s)->Camera::CleanupResources(); }
void _ZN6Camera6SetPosERK7Vector3(void *s, const void *v)
{ ((Camera *)s)->Camera::SetPos(*(const Vector3 *)v); }
void _ZN6Camera9SetLookAtERK7Vector3(void *s, const void *v)
{ ((Camera *)s)->Camera::SetLookAt(*(const Vector3 *)v); }
/* No enrolled TU implements Camera::InitResources (the src shadow TU uses a
   local struct and a different mangling). Honest no-op success: the object
   arrives zeroed with a vptr, and Behavior/Render (real TUs) cope. */
int _ZN6Camera13InitResourcesEv(void *s)
{ ((void)s); return 1; }
/* Camera::C1 as FACTORY (matches the registry's port_factory_camera,
   which passes 0). Allocates, then placement-constructs through the
   minimal host Camera::Camera (port/hal/base_methods.cpp): the dActor
   chain is real, the derived part starts zeroed for InitResources. */
void *_ZN6CameraC1Ev(void *s)
{
    (void)s;
    void *p = func_0203cc0c((unsigned)sizeof(Camera));
    if (!p) return 0;
    ::new (p) Camera();
    return p;
}
void _ZN6CameraD1Ev(void *s)
{ ((Camera *)s)->~Camera(); }
void _ZN6CameraD0Ev(void *s)
{ ((Camera *)s)->~Camera(); }
int _ZNK6Camera12IsUnderwaterEv(const void *s)
{ return ((const Camera *)s)->Camera::IsUnderwater(); }
}  // extern "C"

/* ---- Stage / dScene_c ---- */
#include "Stage.h"
#include "dScene_c.h"
extern "C" {
void _ZN5Stage10CheckInputEv()
{ Stage::CheckInput(); }
void _ZN5Stage9LoadModelEv(void *s)
{ ((Stage *)s)->Stage::LoadModel(); }
/* Stage::Stage as FACTORY (matches stage_bridges.cpp's caller: no args,
   returns the object). The matched src TU was never enrolled, and no
   MSVC Stage::Stage exists, so placement-new is impossible. Minimal
   viable Stage: 0x9c8 zeroed bytes via the real allocator, vptr on the
   trap table (first dispatch names its slot instead of jumping wild),
   dBgW_Kc constructed at +0x91c where the ROM keeps it. ActorBase list
   nodes stay empty (Stage rides no processing list) and the tree head
   seats later; both print warnings, not faults. */
extern "C" void *func_0203cc0c(unsigned size);
extern "C" void _ZN7dBgW_KcC1Ev(void *self);
extern "C" void *_ZTV5Stage[];
#include <cstring>
void *_ZN5StageC1Ev(void)
{
    char *s = (char *)func_0203cc0c(0x9c8);
    if (!s) return 0;
    std::memset(s, 0, 0x9c8);
    *(void **)s = _ZTV5Stage;
    _ZN7dBgW_KcC1Ev(s + 0x91c);
    return s;
}
void _ZN8dScene_c14StartSceneFadeEjjt(unsigned a, unsigned b, int c)
{ dScene_c::StartSceneFade(a, b, (unsigned short)c); }
void _ZN8dScene_c20SetAndStopColorFaderEv()
{ dScene_c::SetAndStopColorFader(); }
void _ZN8dScene_c22ResetHardwareRegistersEv()
{ dScene_c::ResetHardwareRegisters(); }
}  // extern "C"

/* ---- SaveData / Message / PathPtr / Animation ---- */
#include "SaveData.h"
#include "Message.h"
#include "PathPtr.h"
#include "Animation.h"
extern "C" {
int _ZN8SaveData16CanPlayerHaveCapEv()
{ return SaveData::CanPlayerHaveCap(); }
int _ZN8SaveData16HasPlayerLostCapEv()
{ return SaveData::HasPlayerLostCap(); }
void _ZN8SaveData17SetCharacterIntroEi(int c)
{ SaveData::SetCharacterIntro(c); }
int _ZN8SaveData19IsCharacterUnlockedEj(unsigned c)
{ return SaveData::IsCharacterUnlocked(c); }
int _ZN8SaveData22NumGlowingRabbitsFoundEv()
{ return SaveData::NumGlowingRabbitsFound(); }
unsigned char _ZN8SaveData26CountStarsCollectedInLevelEj(unsigned c)
{ return SaveData::CountStarsCollectedInLevel(c); }
void _ZN7Message11PrepareTalkEv()
{ Message::PrepareTalk(); }
void _ZN7Message13DisplaySavingEt(int m)
{ Message::DisplaySaving((unsigned short)m); }
void _ZN7Message7EndTalkEv()
{ Message::EndTalk(); }
void _ZN7PathPtrC1Ev(void *s)
{ ::new (s) PathPtr(); }
/* NOTE: `this` is FIRST. Every caller (Player, daMip_c, daMky_c,
   daBgSnmBdy_c, level_boot's probe) passes (path, out, idx); an earlier
   revision had (res, s) swapped, which built a fine link and then read
   the output vector as the object. */
void _ZNK7PathPtr7GetNodeER7Vector3j(const void *s, void *res, unsigned idx)
{ ((const PathPtr *)s)->PathPtr::GetNode(*(Vector3 *)res, idx); }
int _ZN9Animation8GetFlagsEv(void *s)
{ return ((Animation *)s)->Animation::GetFlags(); }
}  // extern "C"

/* (dCc_c faces live above, by the dCcAcPos_c group.) */

/* ---- dBgW_KcMbg triple faces (mmc shims + new TUs call by C name) ---- */

/* ---- dBgCh_Gnd / dBgCh_Lin ---- */
#include "dBgCh_Gnd.h"
#include "dBgCh_Lin.h"
extern "C" {
void _ZN9dBgCh_GndC1Ev(void *s)
{ ::new (s) dBgCh_Gnd(); }
void _ZN9dBgCh_GndD1Ev(void *s)
{ ((dBgCh_Gnd *)s)->~dBgCh_Gnd(); }
void _ZN9dBgCh_Gnd12SetObjAndPosERK7Vector3P8dActor_c(void *s, const void *v,
                                                      void *a)
{ ((dBgCh_Gnd *)s)->dBgCh_Gnd::SetObjAndPos(*(const Vector3 *)v,
                                            (dActor_c *)a); }
int _ZN9dBgCh_Gnd10DetectClsnEv(void *s)
{ return ((dBgCh_Gnd *)s)->dBgCh_Gnd::DetectClsn(); }
void _ZN9dBgCh_LinC1Ev(void *s)
{ ::new (s) dBgCh_Lin(); }
void _ZN9dBgCh_LinD1Ev(void *s)
{ ((dBgCh_Lin *)s)->~dBgCh_Lin(); }
void _ZN9dBgCh_Lin13SetObjAndLineERK7Vector3S2_P8dActor_c(void *s,
                                                          const void *a,
                                                          const void *b,
                                                          void *actor)
{ ((dBgCh_Lin *)s)->dBgCh_Lin::SetObjAndLine(*(const Vector3 *)a,
                                             *(const Vector3 *)b,
                                             (dActor_c *)actor); }
void _ZN9dBgCh_Lin10GetClsnPosEv(void *res, void *s)
{ *(Vector3 *)res = ((dBgCh_Lin *)s)->dBgCh_Lin::GetClsnPos(); }
int _ZN9dBgCh_Lin10DetectClsnEv(void *s)
{ return ((dBgCh_Lin *)s)->dBgCh_Lin::DetectClsn(); }
}  // extern "C"

/* ---- dBgCh / dBgPi / SphCrr ---- */
#include "dBgPi.h"
#include "dBgCh_SphCrr.h"
extern "C" {
void _ZN5dBgCh19StartDetectingToxicEv(void *s)
{ ((dBgCh *)s)->dBgCh::StartDetectingToxic(); }
void _ZN5dBgCh19StartDetectingWaterEv(void *s)
{ ((dBgCh *)s)->dBgCh::StartDetectingWater(); }
void _ZN5dBgCh21StopDetectingOrdinaryEv(void *s)
{ ((dBgCh *)s)->dBgCh::StopDetectingOrdinary(); }
void *_ZN5dBgPiaSERKS_(void *d, const void *s)
{ return &(((dBgPi *)d)->operator=(*(const dBgPi *)s)); }
void _ZN5dBgPiD1Ev(void *s)
{ ((dBgPi *)s)->~dBgPi(); }
void _ZN5dBgPiC1Ev(void *s)
{ ::new (s) dBgPi(); }
int _ZNK5dBgPi9GetClsnIDEv(const void *s)
{ return ((const dBgPi *)s)->dBgPi::GetClsnID(); }
void _ZN12dBgCh_SphCrrC1Ev(void *s)
{ ::new (s) dBgCh_SphCrr(); }
void _ZN12dBgCh_SphCrrD1Ev(void *s)
{ ((dBgCh_SphCrr *)s)->~dBgCh_SphCrr(); }
void _ZN12dBgCh_SphCrr14SetFloorResultERK5dBgPi(void *s, const void *r)
{ ((dBgCh_SphCrr *)s)->dBgCh_SphCrr::SetFloorResult(*(const dBgPi *)r); }
void _ZN12dBgCh_SphCrr15SetObjAndSphereERK7Vector35Fix12IiEP8dActor_c(
    void *s, const void *v, int r, void *a)
{ ((dBgCh_SphCrr *)s)->dBgCh_SphCrr::SetObjAndSphere(*(const Vector3 *)v, Fix12<int>{r},
                                                    (dActor_c *)a); }
}  // extern "C"

/* ---- heap iterators/allocators, MaterialChanger, TextureSequence ---- */
#include "NestedHeapIterator.h"
#include "SolidHeapAllocator.h"
#include "MaterialChanger.h"
#include "TextureSequence.h"
extern "C" {
void _ZN18NestedHeapIterator5AddAtEP13HeapAllocatorS1_(void *s, void *n,
                                                       void *at)
{ ((NestedHeapIterator *)s)->NestedHeapIterator::AddAt((HeapAllocator *)n,
                                                       (HeapAllocator *)at); }
void _ZN18SolidHeapAllocator5ResetEj(void *s, unsigned a)
{ ((SolidHeapAllocator *)s)->SolidHeapAllocator::Reset(a); }
void *_ZN18SolidHeapAllocator8AllocateEji(void *s, unsigned a, int b)
{ return ((SolidHeapAllocator *)s)->SolidHeapAllocator::Allocate(a, b); }
void _ZN18SolidHeapAllocator9LoadStateEj(void *s, unsigned a)
{ ((SolidHeapAllocator *)s)->SolidHeapAllocator::LoadState(a); }
void _ZN18SolidHeapAllocator9SaveStateEj(void *s, unsigned a)
{ ((SolidHeapAllocator *)s)->SolidHeapAllocator::SaveState(a); }
void _ZN15MaterialChangerC1Ev(void *s)
{ ::new (s) MaterialChanger(); }
void _ZN15MaterialChangerD1Ev(void *s)
{ ((MaterialChanger *)s)->~MaterialChanger(); }
void _ZN15TextureSequenceC1Ev(void *s)
{ ::new (s) TextureSequence(); }
void _ZN15TextureSequenceD1Ev(void *s)
{ ((TextureSequence *)s)->~TextureSequence(); }
}  // extern "C"

/* ---- actor kits: brick block, question block, exit/door, water, mist ---- */
#include "BigBrickBlock.h"
#include "daObjHatenaBlock_c.h"
#include "daChRoom_c.h"
#include "daObjMcWater_c.h"
#include "daObjWaterfall_c.h"
extern "C" {
int _ZN13BigBrickBlock13InitResourcesEv(void *s)
{ return ((BigBrickBlock *)s)->BigBrickBlock::InitResources(); }
void _ZN13BigBrickBlockD0Ev(void *s)
{ ((BigBrickBlock *)s)->~BigBrickBlock(); }
void _ZN13BigBrickBlockD1Ev(void *s)
{ ((BigBrickBlock *)s)->~BigBrickBlock(); }
int _ZN13BigBrickBlock16HasNonzeroAngleXEv(void *s)
{ return ((BigBrickBlock *)s)->BigBrickBlock::HasNonzeroAngleX() ? 1 : 0; }
int _ZN13QuestionBlock13InitResourcesEv(void *s)
{ return ((daObjHatenaBlock_c *)s)->daObjHatenaBlock_c::InitResources(); }
int _ZN11VirtualDoor13InitResourcesEv(void *s)
{ return ((daChRoom_c *)s)->daChRoom_c::InitResources(); }
int _ZN11VirtualDoor16CleanupResourcesEv(void *s)
{ return ((daChRoom_c *)s)->daChRoom_c::CleanupResources(); }
void _ZN11VirtualDoor16OnPendingDestroyEv(void *s)
{ ((daChRoom_c *)s)->daChRoom_c::OnPendingDestroy(); }
int _ZN11VirtualDoor6RenderEv(void *s)
{ return ((daChRoom_c *)s)->daChRoom_c::Render(); }
int _ZN11VirtualDoor8BehaviorEv(void *s)
{ return ((daChRoom_c *)s)->daChRoom_c::Behavior(); }
void _ZN11CastleWater16OnPendingDestroyEv(void *s)
{ ((daObjMcWater_c *)s)->daObjMcWater_c::OnPendingDestroy(); }
}  // extern "C"

/* ---- actor kits: fish, bird, rabbit, lakitu, flag, cannon, mist ---- */
#include "Fish.h"
#include "daSBird_c.h"
#include "daMip_c.h"
#include "daJgm_c.h"
#include "daMcFlag_c.h"
#include "Cannon.h"
#include "EnemySwitchTag.h"
extern "C" {
void _ZN4Fish16OnPendingDestroyEv(void *s)
{ ((Fish *)s)->Fish::OnPendingDestroy(); }
int _ZN4Bird16CleanupResourcesEv(void *s)
{ return ((daSBird_c *)s)->daSBird_c::CleanupResources(); }
void _ZN4Bird16OnPendingDestroyEv(void *s)
{ ((daSBird_c *)s)->daSBird_c::OnPendingDestroy(); }
int _ZN6Rabbit16CleanupResourcesEv(void *s)
{ return ((daMip_c *)s)->daMip_c::CleanupResources(); }
void _ZN6Rabbit16OnPendingDestroyEv(void *s)
{ ((daMip_c *)s)->daMip_c::OnPendingDestroy(); }
int _ZN6Rabbit8BehaviorEv(void *s)
{ return ((daMip_c *)s)->daMip_c::Behavior(); }
void _ZN6RabbitD0Ev(void *s)
{ ((daMip_c *)s)->~daMip_c(); }
void _ZN6RabbitD1Ev(void *s)
{ ((daMip_c *)s)->~daMip_c(); }
int _ZN9LakituBro16CleanupResourcesEv(void *s)
{ return ((daJgm_c *)s)->daJgm_c::CleanupResources(); }
void _ZN9LakituBro16OnPendingDestroyEv(void *s)
{ ((daJgm_c *)s)->daJgm_c::OnPendingDestroy(); }
void _ZN9LakituBroD0Ev(void *s)
{ ((daJgm_c *)s)->~daJgm_c(); }
void _ZN9LakituBroD1Ev(void *s)
{ ((daJgm_c *)s)->~daJgm_c(); }
int _ZN8DockPole16CleanupResourcesEv(void *s)
{ return ((daMcFlag_c *)s)->daMcFlag_c::CleanupResources(); }
int _ZN6Cannon16CleanupResourcesEv(void *s)
{ return ((Cannon *)s)->Cannon::CleanupResources(); }
int _ZN14EnemySwitchTag16CleanupResourcesEv(void *s)
{ return ((EnemySwitchTag *)s)->EnemySwitchTag::CleanupResources(); }
void _ZN14EnemySwitchTag16OnPendingDestroyEv(void *s)
{ ((EnemySwitchTag *)s)->EnemySwitchTag::OnPendingDestroy(); }
int _ZN14EnemySwitchTag6RenderEv(void *s)
{ return ((EnemySwitchTag *)s)->EnemySwitchTag::Render(); }
void _ZN7daJgm_c8SetStateEi(void *s)
{
    /* The TU calls this with self only (its own decl is (void*)); the
       method takes the state too. InitResources' tail enters the initial
       state, which is 0 (mStateHandlers = table[0]). */
    ((daJgm_c *)s)->daJgm_c::SetState(0);
}
}  // extern "C"
