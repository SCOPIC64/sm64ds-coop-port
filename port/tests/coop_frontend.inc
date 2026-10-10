// Included after the window/presentation helpers in walk_window.cpp.
// Runs in the game process and reuses its HWND for the adventure.
#include "coop_menu_media.inc"
#include "coop_menu_audio.inc"
#include "coop_menu_backdrop.inc"
static const uint32_t* g_coop_backdrop = 0;
static void coop_draw(ntr::Framebuffer& fb, unsigned frame, const char* loading = 0)
{
    coop_frontend::Canvas canvas = { &fb.px[0][0], ntr::active_w,
        ntr::active_h, ntr::SCREEN_W, OVL_FONT };
    coop_frontend::draw(canvas, g_coop_menu, frame, loading, g_coop_backdrop);
}

#include "coop_setup.inc"

static bool coop_frontend_wanted()
{
    const char* bypass[] = { "SM64DS_CLASSIC_MENU", "SM64DS_SCENE",
        "SM64DS_MENU_BACKDROP_CHILD",
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
        "SM64DS_BOOT_CLASSIC", "SM64DS_CHARACTER" };
    for (const char* name : clear) _putenv_s(name, "");
    if (action == coop_frontend::START_SOLO) {
        char slot[] = { static_cast<char>('0' + g_coop_menu.slot), 0 };
        _putenv_s("SM64DS_BOOT_FILE", slot);
        _putenv_s("SM64DS_SKIP_MENU", "1");
        g_coop_loading = true;
    } else {
        const bool host = action == coop_frontend::START_HOST;
        char slot[] = { static_cast<char>('0' + g_coop_menu.slot), 0 };
        _putenv_s("SM64DS_BOOT_FILE", host ? slot : "");
        _putenv_s("SM64DS_SKIP_MENU", "1");
        g_coop_loading = host;
        _putenv_s("SM64DS_COMMS_ROLE", host ? "parent" : "child");
        _putenv_s("SM64DS_COMMS_PORT", g_coop_menu.port);
        _putenv_s("SM64DS_COMMS_FANOUT", "1");
        _putenv_s("SM64DS_NETMODE", "rollback");
        _putenv_s("SM64DS_PARTY", "1");
        _putenv_s("SM64DS_LEVEL", host ? "" : "1");
        if (!host) _putenv_s("SM64DS_SAVE_PATH", "save/coop-join.sav");
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
    g_coop_menu.movement_mod=host_setting_movement_mod();
    g_coop_menu.object_distance=host_setting_object_distance();
    g_coop_menu.fps = host_setting_frame_rate();
    g_coop_menu.smooth = host_setting_smooth_motion() != 0;
    g_coop_menu.names = host_setting_name_tags() != 0;
    g_coop_menu.mouse_capture = host_setting_mouse_capture() != 0;
    int volume = host_setting_volume();
    g_coop_menu.volume = volume < 0 ? 80 : volume;
    g_coop_menu.music = host_setting_menu_music();
    g_coop_menu.background = host_setting_menu_background();
    g_coop_menu.menu_sounds = host_setting_menu_sounds() != 0;
    snprintf(g_coop_menu.music_file,sizeof g_coop_menu.music_file,"%s",host_setting_menu_music_file());
    snprintf(g_coop_menu.background_file,sizeof g_coop_menu.background_file,"%s",host_setting_menu_background_file());
    CoopMediaLibrary media;
    media.refresh(g_coop_menu);
    const char* save_path = getenv("SM64DS_SAVE_PATH");
    coop_frontend::read_save_previews(save_path ? save_path : "save/sm64ds.sav", g_coop_menu.saves);
    g_entry_hwnd = host_window_open(0, &g_entry_hdc, "Super Mario 64 DS Co-op");
    if (!g_entry_hwnd) return -1;
#ifdef SM64DS_HANDHELD
    fullscreen_toggle(g_entry_hwnd);
#endif
    // Reserve the game's fixed DS ranges before allocating this temporary
    // buffer (main already ran io_init). Another static framebuffer would
    // enlarge the loaded image and can change its collision with those ranges.
    struct MenuBuffer {
        ntr::Framebuffer* data;
        MenuBuffer() : data(static_cast<ntr::Framebuffer*>(VirtualAlloc(0,
            sizeof(ntr::Framebuffer), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE))) {}
        ~MenuBuffer() {
            if (g_present_fb == data) g_present_fb = 0;
            if (data) VirtualFree(data, 0, MEM_RELEASE);
        }
    } menu_buffer;
    if (!menu_buffer.data) {
        fprintf(stderr, "[coop-menu] could not allocate the menu screen\n");
        return -1;
    }
    ntr::Framebuffer& menu_fb = *menu_buffer.data;
    g_present_hwnd = g_entry_hwnd;
    g_present_hdc = g_entry_hdc;
    g_present_bi = &g_bi;
    g_present_fb = &menu_fb;
    g_coop_active = true;
    CoopMenuAudio music(media);
    CoopBackdrop backdrop(media);
    struct ClearBackdrop { ~ClearBackdrop() { g_coop_backdrop = 0; } } clear_backdrop;
    if (getenv("SM64DS_MENU_AUDIO_SELFTEST")) {
        bool ok = music.test();
        g_coop_active = false;
        fprintf(stderr, "[coop-menu] music and SFX selftest %s\n", ok ? "PASS" : "FAIL");
        return ok ? 0 : -1;
    }
    if(getenv("SM64DS_MENU_CUSTOM_MEDIA_SELFTEST")) {
        bool ok=!media.music.empty() && !media.backgrounds.empty() && music.test_custom(g_coop_menu);
        for(size_t i=0;i<media.backgrounds.size();++i) {
            std::vector<uint32_t> pixels;
            bool pass=coop_media::image(media.backgrounds[i].utf8.c_str(),pixels,512,384);
            ok=ok&&pass;
            fprintf(stderr,"[coop-menu] custom background %s: %s\n",pass?"PASS":"FAIL",media.backgrounds[i].label.c_str());
        }
        g_coop_menu.background=coop_frontend::BACKGROUND_BUILTIN_COUNT;
        backdrop.tick(g_coop_menu.background); g_coop_backdrop=backdrop.image();
        if(ok){coop_draw(menu_fb,0);ntr::ppu_write_bmp("custom-menu.bmp",menu_fb);}
        g_coop_active=false;
        fprintf(stderr,"[coop-menu] custom media selftest %s\n",ok?"PASS":"FAIL");return ok?0:-1;
    }
    if(getenv("SM64DS_MENU_STAFF_ROLL_SELFTEST")) {
        const int choices[]={0,2,0};
        for(int choice:choices) {
            DWORD deadline=GetTickCount()+15000;
            backdrop.tick(choice);
            while(backdrop.serial()<15 && static_cast<LONG>(GetTickCount()-deadline)<0){backdrop.tick(choice);Sleep(16);}
            bool pass=backdrop.serial()>=15 && ((backdrop.cinematic_frames()>0)==(choice==2));
            fprintf(stderr,"[coop-menu] same-course switch %d %s\n",choice,pass?"PASS":"FAIL");
            if(!pass)return -1;
        }
        g_coop_menu.background=2;
        DWORD deadline=GetTickCount()+240000;
        bool sampled[20]={},moving[20]={};int first_eye[20][3]={};
        unsigned first_visit=backdrop.credits_visits();bool looped=false,ok=true;
        while(static_cast<LONG>(GetTickCount()-deadline)<0) {
            backdrop.tick(2);int shot=backdrop.shot();unsigned cursor=backdrop.credits_cursor();
            if(shot>=0 && shot<20 && backdrop.cinematic_frames() && cursor>=30 && !sampled[shot]) {
                sampled[shot]=true;memcpy(first_eye[shot],backdrop.camera_eye(),sizeof first_eye[shot]);
            }
            if(shot>=0 && shot<20 && cursor>=100 && sampled[shot] && !moving[shot]) {
                moving[shot]=memcmp(first_eye[shot],backdrop.camera_eye(),sizeof first_eye[shot])!=0;
                char file[64];snprintf(file,sizeof file,"staff-roll-%02d.bmp",shot);
                ok=ntr::ppu_write_bmp_px(file,backdrop.image(),512,384) && ok;
                ok=ok && backdrop.credits_level()==coop_staff_roll::levels[shot];
                fprintf(stderr,"[coop-menu] original DS credits shot %d level %d cursor=%u camera=%s\n",shot,
                    backdrop.credits_level(),cursor,moving[shot]?"MOVING":"STILL");
            }
            if(backdrop.credits_visits()-first_visit>=21 && shot==0 && cursor>=100){looped=true;break;}
            Sleep(16);
        }
        for(int shot=0;shot<20;++shot)ok=ok && sampled[shot] && moving[shot];
        ok=ok && looped;g_coop_backdrop=backdrop.image();coop_draw(menu_fb,0);
        ok=ntr::ppu_write_bmp("staff-roll-menu.bmp",menu_fb) && ok;g_coop_active=false;
        fprintf(stderr,"[coop-menu] original DS staff roll full loop %s visits=%u\n",ok?"PASS":"FAIL",backdrop.credits_visits());
        return ok?0:-1;
    }
    // ROM-free startup check of the actual host and framebuffer. Run only in
    // an isolated folder: screenshots are written beside the executable.
    if (getenv("SM64DS_MENU_SELFTEST")) {
        bool ok = true;
        const char* bg_test = getenv("SM64DS_MENU_BACKDROP_SELFTEST");
        if (bg_test) {
            g_coop_menu.background = atoi(bg_test) - 1;
            if (g_coop_menu.background < 0 || g_coop_menu.background >= coop_frontend::BACKGROUND_BUILTIN_COUNT) return -1;
            DWORD deadline = GetTickCount() + 30000;
            while (backdrop.serial() < 90 && static_cast<LONG>(GetTickCount()-deadline) < 0) {
                backdrop.tick(g_coop_menu.background); Sleep(16);
            }
            ok = backdrop.serial() >= 90;
            if(g_coop_menu.background==2 && getenv("SM64DS_MENU_TOUR_TEST")) {
                while((backdrop.credits_visits()<2 || backdrop.serial()<45) && static_cast<LONG>(GetTickCount()-deadline)<0) {
                    backdrop.tick(g_coop_menu.background);Sleep(16);
                }
                ok=ok && backdrop.credits_visits()>=2 && backdrop.serial()>=45;
                fprintf(stderr,"[coop-menu] tour transition selftest %s credits=%u\n",ok?"PASS":"FAIL",backdrop.credits_visits());
            }
            g_coop_backdrop = backdrop.image();
            fprintf(stderr,"[coop-menu] live backdrop selftest %s serial=%u\n",ok?"PASS":"FAIL",backdrop.serial());
        }
        for (int page = coop_frontend::HOME; page <= coop_frontend::EXIT_CONFIRM; ++page) {
            g_coop_menu.open(static_cast<coop_frontend::Page>(page));
            coop_draw(menu_fb, 0); present();
            char name[48]; snprintf(name, sizeof name, "coop-menu-%02d.bmp", page);
            if (!ntr::ppu_write_bmp(name, menu_fb)) ok = false;
        }
        g_coop_active = false;
        fprintf(stderr, "[coop-menu] startup selftest %s\n", ok ? "PASS" : "FAIL");
        return ok ? 0 : -1;
    }
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
            g_coop_sound = i == coop_frontend::ACCEPT ? 30 : i == coop_frontend::BACK ? 123 : 0;
            coop_frontend::Action a = g_coop_menu.key(static_cast<coop_frontend::Key>(i));
            if (a != coop_frontend::NONE) g_coop_action = a;
        }
        if (g_mouse_click_new) {
            coop_frontend::Canvas c = { &menu_fb.px[0][0], ntr::active_w,
                ntr::active_h, ntr::SCREEN_W, OVL_FONT };
            int row = coop_frontend::hit_row(c, g_coop_menu, g_mouse_click_x, g_mouse_click_y);
            if (row >= 0) {
                g_coop_menu.row = row;
                g_coop_sound = 30;
                g_coop_action = g_coop_menu.key(coop_frontend::ACCEPT);
            }
            g_mouse_click_new = 0;
        }
        coop_frontend::Action action = g_coop_action;
        g_coop_action = coop_frontend::NONE;
        if (action == coop_frontend::QUIT) { g_coop_active = false; return 0; }
        if (action == coop_frontend::NEXT_SONG) {
            if(g_coop_menu.music==coop_frontend::MUSIC_OFF)g_coop_menu.music=coop_frontend::MUSIC_RANDOM;
            else if(g_coop_menu.music>=0)g_coop_menu.music=(g_coop_menu.music+1)%(coop_frontend::SONG_COUNT+g_coop_menu.custom_music_count);
            music.next(g_coop_menu);
        }
        if(action==coop_frontend::IMPORT_MUSIC || action==coop_frontend::IMPORT_BACKGROUND || action==coop_frontend::RELOAD_MEDIA) {
            media.selection(g_coop_menu);
            if(action==coop_frontend::RELOAD_MEDIA) {
                media.refresh(g_coop_menu);snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Custom files refreshed.");
            } else media.add(action==coop_frontend::IMPORT_MUSIC,g_entry_hwnd,g_coop_menu);
            music.invalidate();backdrop.invalidate();
        }
        if (action == coop_frontend::SAVE_OPTIONS) {
            media.selection(g_coop_menu);
            if (host_setting_save_frontend_gameplay(g_coop_menu.camera, g_coop_menu.movement,
                    g_coop_menu.fps, g_coop_menu.smooth, g_coop_menu.names, g_coop_menu.volume,
                    g_coop_menu.mouse_capture, g_coop_menu.music, g_coop_menu.background, g_coop_menu.menu_sounds,
                    g_coop_menu.music_file,g_coop_menu.background_file,g_coop_menu.movement_mod,g_coop_menu.object_distance))
                g_coop_menu.open(g_coop_menu.after_save);
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
        if (g_coop_menu.background >= coop_frontend::BACKGROUND_BUILTIN_COUNT || coop_assets_ready()) {
            backdrop.tick(g_coop_menu.background);
            g_coop_backdrop = backdrop.image();
        }
        music.tick(g_coop_menu, g_coop_sound); g_coop_sound = -1;
        coop_draw(menu_fb, frame++);
        present();
        Sleep(16);
    }
}
