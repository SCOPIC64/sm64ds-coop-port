// Gate 12: the interactive window. Mario walks under keyboard control.
//
// Same staging as smoke_player (sinits, spawn context, castle-grounds KCL,
// InitResources, St_Wait), then a Win32 frame loop: keys write the pad
// block, Stage::CheckInput turns them into the stick record, Player::Behavior
// and Camera::Behavior tick, Camera::Render builds the projection and the
// view matrix, and the framebuffer blits into the client.
//
// Since gate 13 the camera IS the game's: the Camera actor at 0x14C, its
// 19-state machine, its mode-preset table and its own published heading --
// which is what turns "forward" on the stick into a world direction.
//
//   WASD / arrows  walk    Q/E  orbit    C  snap behind    ESC  quit
//
// THE GAME'S OWN BOOT AND THE GAME'S OWN PHYSICS ARE THE DEFAULT. ov009 is
// mounted, Stage::LoadClsnAndObjects runs against it, the level's entrance
// record spawns the Player and the Camera, and the ground and wall contact
// come from WithMeshClsn's own tracking through the hosted sphere pass. No
// harness stands in for anything in the physics loop.
//
// Env: SM64DS_LEGACY_BOOT=1 the pre-gate-14 harness staging instead of the
//                           level's own boot (hand-built spawn context, KCL
//                           mounted by hand, no entrance record)
//      SM64DS_FAKE_SNAP=1   the pre-sphere collision scaffolding: the level
//                           collider owned by the Player, the harness ground
//                           snap and the harness wall clamp. Retired, kept
//                           for A/B and for shots that need Mario planted.
//      SM64DS_NO_SPHERE=1   stub the sphere pass out (port/hal/clsn_vtable),
//                           which is the honest way to see what the
//                           scaffolding was covering for: a 28-unit bob at
//                           20 Hz that never settles
//      SM64DS_OLD_CAMERA=1  the pre-gate-13 hand-tuned follow rig
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

/* user32/gdi32 are loaded DYNAMICALLY after io_init: a static import chain
   initializes the desktop heap before main, and on 32-bit that mapping can
   land inside the fixed DS regions (0x04000000..0x07ffffff), killing
   io_init deterministically. Resolving late keeps the address space ours
   first. */
struct WinApi {
    ATOM(WINAPI *RegisterClassA_)(const WNDCLASSA *);
    HWND(WINAPI *CreateWindowExA_)(DWORD, LPCSTR, LPCSTR, DWORD, int, int,
                                   int, int, HWND, HMENU, HINSTANCE, LPVOID);
    LRESULT(WINAPI *DefWindowProcA_)(HWND, UINT, WPARAM, LPARAM);
    BOOL(WINAPI *PeekMessageA_)(MSG *, HWND, UINT, UINT, UINT);
    BOOL(WINAPI *TranslateMessage_)(const MSG *);
    LRESULT(WINAPI *DispatchMessageA_)(const MSG *);
    void(WINAPI *PostQuitMessage_)(int);
    HDC(WINAPI *GetDC_)(HWND);
    HCURSOR(WINAPI *LoadCursorA_)(HINSTANCE, LPCSTR);
    BOOL(WINAPI *AdjustWindowRect_)(RECT *, DWORD, BOOL);
    SHORT(WINAPI *GetAsyncKeyState_)(int);
    int(WINAPI *StretchDIBits_)(HDC, int, int, int, int, int, int, int, int,
                                const void *, const BITMAPINFO *, UINT, DWORD);
};
static WinApi W;

/* XInput, loaded dynamically like user32 (no static import chain) */
struct XPad {
    unsigned long packet;
    unsigned short buttons;
    unsigned char lt, rt;
    short lx, ly, rx, ry;
};
static DWORD(WINAPI *XInputGetState_)(DWORD, XPad *);

static bool winapi_load(void)
{
    HMODULE u = LoadLibraryA("user32.dll");
    HMODULE g = LoadLibraryA("gdi32.dll");
    if (!u || !g) return false;
    W.RegisterClassA_ = (decltype(W.RegisterClassA_))GetProcAddress(u, "RegisterClassA");
    W.CreateWindowExA_ = (decltype(W.CreateWindowExA_))GetProcAddress(u, "CreateWindowExA");
    W.DefWindowProcA_ = (decltype(W.DefWindowProcA_))GetProcAddress(u, "DefWindowProcA");
    W.PeekMessageA_ = (decltype(W.PeekMessageA_))GetProcAddress(u, "PeekMessageA");
    W.TranslateMessage_ = (decltype(W.TranslateMessage_))GetProcAddress(u, "TranslateMessage");
    W.DispatchMessageA_ = (decltype(W.DispatchMessageA_))GetProcAddress(u, "DispatchMessageA");
    W.PostQuitMessage_ = (decltype(W.PostQuitMessage_))GetProcAddress(u, "PostQuitMessage");
    W.GetDC_ = (decltype(W.GetDC_))GetProcAddress(u, "GetDC");
    W.LoadCursorA_ = (decltype(W.LoadCursorA_))GetProcAddress(u, "LoadCursorA");
    W.AdjustWindowRect_ = (decltype(W.AdjustWindowRect_))GetProcAddress(u, "AdjustWindowRect");
    W.GetAsyncKeyState_ = (decltype(W.GetAsyncKeyState_))GetProcAddress(u, "GetAsyncKeyState");
    W.StretchDIBits_ = (decltype(W.StretchDIBits_))GetProcAddress(g, "StretchDIBits");
    {
        const char *dlls[] = {"xinput1_4.dll", "xinput1_3.dll",
                              "xinput9_1_0.dll"};
        for (int i = 0; i < 3 && !XInputGetState_; ++i)
            if (HMODULE x = LoadLibraryA(dlls[i]))
                XInputGetState_ = (decltype(XInputGetState_))GetProcAddress(
                    x, "XInputGetState");
    }
    return W.RegisterClassA_ && W.CreateWindowExA_ && W.DefWindowProcA_ &&
           W.PeekMessageA_ && W.StretchDIBits_ && W.GetAsyncKeyState_;
}

#include "ntr/gx.h"
#include "ntr/mmio.h"
#include "ntr/ppu.h"

#include "fault_probe.h"

typedef unsigned int u32;

extern "C" {
void *_ZN6PlayerC1Ev(void *self);
void *_ZN4Heap13SetupRootHeapEv(void);
void *_ZN9ActorBasenwEj(unsigned size);
extern int data_0209b3ec[12];
extern unsigned short data_020a4b54;
extern void **data_020a4bb8;
extern void *data_020a0eac_c;
extern void *data_020a0ea0;
void hal_fill_model_vtable(void);
void hal_fill_shadow_vtable(void);
void hal_fill_mmc_vtable(void);
void hal_fill_modelanim2_vtable(void);
int hal_player_init_resources(void *p);
int hal_player_st_wait_init(void *p);
int hal_player_st_wait_main(void *p);
int hal_player_behavior(void *p);
int hal_player_process(void *p);   /* gate 15: BeforeBehavior/Behavior/After */
void hal_render_player_world(void *p);
extern char data_0209f4a0[];
extern int data_0209f4a6[];   /* pad stick WORLD angle -- auto_bss split
                                 symbol, NOT data_0209f4a0+6 on host */
/* the real input processor (Stage::CheckInput) and its environment */
void _ZN5Stage10CheckInputEv(void);
unsigned int _ZNK6Player14GetBodyModelIDEjb(char *, unsigned int, char);
extern int data_0209f498[];    /* CheckInput's own Ctrl[4] block */
extern int data_0209f4a2[];    /* split: stick nx */
extern int data_0209f4a4[];    /* split: stick ny */
extern unsigned char data_0209f4ac[]; /* split: touching */
extern int data_020a0e58[];    /* PadData[4]: u16 held, u16 pressed */
extern int data_020a0de8[];    /* TouchData[4], zero = no touch */
extern unsigned char data_0209f21c;   /* controller count */
extern int data_0209f350[];    /* per-pad status */
extern int data_020a1164[];    /* camera per-player block; +0 = angle
                                  (GetAngleToCamera reads it) */
extern int data_0209caa0[];
extern unsigned char data_0209d660;
extern int data_0209fc48;
extern unsigned char data_0209f2d8;
extern int data_0209214c[];    /* button remap pointer table (ROM DS
                                  pointers -- repointed at staging) */
extern char data_0209f49c[];   /* held buttons (bit 1 = A/jump held) */
extern char data_0209f49e[];   /* pressed-this-frame (bit 1 = jump) */
extern int data_0209b468[4];   /* actor list head (stomp tracker) */
extern unsigned char data_020a0e40[];
extern short data_02092144[];
extern unsigned char data_ov002_0211049c[];  /* St_Wait state object */
void port_ov002_patch(void);
void __sinit_ov002_02100560(void); void __sinit_ov002_02100938(void);
void __sinit_ov002_02100adc(void); void __sinit_ov002_02100c50(void);
void __sinit_ov002_02100d44(void); void __sinit_ov002_02100e50(void);
void __sinit_ov002_02100ec4(void); void __sinit_ov002_02100f84(void);
void __sinit_ov002_02101064(void); void __sinit_ov002_02101478(void);
void __sinit_ov002_021014e4(void); void __sinit_ov002_02101588(void);
void __sinit_ov002_02101738(void); void __sinit_ov002_02101894(void);
void __sinit_ov002_02101900(void); void __sinit_ov002_02101968(void);
void __sinit_ov002_021019d0(void); void __sinit_ov002_02106e40(void);
void __sinit_ov002_02107118(void); void __sinit_ov002_021071f4(void);
void __sinit_ov002_02107298(void); void __sinit_ov002_02107304(void);
void __sinit_ov002_02107370(void); void __sinit_ov002_02107f88(void);
void __sinit_ov002_0210804c(void); void __sinit_ov002_02108094(void);
void *_ZN13SharedFilePtr9ConstructEj(void *, unsigned);
void _ZN12MeshColliderC1Ev(void *);
void *_ZN12MeshCollider8LoadFileER13SharedFilePtr(void *);
void _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(void *, void *, void *);
int _ZN16MeshColliderBase6EnableEP5Actor(void *, void *);
void *_ZN5ModelC1Ev(void *);
void *_ZN5Model8LoadFileER13SharedFilePtr(void *);
void _ZN9ModelBase7SetFileEP8BMD_Fileii(void *, void *, int, int);
void hal_render_model(void *model, int scaleShift);
void _ZN13RaycastGroundC1Ev(void *);
void _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(void *, const void *,
                                                        void *);
int _ZN13RaycastGround10DetectClsnEv(void *);
void _ZN4BgCh19StartDetectingWaterEv(void *);
void _ZN4BgCh21StopDetectingOrdinaryEv(void *);
int SurfaceInfo_TestFlag0x20(const int *);
int hal_ground_ray(void *mc, int x, int y, int z, int reach, int *out_y);
int hal_line_ray(void *mc, const int *a, const int *b, int *out);
void _ZN12WithMeshClsn13SetGroundFlagEv(void *);
int func_02035354(void *, void *);
int func_020393b4(void *);
/* the real Camera actor (gate 13) */
void hal_fill_camera_vtable(void);
int hal_camera_check_layout(void);
void *hal_camera_new(void);
int hal_camera_init_resources(void *cam);
int hal_camera_behavior(void *cam);
int hal_camera_render(void *cam);
void func_0203e0ac(void);
extern void *data_0209f318;          /* the Camera singleton */
extern signed char data_02092120;    /* currently shown area, -1 = none */
extern unsigned char data_0209f250;  /* local player index */
extern void *data_0209f394[];        /* per-player Actor* */
extern unsigned char data_0209f1f8;  /* view-object count */
extern signed char data_0209f2f8;    /* level/sublevel id (weather select) */
extern int data_0209f32c[];          /* water level */
extern int data_0209f20c[], data_0209f294[], data_0209f2c4[];
extern int data_0209b454[];
extern int data_0209ee90[];
extern int data_020a4b60[];
extern int g_walk_dbg[16];     /* collision-walk telemetry (port/unmatched) */
/* the real level boot (hal/level_boot.cpp) */
void port_ov009_probe(void);
void *port_stage_a_boot(void *mc, int spawn_entrances);
void port_stage_a_probe(void *mc);
int port_stage_path_guard(void *player);
void port_stage_a2_seat(void);
/* the actor registry and the ROM's own processing lists (hal/actor_registry) */
void port_actor_tick(void);          /* phases 4/2/3: cleanup, init, behaviour */
void port_actor_render(void);        /* phase 5: the render bucket */
void port_actor_scene_pass(void);    /* phase 1: scene-tree housekeeping */
void port_actor_census(void);
void port_actor_lists_probe(void);
}

