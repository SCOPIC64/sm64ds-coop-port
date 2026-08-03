// Gate 12: the interactive window. Mario walks under keyboard control.
//
// Same staging as smoke_player (sinits, spawn context, castle-grounds KCL,
// InitResources, St_Wait), then a Win32 frame loop: keys write the pad
// block and desired heading, Player::Behavior ticks, the world renders
// through the ntr GX (camera folded into the projection matrix; models
// keep their world mat4x3), and the framebuffer blits 3x into the client.
//
//   WASD / arrows  walk        ESC  quit
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
void hal_render_player_world(void *p);
extern char data_0209f4a0[];
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
int hal_ground_ray(void *mc, int x, int y, int z, int reach, int *out_y);
int hal_line_ray(void *mc, const int *a, const int *b, int *out);
void _ZN12WithMeshClsn13SetGroundFlagEv(void *);
int func_02035354(void *, void *);
int func_020393b4(void *);
}

#ifdef NTR_HIRES
static const int ZOOM = 1;
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
   built in floats on world units (fx / 4096) and pushed as 4096-fixed */
static void push_camera(const float eye[3], const float at[3])
{
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

    const float fovy = 55.0f * 3.14159265f / 180.0f;
    const float aspect = (float)ntr::SCREEN_W / ntr::SCREEN_H;
    const float f = 1.0f / tanf(fovy * 0.5f);
    const float zn = 0.8f, zf = 6400.0f;
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
    /* world = KCL file x16; the castle roof surface is at 1229 */
    int spawn_x = 0, spawn_y = 1232, spawn_z = -800;
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

    data_02092144[0] = 8 << 8;
    data_020a4b54 = 0;
    static unsigned short spawn_info[4] = {0, 0, 100, 100};
    data_020a4bb8[0] = spawn_info;
    data_020a0eac_c = data_020a0ea0;

    void *player = _ZN9ActorBasenwEj(0x800);
    _ZN6PlayerC1Ev(player);
    if (hal_player_init_resources(player) != 1) return 3;

    /* the castle grounds floor, gate-8 recipe */
    char *c = (char *)player;
    {
        static struct { unsigned short id; unsigned char refs; void *p; } kp;
        _ZN13SharedFilePtr9ConstructEj(&kp, 1941);
        static char mc_storage[0x60];
        g_mc = mc_storage;
        _ZN12MeshColliderC1Ev(mc_storage);
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
           non-null owner fed it a junk ID and it faulted). Real collision
           stays opt-in until the full frame survives. */
        _ZN16MeshColliderBase6EnableEP5Actor(
            mc_storage, getenv("SM64DS_REAL_CLSN") ? (void *)0
                                                   : (void *)player);
        /* LEVEL SCALE: stage KCL files are world/16 (the engine's world
           is what the physics tables assume: Mario ~148 units tall, jump
           ~190, the ~32000-unit position clamp). The collider scale pair
           maps world<->file inside the walk; actor colliders keep 1.0. */
        *(int *)(mc_storage + 0x2c) = 0x10000;   /* file -> world, 16.0 */
        *(int *)(mc_storage + 0x38) = 0x100;     /* world -> file, 1/16 */
        /* the octree box is power-of-two PADDED (its center is way off the
           real stage); the geometry lives near the origin, so spawn there,
           a few units up -- the first frames drop him onto the lawn */
        {
            const char *sp = getenv("SM64DS_SPAWN");
            if (sp) sscanf(sp, "%d,%d,%d", &spawn_x, &spawn_y, &spawn_z);
            *(int *)(c + 0x5c) = spawn_x << 12;
            *(int *)(c + 0x60) = spawn_y << 12;
            *(int *)(c + 0x64) = spawn_z << 12;
        }
    }
    /* the level model: main_castle_all.bmd (handle 1943, same stage as the
       KCL); world-space verts scaled by the BMD header's scaleShift */
    static char level_storage[0x50];
    int level_shift = 0;
    {
        static struct { unsigned short id; unsigned char refs; void *p; } mp;
        _ZN13SharedFilePtr9ConstructEj(&mp, 1943);
        _ZN5ModelC1Ev(level_storage);
        void *bmd = _ZN5Model8LoadFileER13SharedFilePtr(&mp);
        if (bmd) {
            level_shift = *(int *)bmd;   /* BMD header word 0 */
            _ZN9ModelBase7SetFileEP8BMD_Fileii(level_storage, bmd, 0, -1);
            printf("level model loaded, scaleShift %d\n", level_shift);
        } else {
            fprintf(stderr, "level model load failed (handle 1943)\n");
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
        for (int gz = -100; gz <= 100; gz += 50) {
            char row[64] = {0};
            int ri = 0;
            for (int gx = -100; gx <= 100; gx += 50) {
                int gy = 0;
                int h = hal_ground_ray(g_mc, gx << 12, 100 << 12,
                                       gz << 12, 300 << 12, &gy);
                ri += snprintf(row + ri, sizeof row - ri, "%7.1f",
                               h ? gy / 4096.0f : -999.0f);
            }
            printf("floor z=%4d: %s\n", gz, row);
        }
    }

    data_020a0e40[0] = 0;
    /* InitResources parked the state machine in St_LevelEnter (a no-op
       until a real level boot); clear it so the wait ticks drive until the
       first stick input ChangeStates into walk -- the smoke's proven flow */
    *(void **)(c + 0x370) = 0;
    /* no path binding: the level spawn entry's path param, 0xff = none.
       The fake spawn context zero-fills it, and path 0 sends the real
       ground tracking into PathPtr walks over files no level boot has
       loaded (frame-1 fault under SM64DS_REAL_CLSN) */
    *(unsigned int *)(c + 0x670) = 0xff;
    if (getenv("PORT_WATCH_HEAD"))
        port_watch_words(data_0209b468, 4);
    hal_player_st_wait_init(player);

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
        if (selftest) dz = 1;
        if (W.GetAsyncKeyState_('W') < 0 || W.GetAsyncKeyState_(VK_UP) < 0) dz += 1;
        if (W.GetAsyncKeyState_('S') < 0 || W.GetAsyncKeyState_(VK_DOWN) < 0) dz -= 1;
        if (W.GetAsyncKeyState_('A') < 0 || W.GetAsyncKeyState_(VK_LEFT) < 0) dx -= 1;
        if (W.GetAsyncKeyState_('D') < 0 || W.GetAsyncKeyState_(VK_RIGHT) < 0) dx += 1;
        int orbiting = 0;
        if (W.GetAsyncKeyState_('Q') < 0) { cam_yaw -= 0.045f; orbiting = 1; }
        if (W.GetAsyncKeyState_('E') < 0) { cam_yaw += 0.045f; orbiting = 1; }
        if (dx || dz) {
            *(short *)(data_0209f4a0 + 0) = 0x1000;
            float head = cam_yaw + atan2f((float)dx, (float)dz);
            unsigned short ang =
                (unsigned short)(int)(head * (32768.0f / 3.14159265f));
            *(short *)(c + 0x69c) = (short)ang;   /* mDesiredAngleY */
            if (!orbiting) {
                /* ease in behind the walk direction, but ONLY when he
                   walks away-ish from the camera; chasing a heading that
                   points back at the lens spun the camera in circles
                   whenever he ran toward the screen */
                float d = head - cam_yaw;
                while (d > 3.14159265f) d -= 2 * 3.14159265f;
                while (d < -3.14159265f) d += 2 * 3.14159265f;
                if (d > -1.35f && d < 1.35f)
                    cam_yaw += d * 0.015f;
            }
        } else {
            *(short *)(data_0209f4a0 + 0) = 0;
        }

        /* Space -> jump: DS B button (bit 1). Bit 0 (A) is punch -- proven
           live: pressing it fires St_PunchKick through the checker
           (func_ov002_020d36d8). The B-pressed edge is the jump entry. */
        {
            static int space_was;
            int space = W.GetAsyncKeyState_(VK_SPACE) < 0;
            /* selftest: synthetic hop at frame 30 (walking start speed) */
            if (selftest && frame >= 30 && frame <= 33) space = 1;
            unsigned short held = *(unsigned short *)(data_0209f49c + 0);
            held = (unsigned short)((held & ~2u) | (space ? 2u : 0u));
            *(unsigned short *)(data_0209f49c + 0) = held;
            *(unsigned short *)(data_0209f49e + 0) =
                (unsigned short)((space && !space_was) ? 2u : 0u);
            space_was = space;
        }

        /* the real ground tracking rewrites the path binding (c+0x670)
           from KCL surface attributes every contact frame; keep it at
           0xff (none) until a level boot seats the real path table
           (data_020a0d84 is null on host, the walk faults) */
        *(unsigned int *)(c + 0x670) = 0xff;

        if (selftest && frame == 1 && getenv("SM64DS_REAL_CLSN")) {
            extern void *data_020a0c80[];
            fprintf(stderr, "[dump] slots:");
            for (int i = 0; i < 8; ++i)
                fprintf(stderr, " %p", data_020a0c80[i]);
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
        if (*(void **)(c + 0x370))
            hal_player_behavior(player);
        else
            hal_player_st_wait_main(player);
        if (selftest && frame == 0)
            fprintf(stderr, "[w] ticked\n");
        /* harness clamp: the walk accel runs uncapped under the fake
           input-mode data (fidelity note in the port memory); keep the
           demo controllable until the real input processor is hosted */
        if (*(int *)(c + 0x98) > 0x3000) *(int *)(c + 0x98) = 0x3000;

        if (selftest)
            fprintf(stderr, "[f%03d] y=%.2f vy=%d st=%p\n", frame,
                    *(int *)(c + 0x60) / 4096.0f, *(int *)(c + 0xa8),
                    *(void **)(c + 0x370));
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

        /* harness ground snap: a real KCL ray under Mario each frame.
           OFF under SM64DS_REAL_CLSN -- the game's own tracking grounds
           him there, and a SetGroundFlag without the rest of the surface
           record (collider slot, triangle) feeds UpdateExtraContinous a
           half-empty record that faults */
        {
            int gy;
            int mx = *(int *)(c + 0x5c), my = *(int *)(c + 0x60),
                mz = *(int *)(c + 0x64);
            if (!getenv("SM64DS_REAL_CLSN") &&
                hal_ground_ray(g_mc, mx, my + (320 << 12), mz, 1280 << 12,
                               &gy)) {
                /* never re-ground a rising jump: the snap + SetGroundFlag
                   on the first ascent frame would land him instantly */
                if (*(int *)(c + 0xa8) <= 0 && my <= gy + 0x8000) {
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
            /* wall stop: the sphere push-out (the game's wall pass) is
               still stubbed, so clamp motion against the KCL directly --
               a chest-height segment from last frame's position to this
               one catches any wall crossed, and a stopped Mario keeps
               his feet (y stays the snap's business) */
            {
                int nx = *(int *)(c + 0x5c), nz = *(int *)(c + 0x64);
                int ny = *(int *)(c + 0x60);
                int wy = ny + (55 << 12);           /* chest height */
                int a[3] = {prev_pos[0], wy, prev_pos[2]};
                int b[3] = {nx, wy, nz};
                int clip[3];
                long long ddx = (long long)nx - prev_pos[0];
                long long ddz = (long long)nz - prev_pos[2];
                if ((ddx | ddz) && hal_line_ray(g_mc, a, b, clip)) {
                    /* stop 30 units short of the wall along the motion */
                    long long len2 = ddx * ddx + ddz * ddz;
                    double len = len2 > 0 ? sqrt((double)len2) : 1.0;
                    double ux = ddx / len, uz = ddz / len;
                    *(int *)(c + 0x5c) =
                        clip[0] - (int)(ux * (30 << 12));
                    *(int *)(c + 0x64) =
                        clip[2] - (int)(uz * (30 << 12));
                    *(int *)(c + 0x98) = 0;         /* mHorzSpeed */
                }
                prev_pos[0] = *(int *)(c + 0x5c);
                prev_pos[1] = *(int *)(c + 0x60);
                prev_pos[2] = *(int *)(c + 0x64);
            }

            /* fell out of the world (walked or jumped past the KCL):
               back to the spawn point instead of an endless dive */
            if (my < (-4800 << 12)) {
                *(int *)(c + 0x5c) = spawn_x << 12;
                *(int *)(c + 0x60) = spawn_y << 12;
                *(int *)(c + 0x64) = spawn_z << 12;
                *(int *)(c + 0x98) = 0;   /* mHorzSpeed */
                *(int *)(c + 0xa8) = 0;   /* mVertSpeed */
            }
        }

        /* render: camera behind and above Mario, looking at him */
        ntr::gx_reset();
        NTR_MMIO(uint32_t, 0x04000580) =
            0u | (0u << 8) | (255u << 16) | (191u << 24);
        ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
        ntr::gx_enable_lights(0x1);
        float px = *(int *)(c + 0x5c) / 4096.0f;
        float py = *(int *)(c + 0x60) / 4096.0f;
        float pz = *(int *)(c + 0x64) / 4096.0f;
        if (selftest && frame == 0) {
            /* world-space bounds probe: identity matrices, read the raw
               projected coords (with identity proj they ARE world coords) */
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
            printf("probe: mario fx pos (%d, %d, %d) -> units (%.1f, %.1f, %.1f)\n",
                   *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64),
                   px, py, pz);
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
            /* world units back out of the identity-projection screen coords:
               wx = xs/(W/2)-1, wy = 1-ys/(H/2), wz = zs*2-1. Ground-truth
               the model roof at the spawn column (KCL says 73.3 there). */
            {
                const float hw = ntr::SCREEN_W * 0.5f,
                            hh = ntr::SCREEN_H * 0.5f;
                float top = -1e30f, wxmin = 1e30f, wxmax = -1e30f,
                      wymin = 1e30f, wymax = -1e30f;
                for (size_t i = 0; i < n; ++i)
                    for (int v = 0; v < 3; ++v) {
                        const float wx = ta[i].v[v].x / hw - 1.0f;
                        const float wy = 1.0f - ta[i].v[v].y / hh;
                        const float wz = ta[i].v[v].z * 2.0f - 1.0f;
                        if (wx < wxmin) wxmin = wx;
                        if (wx > wxmax) wxmax = wx;
                        if (wy < wymin) wymin = wy;
                        if (wy > wymax) wymax = wy;
                        if (wx > -10 && wx < 10 && wz > -60 && wz < -40 &&
                            wy > top)
                            top = wy;
                    }
                printf("probe: model WORLD x[%.1f..%.1f] y[%.1f..%.1f], "
                       "roof col top=%.1f (KCL says 73.3)\n",
                       wxmin, wxmax, wymin, wymax, top);
            }
            ntr::gx_reset();
            NTR_MMIO(uint32_t, 0x04000580) =
                0u | (0u << 8) | (255u << 16) | (191u << 24);
            ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
            ntr::gx_enable_lights(0x1);
        }
        /* Follow camera at near eye level (Mario is ~14 units tall). The
           old version LIFTED the eye onto whatever hill sat behind him,
           which looked down at a grazing angle -- terrain read flat and the
           view stretched. Now: shoulder-height offset, occlusion resolved
           by pulling IN along the view ray, and smoothing so the eye never
           snaps. */
        /* SM64DS-like framing: the camera rides well back and above so
           Mario reads small against the world (close-in framing made a
           correctly-sized Mario loom over everything behind him) */
        float want_eye[3] = {px - 1050.0f * sinf(cam_yaw), py + 340.0f,
                             pz - 1050.0f * cosf(cam_yaw)};
        float at[3] = {px, py + 110.0f, pz};
        if (getenv("SM64DS_ORBIT")) {
            /* debug: whole-stage orbit shot to judge proportions */
            want_eye[0] = 2560.0f; want_eye[1] = 1920.0f;
            want_eye[2] = -2560.0f;
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
        if (selftest && frame == 0)
            fprintf(stderr, "[w] render\n");
        if (selftest && frame == 0) {
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
                               (uint32_t)(int)(px * 4096),
                               (uint32_t)(int)(py * 4096),
                               (uint32_t)(int)(pz * 4096), 4096};
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
            push_camera(eye, at);
        }
        if (!getenv("SM64DS_NO_LEVEL"))
            hal_render_model(level_storage, level_shift);
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
                    eye[0], eye[1], eye[2], at[0], at[1], at[2]);
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
        if (selftest && ++frame >= selftest) {
            ntr::ppu_write_bmp("walk_window_selftest.bmp", fb);
            printf("selftest: %d frames, pos=(%d, %d, %d)\n", frame,
                   *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64));
            return 0;
        }
        Sleep(selftest ? 0 : 16);
    }
}
