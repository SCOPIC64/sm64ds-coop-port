// Shares the front-end navigation with the adventure's Start/Escape overlay.
// Solo pauses the simulation; a network session keeps ticking with neutral
// local input, so opening a menu does not freeze the other consoles.
static void coop_pause_close() {
    g_coop_pause=false;g_coop_active=false;menu_on=0;
    g_coop_start_swallow=true;g_coop_pad_swallow=g_coop_pad_held;
    memset(key_stale,1,sizeof key_stale);
    fprintf(stderr,"[pause] resumed frame=%d\n",port_rom_frame());
}
static bool coop_pause_input(int pad_live,const XPad* pad) {
    if(g_selftest && !getenv("SM64DS_PAUSE_SELFTEST"))return false;
    if(rb_replaying())return g_coop_pause;
    static bool previous_start=false;
    const bool start=key_act(HOST_KEY_START) || key_live(VK_ESCAPE) ||
        (pad_live && pad_act(pad,HOST_PAD_START));
    const bool edge=start&&!previous_start;previous_start=start;
    if(!start)g_coop_start_swallow=false;
    g_coop_pad_held=pad_live?pad->buttons:0;
    g_coop_pad_swallow&=g_coop_pad_held;
    unsigned buttons=0;
    if(pad_live && hal_window_focused()) {
        buttons=pad->buttons;
        if(pad->ly>16000)buttons|=1;if(pad->ly<-16000)buttons|=2;
        if(pad->lx<-16000)buttons|=4;if(pad->lx>16000)buttons|=8;
    }
    // Scripted keys exercise this same path without posting fake window messages.
    const int keys[]={VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_RETURN,VK_ESCAPE};
    const unsigned masks[]={1,2,4,8,0x1000,0x2000};
    for(int i=0;i<6;++i)if(host_key_script(keys[i]))buttons|=masks[i];
    static unsigned previous_buttons=~0u;
    const unsigned pressed=buttons&~previous_buttons;previous_buttons=buttons;
    if(!g_coop_pause) {
        if(!edge || menu_on || !g_menu_host.player || data_0209fc48 ||
            ((data_0209f20c[0]|data_0209f294[0]|data_0209f2c4[0])&255))return false;
        coop_read_options();coop_refresh_mods();
        g_coop_menu.in_game=true;g_coop_menu.open(coop_frontend::PAUSE);
        g_coop_action=coop_frontend::NONE;g_coop_pause=g_coop_active=true;
        g_coop_start_swallow=true;
        menu_on=port::comms_loopback_stats().installed?0:1;
        mo_capture_set(g_present_hwnd,0);
        fprintf(stderr,"[pause] opened frame=%d network=%d\n",port_rom_frame(),port::comms_loopback_stats().installed);
        return true;
    }
    if(edge && pad_live && pad_act(pad,HOST_PAD_START)) {
        coop_pause_close();return true;
    }
    for(int i=0;i<6;++i)if(pressed&masks[i]) {
        auto action=g_coop_menu.key(static_cast<coop_frontend::Key>(i));
        if(action!=coop_frontend::NONE)g_coop_action=action;
    }
    if(g_mouse_click_new) {
        uint32_t scratch=0;
        coop_frontend::Canvas c={&scratch,ntr::active_w,ntr::active_h,ntr::SCREEN_W,OVL_FONT};
        int row=coop_frontend::hit_row(c,g_coop_menu,g_mouse_click_x,g_mouse_click_y);
        if(row>=0){g_coop_menu.row=row;g_coop_action=g_coop_menu.key(coop_frontend::ACCEPT);}
        g_mouse_click_new=0;
    }
    auto action=g_coop_action;g_coop_action=coop_frontend::NONE;
    coop_tools_action(action);
    if(action==coop_frontend::RESUME){coop_pause_close();return true;}
    if(action==coop_frontend::QUIT){W.PostQuitMessage_(0);return true;}
    if(action==coop_frontend::SAVE_OPTIONS) {
        if(host_setting_save_frontend_gameplay(g_coop_menu.camera,g_coop_menu.movement,
            g_coop_menu.fps,g_coop_menu.smooth,g_coop_menu.names,g_coop_menu.volume,
            g_coop_menu.mouse_capture,g_coop_menu.music,g_coop_menu.background,g_coop_menu.menu_sounds,
            g_coop_menu.music_file,g_coop_menu.background_file,0,g_coop_menu.object_distance)) {
            if(cam_mode!=g_coop_menu.camera){cam_mode=g_coop_menu.camera;sm64_camera.live=false;an_pivot_live=0;}
            g_run_mode=g_coop_menu.movement;
            g_coop_menu.open(g_coop_menu.after_save);
        } else snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Could not save settings.");
    }
    if(action==coop_frontend::IMPORT_MUSIC || action==coop_frontend::IMPORT_BACKGROUND ||
        action==coop_frontend::RELOAD_MEDIA || action==coop_frontend::NEXT_SONG)
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Set menu music/background on the title screen.");
    return true;
}
static void coop_pause_draw(const OvlSurface& surf) {
    coop_refresh_mods();
    coop_frontend::Canvas canvas={surf.px,ntr::active_w,ntr::active_h,surf.stride,OVL_FONT};
    coop_frontend::draw(canvas,g_coop_menu,0,nullptr,surf.px,ntr::active_w,ntr::active_h,surf.stride);
}