#ifdef NTR_HIRES
static const int ZOOM = 1;
#elif defined(NTR_HIRES2)
static const int ZOOM = 2;
#else
static const int ZOOM = 3;
#endif
static void *g_mc;

static LRESULT CALLBACK wndproc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_DESTROY) { W.PostQuitMessage_(0); return 0; }
    if (m == WM_KEYDOWN && w == VK_ESCAPE) { W.PostQuitMessage_(0); return 0; }
    return W.DefWindowProcA_(h, m, w, l);
}

/* camera folded into the GX projection matrix: P(perspective) * V(lookAt),
   built in floats and pushed as 4096-fixed.
   ITS CALLERS THINK IN WORLD UNITS and the frame is drawn in SCENE units
   (world >> 3, Camera::Render's own conversion), so the eye, the look-at and
   the near/far planes all come across the same divide right here. That keeps
   the hand-tuned rig behind SM64DS_OLD_CAMERA readable in the units its
   occlusion rays and standoff distances are written in. */
static void push_camera(const float eye_w[3], const float at_w[3])
{
    const float eye[3] = {eye_w[0] / 8, eye_w[1] / 8, eye_w[2] / 8};
    const float at[3] = {at_w[0] / 8, at_w[1] / 8, at_w[2] / 8};
    float fz[3] = {at[0] - eye[0], at[1] - eye[1], at[2] - eye[2]};
    float ln = sqrtf(fz[0] * fz[0] + fz[1] * fz[1] + fz[2] * fz[2]);
    for (int i = 0; i < 3; ++i) fz[i] /= (ln > 1e-6f ? ln : 1.0f);
    float up[3] = {0, 1, 0};
    float sx[3] = {fz[1] * up[2] - fz[2] * up[1],
                   fz[2] * up[0] - fz[0] * up[2],
                   fz[0] * up[1] - fz[1] * up[0]};
    ln = sqrtf(sx[0] * sx[0] + sx[1] * sx[1] + sx[2] * sx[2]);
    for (int i = 0; i < 3; ++i) sx[i] /= (ln > 1e-6f ? ln : 1.0f);
    float uy[3] = {sx[1] * fz[2] - sx[2] * fz[1],
                   sx[2] * fz[0] - sx[0] * fz[2],
                   sx[0] * fz[1] - sx[1] * fz[0]};

    /* row-vector convention to match the GX (v * M) */
    float V[16] = {
        sx[0], uy[0], -fz[0], 0,
        sx[1], uy[1], -fz[1], 0,
        sx[2], uy[2], -fz[2], 0,
        -(sx[0] * eye[0] + sx[1] * eye[1] + sx[2] * eye[2]),
        -(uy[0] * eye[0] + uy[1] * eye[1] + uy[2] * eye[2]),
        (fz[0] * eye[0] + fz[1] * eye[1] + fz[2] * eye[2]), 1};

    /* the ROM's fov: camera presets set the half-angle field to 0xbb0
       DS units = 16.46 deg, so the game renders ~33 deg vertical. The
       old 55 was a wide-angle lens -- it shrank and warped the world
       around Mario no matter how right the geometry was. */
    const float fovy = 32.9f * 3.14159265f / 180.0f;
    const float aspect = (float)ntr::SCREEN_W / ntr::SCREEN_H;
    const float f = 1.0f / tanf(fovy * 0.5f);
    const float zn = 3.0f / 8, zf = 25600.0f / 8;
    float P[16] = {f / aspect, 0, 0, 0,
                   0, f, 0, 0,
                   0, 0, (zf + zn) / (zn - zf), -1,
                   0, 0, 2 * zf * zn / (zn - zf), 0};

    float M[16];
    for (int r = 0; r < 4; ++r)
        for (int c2 = 0; c2 < 4; ++c2) {
            float s = 0;
            for (int k = 0; k < 4; ++k) s += V[r * 4 + k] * P[k * 4 + c2];
            M[r * 4 + c2] = s;
        }
    NTR_MMIO(uint32_t, 0x04000440) = 0;
    for (int i = 0; i < 16; ++i)
        NTR_MMIO(uint32_t, 0x04000458) = (uint32_t)(int32_t)(M[i] * 4096.0f);
    NTR_MMIO(uint32_t, 0x04000440) = 1;
    NTR_MMIO(uint32_t, 0x04000454) = 0;
}

