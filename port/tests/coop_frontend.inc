// Included after the window/presentation helpers in walk_window.cpp.
// Runs in the game process and reuses its HWND for the adventure.
static void coop_draw(ntr::Framebuffer& fb, unsigned frame, const char* loading = 0)
{
    coop_frontend::Canvas canvas = { &fb.px[0][0], ntr::active_w,
        ntr::active_h, ntr::SCREEN_W, OVL_FONT };
    coop_frontend::draw(canvas, g_coop_menu, frame, loading);
}

#include "coop_setup.inc"

static bool coop_frontend_wanted()
{
    const char* bypass[] = { "SM64DS_CLASSIC_MENU", "SM64DS_SCENE",
        "SM64DS_LEVEL", "SM64DS_VS_MAP", "SM64DS_WINDOW_SELFTEST",
        "SM64DS_SCENE_FRAMES", "SM64DS_DUMP_LEVEL_NAMES", "SM64DS_BOOT_FILE" };
    for (const char* name : bypass) if (getenv(name)) return false;
    return true;
}

static void coop_session_environment(coop_frontend::Action action)
{
    // The CRT's cached getenv environment needs _putenv_s, not just Win32.
    const char* clear[] = { "SM64DS_COMMS_ROLE", "SM64DS_COMMS_HOST",
        "SM64DS_COMMS_PORT", "SM64DS_COMMS_SLOT", "SM64DS_COMMS_BIND_ANY",
        "SM64DS_COMMS_FANOUT", "SM64DS_COMMS_INJECT", "SM64DS_COMMS_REPORT",
        "SM64DS_PARTY", "SM64DS_LEVEL", "SM64DS_SCENE", "SM64DS_VS_MAP",
        "SM64DS_BOOT_CLASSIC", "SM64DS_CHARACTER", "SM64DS_SAVE_PATH" };
    for (const char* name : clear) _putenv_s(name, "");
    if (action == coop_frontend::START_SOLO) {
        char slot[] = { static_cast<char>('0' + g_coop_menu.slot), 0 };
        _putenv_s("SM64DS_BOOT_FILE", slot);
        _putenv_s("SM64DS_SKIP_MENU", "1");
        g_coop_loading = true;
    } else {
        const bool host = action == coop_frontend::START_HOST;
        _putenv_s("SM64DS_BOOT_FILE", "");
        _putenv_s("SM64DS_COMMS_ROLE", host ? "parent" : "child");
        _putenv_s("SM64DS_COMMS_PORT", g_coop_menu.port);
        _putenv_s("SM64DS_COMMS_FANOUT", "1");
        _putenv_s("SM64DS_NETMODE", "rollback");
        _putenv_s("SM64DS_PARTY", "1");
        _putenv_s("SM64DS_LEVEL", "1");
        _putenv_s("SM64DS_SAVE_PATH", host ? "coop-host.sav" : "coop-join.sav");
        char character[] = { static_cast<char>('0' + g_coop_menu.character), 0 };
        _putenv_s("SM64DS_CHARACTER", character);
        if (host) _putenv_s("SM64DS_COMMS_BIND_ANY", "1");
        else {
            char address[32];
            snprintf(address, sizeof address, "%s:%s", g_coop_menu.address, g_coop_menu.port);
            _putenv_s("SM64DS_COMMS_HOST", address);
            _putenv_s("SM64DS_COMMS_SLOT", "1");
        }
    }
}

static int coop_frontend_run()
{
    if (!coop_frontend_wanted()) return 1;
    g_coop_menu.camera = host_setting_camera_mode();
    g_coop_menu.movement = host_setting_run_mode();
    g_coop_menu.fps = host_setting_frame_rate();
    g_coop_menu.smooth = host_setting_smooth_motion() != 0;
    g_coop_menu.names = host_setting_name_tags() != 0;
    int volume = host_setting_volume();
    g_coop_menu.volume = volume < 0 ? 80 : volume;
    g_entry_hwnd = host_window_open(0, &g_entry_hdc, "Super Mario 64 DS Co-op");
    if (!g_entry_hwnd) return -1;
#ifdef SM64DS_HANDHELD
    fullscreen_toggle(g_entry_hwnd);
#endif
    static ntr::Framebuffer menu_fb;
    g_present_hwnd = g_entry_hwnd;
    g_present_hdc = g_entry_hdc;
    g_present_bi = &g_bi;
    g_present_fb = &menu_fb;
    g_coop_active = true;
    unsigned previous = ~0u;
    unsigned frame = 0;
    for (;;) {
        MSG message;
        while (W.PeekMessageA_(&message, 0, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) { g_coop_active = false; return 0; }
            W.TranslateMessage_(&message);
            W.DispatchMessageA_(&message);
        }
        XPad pad = {};
        unsigned buttons = 0;
        if (port_pad_poll(&pad) && hal_window_focused()) {
            buttons = pad.buttons;
            if (pad.ly > 16000) buttons |= 1;
            if (pad.ly < -16000) buttons |= 2;
            if (pad.lx < -16000) buttons |= 4;
            if (pad.lx > 16000) buttons |= 8;
        }
        unsigned pressed = buttons & ~previous;
        previous = buttons;
        const unsigned masks[] = { 1, 2, 4, 8, 0x1000, 0x2000 };
        for (int i = 0; i < 6; ++i) if (pressed & masks[i]) {
            coop_frontend::Action a = g_coop_menu.key(static_cast<coop_frontend::Key>(i));
            if (a != coop_frontend::NONE) g_coop_action = a;
        }
        if (g_mouse_click_new) {
            coop_frontend::Canvas c = { &menu_fb.px[0][0], ntr::active_w,
                ntr::active_h, ntr::SCREEN_W, OVL_FONT };
            int row = coop_frontend::hit_row(c, g_coop_menu, g_mouse_click_x, g_mouse_click_y);
            if (row >= 0) {
                g_coop_menu.row = row;
                g_coop_action = g_coop_menu.key(coop_frontend::ACCEPT);
            }
            g_mouse_click_new = 0;
        }
        coop_frontend::Action action = g_coop_action;
        g_coop_action = coop_frontend::NONE;
        if (action == coop_frontend::QUIT) { g_coop_active = false; return 0; }
        if (action == coop_frontend::SAVE_OPTIONS) {
            if (host_setting_save_frontend(g_coop_menu.camera, g_coop_menu.movement,
                    g_coop_menu.fps, g_coop_menu.smooth, g_coop_menu.names, g_coop_menu.volume))
                g_coop_menu.open(coop_frontend::HOME);
            else snprintf(g_coop_menu.message, sizeof g_coop_menu.message,
                          "Could not save settings. Check folder access.");
        }
        if (action == coop_frontend::START_SOLO || action == coop_frontend::START_HOST ||
            action == coop_frontend::START_JOIN) {
            g_coop_active = false;
            bool ready = coop_setup_assets(menu_fb);
            g_coop_active = true;
            if (!ready) continue;
            coop_session_environment(action);
            coop_draw(menu_fb, frame, "GETTING READY");
            present();
            g_coop_active = false;
            memset(key_stale, 1, sizeof key_stale);
            g_mouse_click_new = g_mouse_left_down = 0;
            fprintf(stderr, "[coop-menu] start action %d, same process and window\n", static_cast<int>(action));
            return 1;
        }
        coop_draw(menu_fb, frame++);
        present();
        Sleep(16);
    }
}
