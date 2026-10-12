// Host UI only: preferences, text chat and moderation never write DS state.
#include "coop_palette_library.inc"
static std::string g_coop_chat_input,g_coop_chat_notice;
static DWORD g_coop_chat_seen=0;
static bool g_coop_chat_skip_char=false;
static int g_coop_chat_sequence=0;
static bool g_coop_color_editing=false;
static int g_coop_color_character=-1,g_coop_color_part=-1;
static std::string g_coop_color_loaded;
static int g_coop_slider_drag=-1;
static unsigned g_coop_bind_previous=~0u;

static void coop_binding_label(int code,char* out,size_t size) {
    if(!code){snprintf(out,size,"UNBOUND");return;}
    if(code<0) {
        const int bits[]={1,2,4,8,0x10,0x20,0x40,0x80,0x100,0x200,0x1000,0x2000,0x4000,0x8000,0x10000,0x20000};
        const char* names[]={"DPAD UP","DPAD DOWN","DPAD LEFT","DPAD RIGHT","START","BACK","L STICK","R STICK","LB","RB","A","B","X","Y","LT","RT"};
        for(int i=0;i<16;++i)if(-code==bits[i]){snprintf(out,size,"PAD %s",names[i]);return;}
        snprintf(out,size,"PAD %X",-code);return;
    }
    if(code>='A'&&code<='Z'){snprintf(out,size,"KEY %c",code);return;}
    if(code>=VK_F1&&code<=VK_F24){snprintf(out,size,"KEY F%d",code-VK_F1+1);return;}
    const int keys[]={VK_SPACE,VK_RETURN,VK_SHIFT,VK_CONTROL,VK_MENU,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_BACK};
    const char* names[]={"SPACE","ENTER","SHIFT","CTRL","ALT","UP","DOWN","LEFT","RIGHT","BACKSPACE"};
    for(int i=0;i<10;++i)if(code==keys[i]){snprintf(out,size,"KEY %s",names[i]);return;}
    snprintf(out,size,"KEY %02X",code);
}
static bool coop_preferences_save() {
    auto& prefs=coop_prefs::get();
    if(coop_prefs::Preferences::valid_name(g_coop_menu.username))prefs.name=g_coop_menu.username;
    if(!prefs.save()){snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Could not save your player profile.");return false;}
    std::string local;for(int c=0;c<4;++c){if(c)local+=',';local+=prefs.palette(c);}
    const char* previous=getenv("SM64DS_LOCAL_PALETTES");
    if(!previous||local!=previous){_putenv_s("SM64DS_LOCAL_PALETTES",local.c_str());ntr::gx_invalidate_textures();}
    if(g_coop_menu.page==coop_frontend::COLORS&&g_coop_online_plan.valid)
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"Saved. Online colors apply in your next lobby.");
    return true;
}
static void coop_binding_store(int code) {
    auto& prefs=coop_prefs::get();prefs.custom_controls=true;
    prefs.bindings[g_coop_menu.binding_action][g_coop_menu.binding_slot]=code;
    g_coop_binding_capture=0;coop_preferences_save();
    memset(key_stale,1,sizeof key_stale);g_coop_menu.message[0]=0;
}
static bool coop_preferences_action(coop_frontend::Action action) {
    bindings_load();auto& p=coop_prefs::get();
    if(g_coop_palettes.action(action,g_coop_menu,p)) {
        g_coop_color_part=-1;coop_preferences_save();return true;
    }
    if(action==coop_frontend::BIND_KEY||action==coop_frontend::BIND_PAD) {
        g_coop_binding_capture=action==coop_frontend::BIND_KEY?1:2;g_coop_bind_previous=~0u;
        snprintf(g_coop_menu.message,sizeof g_coop_menu.message,"%s - ESC cancels",g_coop_binding_capture==1?"Press a keyboard key":"Release, then press a controller button");return true;
    }
    if(action==coop_frontend::CLEAR_BIND){coop_binding_store(0);return true;}
    if(action==coop_frontend::RESET_BINDS){p.defaults();p.custom_controls=true;coop_preferences_save();return true;}
    if(action==coop_frontend::DEFAULT_COLOR||action==coop_frontend::RESET_COLORS) {
        if(action==coop_frontend::DEFAULT_COLOR)p.colors[g_coop_menu.color_character][g_coop_menu.color_part].clear();
        else for(auto& color:p.colors[g_coop_menu.color_character])color.clear();
        g_coop_color_part=-1;coop_preferences_save();return true;
    }
    return false;
}
static bool coop_preferences_tick(const XPad* pad) {
    bindings_load();auto& p=coop_prefs::get();
    if(g_coop_menu.page==coop_frontend::PALETTES)g_coop_palettes.tick(g_coop_menu);
    if(g_coop_menu.page==coop_frontend::CONTROLS) {
        snprintf(g_coop_menu.binding_name,sizeof g_coop_menu.binding_name,"%s",coop_prefs::action_name(g_coop_menu.binding_action));
        for(int s=0;s<3;++s)coop_binding_label(p.bindings[g_coop_menu.binding_action][s],g_coop_menu.binding_labels[s],32);
    }
    if(g_coop_menu.page==coop_frontend::COLORS) {
        uint32_t scratch=0;coop_frontend::Canvas c={&scratch,ntr::active_w,ntr::active_h,ntr::SCREEN_W,OVL_FONT};
        if(!g_mouse_left_down)g_coop_slider_drag=-1;
        if(g_mouse_click_new&&!g_coop_menu.editing) {
            int row=coop_frontend::hit_row(c,g_coop_menu,g_mouse_click_x,g_mouse_click_y);
            if(row>=3&&row<=5){g_coop_slider_drag=row;g_coop_menu.row=row;g_mouse_click_new=0;}
        }
        if(g_coop_slider_drag>=3&&g_mouse_left_down&&!g_coop_menu.editing) {
            auto rect=coop_frontend::button_rect(c,g_coop_menu,g_coop_slider_drag);
            int left=rect.x+rect.w*27/100,width=rect.w*55/100;
            int value=(std::max)(0,(std::min)(255,(g_mouse_click_x-left)*255/(std::max)(1,width)));
            unsigned rgb=static_cast<unsigned>(std::strtoul(g_coop_menu.color_hex,nullptr,16)),shift=8*(5-g_coop_slider_drag);
            rgb=(rgb&~(255u<<shift))|(static_cast<unsigned>(value)<<shift);
            snprintf(g_coop_menu.color_hex,sizeof g_coop_menu.color_hex,"%06x",rgb);
        }
        if(!g_coop_menu.editing&&g_coop_color_character==g_coop_menu.color_character&&g_coop_color_part==g_coop_menu.color_part&&
                g_coop_color_loaded!=g_coop_menu.color_hex) {
            p.colors[g_coop_color_character][g_coop_color_part]=g_coop_menu.color_hex;
            g_coop_color_loaded=g_coop_menu.color_hex;
            coop_preferences_save();
        }
        if(g_coop_color_character!=g_coop_menu.color_character||g_coop_color_part!=g_coop_menu.color_part) {
            g_coop_color_character=g_coop_menu.color_character;g_coop_color_part=g_coop_menu.color_part;
            const std::string& custom=p.colors[g_coop_color_character][g_coop_color_part];
            auto stock=coop_palettes::stock(g_coop_color_character);
            snprintf(g_coop_menu.color_hex,sizeof g_coop_menu.color_hex,"%s",(custom.empty()?stock[g_coop_color_part]:custom).c_str());
            g_coop_color_loaded=g_coop_menu.color_hex;
        }
        g_coop_color_editing=g_coop_menu.editing;
    }else g_coop_color_editing=false;
    if(g_coop_binding_capture==2&&pad) {
        unsigned buttons=pad_word_with_triggers(pad),pressed=buttons&~g_coop_bind_previous;
        g_coop_bind_previous=buttons;
        if(pressed){unsigned bit=pressed&(~pressed+1u);coop_binding_store(-static_cast<int>(bit));return true;}
    }
    if(!g_coop_active&&pad&&!g_coop_chat_open) {
        static bool old=false;bool now=false;
        for(int code:p.bindings[coop_prefs::CHAT])if(code<0&&pad_mask_down(pad,-code))now=true;
        if(now&&!old&&!coop_lobby().snapshot().room.empty())g_coop_chat_open=true;old=now;
    }
    return g_coop_binding_capture!=0;
}
static bool coop_social_message(UINT message,WPARAM key,LPARAM flags) {
    if(message!=WM_KEYDOWN&&message!=WM_SYSKEYDOWN&&message!=WM_CHAR)return false;
    if(g_coop_binding_capture) {
        if(message!=WM_CHAR&&!(flags&(1<<30))) {
            if(key==VK_ESCAPE){g_coop_binding_capture=0;g_coop_menu.message[0]=0;}
            else if(g_coop_binding_capture==1)coop_binding_store(static_cast<int>(key));
        }
        return true;
    }
    if(g_coop_chat_open) {
        if(message==WM_CHAR) {
            if(g_coop_chat_skip_char){g_coop_chat_skip_char=false;return true;}
            if(key=='\r') {
                if(!g_coop_chat_input.empty()) {
                    if(coop_lobby().request(5,g_coop_chat_input)){g_coop_chat_input.clear();g_coop_chat_open=false;memset(key_stale,1,sizeof key_stale);}
                    else g_coop_chat_notice="Please wait for the previous message.";
                }else g_coop_chat_open=false;memset(key_stale,1,sizeof key_stale);
            }else if(key=='\b'){if(!g_coop_chat_input.empty())g_coop_chat_input.pop_back();}
            else if(key>=32&&key<=126&&g_coop_chat_input.size()<200)g_coop_chat_input+=static_cast<char>(key);
        }else if(key==VK_ESCAPE){g_coop_chat_input.clear();g_coop_chat_open=false;memset(key_stale,1,sizeof key_stale);}
        return true;
    }
    if(message==WM_KEYDOWN&&!(flags&(1<<30))&&(!g_coop_active||g_coop_menu.page==coop_frontend::LOBBY)) {
        auto& p=coop_prefs::get();for(int code:p.bindings[coop_prefs::CHAT])if(code>0&&key==static_cast<WPARAM>(code)&&!coop_lobby().snapshot().room.empty()) {
            g_coop_chat_open=true;g_coop_chat_skip_char=(key>='A'&&key<='Z')||(key>='0'&&key<='9')||key==VK_SPACE||(key>=0xba&&key<=0xc0)||(key>=0xdb&&key<=0xde);
            g_coop_chat_notice.clear();return true;
        }
    }
    return false;
}
static void coop_chat_draw(const OvlSurface& surface) {
    auto online=coop_lobby().snapshot();
    if(online.removed&&g_coop_online_plan.valid){W.PostQuitMessage_(0);return;}
    if(online.room.empty())return;
    if(!online.chat.empty()&&online.chat.back().seq!=g_coop_chat_sequence){g_coop_chat_sequence=online.chat.back().seq;g_coop_chat_seen=GetTickCount();}
    if(!g_coop_chat_open&&GetTickCount()-g_coop_chat_seen>9000)return;
    coop_frontend::Canvas c={surface.px,ntr::active_w,ntr::active_h,surface.stride,OVL_FONT};
    coop_canvas_prepare(c);
    int scale=c.scale(),width=(std::min)(c.width-16,72*6*scale),x=8,y=c.height-8;
    int columns=(std::max)(20,width/(6*scale));
    std::vector<std::string> lines;
    size_t start=online.chat.size()>5?online.chat.size()-5:0;
    for(size_t n=start;n<online.chat.size();++n) {
        std::string line=online.chat[n].name+": "+online.chat[n].text;
        while(!line.empty()){lines.push_back(line.substr(0,columns));line.erase(0,(std::min)(line.size(),static_cast<size_t>(columns)));}
    }
    if(lines.size()>8)lines.erase(lines.begin(),lines.end()-8);
    if(!g_coop_chat_notice.empty())lines.push_back(g_coop_chat_notice);
    if(g_coop_chat_open){lines.push_back("> "+g_coop_chat_input.substr(g_coop_chat_input.size()>static_cast<size_t>(columns-3)?g_coop_chat_input.size()-(columns-3):0)+"_");lines.push_back("ENTER: SEND  ESC: CANCEL  /help");}
    if(!online.status.empty()&&online.status!="Sending..."&&online.status!="Connecting to lobby...")lines.push_back(online.status.substr(0,columns));
    int height=static_cast<int>(lines.size())*10*scale+8;c.shade(x,y-height,width,height,175);
    for(size_t i=0;i<lines.size();++i)c.shadow(x+4,y-height+4+static_cast<int>(i)*10*scale,lines[i].c_str(),0xffffffffu,scale);
}