int main(void)
{
    /* world = KCL file x64. Default spawn: north end of the stone
       bridge (deck ~892), facing the walk south across it -- the shot
       that calibrates against real-game footage. Roof surface = 4916,
       lawn = 784 (SM64DS_SPAWN overrides, world units). */
    int spawn_x = 0, spawn_y = 960, spawn_z = 1000;
    /* THE PHYSICS SCAFFOLDING IS RETIRED. It existed for exactly one reason:
       WithMeshClsn's continuous update finds a floor two ways, the swept head
       segment (RaycastLine, hosted, and real since gate 15 gave it a real
       prev position) and the SPHERE, and the sphere is what holds a STANDING
       actor up. With MeshCollider::DetectClsn(SphereClsn &) stubbed the
       Player sank about vo before the head sweep crossed the floor plane and
       shoved him back, which read as a 28-unit bob at 20 Hz that never
       settled, so the harness ground snap and wall clamp had to stay on.

       The sphere pass is hosted now (port/unmatched/
       MeshCollider_DetectClsn_Sphere.cpp), and standing is exact: the same
       idle spot holds one value for 600 frames, and a 2700-unit drop lands
       and stops dead where the same drop under the stub bobbed 912/940/927
       or sank 439 -> 373 over 200 frames.

       SM64DS_FAKE_SNAP=1 brings the whole old configuration back in one
       switch -- the level collider owned by the Player, which makes the
       game's own ground tracking a no-op (see the Enable call below), plus
       the snap and the wall clamp on top. Kept for the A/B and for shots
       that need Mario planted regardless. */
    const int fake_snap = getenv("SM64DS_FAKE_SNAP") != 0;
    const int ground_snap = fake_snap;
    const int wall_stop = fake_snap;
    PORT_INSTALL_FAULT_PROBE();
    port_install_watchdog();
    setvbuf(stdout, NULL, _IONBF, 0);
    if (!ntr::io_init()) { fprintf(stderr, "io_init failed\n"); return 2; }
    if (!winapi_load()) { fprintf(stderr, "winapi_load failed\n"); return 2; }
    if (!_ZN4Heap13SetupRootHeapEv()) return 2;
    memset(data_0209b3ec, 0, 48);
    data_0209b3ec[0] = data_0209b3ec[4] = data_0209b3ec[8] = 0x1000;
    hal_fill_model_vtable();
    hal_fill_shadow_vtable();
    hal_fill_mmc_vtable();
    hal_fill_modelanim2_vtable();

    port_ov002_patch();
    __sinit_ov002_02100560(); __sinit_ov002_02100938();
    __sinit_ov002_02100adc(); __sinit_ov002_02100c50();
    __sinit_ov002_02100d44(); __sinit_ov002_02100e50();
    __sinit_ov002_02100ec4(); __sinit_ov002_02100f84();
    __sinit_ov002_02101064(); __sinit_ov002_02101478();
    __sinit_ov002_021014e4(); __sinit_ov002_02101588();
    __sinit_ov002_02101738(); __sinit_ov002_02101894();
    __sinit_ov002_02101900(); __sinit_ov002_02101968();
    __sinit_ov002_021019d0(); __sinit_ov002_02106e40();
    __sinit_ov002_02107118(); __sinit_ov002_021071f4();
    __sinit_ov002_02107298(); __sinit_ov002_02107304();
    __sinit_ov002_02107370(); __sinit_ov002_02107f88();
    __sinit_ov002_0210804c(); __sinit_ov002_02108094();

    /* THE GAME'S OWN LEVEL BOOT, now the default: ov009 mounted,
       Stage::LoadClsnAndObjects run against it, and the level's own entrance
       record spawning the Player and the Camera. SM64DS_LEGACY_BOOT=1 goes
       back to the harness staging (hand-built spawn context, KCL mounted by
       hand); SM64DS_BOOT_NOSPAWN=1 holds the entrance table off, which is
       stage A1, the same boot with nothing spawning. SM64DS_REAL_BOOT is
       still accepted and is now a no-op. */
    const int real_boot = getenv("SM64DS_LEGACY_BOOT") == 0;
    const int boot_spawns = real_boot && getenv("SM64DS_BOOT_NOSPAWN") == 0;
    if (real_boot)
        port_ov009_probe();

    data_02092144[0] = 8 << 8;
    if (!boot_spawns) {
        /* the fake spawn context: actor id 0 with invented priorities, which
           is what the hand-built Player was constructed under. The real boot
           reads the ROM's own SpawnInfo out of the registry instead. */
        data_020a4b54 = 0;
        static unsigned short spawn_info[4] = {0, 0, 100, 100};
        data_020a4bb8[0] = spawn_info;
    }
    data_020a0eac_c = data_020a0ea0;

    /* Game mode 0 (adventure) -- LoadClsnAndObjects branches its minimap
       and HUD spawns on this, and Stage::CheckInput reads it later. */
    data_0209f2d8 = 0;

    /* The collision object itself is the harness's either way; what fills it
       is the question. Under SM64DS_REAL_BOOT the game's own
       Stage::LoadClsnAndObjects does it -- and it runs BEFORE the Player,
       because on the real boot the entrance spawns the Player and
       Player::InitResources reads the world-Y bounds the boot just set. */
    static char mc_storage[0x60];
    unsigned level_bmd = 1943;
    g_mc = mc_storage;
    _ZN12MeshColliderC1Ev(mc_storage);
    if (real_boot) {
        /* Door and exit stay off in both stages -- their actors are Stage B.
           With SM64DS_BOOT_NOSPAWN the entrance table goes off too and the
           sub-table is dropped, which is stage A1: geometry only. */
        if (boot_spawns)
            port_stage_a2_seat();
        void *lvl = port_stage_a_boot(mc_storage, boot_spawns);
        level_bmd = *(unsigned short *)((char *)lvl + 8);
        port_stage_a_probe(mc_storage);
        if (boot_spawns) {
            port_actor_census();
            port_actor_lists_probe();
        }
    }

    void *player;
    if (boot_spawns) {
        /* THE ENTRANCE SPAWNED HIM. data_0209f394[0] is where
           LoadEntranceObjects parked the actor it made from entrance record
           0 -- position, rotation, area and entrance type all the level's
           own, and Player::InitResources already run through the spawn
           spine's init Process. */
        player = data_0209f394[0];
        if (!player) {
            fprintf(stderr, "the entrance spawned no player\n");
            return 3;
        }
    } else {
        player = _ZN9ActorBasenwEj(0x800);
        _ZN6PlayerC1Ev(player);
        if (hal_player_init_resources(player) != 1) return 3;
    }

    /* the castle grounds floor, gate-8 recipe */
    char *c = (char *)player;
    if (!real_boot) {
        static struct { unsigned short id; unsigned char refs; void *p; } kp;
        _ZN13SharedFilePtr9ConstructEj(&kp, 1941);
        char *kcl = (char *)_ZN12MeshCollider8LoadFileER13SharedFilePtr(&kp);
        if (!kcl) return 4;
        static char clps[0x100];
        _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(mc_storage, kcl,
                                                          clps);
        /* ROOT CAUSE (found 2026-08-02): the level collider's OWNER feeds
           func_02035354's self-collision exclusion. Enabling it with the
           player as owner makes every player ground/wall probe skip the
           level -- which is why the game's own tracking never grounded and
           the harness snap exists. The game's own convention for level
           geometry is Enable(NULL): func_020395fc then stores owner 0 and
           clsnID -1, so level hits skip the FindWithID actor walk (a fake
           non-null owner fed it a junk ID and it faulted).
           NOW THE DEFAULT (gate 13): the real Camera seeds its own probes
           with the Player as their owner (func_0200897c), so under the old
           owner=player configuration every camera ray would exclude the
           level and the camera would sit inside geometry. The harness's own
           probes (hal_ground_ray / hal_line_ray) work under a NULL owner
           either way. SM64DS_FAKE_SNAP=1 brings the harness ground snap
           back on top for shots that need Mario planted. */
        _ZN16MeshColliderBase6EnableEP5Actor(
            mc_storage, fake_snap ? (void *)player : (void *)0);
        /* NO SCALE PAIR HERE ANY MORE. world = KCL raw << 6 is the walk's
           own business now (the ROM's `asr #6`, see
           port/unmatched/MeshCollider_DetectClsn_Sphere.cpp), and the real
           boot writes nothing after SetFile either -- which is the point:
           the harness staging and the level's own boot leave the collider
           in exactly the same state. */
    }
    /* the octree box is power-of-two PADDED (its center is way off the
       real stage); the geometry lives near the origin, so spawn there,
       a few units up -- the first frames drop him onto the lawn.
       Under the entrance boot there is nothing to invent: the level's own
       entrance record already put him at the castle gate, and SM64DS_SPAWN
       is the only way to move him. */
    {
        const char *sp = getenv("SM64DS_SPAWN");
        if (sp) sscanf(sp, "%d,%d,%d", &spawn_x, &spawn_y, &spawn_z);
        if (sp || !boot_spawns) {
            *(int *)(c + 0x5c) = spawn_x << 12;
            *(int *)(c + 0x60) = spawn_y << 12;
            *(int *)(c + 0x64) = spawn_z << 12;
        } else {
            spawn_x = *(int *)(c + 0x5c) >> 12;
            spawn_y = *(int *)(c + 0x60) >> 12;
            spawn_z = *(int *)(c + 0x64) >> 12;
        }
        printf("player at (%d, %d, %d) yaw %04x state %p step %u/%u "
               "path %02x\n",
               *(int *)(c + 0x5c) >> 12, *(int *)(c + 0x60) >> 12,
               *(int *)(c + 0x64) >> 12,
               (unsigned short)*(short *)(c + 0x8e), *(void **)(c + 0x370),
               *(unsigned char *)(c + 0x6e3), *(unsigned char *)(c + 0x6e5),
               *(unsigned *)(c + 0x670));
        if (getenv("PORT_WATCH_POS"))
            port_watch_words(c + 0x5c, 3);
    }
    /* THE RADIUS LEVER IS NOT HERE. WithMeshClsn+0x18 is the radius
       UpdateExtraContinous hands to SphereClsn::SetObjAndSphere and +0x1c the
       vertical offset it adds to pos first, but Player::Behavior RECOMPUTES
       BOTH every frame from the mega/balloon factor and sets them to the SAME
       value, so writing either one here is overwritten before it is read --
       and the game's own mega lever moves them together, which cancels out of
       the resting height entirely (floor - vo + radius). Scaling the sphere's
       radius alone has to happen at the sphere: SM64DS_SPHERE_RADIUS_PCT in
       port/hal/clsn_vtable.cpp. */
    /* the level model: main_castle_all.bmd -- handle 1943 by hand, and under
       the real boot the LVL_Overlay's own bmdFileId, which is the same 1943
       (the harness had guessed right); world-space verts scaled by the BMD
       header's scaleShift */
    static char level_storage[0x50];
    int level_shift = 0;
    {
        static struct { unsigned short id; unsigned char refs; void *p; } mp;
        _ZN13SharedFilePtr9ConstructEj(&mp, level_bmd);
        _ZN5ModelC1Ev(level_storage);
        void *bmd = _ZN5Model8LoadFileER13SharedFilePtr(&mp);
        if (bmd) {
            level_shift = *(int *)bmd;   /* BMD header word 0 */
            _ZN9ModelBase7SetFileEP8BMD_Fileii(level_storage, bmd, 0, -1);
            printf("level model loaded, scaleShift %d\n", level_shift);
        } else {
            fprintf(stderr, "level model load failed (handle %u)\n", level_bmd);
        }
    }

    /* diagnostic: a direct ground ray at the spawn separates a filter
       problem (player flags) from a registry problem (nothing hittable) */
    {
        static char rg[0x50];
        int pos[3] = {*(int *)(c + 0x5c), *(int *)(c + 0x60),
                      *(int *)(c + 0x64)};
        _ZN13RaycastGroundC1Ev(rg);
        _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(rg, pos, player);
        rg[4] |= 1;   /* BgCh collide-ordinary (the gate-8 predicate bit) */
        *(int *)(rg + 0x4c) = 0x100000;   /* reach: 256 units down */
        int hit = _ZN13RaycastGround10DetectClsnEv(rg);
        printf("ground probe at spawn: hit=%d ground_y=%d (%.1f units)\n",
               hit, *(int *)(rg + 0x3c), *(int *)(rg + 0x3c) / 4096.0f);
        {
            extern void *data_020a0c80[];
            int direct = ((int(__fastcall *)(void *, void *, void *))(
                ((void ***)g_mc)[0][6]))(g_mc, 0, rg);
            printf("registry[0]=%p mc=%p direct-slot6=%d gy=%.1f flag=%d\n",
                   data_020a0c80[0], g_mc, direct,
                   *(int *)(rg + 0x44) / 4096.0f, rg[0x48]);
            /* the method's own first block, replicated by hand */
            char *o = (char *)data_020a0c80[0];
            printf("manual: o=%p p=%p ray_fc=%d head4=%d reach=%d\n", o,
                   *(void **)(o + 4), *(int *)(rg + 0xc), rg[4],
                   *(int *)(rg + 0x4c));
            {
                extern int func_02035354(void *, void *);
                extern int func_020393b4(void *);
                void *p2 = (void *)func_020393b4(o);
                int f = func_02035354(rg, p2);
                printf("manual filter(rg, p)=%d p2=%p\n", f, p2);
            }
        }
        /* floor map: direct line walks over a coarse grid */
        for (int gz = -400; gz <= 400; gz += 200) {
            char row[64] = {0};
            int ri = 0;
            for (int gx = -400; gx <= 400; gx += 200) {
                int gy = 0;
                int h = hal_ground_ray(g_mc, gx << 12, 6000 << 12,
                                       gz << 12, 7000 << 12, &gy);
                ri += snprintf(row + ri, sizeof row - ri, "%7.1f",
                               h ? gy / 4096.0f : -999.0f);
            }
            printf("floor z=%4d: %s\n", gz, row);
        }
        /* SM64DS_WATER_MAP=1: WHERE THE WATER IS, asked exactly the way the
           Player asks. func_ov002_020c14b8's second ray is a RaycastGround
           with StartDetectingWater + StopDetectingOrdinary, and the surface it
           accepts is one whose CLPS carries flag 0x20; the answer lands in
           Player+0x64c and is what decides walk -> swim. This runs that same
           query over a grid, straight down from well above the level, and
           prints the water height it finds. A blank map with a water collider
           registered ([clsnreg]) means the query never reaches the mesh; a map
           with water in it means the query works and a probe that found none
           was simply standing somewhere dry. */
        if (getenv("SM64DS_WATER_MAP")) {
            /* SM64DS_WATER_MAP=<step>: floor height per cell, with a `~` on
               any cell whose floor is UNDER the water the query found there.
               `~` next to a plain number is a bank, which is where the
               walk<->swim transition can be exercised. */
            int step = atoi(getenv("SM64DS_WATER_MAP"));
            if (step < 250) step = 1000;
            printf("[watermap] step %d, rays from y=6000 reach 12000; "
                   "'~' = floor under water, '-' = no floor\n", step);
            for (int gz = -8000; gz <= 8000; gz += step) {
                char row[512];
                int ri = 0;
                for (int gx = -8000; gx <= 8000; gx += step) {
                    static char rgw[0x50];
                    int pos[3] = {gx << 12, 6000 << 12, gz << 12};
                    _ZN13RaycastGroundC1Ev(rgw);
                    _ZN4BgCh19StartDetectingWaterEv(rgw);
                    _ZN4BgCh21StopDetectingOrdinaryEv(rgw);
                    _ZN13RaycastGround12SetObjAndPosERK7Vector3P5Actor(
                        rgw, pos, player);
                    *(int *)(rgw + 0x4c) = 12000 << 12;
                    int wet = _ZN13RaycastGround10DetectClsnEv(rgw) &&
                              SurfaceInfo_TestFlag0x20((int *)(rgw + 0x14));
                    int wy = *(int *)(rgw + 0x44);
                    int gy = 0;
                    int g = hal_ground_ray(g_mc, gx << 12, 6000 << 12,
                                           gz << 12, 12000 << 12, &gy);
                    if (!g)
                        ri += snprintf(row + ri, sizeof row - ri, "%7s", "-");
                    else
                        ri += snprintf(row + ri, sizeof row - ri, "%6.0f%c",
                                       gy / 4096.0f,
                                       (wet && gy < wy) ? '~' : ' ');
                }
                printf("[watermap] z=%6d %s\n", gz, row);
            }
        }
    }

    data_020a0e40[0] = 0;
    /* input processor staging: route Stage::CheckInput to its main
       path (mode flags), one controller, pad 0 active, mode 0 (D-pad
       drives the stick fields) */
    data_0209caa0[2] |= 0x80;
    data_0209d660 = 0;
    data_0209fc48 = 0;
    data_0209f21c = 1;
    data_0209f350[0] = 0;
    /* the ROM's button-remap tables are DS pointers (0x0207xxxx) the
       host has no image behind; buttons are written directly to the
       Ctrl fields anyway, so give CheckInput's remap loop zeros */
    {
        static unsigned short zero_btn_map[32];
        for (int i = 0; i < 4; ++i)
            ((unsigned short **)data_0209214c)[i] = zero_btn_map;
    }
    /* InitResources parked the state machine in St_LevelEnter, which was a
       no-op state under the fake spawn context: its entrance-anim table read
       junk and its Main was not hosted. Clearing it let the wait ticks drive
       until the first stick input. Under the entrance boot the park is
       CORRECT -- the level's own entrance record chose the state and its
       step -- so it stands, and St_LevelEnter runs the entry animation until
       it hands over to Wait. */
    if (!boot_spawns)
        *(void **)(c + 0x370) = 0;
    /* no path binding: the level spawn entry's path param, 0xff = none.
       The fake spawn context zero-fills it, and path 0 sends the real
       ground tracking into PathPtr walks over a table no level boot has
       seated (frame-1 fault under the game's own tracking). The real boot
       seats data_020a0d84/d88/d8c, so the pin comes off with it. */
    if (!real_boot)
        *(unsigned int *)(c + 0x670) = 0xff;
    if (getenv("PORT_WATCH_HEAD"))
        port_watch_words(data_0209b468, 4);
    if (!boot_spawns)
        hal_player_st_wait_init(player);

    /* ---- the real Camera actor (gate 13) -----------------------------
       The default since the camera came up clean over 400 frames.
       SM64DS_OLD_CAMERA=1 brings back the hand-tuned follow rig; arming
       data_0209f318 also wakes ~30 dormant ov002 call sites that funnel
       into Camera::ChangeState, so the escape hatch is worth keeping.

       Order matters. The vtable has to be up before InitResources,
       because InitResources ends in a virtual call to slot 9 (Render);
       and data_0209f318 is armed LAST, after the object is fully built,
       so nothing reaches a half-initialized camera. */
    void *cam = 0;
    const int real_camera = getenv("SM64DS_OLD_CAMERA") == 0;
    if (boot_spawns) {
        /* THE ENTRANCE SPAWNED IT TOO. LoadEntranceObjects finishes by
           spawning actor 0x14c with the entrance id it just read and parking
           the result in data_0209f318, which is exactly what the block below
           did by hand -- vtable, spawn context, InitResources, arm. */
        cam = data_0209f318;
        if (!cam) {
            fprintf(stderr, "the entrance spawned no camera\n");
            return 5;
        }
        printf("camera at %p (entrance-spawned), mode %p, state %p, fov %d\n",
               cam, *(void **)((char *)cam + 0x13c),
               *(void **)((char *)cam + 0x138),
               *(short *)((char *)cam + 0x17a));
    } else if (real_camera) {
        /* engine state the camera boot reads. All of it is what a level
           with no Stage loader looks like: no view objects, no weather,
           no area shown yet, the local player at index 0. */
        data_02092120 = -1;          /* no area shown -> ChangeArea skips Hide */
        data_0209f250 = 0;           /* local player index */
        data_0209f394[0] = player;   /* the actor the camera follows */
        data_0209f1f8 = 0;           /* view-object count */
        data_0209f2f8 = 0;           /* sublevel id: no weather system */
        data_0209f32c[0] = 0;        /* water level */
        data_0209fc48 = 0;           /* not in a cutscene */
        data_0209f20c[0] = data_0209f294[0] = data_0209f2c4[0] = 0;
        data_0209b454[0] = 0;
        data_0209ee90[0x44 / 4] = 0x1000;   /* scaleW, what Render feeds
                                               PerspectiveW_ (R10) */
        /* spawn context: actor 0x14C, spawn param 0 (entrance 0). The
           param lands at actor+8 and picks func_0200cf40's branch --
           anything but 0xf takes the view-object path, which is the one
           that selects mode 10, the gameplay camera. It is safe here
           because data_0209f354 points at a real (zeroed) table. */
        {
            static unsigned short cam_spawn_info[4] = {0, 0, 0x14c, 0};
            data_020a4bb8[0x14c] = cam_spawn_info;
            data_020a4b54 = 0x14c;
            data_020a4b60[0] = 0;
        }
        if (!hal_camera_check_layout())
            fprintf(stderr, "[cam] LAYOUT CHECK FAILED -- expect nonsense\n");
        hal_fill_camera_vtable();
        cam = hal_camera_new();          /* the ctor allocates its own 0x1a8 */
        if (!cam) { fprintf(stderr, "camera alloc failed\n"); return 5; }
        if (hal_camera_init_resources(cam) != 1)
            fprintf(stderr, "[cam] InitResources did not return 1\n");
        data_0209f318 = cam;             /* ARMED LAST */
        printf("camera at %p, mode %p, state %p, fov %d, near %d far %d\n",
               cam, *(void **)((char *)cam + 0x13c),
               *(void **)((char *)cam + 0x138),
               *(short *)((char *)cam + 0x17a),
               *(int *)((char *)cam + 0xfc), *(int *)((char *)cam + 0x100));
    }

    /* window */
    WNDCLASSA wc = {};
    wc.lpfnWndProc = wndproc;
    wc.hInstance = GetModuleHandleA(0);
    wc.hCursor = W.LoadCursorA_(0, (LPCSTR)IDC_ARROW);
    wc.lpszClassName = "sm64ds_walk";
    W.RegisterClassA_(&wc);
    RECT r = {0, 0, ntr::SCREEN_W * ZOOM, ntr::SCREEN_H * ZOOM};
    W.AdjustWindowRect_(&r, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME, FALSE);
    HWND hwnd = W.CreateWindowExA_(0, "sm64ds_walk",
                              "SM64DS port -- WASD walk, ESC quit",
                              (WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME) |
                                  WS_VISIBLE,
                              CW_USEDEFAULT, CW_USEDEFAULT,
                              r.right - r.left, r.bottom - r.top, 0, 0,
                              wc.hInstance, 0);
    HDC hdc = W.GetDC_(hwnd);
    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = ntr::SCREEN_W;
    bi.bmiHeader.biHeight = -ntr::SCREEN_H;   /* top-down */
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    /* SM64DS_WINDOW_SELFTEST=N: run N frames with W held, dump the last
       framebuffer next to the exe, exit -- CI-checkable without a user */
    const char *st = getenv("SM64DS_WINDOW_SELFTEST");
    const int selftest = st ? atoi(st) : 0;
    int frame = 0;
    float cam_yaw = 0.0f;   /* camera heading around Mario, radians */
    float cam_pitch = 0.13f; /* camera tilt above level, radians (R/F) */

    static ntr::Framebuffer fb;
    MSG msg;
    for (;;) {
        while (W.PeekMessageA_(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            W.TranslateMessage_(&msg);
            W.DispatchMessageA_(&msg);
        }

        /* keys -> pad block + desired heading, CAMERA-RELATIVE: W walks
           away from the camera whatever way it faces. cam_yaw is the
           camera's heading around Mario (radians; 0 = classic behind-south
           view). Q/E orbit it; when Mario walks and Q/E are idle it eases
           in behind his motion like the real game's lazy camera. */
        int dx = 0, dz = 0;
        if (selftest && !getenv("SM64DS_SELFTEST_IDLE")) {
            dz = 1;
            /* turn probe: hold "A" from frame 60 -- position x must curve */
            if (getenv("SM64DS_SELFTEST_TURN") && frame >= 60) dx = -1;
            /* release probe: let go at speed (the brake/skid path) */
            if (getenv("SM64DS_SELFTEST_RELEASE") && frame >= 50) dz = 0;
            /* reversal probe: hard 180 at speed (the skid-turn path) */
            if (getenv("SM64DS_SELFTEST_REVERSE") && frame >= 50) dz = -1;
        }
        if (W.GetAsyncKeyState_('W') < 0 || W.GetAsyncKeyState_(VK_UP) < 0) dz += 1;
        if (W.GetAsyncKeyState_('S') < 0 || W.GetAsyncKeyState_(VK_DOWN) < 0) dz -= 1;
        if (W.GetAsyncKeyState_('A') < 0 || W.GetAsyncKeyState_(VK_LEFT) < 0) dx -= 1;
        if (W.GetAsyncKeyState_('D') < 0 || W.GetAsyncKeyState_(VK_RIGHT) < 0) dx += 1;
        /* gamepad: left stick / d-pad walk, right stick orbits + tilts */
        static XPad pad;
        int pad_live = XInputGetState_ && XInputGetState_(0, &pad) == 0;
        int orbiting = 0;
        if (pad_live) {
            if (pad.ly > 12000 || (pad.buttons & 1)) dz += 1;
            if (pad.ly < -12000 || (pad.buttons & 2)) dz -= 1;
            if (pad.lx < -12000 || (pad.buttons & 4)) dx -= 1;
            if (pad.lx > 12000 || (pad.buttons & 8)) dx += 1;
            if (pad.rx < -10000 || pad.rx > 10000) {
                cam_yaw += 0.045f * (pad.rx / 32768.0f);
                orbiting = 1;
            }
            if (pad.ry > 10000 && cam_pitch < 0.85f) cam_pitch += 0.02f;
            if (pad.ry < -10000 && cam_pitch > -0.15f) cam_pitch -= 0.02f;
        }
        if (W.GetAsyncKeyState_('Q') < 0) { cam_yaw -= 0.045f; orbiting = 1; }
        if (W.GetAsyncKeyState_('E') < 0) { cam_yaw += 0.045f; orbiting = 1; }
        if (W.GetAsyncKeyState_('R') < 0 && cam_pitch < 0.85f)
            cam_pitch += 0.02f;
        if (W.GetAsyncKeyState_('F') < 0 && cam_pitch > -0.15f)
            cam_pitch -= 0.02f;
        /* THE GAME'S OWN INPUT PROCESSOR: keys become raw DS pad bits,
           Stage::CheckInput turns them into the stick record (mag, dir,
           binang -- the D-pad path, mode 0), and Player::Behavior folds
           in the camera angle via GetAngleToCamera, which reads the
           angle the harness publishes below. No hand-built headings. */
        {
            unsigned short raw = 0;
            if (dz > 0) raw |= 0x40;   /* up    */
            if (dz < 0) raw |= 0x80;   /* down  */
            if (dx < 0) raw |= 0x20;   /* left  */
            if (dx > 0) raw |= 0x10;   /* right */
            static unsigned short raw_prev;
            *(unsigned short *)((char *)data_020a0e58 + 0) = raw;
            *(unsigned short *)((char *)data_020a0e58 + 2) =
                (unsigned short)(raw & (unsigned short)~raw_prev);
            raw_prev = raw;
            /* the angle FROM Mario TO the camera (what the name
               GetAngleToCamera means): the D-pad table's "up" entry is
               0x8000, so up + angle-to-camera = away from the lens.
               Under the real camera this is NOT written by hand: the
               camera publishes its own heading through func_0203dafc ->
               data_020a1040 -> func_0203e0ac -> data_020a1154, and
               GetAngleToCamera reads the far end of that chain. */
            if (!real_camera)
                *(short *)((char *)data_020a1164 + 0) =
                    (short)((int)(cam_yaw * (32768.0f / 3.14159265f)) + 0x8000);
            _ZN5Stage10CheckInputEv();
            /* the matched TU writes its own data_0209f498 block; older
               TUs read per-field split symbols -- copy the record out */
            {
                const char *q = (const char *)data_0209f498;
                *(short *)(data_0209f4a0 + 0) = *(const short *)(q + 0x08);
                *(short *)data_0209f4a2 = *(const short *)(q + 0x0a);
                *(short *)data_0209f4a4 = *(const short *)(q + 0x0c);
                *(short *)data_0209f4a6 = *(const short *)(q + 0x0e);
                data_0209f4ac[0] = *(const unsigned char *)(q + 0x14);
            }
            /* camera lazy-follow, from the same intended direction */
            if ((dx || dz) && !orbiting) {
                float head = cam_yaw + atan2f((float)-dx, (float)dz);
                float d = head - cam_yaw;
                while (d > 3.14159265f) d -= 2 * 3.14159265f;
                while (d < -3.14159265f) d += 2 * 3.14159265f;
                if (d > -1.35f && d < 1.35f)
                    cam_yaw += d * 0.015f;
            }
        }

        /* Buttons -> the Ctrl held/pressed fields directly (CheckInput's
           remap tables are ROM pointers with no host image). DS bits:
           1 = A (punch), 2 = B (jump), 0x100 = R (crouch), 0x800 = the
           dash button the walk core reads. */
        {
            static unsigned short btn_was;
            unsigned short btn = 0;
            if (W.GetAsyncKeyState_(VK_SPACE) < 0) btn |= 2;
            if (W.GetAsyncKeyState_(VK_SHIFT) < 0) btn |= 0x800;
            if (W.GetAsyncKeyState_(VK_CONTROL) < 0) btn |= 0x100;
            if (W.GetAsyncKeyState_('X') < 0) btn |= 1;
            if (pad_live) {
                if (pad.buttons & 0x1000) btn |= 2;      /* A -> jump  */
                if (pad.buttons & 0x4000) btn |= 1;      /* X -> punch */
                if (pad.buttons & 0x2000) btn |= 0x800;  /* B -> dash  */
                if ((pad.buttons & 0x0300) || pad.lt > 100 || pad.rt > 100)
                    btn |= 0x100;                        /* LB/RB/trig -> crouch */
            }
            /* selftest: synthetic hop at frame 30 (walking start speed) */
            if (selftest && frame >= 30 && frame <= 33 &&
                !getenv("SM64DS_SELFTEST_DASHJUMP") &&
                !getenv("SM64DS_SELFTEST_PUNCH") &&
                !getenv("SM64DS_SELFTEST_IDLE"))
                btn |= 2;
            if (selftest && getenv("SM64DS_SELFTEST_DASH") && frame >= 20)
                btn |= 0x800;
            /* full-speed sprint jump: dash from f20, jump at f60 */
            if (selftest && getenv("SM64DS_SELFTEST_DASHJUMP")) {
                if (frame >= 20) btn |= 0x800;
                if (frame >= 60 && frame <= 63) btn |= 2;
            }
            /* punch probe: A-button edge at f40 */
            if (selftest && getenv("SM64DS_SELFTEST_PUNCH") &&
                frame >= 40 && frame <= 42)
                btn |= 1;
            /* SWIM probe: a B-button STROKE every 24 frames. Swimming is the
               one locomotion in the game the stick alone cannot drive --
               St_Swim_Main moves him on the stroke, not on the tilt -- so a
               selftest that only writes the stick floats in place forever.
               This is what makes "walk in, swim across, climb out" runnable
               without a person on the pad. */
            if (selftest && getenv("SM64DS_SELFTEST_SWIM") &&
                (frame % 24) < 3)
                btn |= 2;
            /* camera orbit through the game's own reader: func_02009e70
               tests data_0209f49c & 0x4300 -- L (0x200) rotates left, R
               (0x100) rotates right, 0x4000 is the snap-behind the input
               layer synthesizes. R doubles as crouch on the DS too, so E
               crouching as it orbits is the hardware's behaviour, not a
               harness artefact. */
            if (real_camera) {
                if (W.GetAsyncKeyState_('Q') < 0) btn |= 0x200;
                if (W.GetAsyncKeyState_('E') < 0) btn |= 0x100;
                if (W.GetAsyncKeyState_('C') < 0) btn |= 0x4000;
                if (pad_live) {
                    if (pad.rx < -10000) btn |= 0x200;
                    if (pad.rx > 10000) btn |= 0x100;
                }
                /* orbit probe: hold the rotate-right bit from frame 20 --
                   the camera's own heading and the angle it publishes must
                   both move, and W must keep walking away from the lens */
                if (selftest && getenv("SM64DS_SELFTEST_ORBIT") && frame >= 20)
                    btn |= 0x100;
            }
            *(unsigned short *)(data_0209f49c + 0) = btn;
            *(unsigned short *)(data_0209f49e + 0) =
                (unsigned short)(btn & (unsigned short)~btn_was);
            btn_was = btn;
        }

        /* the real ground tracking rewrites the path binding (c+0x670)
           from KCL surface attributes every contact frame; keep it at
           0xff (none) until a level boot seats the real path table
           (data_020a0d84 is null on host, the walk faults) */
        if (!real_boot)
            *(unsigned int *)(c + 0x670) = 0xff;

        if (selftest && frame == 1 && getenv("SM64DS_DUMP_CLSN")) {
            extern void *data_020a0c80[];
            fprintf(stderr, "[dump] slots:");
            for (int i = 0; i < 8; ++i)
                fprintf(stderr, " %p", data_020a0c80[i]);
            fprintf(stderr, "\n[dump] cylinder c+0x2d4 (the CylinderClsn "
                    "Behavior hands UpdatePos):\n");
            for (int off = 0; off < 0x40; off += 16) {
                fprintf(stderr, "  +%03x:", 0x2d4 + off);
                for (int k = 0; k < 4; ++k)
                    fprintf(stderr, " %08x",
                            *(unsigned *)(c + 0x2d4 + off + 4 * k));
                fprintf(stderr, "\n");
            }
            fprintf(stderr, "[dump] speed c+0xa4: %d %d %d\n",
                    *(int *)(c + 0xa4), *(int *)(c + 0xa8), *(int *)(c + 0xac));
            fprintf(stderr, "\n[dump] wmc c+0x380:\n");
            for (int off = 0; off < 0xa0; off += 16) {
                fprintf(stderr, "  +%03x:", 0x380 + off);
                for (int k = 0; k < 4; ++k)
                    fprintf(stderr, " %08x",
                            *(unsigned *)(c + 0x380 + off + 4 * k));
                fprintf(stderr, "\n");
            }
        }

        static int prev_pos[3], prev_live;
        if (!prev_live) {
            prev_live = 1;
            prev_pos[0] = *(int *)(c + 0x5c);
            prev_pos[1] = *(int *)(c + 0x60);
            prev_pos[2] = *(int *)(c + 0x64);
        }

        /* until the first ChangeState seats the current-state pointer,
           tick the wait state directly (the smoke's exact flow); Behavior
           owns the frame once the state machine is live */
        if (selftest && frame == 0)
            fprintf(stderr, "[w] tick st=%p\n", *(void **)(c + 0x370));
        if (selftest && frame < 2 && getenv("SM64DS_TRACE_SURF"))
            fprintf(stderr, "[pre%03d] pos=(%.1f,%.1f,%.1f) speed=(%d,%d,%d) "
                    "horz=%d ang=%04x step=%u/%u timer=%u\n", frame,
                    *(int *)(c + 0x5c) / 4096.0f, *(int *)(c + 0x60) / 4096.0f,
                    *(int *)(c + 0x64) / 4096.0f, *(int *)(c + 0xa4),
                    *(int *)(c + 0xa8), *(int *)(c + 0xac), *(int *)(c + 0x98),
                    (unsigned short)*(short *)(c + 0x8e),
                    *(unsigned char *)(c + 0x6e3), *(unsigned char *)(c + 0x6e5),
                    *(unsigned short *)(c + 0x6a6));
        /* THE GAME'S OWN PER-FRAME TICK (gate 15). The ROM's processing list
           never calls Behavior bare: it calls func_02043288, ActorBase::Process
           over vtable slots 7/6/8, and slot 7 -- Actor::BeforeBehavior -- is
           what copies pos into PREV POS. Prev pos is the start of every line
           the continuous mesh-collision update casts, so with it stale at the
           constructor's zero the first frame swept a segment from the world
           origin to the gate and dropped Mario on the first floor it crossed.
           The legacy staging keeps the bare call: its hand-built spawn context
           has no area shown, and BeforeBehavior would cull the actor. */
        /* Under the real boot that tick is no longer the Player's alone: the
           level spawned other actors and they are on the same lists he is.
           port_actor_tick runs func_02044120's first three phases -- cleanup,
           the init pass for anything spawned since last frame, then behaviour
           in priority order -- which reaches him through the same
           func_02043288 the harness used to call by hand. */
        if (boot_spawns) {
            /* Nothing to undo before the tick any more. The view matrix is
               the one Camera::Render published, in the ROM's own scene units,
               and Actor::BeforeBehavior reads exactly those three words to
               place every actor for the Clipper. */
            port_actor_tick();
        } else if (*(void **)(c + 0x370)) {
            hal_player_behavior(player);
        } else {
            hal_player_st_wait_main(player);
        }
        /* the real boot seats the path table, so the tracking's own binding
           stands -- except where the port's unfilled floor record invents
           one the level cannot produce (hal/level_boot.cpp) */
        if (real_boot)
            port_stage_path_guard(player);
        if (selftest && frame == 0)
            fprintf(stderr, "[w] ticked\n");
        /* the camera's own frame: Behavior runs the state machine and
           hands its heading to func_0203dafc (which writes the LOCAL comms
           record), then func_0203e0ac -- the single-player echo of
           func_0203df40 -- copies that record into the four per-player
           records GetAngleToCamera reads. Without the second call the
           published angle never moves and Mario walks relative to a stale
           heading. */
        if (real_camera) {
            hal_camera_behavior(cam);
            func_0203e0ac();
        }
        /* no speed clamp: the accel tables get real input-mode data now
           that Stage::CheckInput fills the record (the old runaway came
           from fake mode bytes) */

        if (selftest) {
            void *st = *(void **)(c + 0x370);
            unsigned bid =
                _ZNK6Player14GetBodyModelIDEjb(c, *(int *)(c + 8) & 0xff, 0);
            char *ma = ((char **)(c + 0xdc))[bid];
            unsigned bh = 2166136261u;
            if (ma) {
                const unsigned char *bb =
                    (const unsigned char *)*(char **)(ma + 0x14);
                for (int k = 0; k < 0x300; ++k)
                    bh = (bh ^ bb[k]) * 16777619u;
            }
            fprintf(stderr,
                    "[f%03d] pos=(%.1f,%.1f,%.1f) spd=%d st=%08x mag=%d "
                    "body=%u anim(len=%u fl=%u cur=%.1f) bones=%08x path=%x\n",
                    frame, *(int *)(c + 0x5c) / 4096.0f,
                    *(int *)(c + 0x60) / 4096.0f,
                    *(int *)(c + 0x64) / 4096.0f, *(int *)(c + 0x98),
                    st ? *(unsigned *)st : 0u, *(short *)(data_0209f4a0 + 0),
                    bid, ma ? (*(unsigned *)(ma + 0x54)) & 0x3FFFFFFF : 0,
                    ma ? (*(unsigned *)(ma + 0x54)) >> 30 : 0,
                    ma ? *(int *)(ma + 0x58) / 4096.0f : 0.0f, bh,
                    *(unsigned *)(c + 0x670));
            /* SM64DS_TRACE_SURF=1: the surface record the ground tracking
               pulled out of the CLPS entry under his feet, plus the last
               triangle the octree walk accepted. This is how the unfilled
               WithMeshClsn floor record was caught -- the walk's last
               triangle stays put while the record changes underneath. */
            if (getenv("SM64DS_TRACE_SURF")) {
                /* the floor ClsnResult itself: WithMeshClsn + 0x20 (the
                   SphereClsn sub-object) + 0x74 (func_02037938). Its first
                   two SurfaceInfo words ARE the CLPS entry, so a record the
                   walk really filled reads back as one of the level's 22
                   entries -- 000?0fc? / 000000ff for castle grounds. */
                const int *fr = (const int *)(c + 0x380 + 0x20 + 0x74);
                fprintf(stderr, "       surf path=%x t=%d %d %d %d %d "
                        "lastTri=%d attr=%x clps=%08x/%08x tri=%u slot=%u\n",
                        *(unsigned *)(c + 0x670), *(int *)(c + 0x66c),
                        *(int *)(c + 0x660), *(int *)(c + 0x65c),
                        *(int *)(c + 0x664), *(int *)(c + 0x658),
                        g_walk_dbg[13], g_walk_dbg[14], fr[1], fr[2],
                        *(const unsigned short *)((const char *)fr + 0x18),
                        *(const unsigned short *)((const char *)fr + 0x1a));
            }
            /* SM64DS_TRACE_WATER=1: the water chain, end to end, in the order
               the ROM runs it. func_ov002_020c14b8 casts a RaycastGround from
               300 units up with StartDetectingWater + StopDetectingOrdinary
               and, on a surface whose CLPS carries flag 0x20, writes the hit
               height into Player+0x64c and into the global data_0209f32c.
               func_ov002_020c0fb4 then hands +0x64c to func_ov002_020c0d90,
               which is the walk->swim decision. So:
                 water=-2147483648 (0x80000000)  the probe found no water
                 water=<y> with swim never entered  the decision refused
               and 0x706/0x707 are mIsUnderwater / mIsInShallowWater. */
            if (getenv("SM64DS_TRACE_WATER"))
                fprintf(stderr, "       water probe=%d (%.1f) global=%d (%.1f) "
                        "y=%.1f under=%u shallow=%u airborne=%u vspd=%d\n",
                        *(int *)(c + 0x64c), *(int *)(c + 0x64c) / 4096.0f,
                        data_0209f32c[0], data_0209f32c[0] / 4096.0f,
                        *(int *)(c + 0x60) / 4096.0f,
                        *(unsigned char *)(c + 0x706),
                        *(unsigned char *)(c + 0x707),
                        *(unsigned char *)(c + 0x6de), *(int *)(c + 0xa8));
        }
        if (selftest) {
            /* actor-list-head stomp tracker (the f015 0x1000 write) */
            static int prev_head[4], head_live;
            if (head_live &&
                memcmp(prev_head, data_0209b468, sizeof prev_head))
                fprintf(stderr, "[head] f%03d %08x %08x %08x %08x\n",
                        frame, data_0209b468[0], data_0209b468[1],
                        data_0209b468[2], data_0209b468[3]);
            memcpy(prev_head, data_0209b468, sizeof prev_head);
            head_live = 1;
        }

        /* RETIRED harness ground snap: a real KCL ray under Mario each
           frame. OFF unless SM64DS_FAKE_SNAP=1 brings the whole pre-sphere
           configuration back. It existed because the game's own tracking
           could not ground him with the sphere pass stubbed; the pass is
           hosted now and holds him exactly, so this is an A/B switch and a
           way to plant him for shots, not a mode the port runs in. */
        {
            int gy;
            int mx = *(int *)(c + 0x5c), my = *(int *)(c + 0x60),
                mz = *(int *)(c + 0x64);
            /* ray starts just above STEP height: starting a body-height
               up let the walk grab canopies/domes overhead and teleport
               him upward (the "camera is fucked" y-pops) */
            if (ground_snap &&
                hal_ground_ray(g_mc, mx, my + (100 << 12), mz, 5220 << 12,
                               &gy)) {
                /* never re-ground a rising jump: the snap + SetGroundFlag
                   on the first ascent frame would land him instantly */
                if (*(int *)(c + 0xa8) <= 0 && my <= gy + 0x20000) {
                    *(int *)(c + 0x60) = gy;
                    if (*(int *)(c + 0xa8) < 0)
                        *(int *)(c + 0xa8) = 0;   /* mVertSpeed */
                    _ZN12WithMeshClsn13SetGroundFlagEv(c + 0x380);
                    /* landing signal: St_Jump/Fall exit on this byte;
                       the real WithMeshClsn tracking will own it once
                       the continuous update runs on host */
                    *(unsigned char *)(c + 0x6de) = 0;
                }
            }
            /* RETIRED harness wall clamp, off with the snap and for the same
               reason: the game's wall pass IS its ground pass, one
               MeshCollider::DetectClsn(SphereClsn &) returning floor, wall
               and ceiling as a three-bit mask. Hosted, it stops him against
               the castle's outer wall and lets him SLIDE ALONG it, where
               this clamp only ever stopped him dead 120 units short. */
            if (wall_stop) {
                int nx = *(int *)(c + 0x5c), nz = *(int *)(c + 0x64);
                int ny = *(int *)(c + 0x60);
                int wy = ny + (120 << 12);          /* chest height */
                int a[3] = {prev_pos[0], wy, prev_pos[2]};
                int b[3] = {nx, wy, nz};
                int clip[3];
                long long ddx = (long long)nx - prev_pos[0];
                long long ddz = (long long)nz - prev_pos[2];
                if ((ddx | ddz) && hal_line_ray(g_mc, a, b, clip)) {
                    /* stop 120 units short of the wall along the motion */
                    long long len2 = ddx * ddx + ddz * ddz;
                    double len = len2 > 0 ? sqrt((double)len2) : 1.0;
                    double ux = ddx / len, uz = ddz / len;
                    *(int *)(c + 0x5c) =
                        clip[0] - (int)(ux * (120 << 12));
                    *(int *)(c + 0x64) =
                        clip[2] - (int)(uz * (120 << 12));
                    *(int *)(c + 0x98) = 0;         /* mHorzSpeed */
                }
            }
            /* the harness's own last-frame position, which only the wall
               stop reads -- the game's is Actor+0x68 and BeforeBehavior
               owns it now (gate 15) */
            prev_pos[0] = *(int *)(c + 0x5c);
            prev_pos[1] = *(int *)(c + 0x60);
            prev_pos[2] = *(int *)(c + 0x64);

            /* fell out of the world (walked or jumped past the KCL):
               back to the spawn point instead of an endless dive */
            if (my < (-19200 << 12)) {
                *(int *)(c + 0x5c) = spawn_x << 12;
                *(int *)(c + 0x60) = spawn_y << 12;
                *(int *)(c + 0x64) = spawn_z << 12;
                *(int *)(c + 0x98) = 0;   /* mHorzSpeed */
                *(int *)(c + 0xa8) = 0;   /* mVertSpeed */
            }
        }

        /* render: camera behind and above Mario, looking at him */
        ntr::gx_reset();
        /* the real Camera writes CLEAR_COLOR itself, out of its own
           0x10c..0x10f bytes -- which hold exactly this value */
        if (!real_camera)
            NTR_MMIO(uint32_t, 0x04000580) =
                0u | (0u << 8) | (255u << 16) | (191u << 24);
        ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
        ntr::gx_enable_lights(0x1);
        float px = *(int *)(c + 0x5c) / 4096.0f;
        float py = *(int *)(c + 0x60) / 4096.0f;
        float pz = *(int *)(c + 0x64) / 4096.0f;
        /* SCENE units everywhere below: pos >> 3, the ROM's own conversion. */
        const float sx = px / 8.0f, sy = py / 8.0f, sz = pz / 8.0f;
        if (selftest && frame == 0) {
            /* scene-space bounds probe: identity matrices, read the raw
               projected coords (with identity proj they ARE scene coords).
               Runs under either camera -- it resets the geometry state at both
               ends and the real camera republishes its matrices right after,
               so the check is available in the default configuration. */
            ntr::gx_reset();
            hal_render_player_world(player);
            size_t n = 0;
            const ntr::GxTriangle *ta = ntr::gx_polygons(n);
            float mn[3] = {1e30f, 1e30f, 1e30f}, mx[3] = {-1e30f, -1e30f, -1e30f};
            for (size_t i = 0; i < n; ++i)
                for (int v = 0; v < 3; ++v) {
                    float xyz[3] = {ta[i].v[v].x, ta[i].v[v].y, ta[i].v[v].z};
                    for (int k = 0; k < 3; ++k) {
                        if (xyz[k] < mn[k]) mn[k] = xyz[k];
                        if (xyz[k] > mx[k]) mx[k] = xyz[k];
                    }
                }
            printf("probe: %zu tris, x[%.1f..%.1f] y[%.1f..%.1f] z[%.1f..%.1f]\n",
                   n, mn[0], mx[0], mn[1], mx[1], mn[2], mx[2]);
            {
                /* Mario's own rendered height, the one number the migration
                   has to hold: identity projection, so the raw y span IS his
                   size in scene units. x8 back to world for the historic
                   reading (~145). */
                const float hh2 = ntr::SCREEN_H * 0.5f;
                const float ylo = 1.0f - mx[1] / hh2, yhi = 1.0f - mn[1] / hh2;
                printf("probe: mario scene y[%.2f..%.2f] height %.2f scene "
                       "(%.1f world)\n", ylo, yhi, yhi - ylo,
                       (yhi - ylo) * 8.0f);
            }
            printf("probe: mario fx pos (%d, %d, %d) -> world (%.1f, %.1f, %.1f)"
                   " scene (%.1f, %.1f, %.1f)\n",
                   *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64),
                   px, py, pz, sx, sy, sz);
            printf("probe: player scale vec c+0x80 = (%d, %d, %d) fx\n",
                   *(int *)(c + 0x80), *(int *)(c + 0x84), *(int *)(c + 0x88));
            ntr::gx_reset();
            hal_render_model(level_storage, level_shift);
            n = 0;
            ta = ntr::gx_polygons(n);
            for (int k = 0; k < 3; ++k) { mn[k] = 1e30f; mx[k] = -1e30f; }
            for (size_t i = 0; i < n; ++i)
                for (int v = 0; v < 3; ++v) {
                    float xyz[3] = {ta[i].v[v].x, ta[i].v[v].y, ta[i].v[v].z};
                    for (int k = 0; k < 3; ++k) {
                        if (xyz[k] < mn[k]) mn[k] = xyz[k];
                        if (xyz[k] > mx[k]) mx[k] = xyz[k];
                    }
                }
            printf("probe: level %zu tris, x[%.0f..%.0f] y[%.0f..%.0f] z[%.0f..%.0f]\n",
                   n, mn[0], mx[0], mn[1], mx[1], mn[2], mx[2]);
            /* scene units back out of the identity-projection screen coords:
               sx = xs/(W/2)-1, sy = 1-ys/(H/2), sz = zs*2-1. VISUAL FLOOR
               CHECK: highest mesh vertex in Mario's column at/below his head
               height vs the KCL ground there -- a nonzero delta is the
               feet-sinking gap. The KCL is world fx, so it comes across the
               same >>3 the render does. */
            {
                const float hw = ntr::SCREEN_W * 0.5f,
                            hh = ntr::SCREEN_H * 0.5f;
                float vis = -1e30f, wxmin = 1e30f, wxmax = -1e30f,
                      wymin = 1e30f, wymax = -1e30f;
                for (size_t i = 0; i < n; ++i) {
                    float X[3], Y[3], Z[3];
                    for (int v = 0; v < 3; ++v) {
                        X[v] = ta[i].v[v].x / hw - 1.0f;
                        Y[v] = 1.0f - ta[i].v[v].y / hh;
                        Z[v] = ta[i].v[v].z * 2.0f - 1.0f;
                        if (X[v] < wxmin) wxmin = X[v];
                        if (X[v] > wxmax) wxmax = X[v];
                        if (Y[v] < wymin) wymin = Y[v];
                        if (Y[v] > wymax) wymax = Y[v];
                    }
                    /* interpolate the tri surface at (sx, sz) */
                    {
                        const float d = (Z[1] - Z[2]) * (X[0] - X[2]) +
                                        (X[2] - X[1]) * (Z[0] - Z[2]);
                        if (d > 1e-6f || d < -1e-6f) {
                            const float a =
                                ((Z[1] - Z[2]) * (sx - X[2]) +
                                 (X[2] - X[1]) * (sz - Z[2])) / d;
                            const float b =
                                ((Z[2] - Z[0]) * (sx - X[2]) +
                                 (X[0] - X[2]) * (sz - Z[2])) / d;
                            const float c2 = 1.0f - a - b;
                            if (a > -0.01f && b > -0.01f && c2 > -0.01f) {
                                const float wy =
                                    a * Y[0] + b * Y[1] + c2 * Y[2];
                                if (wy < sy + 150.0f / 8 && wy > vis) vis = wy;
                            }
                        }
                    }
                }
                {
                    int kgy = 0;
                    int kh = hal_ground_ray(g_mc, *(int *)(c + 0x5c),
                                            (int)((py + 1200) * 4096),
                                            *(int *)(c + 0x64),
                                            6000 << 12, &kgy);
                    const float kscene = kgy / 4096.0f / 8.0f;
                    printf("probe: model SCENE x[%.1f..%.1f] y[%.1f..%.1f] "
                           "(world x[%.1f..%.1f] y[%.1f..%.1f])\n",
                           wxmin, wxmax, wymin, wymax, wxmin * 8, wxmax * 8,
                           wymin * 8, wymax * 8);
                    printf("probe: FLOOR at scene col (%.1f,%.1f): visual=%.2f "
                           "kcl=%s%.2f delta=%.2f scene (%.1f world)\n",
                           sx, sz, vis, kh ? "" : "MISS ", kscene,
                           kh ? vis - kscene : 0.0f,
                           kh ? (vis - kscene) * 8.0f : 0.0f);
                }
            }
            ntr::gx_reset();
            NTR_MMIO(uint32_t, 0x04000580) =
                0u | (0u << 8) | (255u << 16) | (191u << 24);
            ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
            ntr::gx_enable_lights(0x1);
        }
        float dbg_eye[3] = {0, 0, 0}, dbg_at[3] = {0, 0, 0};
        if (real_camera) {
            /* THE CAMERA'S OWN FRAME. Render builds the projection from
               the mode preset (PerspectiveW_ -> MTX_LOAD_4x4) and the view
               matrix through LookAt_, then View::Render -> CopyToViewMat
               parks it in data_0209b3ec and its inverse in data_0209b41c.
               Model::Render composes every model matrix with data_0209b3ec
               in software, so THAT is where the camera reaches the raster,
               not the GX position stack. */
            hal_camera_render(cam);
            /* THE ACTOR RENDER BUCKET GOES HERE, and the reason is the shim
               immediately below. Processing list 5 is the game's own render
               pass -- func_0204322c over slots 9/10/11, in render-priority
               order -- and everything on it is ROM code working in SCENE
               units: Tree::Render clips its cylinders through the Clipper
               with data_0209b3ec as it stands and writes scene-unit
               translations into its Models. The shim converts that same view
               matrix for the port's own world-unit models. So the bucket runs
               BEFORE the conversion and the harness's two draws after it, and
               each side gets the matrix it was written against. The raster is
               z-buffered, so drawing the actors ahead of the level model
               costs nothing.
               SM64DS_NO_ACTORS=1 takes the bucket out for the A/B. */
            if (boot_spawns && !getenv("SM64DS_NO_ACTORS")) {
                size_t before = 0, after = 0;
                if (selftest) ntr::gx_polygons(before);
                port_actor_render();
                if (selftest) {
                    const ntr::GxTriangle *at = ntr::gx_polygons(after);
                    if (frame == 0 || getenv("SM64DS_TRACE_ACTOR_TRIS")) {
                        float mnx = 1e30f, mxx = -1e30f, mny = 1e30f,
                              mxy = -1e30f, mnz = 1e30f, mxz = -1e30f;
                        for (size_t i = before; i < after; ++i)
                            for (int v = 0; v < 3; ++v) {
                                float X = at[i].v[v].x, Y = at[i].v[v].y,
                                      Z = at[i].v[v].z;
                                if (X < mnx) mnx = X;
                                if (X > mxx) mxx = X;
                                if (Y < mny) mny = Y;
                                if (Y > mxy) mxy = Y;
                                if (Z < mnz) mnz = Z;
                                if (Z > mxz) mxz = Z;
                            }
                        printf("[actors] render bucket: %zu triangles, screen "
                               "x[%.0f..%.0f] y[%.0f..%.0f] z[%.3f..%.3f]\n",
                               after - before, mnx, mxx, mny, mxy, mnz, mxz);
                        if (getenv("SM64DS_TRACE_ACTOR_TRIS"))
                            for (size_t i = before; i < after && i < before + 4;
                                 ++i)
                                printf("         tri (%.1f,%.1f,%.4f) "
                                       "(%.1f,%.1f,%.4f) (%.1f,%.1f,%.4f) "
                                       "tex %p %dx%d cull %u alpha %u\n",
                                       at[i].v[0].x, at[i].v[0].y, at[i].v[0].z,
                                       at[i].v[1].x, at[i].v[1].y, at[i].v[1].z,
                                       at[i].v[2].x, at[i].v[2].y, at[i].v[2].z,
                                       (const void *)at[i].tex, at[i].tw,
                                       at[i].th, at[i].cull, at[i].alpha);
                    }
                }
            }
            /* THE VIEW MATRIX IS USED AS THE ROM PRODUCED IT. Camera::Render
               feeds LookAt_ eye and lookAt as (v + 4) >> 3, so its translation
               row is in scene units and its rotation rows are plain unit
               vectors -- and every model matrix in the frame is now scene
               units too, so Model::Render's compose
               (out.t = model.t * view.R + view.t) has both terms in the same
               space. The R6 shim that scaled this row by 8 for the harness's
               world-unit models is gone, and with it the reason
               Actor::BeforeBehavior had to be handed the row back: it
               multiplies every actor's position through these same words for
               the Clipper, and an eight-times-too-long row read every actor
               with a cull distance as eight times too far away. */
            {
                /* FIELD-MAP CORRECTION, measured here: 0x8c is the camera
                   POSITION and 0x80 the point it looks at, not the other
                   way round. G3i::LookAt_ translates its matrix by the
                   `at` argument, and Camera::Render passes 0x8c there;
                   Camera::Behavior's own Vec3_HorzAngle(0x80, 0x8c) then
                   reads "from the focus toward the camera", which is what
                   the name GetAngleToCamera promises. */
                const int *ce = (const int *)((char *)cam + 0x8c);
                const int *cl = (const int *)((char *)cam + 0x80);
                for (int k = 0; k < 3; ++k) {
                    dbg_eye[k] = ce[k] / 4096.0f;
                    dbg_at[k] = cl[k] / 4096.0f;
                }
                if (selftest) {
                    float ddx = dbg_eye[0] - dbg_at[0];
                    float ddy = dbg_eye[1] - dbg_at[1];
                    float ddz = dbg_eye[2] - dbg_at[2];
                    fprintf(stderr,
                            "[cam] f%03d eye(%.1f,%.1f,%.1f) "
                            "at(%.1f,%.1f,%.1f) head=%04x pitch=%04x "
                            "fov=%d angle=%04x dist=%.0f\n",
                            frame, dbg_eye[0], dbg_eye[1], dbg_eye[2],
                            dbg_at[0], dbg_at[1], dbg_at[2],
                            (unsigned short)*(short *)((char *)cam + 0x17c),
                            (unsigned short)*(short *)((char *)cam + 0x17e),
                            *(short *)((char *)cam + 0x17a),
                            (unsigned short)*(short *)((char *)data_020a1164),
                            sqrtf(ddx * ddx + ddy * ddy + ddz * ddz));
                }
            }
        } else {
        /* Follow camera at near eye level (Mario is ~14 units tall). The
           old version LIFTED the eye onto whatever hill sat behind him,
           which looked down at a grazing angle -- terrain read flat and the
           view stretched. Now: shoulder-height offset, occlusion resolved
           by pulling IN along the view ray, and smoothing so the eye never
           snaps. */
        /* SM64DS-like framing: well back, and LOW -- a nearly level view.
           The old +340 fixed eye height sat above the moat rim and every
           low area's walls, looking steeply down, which foreshortened
           the terrain ("squashed, worse the lower you go"). cam_pitch
           tilts the rig (R/F keys), default ~7 degrees. */
        float cd = 3000.0f * cosf(cam_pitch), ch = 3000.0f * sinf(cam_pitch);
        float want_eye[3] = {px - cd * sinf(cam_yaw), py + 200.0f + ch,
                             pz - cd * cosf(cam_yaw)};
        float at[3] = {px, py + 200.0f, pz};
        if (getenv("SM64DS_ORBIT")) {
            /* debug: whole-stage orbit shot to judge proportions */
            want_eye[0] = 10240.0f; want_eye[1] = 7680.0f;
            want_eye[2] = -10240.0f;
            at[0] = 0.0f; at[1] = 0.0f; at[2] = 0.0f;
        }
        {
            /* occlusion: pull the eye in front of anything between it and
               Mario (cast from the look-at toward the eye) */
            int a[3] = {(int)(at[0] * 4096), (int)(at[1] * 4096),
                        (int)(at[2] * 4096)};
            int b[3] = {(int)(want_eye[0] * 4096), (int)(want_eye[1] * 4096),
                        (int)(want_eye[2] * 4096)};
            int clip[3];
            if (hal_line_ray(g_mc, a, b, clip)) {
                for (int k = 0; k < 3; ++k)
                    want_eye[k] = clip[k] / 4096.0f * 0.9f + at[k] * 0.1f;
                /* never collapse onto Mario: in enclosed areas (the moat,
                   doorways) the pull glued the lens to the nearest wall
                   and the framing went face-cam; keep a minimum standoff
                   even if it means the near wall clips */
                {
                    float dx = want_eye[0] - at[0], dy = want_eye[1] - at[1],
                          dz = want_eye[2] - at[2];
                    float d = sqrtf(dx * dx + dy * dy + dz * dz);
                    const float MIN_D = 1280.0f;
                    if (d > 1.0f && d < MIN_D) {
                        float g2 = MIN_D / d;
                        want_eye[0] = at[0] + dx * g2;
                        want_eye[1] = at[1] + dy * g2;
                        want_eye[2] = at[2] + dz * g2;
                    }
                }
            }
        }
        static float eye[3];
        static int eye_live;
        if (!eye_live) {
            eye_live = 1;
            for (int k = 0; k < 3; ++k) eye[k] = want_eye[k];
        }
        for (int k = 0; k < 3; ++k)
            eye[k] += (want_eye[k] - eye[k]) * 0.2f;
        push_camera(eye, at);
        for (int k = 0; k < 3; ++k) { dbg_eye[k] = eye[k]; dbg_at[k] = at[k]; }
        }   /* else: !real_camera */
        if (selftest && frame == 0)
            fprintf(stderr, "[w] render\n");
        if (selftest && frame == 0 && !real_camera) {
            /* GX isolation: hand-feed one triangle at Mario's position
               through raw MMIO -- no game code. Centered = GX + camera
               fine; offset = my matrix push is wrong. */
            {
                float gp[16];
                ntr::gx_debug_proj(gp);
                fprintf(stderr, "[w] g.proj rows:\n");
                for (int r = 0; r < 4; ++r)
                    fprintf(stderr, "  %8.3f %8.3f %8.3f %8.3f\n",
                            gp[r*4], gp[r*4+1], gp[r*4+2], gp[r*4+3]);
            }
            NTR_MMIO(uint32_t, 0x04000440) = 1;   /* MTX_MODE position */
            uint32_t tr[16] = {4096, 0, 0, 0, 0, 4096, 0, 0,
                               0, 0, 4096, 0,
                               (uint32_t)(int)(sx * 4096),
                               (uint32_t)(int)(sy * 4096),
                               (uint32_t)(int)(sz * 4096), 4096};
            for (int i = 0; i < 16; ++i)
                NTR_MMIO(uint32_t, 0x04000458) = tr[i];
            NTR_MMIO(uint32_t, 0x04000500) = 0;   /* BEGIN_VTXS tris */
            /* small triangle around the origin (4.12: 0x1000 = 1.0) */
            NTR_MMIO(uint32_t, 0x0400048C) = 0x0000F000u;      /* (-1, 0) */
            NTR_MMIO(uint32_t, 0x0400048C) = 0x00000000u;      /* z 0 */
            NTR_MMIO(uint32_t, 0x0400048C) = 0x00001000u;      /* (+1, 0) */
            NTR_MMIO(uint32_t, 0x0400048C) = 0x00000000u;
            NTR_MMIO(uint32_t, 0x0400048C) = 0x10000000u;      /* (0, +1) */
            NTR_MMIO(uint32_t, 0x0400048C) = 0x00000000u;
            NTR_MMIO(uint32_t, 0x04000504) = 0;   /* END_VTXS */
            size_t hn = 0;
            const ntr::GxTriangle *ht = ntr::gx_polygons(hn);
            if (hn)
                fprintf(stderr, "[w] handfed tri screen (%.0f,%.0f) (%.0f,%.0f)"
                        " (%.0f,%.0f)\n",
                        ht[hn-1].v[0].x, ht[hn-1].v[0].y, ht[hn-1].v[1].x,
                        ht[hn-1].v[1].y, ht[hn-1].v[2].x, ht[hn-1].v[2].y);
            else
                fprintf(stderr, "[w] handfed tri CLIPPED/none\n");
            ntr::gx_reset();
            NTR_MMIO(uint32_t, 0x04000580) =
                0u | (0u << 8) | (255u << 16) | (191u << 24);
            ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
            ntr::gx_enable_lights(0x1);
            push_camera(dbg_eye, dbg_at);
        }
        if (!getenv("SM64DS_NO_LEVEL"))
            hal_render_model(level_storage, level_shift);
        /* phase 1, which is where func_02044120 ends: the scene tree's own
           housekeeping -- priority re-sorts, parent flag propagation, and the
           deferred list insertions for anything that spawned mid-phase. */
        if (boot_spawns)
            port_actor_scene_pass();
        size_t tris_before = 0;
        if (selftest) ntr::gx_polygons(tris_before);
        hal_render_player_world(player);
        if (selftest) {
            size_t tn = 0;
            const ntr::GxTriangle *ta2 = ntr::gx_polygons(tn);
            float mnx = 1e30f, mxx = -1e30f, mny = 1e30f, mxy = -1e30f;
            for (size_t i = tris_before; i < tn; ++i)
                for (int v = 0; v < 3; ++v) {
                    if (ta2[i].v[v].x < mnx) mnx = ta2[i].v[v].x;
                    if (ta2[i].v[v].x > mxx) mxx = ta2[i].v[v].x;
                    if (ta2[i].v[v].y < mny) mny = ta2[i].v[v].y;
                    if (ta2[i].v[v].y > mxy) mxy = ta2[i].v[v].y;
                }
            fprintf(stderr,
                    "[w] mario screen box x[%.0f..%.0f] y[%.0f..%.0f] "
                    "(center %d,%d) eye(%.1f,%.1f,%.1f) at(%.1f,%.1f,%.1f)\n",
                    mnx, mxx, mny, mxy, ntr::SCREEN_W / 2, ntr::SCREEN_H / 2,
                    dbg_eye[0], dbg_eye[1], dbg_eye[2], dbg_at[0], dbg_at[1],
                    dbg_at[2]);
        }
        if (selftest && frame == 0)
            fprintf(stderr, "[w] rendered\n");

        for (int y = 0; y < ntr::SCREEN_H; ++y)
            for (int x = 0; x < ntr::SCREEN_W; ++x)
                fb.px[y][x] = 0xFF101820u;
        ntr::gx_render(fb);
        W.StretchDIBits_(hdc, 0, 0, ntr::SCREEN_W * ZOOM, ntr::SCREEN_H * ZOOM,
                      0, 0, ntr::SCREEN_W, ntr::SCREEN_H, fb.px, &bi,
                      DIB_RGB_COLORS, SRCCOPY);
        if (selftest && (frame % 10) == 0)
            printf("[y] frame %d y=%d units %.1f\n", frame,
                   *(int *)(c + 0x60), *(int *)(c + 0x60) / 4096.0f);
        /* SM64DS_DUMP_FROM/TO: per-frame BMPs across a window, for
           watching an animation play (or fail to) */
        {
            static int dump_from = -1, dump_to = -1, dump_env;
            if (!dump_env) {
                dump_env = 1;
                const char *df = getenv("SM64DS_DUMP_FROM");
                const char *dt = getenv("SM64DS_DUMP_TO");
                if (df) dump_from = atoi(df);
                if (dt) dump_to = atoi(dt);
            }
            if (selftest && dump_from >= 0 && frame >= dump_from &&
                frame <= dump_to) {
                char nm[64];
                snprintf(nm, sizeof nm, "walk_frame_%03d.bmp", frame);
                ntr::ppu_write_bmp(nm, fb);
            }
        }
        if (selftest && ++frame >= selftest) {
            ntr::ppu_write_bmp("walk_window_selftest.bmp", fb);
            printf("selftest: %d frames, pos=(%d, %d, %d)\n", frame,
                   *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64));
            return 0;
        }
        /* pace to 30: SM64DS game logic runs at 30fps (the DS panel
           scans 60 but gameplay ticks every other vblank). Ticking the
           game's per-frame constants at 60Hz doubled every speed --
           the "jump too fast, weird gravity" report. Sleep only the
           remainder of the 33.3ms budget. */
        {
            static LARGE_INTEGER qpf, last;
            LARGE_INTEGER now;
            if (!qpf.QuadPart) QueryPerformanceFrequency(&qpf);
            QueryPerformanceCounter(&now);
            if (!selftest && last.QuadPart) {
                const double el =
                    (now.QuadPart - last.QuadPart) * 1000.0 / qpf.QuadPart;
                if (el < 33.3) Sleep((DWORD)(33.3 - el));
            }
            QueryPerformanceCounter(&last);
        }
    }
}
