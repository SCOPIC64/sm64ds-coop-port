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
}

static const int ZOOM = 3;

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
    const float zn = 0.05f, zf = 200.0f;
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
    PORT_INSTALL_FAULT_PROBE();
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
        _ZN12MeshColliderC1Ev(mc_storage);
        char *kcl = (char *)_ZN12MeshCollider8LoadFileER13SharedFilePtr(&kp);
        if (!kcl) return 4;
        static char clps[0x100];
        _ZN12MeshCollider7SetFileEP8KCL_FileR10CLPS_Block(mc_storage, kcl,
                                                          clps);
        _ZN16MeshColliderBase6EnableEP5Actor(mc_storage, player);
        int ox = *(int *)(kcl + 0), oy = *(int *)(kcl + 4),
            oz = *(int *)(kcl + 8);
        unsigned xm = *(unsigned *)(kcl + 0x10), zm = *(unsigned *)(kcl + 0x18);
        *(int *)(c + 0x5c) = ox + (((int)(~xm + 1)) << 6) / 2;
        *(int *)(c + 0x60) = oy + 0x8000;
        *(int *)(c + 0x64) = oz + (((int)(~zm + 1)) << 6) / 2;
    }
    data_020a0e40[0] = 0;
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

    static ntr::Framebuffer fb;
    MSG msg;
    for (;;) {
        while (W.PeekMessageA_(&msg, 0, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return 0;
            W.TranslateMessage_(&msg);
            W.DispatchMessageA_(&msg);
        }

        /* keys -> pad block + desired heading (heading 0 walks +Z) */
        int dx = 0, dz = 0;
        if (selftest) dz = 1;
        if (W.GetAsyncKeyState_('W') < 0 || W.GetAsyncKeyState_(VK_UP) < 0) dz += 1;
        if (W.GetAsyncKeyState_('S') < 0 || W.GetAsyncKeyState_(VK_DOWN) < 0) dz -= 1;
        if (W.GetAsyncKeyState_('A') < 0 || W.GetAsyncKeyState_(VK_LEFT) < 0) dx -= 1;
        if (W.GetAsyncKeyState_('D') < 0 || W.GetAsyncKeyState_(VK_RIGHT) < 0) dx += 1;
        if (dx || dz) {
            *(short *)(data_0209f4a0 + 0) = 0x1000;
            unsigned short ang =
                (unsigned short)(int)(atan2f((float)dx, (float)dz) *
                                      (32768.0f / 3.14159265f));
            *(short *)(c + 0x69c) = (short)ang;   /* mDesiredAngleY */
        } else {
            *(short *)(data_0209f4a0 + 0) = 0;
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
            ntr::gx_reset();
            NTR_MMIO(uint32_t, 0x04000580) =
                0u | (0u << 8) | (255u << 16) | (191u << 24);
            ntr::gx_set_light(0, -0.4f, -0.6f, -0.7f, 0x7FFF);
            ntr::gx_enable_lights(0x1);
        }
        /* Mario stands ~14 world units tall (probe above) */
        float eye[3] = {px, py + 20.0f, pz - 48.0f};
        float at[3] = {px, py + 8.0f, pz};
        push_camera(eye, at);
        if (selftest && frame == 0)
            fprintf(stderr, "[w] render\n");
        hal_render_player_world(player);
        if (selftest && frame == 0)
            fprintf(stderr, "[w] rendered\n");

        for (int y = 0; y < ntr::SCREEN_H; ++y)
            for (int x = 0; x < ntr::SCREEN_W; ++x)
                fb.px[y][x] = 0xFF101820u;
        ntr::gx_render(fb);
        W.StretchDIBits_(hdc, 0, 0, ntr::SCREEN_W * ZOOM, ntr::SCREEN_H * ZOOM,
                      0, 0, ntr::SCREEN_W, ntr::SCREEN_H, fb.px, &bi,
                      DIB_RGB_COLORS, SRCCOPY);
        if (selftest && ++frame >= selftest) {
            ntr::ppu_write_bmp("walk_window_selftest.bmp", fb);
            printf("selftest: %d frames, pos=(%d, %d, %d)\n", frame,
                   *(int *)(c + 0x5c), *(int *)(c + 0x60), *(int *)(c + 0x64));
            return 0;
        }
        Sleep(selftest ? 0 : 16);
    }
}