// Opt-in check of the real menu handlers and asynchronous lobby client. Use
// an isolated APPDATA and working folder; this writes screenshots/preferences.
static bool coop_social_selftest(ntr::Framebuffer& fb) {
    bool ok=true;
    auto check=[&](const char* name,bool pass){ok=ok&&pass;fprintf(stderr,"[social-test] %s %s\n",pass?"PASS":"FAIL",name);fflush(stderr);};
    auto& prefs=coop_prefs::get();g_selftest=1;
    coop_frontend::Canvas probe={&fb.px[0][0],ntr::active_w,ntr::active_h,ntr::SCREEN_W,OVL_FONT};
    coop_canvas_prepare(probe);check("user logo embedded",probe.logo&&probe.logo_width==1599&&probe.logo_height==1066);
    check("antialiased proportional menu font",probe.measure("WWW",2)>probe.measure("iii",2));
    g_coop_menu.open(coop_frontend::HOME);coop_draw(fb,0);check("logo menu screenshot",ntr::ppu_write_bmp("logo-menu-test.bmp",fb));
    g_coop_menu.open(coop_frontend::CONTROLS);g_coop_menu.binding_action=coop_prefs::ATTACK;
    g_coop_menu.binding_slot=0;coop_preferences_action(coop_frontend::BIND_KEY);coop_social_message(WM_KEYDOWN,'Y',0);
    g_coop_menu.binding_slot=1;coop_preferences_action(coop_frontend::BIND_KEY);coop_social_message(WM_KEYDOWN,'U',0);
    g_coop_menu.binding_slot=2;coop_preferences_action(coop_frontend::BIND_PAD);
    XPad pad={};coop_preferences_tick(&pad);pad.buttons=0x1000;coop_preferences_tick(&pad);
    check("three mixed bindings captured",prefs.bindings[coop_prefs::ATTACK]==std::array<int,3>{'Y','U',-0x1000});
    g_host_key_frame=0;check("Y drives attack",key_act(HOST_KEY_ATTACK)!=0);
    g_host_key_frame=1;check("U drives attack",key_act(HOST_KEY_ATTACK)!=0);
    check("controller A drives attack",pad_act(&pad,HOST_PAD_ATTACK)!=0);
    coop_preferences_tick(nullptr);coop_draw(fb,0);check("controls screenshot",ntr::ppu_write_bmp("controls-test.bmp",fb));
    g_coop_menu.open(coop_frontend::COLORS);g_coop_menu.color_character=0;g_coop_menu.color_part=0;
    coop_preferences_tick(nullptr);g_coop_menu.row=2;g_coop_menu.key(coop_frontend::ACCEPT);
    for(char c:std::string("ff7700"))g_coop_menu.type(c);
    g_coop_menu.key(coop_frontend::ACCEPT);coop_preferences_tick(nullptr);
    check("color edit saved",prefs.colors[0][0]=="ff7700");
    auto slider=coop_frontend::button_rect(probe,g_coop_menu,3);
    g_mouse_click_x=slider.x+slider.w*27/100+slider.w*55/100/2;g_mouse_click_y=slider.y+slider.h/2;
    g_mouse_click_new=1;g_mouse_left_down=1;coop_preferences_tick(nullptr);
    unsigned slider_rgb=static_cast<unsigned>(strtoul(prefs.colors[0][0].c_str(),nullptr,16));
    check("mouse slider changes only red",(slider_rgb&0xffff)==0x7700&&(slider_rgb>>16)>=126&&(slider_rgb>>16)<=128);
    g_mouse_click_x=slider.x+slider.w*90/100;coop_preferences_tick(nullptr);g_mouse_left_down=0;coop_preferences_tick(nullptr);
    check("drag reaches red endpoint",prefs.colors[0][0]=="ff7700");
    coop_draw(fb,0);check("colors screenshot",ntr::ppu_write_bmp("colors-test.bmp",fb));
    g_coop_menu.open(coop_frontend::PALETTES);snprintf(g_coop_menu.palette_name,sizeof g_coop_menu.palette_name,"UI palette");
    coop_preferences_action(coop_frontend::SAVE_PALETTE);prefs.colors[0][0]="000000";
    coop_preferences_action(coop_frontend::APPLY_PALETTE);check("saved preset reapplies cap color",prefs.colors[0][0]=="ff7700");
    coop_preferences_tick(nullptr);coop_draw(fb,0);check("preset menu screenshot",ntr::ppu_write_bmp("presets-test.bmp",fb));
    std::string presence=",\"player_id\":"+coop_online::quote(prefs.id)+",\"palette\":"+coop_online::quote(prefs.palette(0))+",\"character\":0";
    check("create request",coop_lobby().request(1,"|UI-Test",coop_online::encode_save(coop_online::fresh_save()),presence));
    DWORD deadline=GetTickCount()+15000;
    while(coop_lobby().snapshot().room.empty()&&static_cast<LONG>(GetTickCount()-deadline)<0)Sleep(20);
    check("lobby connected",!coop_lobby().snapshot().room.empty());
    g_coop_menu.open(coop_frontend::LOBBY);coop_social_message(WM_KEYDOWN,'T',0);coop_social_message(WM_CHAR,'t',0);
    check("chat opens without inserting T",g_coop_chat_open&&g_coop_chat_input.empty());
    g_host_key_frame=0;check("chat blocks game input",key_act(HOST_KEY_ATTACK)==0&&pad_act(&pad,HOST_PAD_ATTACK)==0);
    for(char c:std::string("UI chat check"))coop_social_message(WM_CHAR,c,0);
    coop_social_message(WM_CHAR,'\r',0);
    bool delivered=false;deadline=GetTickCount()+15000;
    while(!delivered&&static_cast<LONG>(GetTickCount()-deadline)<0){for(const auto& line:coop_lobby().snapshot().chat)if(line.text=="UI chat check")delivered=true;Sleep(20);}
    check("chat delivered to server",delivered&&!g_coop_chat_open);
    auto online=coop_lobby().snapshot();snprintf(g_coop_menu.room_code,sizeof g_coop_menu.room_code,"%s",online.room.c_str());
    g_coop_menu.room_count=1;snprintf(g_coop_menu.room_members[0],sizeof g_coop_menu.room_members[0],"UI-Test");g_coop_menu.room_host=true;
    coop_draw(fb,0);check("chat screenshot",ntr::ppu_write_bmp("chat-test.bmp",fb));
    coop_lobby().request(4);
    int old_w=ntr::active_w,old_h=ntr::active_h;
    fullscreen_toggle(g_entry_hwnd);
    check("fullscreen changes render aspect",g_fullscreen&&ntr::active_w>=old_w);
    coop_draw(fb,0);check("fullscreen screenshot",ntr::ppu_write_bmp("fullscreen-test.bmp",fb));
    fullscreen_toggle(g_entry_hwnd);
    check("windowed render size restored",!g_fullscreen&&ntr::active_w==old_w&&ntr::active_h==old_h);
    g_selftest=0;fprintf(stderr,"[social-test] RESULT %s\n",ok?"PASS":"FAIL");return ok;
}
