// Included after the window/presentation helpers in walk_window.cpp.
// Runs in the game process and reuses its HWND for the adventure.
#include "coop_menu_media.inc"
#include "coop_menu_audio.inc"
#include "coop_menu_backdrop.inc"
#include "hal/coop_session.h"
static coop_online::Plan g_coop_online_plan;
static coop_online::Client& coop_lobby() { static coop_online::Client client; return client; }
static const uint32_t* g_coop_backdrop = 0;
static void coop_draw(ntr::Framebuffer& fb, unsigned frame, const char* loading = 0)
{
    coop_frontend::Canvas canvas = { &fb.px[0][0], ntr::active_w,
        ntr::active_h, ntr::SCREEN_W, OVL_FONT };
    coop_frontend::draw(canvas, g_coop_menu, frame, loading, g_coop_backdrop);
    if(g_coop_menu.page==coop_frontend::LOBBY)coop_chat_draw(ovl_surface(fb));
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

// The ROM's session wait runs after the menu buffer has been released. Keep
// that wait responsive using its own temporary buffer in the same window.
static void coop_wait_for_player(int event)
{
    static ntr::Framebuffer* screen = 0;
    static const ntr::Framebuffer* previous_screen = 0;
    static DWORD last_draw = 0;
    static unsigned frame = 0;
    if (event != 0) {
        if (screen) {
            g_present_fb = previous_screen;
            VirtualFree(screen, 0, MEM_RELEASE);
            screen = 0;
        }
        if (event == 2) {
            const char* error = "No other player connected within five minutes. "
                "Reopen the game and create or join a new lobby.";
            port_startup_error_write(error);
            port_startup_error_show(error);
        }
        return;
    }
    MSG message;
    while (W.PeekMessageA_(&message, 0, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT ||
            (message.message == WM_KEYDOWN && message.wParam == VK_ESCAPE)) {
            fprintf(stderr, "[coop-menu] multiplayer startup cancelled\n");
            fflush(0);
            ExitProcess(0);
        }
        W.TranslateMessage_(&message);
        W.DispatchMessageA_(&message);
    }
    if (!screen) {
        screen = static_cast<ntr::Framebuffer*>(VirtualAlloc(0,
            sizeof(ntr::Framebuffer), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        if (!screen) return;
        previous_screen = g_present_fb;
        last_draw = GetTickCount() - 33;
    }
    const DWORD now = GetTickCount();
    if ((DWORD)(now - last_draw) < 33) return;
    last_draw = now;
    g_present_fb = screen;
    coop_draw(*screen, frame++, "WAITING FOR PLAYER - ESC TO QUIT");
    present();
}

static void coop_session_environment(coop_frontend::Action action)
{
    // The CRT's cached getenv environment needs _putenv_s, not just Win32.
    const char* clear[] = { "SM64DS_COMMS_ROLE", "SM64DS_COMMS_HOST",
        "SM64DS_COMMS_PORT", "SM64DS_COMMS_SLOT", "SM64DS_COMMS_BIND_ANY",
        "SM64DS_COMMS_FANOUT", "SM64DS_COMMS_INJECT", "SM64DS_COMMS_REPORT",
        "SM64DS_PARTY", "SM64DS_LEVEL", "SM64DS_SCENE", "SM64DS_VS_MAP",
        "SM64DS_BOOT_CLASSIC", "SM64DS_CHARACTER", "SM64DS_SYNC",
        "SM64DS_COMMS_WAIT_MS", "SM64DS_COMMS_RELAY", "SM64DS_COMMS_CODE",
        "SM64DS_VS_PLAYERS", "SM64DS_VS_NAMES", "SM64DS_COMMS_TIMEOUT_MS",
        "SM64DS_SHARED_SAVE", "SM64DS_SHARED_SAVE_SLOT", "SM64DS_COMMS_DELAY_FLOOR",
        "SM64DS_PLAYER_PALETTES", "SM64DS_PLAYER_CHARACTERS" };
    for (const char* name : clear) _putenv_s(name, "");
    std::string local_palettes;for(int c=0;c<4;++c){if(c)local_palettes+=',';local_palettes+=coop_prefs::get().palette(c);}
    _putenv_s("SM64DS_LOCAL_PALETTES",local_palettes.c_str());
    if (action == coop_frontend::START_SOLO) {
        char slot[] = { static_cast<char>('0' + g_coop_menu.slot), 0 };
        _putenv_s("SM64DS_BOOT_FILE", slot);
        _putenv_s("SM64DS_SKIP_MENU", "1");
        g_coop_loading = true;
    } else {
        // Both peers enter the same world, from the same host save. A title
        // animation on only the parent races the child's rollback ring.
        for(const auto& entry:coop_online::room_environment(g_coop_online_plan,
                g_coop_menu.slot,g_coop_menu.character,GetCurrentProcessId()))
            _putenv_s(entry.first.c_str(),entry.second.c_str());
        g_coop_loading = false;
        port::comms_wait_hook = coop_wait_for_player;
    }
}

static bool coop_apply_shared_save()
{
    const char* hex=getenv("SM64DS_SHARED_SAVE");
    if(!hex) return true;
    try {
        const auto data=coop_online::decode_save(hex);
        std::memcpy(data_0209caa0,data.data(),data.size());
        const char* slot=getenv("SM64DS_SHARED_SAVE_SLOT");
        ((unsigned char*)data_0209caa0)[0x328]=slot?static_cast<unsigned char>(atoi(slot)):0;
        fprintf(stderr,"[coop-start] shared host save restored before world boot; direct castle entry, lockstep\n");
        return true;
    } catch(const std::exception& error) {
        port_startup_error_write(error.what()); port_startup_error_show(error.what()); return false;
    }
}

static void coop_read_options()
{
    std::snprintf(g_coop_menu.username,sizeof g_coop_menu.username,"%s",coop_prefs::get().name.c_str());
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
}
static void coop_refresh_mods() {
    g_coop_menu.mod_count=(int)sm64ds::mods::count();
    if(g_coop_menu.mod_page*4>=g_coop_menu.mod_count)g_coop_menu.mod_page=0;
    for(int i=0;i<g_coop_menu.mod_count;++i) {
        auto mod=sm64ds::mods::info(i);
        snprintf(g_coop_menu.mod_names[i],64,"%.63s",mod.name.c_str());
        snprintf(g_coop_menu.mod_descriptions[i],96,"%.95s",mod.failed?mod.error.c_str():mod.description.c_str());
        g_coop_menu.mod_enabled[i]=mod.enabled;g_coop_menu.mod_failed[i]=mod.failed;
    }
}
static bool coop_tools_action(coop_frontend::Action action) {
    if(coop_preferences_action(action))return true;
    std::string error;
    if(action==coop_frontend::TOGGLE_MOD || action==coop_frontend::RELOAD_MODS) {
        if(port::comms_loopback_stats().installed) {
            snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Change gameplay mods before a network session.");return true;
        }
        if(action==coop_frontend::TOGGLE_MOD) {
            const int i=g_coop_menu.selected_mod();
            sm64ds::mods::set_enabled(i,!g_coop_menu.mod_enabled[i],error);
        } else {
            const char* root=getenv("SM64DS_MODS");sm64ds::mods::load(root?root:"mods",false,error);
        }
        coop_refresh_mods();
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%.110s",error.empty()?"Mod selection saved.":error.c_str());
        return true;
    }
    if(action==coop_frontend::TEXTURE_CAPTURE) {
        g_coop_menu.capture_textures=!g_coop_menu.capture_textures;
        if(g_coop_menu.capture_textures) {
            std::error_code ec;std::filesystem::create_directories("texture-work/capture",ec);
            if(ec){g_coop_menu.capture_textures=false;error=ec.message();}
        }
        ntr::hdtex_capture("texture-work/capture",g_coop_menu.capture_textures);
        ntr::gx_invalidate_textures();
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%.110s",error.empty()?
            (g_coop_menu.capture_textures?"Capture started. Play levels to collect textures.":"Texture capture stopped."):error.c_str());return true;
    }
    if(action==coop_frontend::TEXTURE_PACK) {
        std::string created;
        bool ok=sm64ds::textures::create_pack("texture-work/capture","mods/resource-packs",created,error);
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%.110s",ok?"Pack created. Edit its PNGs; restart to load it.":error.c_str());return true;
    }
    if(action==coop_frontend::OPEN_TEXTURE_FOLDER) {
        std::error_code ec;std::filesystem::create_directories("texture-work",ec);
        std::filesystem::create_directories("mods/resource-packs",ec);
        // Load the shell after the DS address ranges are reserved. Importing
        // shell32 at process startup can claim those ranges before our TLS hook.
        HMODULE shell=LoadLibraryW(L"shell32.dll");
        using Open=HINSTANCE(WINAPI*)(HWND,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT);
        Open open=shell?reinterpret_cast<Open>(GetProcAddress(shell,"ShellExecuteW")):nullptr;
        if(open) {
            open(g_entry_hwnd,L"open",L"texture-work",0,0,SW_SHOWNORMAL);
            open(g_entry_hwnd,L"open",L"mods/resource-packs",0,0,SW_SHOWNORMAL);
        } else snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Could not open the texture folders.");
        if(shell)FreeLibrary(shell);return true;
    }
    return false;
}
static int coop_frontend_run()
{
    if(!getenv("SM64DS_MENU_BACKDROP_CHILD")) {
        const char* root=getenv("SM64DS_MODS");std::string error;
        if(!sm64ds::mods::load(root?root:"mods",host_setting_movement_mod()!=0,error))
            fprintf(stderr,"[mods] %s\n",error.c_str());
    }
    if (!coop_frontend_wanted()) return 1;
    coop_read_options();coop_refresh_mods();
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
    if(getenv("SM64DS_SOCIAL_SELFTEST")) {
        bool ok=coop_social_selftest(menu_fb);g_coop_active=false;return ok?0:-1;
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
            if(g_coop_menu.background!=2) {
                const int facing=backdrop.mario_front_milli();
                ok=ok && facing>=990;
                fprintf(stderr,"[coop-menu] Mario facing camera %s dot=%d\n",facing>=990?"PASS":"FAIL",facing);
            }
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
        for (int page = coop_frontend::HOME; page <= coop_frontend::COLORS; ++page) {
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
        if(coop_preferences_tick(&pad)){coop_draw(menu_fb,frame++);present();Sleep(10);continue;}
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
        if(action==coop_frontend::CREATE_ROOM || action==coop_frontend::JOIN_ROOM) {
            g_coop_active=false;
            bool assets_ok=coop_setup_assets(menu_fb);
            g_coop_active=true;
            if(assets_ok) {
                try {
                    auto& prefs=coop_prefs::get();prefs.name=g_coop_menu.username;
                    if(!prefs.save())throw std::runtime_error("Could not save your player profile");
                    const char* path=getenv("SM64DS_SAVE_PATH");
                    std::string save=action==coop_frontend::CREATE_ROOM?
                        coop_online::room_save(path?path:"save/sm64ds.sav",g_coop_menu.slot):"";
                    coop_lobby().request(action==coop_frontend::CREATE_ROOM?1:2,
                        std::string(g_coop_menu.room_code)+"|"+prefs.name,save,
                        ",\"player_id\":"+coop_online::quote(prefs.id)+",\"palette\":"+coop_online::quote(prefs.palette(g_coop_menu.character))+
                        ",\"character\":"+std::to_string(g_coop_menu.character));
                } catch(const std::exception& error) {
                    snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%.100s",error.what());
                }
            }
        }
        if(action==coop_frontend::START_ROOM) coop_lobby().request(3);
        if(action==coop_frontend::LEAVE_ROOM) coop_lobby().request(4);
        // Short HTTP polls run on a worker; drawing and input remain responsive.
        const coop_online::Snapshot online=coop_lobby().snapshot();
        g_coop_menu.online_busy=online.busy;
        if(!online.room.empty()) {
            if(g_coop_menu.page!=coop_frontend::LOBBY) g_coop_menu.open(coop_frontend::LOBBY);
            snprintf(g_coop_menu.room_code,sizeof g_coop_menu.room_code,"%s",online.room.c_str());
            g_coop_menu.room_host=online.host; g_coop_menu.room_count=static_cast<int>(online.names.size());
            for(int i=0;i<g_coop_menu.room_count;++i) snprintf(g_coop_menu.room_members[i],sizeof g_coop_menu.room_members[i],"%s",online.names[i].c_str());
        } else if(g_coop_menu.page==coop_frontend::LOBBY && !online.busy) g_coop_menu.open(coop_frontend::HOME);
        if(!online.status.empty()&&(g_coop_menu.page==coop_frontend::HOST||g_coop_menu.page==coop_frontend::JOIN||g_coop_menu.page==coop_frontend::LOBBY)) snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%.100s",online.status.c_str());
        else if(g_coop_menu.page==coop_frontend::LOBBY) snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%s",online.host?"Share this room code. Start when everyone has joined.":"Choose your character before joining. Wait for the host.");
        if(online.plan.valid) {
            g_coop_online_plan=online.plan;
            action=online.host?coop_frontend::START_HOST:coop_frontend::START_JOIN;
        }
        coop_tools_action(action);
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
            if(!coop_preferences_save())continue;
            media.selection(g_coop_menu);
            if (host_setting_save_frontend_gameplay(g_coop_menu.camera, g_coop_menu.movement,
                    g_coop_menu.fps, g_coop_menu.smooth, g_coop_menu.names, g_coop_menu.volume,
                    g_coop_menu.mouse_capture, g_coop_menu.music, g_coop_menu.background, g_coop_menu.menu_sounds,
                    g_coop_menu.music_file,g_coop_menu.background_file,0,g_coop_menu.object_distance))
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
