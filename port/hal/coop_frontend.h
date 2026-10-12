#pragma once

// Port-owned front end. No Nintendo assets, platform calls or game-state writes.
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "coop_menu_music.h"
#include "coop_save_preview.h"

namespace coop_frontend {
enum Page { HOME, PLAY, HOST, JOIN, OPTIONS, PLAYER, CAMERA, CONTROLS,
            DISPLAY, SOUND, MISC, EXIT_CONFIRM, MODS, TEXTURES, PAUSE, LOBBY, COLORS };
enum Action { NONE, START_SOLO, START_HOST, START_JOIN, SAVE_OPTIONS, NEXT_SONG,
              IMPORT_MUSIC, IMPORT_BACKGROUND, RELOAD_MEDIA, QUIT, TOGGLE_MOD, RELOAD_MODS,
              TEXTURE_CAPTURE, TEXTURE_PACK, OPEN_TEXTURE_FOLDER, RESUME,
              CREATE_ROOM, JOIN_ROOM, START_ROOM, LEAVE_ROOM, BIND_KEY, BIND_PAD,
              CLEAR_BIND, RESET_BINDS, DEFAULT_COLOR, RESET_COLORS };
enum { BACKGROUND_BUILTIN_COUNT = 8 };
enum Key { UP, DOWN, LEFT, RIGHT, ACCEPT, BACK };

inline bool valid_address(const char* text) {
    // Deliberately IPv4 only, matching comms_loopback's supported transport.
    if (!text || !*text) return false;
    const char* p = text;
    for (int part = 0; part < 4; ++part) {
        int value = 0, digits = 0;
        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p++ - '0');
            if (++digits > 3 || value > 255) return false;
        }
        if (!digits) return false;
        if (part < 3) { if (*p++ != '.') return false; }
        else if (*p) return false;
    }
    return true;
}

inline bool valid_port(const char* text) {
    if (!text || !*text) return false;
    unsigned value = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        value = value * 10 + (*p - '0');
        if (value > 65534) return false;
    }
    return value >= 1024;
}

struct Menu {
    Page page = HOME;
    Page after_save = HOME;
    int row = 0, slot = 0, character = 0;
    int camera = 0, movement = 1, fps = 0, volume = 80;
    int movement_mod = 0, object_distance = 0; // Old setting retained for one-time migration.
    bool in_game = false, capture_textures = false;
    int mod_count = 0, mod_page = 0;
    char mod_names[48][64] = {}, mod_descriptions[48][96] = {};
    bool mod_enabled[48] = {}, mod_failed[48] = {};
    int shown_mods() const { int n=mod_count-mod_page*4; return n<0?0:n>4?4:n; }
    int selected_mod() const { return mod_page*4+row; }
    int music = MUSIC_RANDOM, background = 0;
    bool menu_sounds = true;
    int custom_music_count = 0, custom_background_count = 0;
    char custom_music[MAX_CUSTOM_MEDIA][48] = {}, custom_backgrounds[MAX_CUSTOM_MEDIA][48] = {};
    char music_file[1024] = "", background_file[1024] = "";
    SavePreview saves[3];
    char now_playing[48] = "";
    bool smooth = true, names = true, mouse_capture = false, editing = false;
    char room_code[7] = "";
    char room_members[7][48] = {};
    int room_count = 0;
    bool room_host = false, online_busy = false;
    char username[17] = "Player", color_hex[7] = "";
    int binding_action=0,binding_slot=0,color_character=0,color_part=0;
    char binding_name[32]="MOVE UP", binding_labels[3][32]={};
    char edit_backup[32] = "";
    char message[128] = "";

    int rows() const {
        switch (page) {
        case HOME: return 5;
        case PLAY: return 3;
        case HOST: return 7;
        case JOIN: return 4;
        case LOBBY: return 10;
        case OPTIONS: return 7;
        case PLAYER: return 6;
        case DISPLAY: return 7;
        case SOUND: return 7;
        case CAMERA: case MISC: return 2;
        case CONTROLS: return 7;
        case COLORS: return 9;
        case EXIT_CONFIRM: return 2;
        case MODS: return shown_mods()+4;
        case TEXTURES: return 4;
        case PAUSE: return 4;
        }
        return 0;
    }
    const char* title() const {
        switch (page) {
        case HOME: return "";
        case PLAY: return "SINGLE PLAYER";
        case HOST: return "HOST A GAME";
        case JOIN: return "JOIN A GAME";
        case LOBBY: return "ONLINE LOBBY";
        case OPTIONS: return "OPTIONS";
        case PLAYER: return "PLAYER";
        case CAMERA: return "CAMERA";
        case DISPLAY: return "DISPLAY";
        case SOUND: return "SOUND";
        case MISC: return "MISC";
        case CONTROLS: return "CONTROLS";
        case COLORS: return "CHARACTER COLORS";
        case EXIT_CONFIRM: return "QUIT THE GAME?";
        case MODS: return "MODS";
        case TEXTURES: return "TEXTURE PACKS";
        case PAUSE: return "PAUSED";
        }
        return "";
    }
    void open(Page target) { page = target; row = 0; editing = false; message[0] = 0; }
    char* field() {
        if (page == JOIN && row == 0) return room_code;
        if (page == PLAYER && row == 3) return username;
        if (page == COLORS && row == 2) return color_hex;
        return nullptr;
    }
    void type(char ch) {
        char* p = editing ? field() : nullptr;
        if (!p) return;
        size_t size = page==PLAYER?sizeof username:sizeof room_code;
        size_t n = std::strlen(p);
        if (ch == '\b') { if (n) p[n - 1] = 0; }
        else if (n + 1 < size && (page==PLAYER?(ch>=32&&ch<=126&&ch!=','&&ch!='|'):
            page==COLORS?((ch>='0'&&ch<='9')||(ch>='a'&&ch<='f')||(ch>='A'&&ch<='F')):
            ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))) {
            if(page==JOIN&&ch >= 'a' && ch <= 'z') ch -= 'a'-'A';
            if(page==COLORS&&ch>='A'&&ch<='F')ch+='a'-'A';
            p[n] = ch; p[n + 1] = 0;
        }
    }
    void label(int index, char* out, size_t size) const {
        static const char* home[] = { "HOST", "JOIN", "MODS", "OPTIONS", "QUIT" };
        static const char* chars[] = { "MARIO", "LUIGI", "WARIO", "YOSHI" };
        static const char* cameras[] = { "ANALOG", "FREE", "DS", "SM64 CAM" };
        static const char* movement_names[] = { "BUTTON", "ANALOG", "AUTO" };
        out[0] = 0;
        if (index < 0 || index >= rows()) return;
        if (page == HOME) std::snprintf(out, size, "%s", home[index]);
        else if (page == PLAY) {
            if (index == 0) std::snprintf(out, size, "SAVE FILE    < %c >", 'A' + slot);
            else std::snprintf(out, size, "%s", index == 1 ? "START GAME" : "BACK");
        } else if (page == HOST && index < 3) {
            const SavePreview& save = saves[index];
            if (save.damaged) std::snprintf(out, size, "FILE %c   DAMAGED", 'A' + index);
            else if (save.exists) std::snprintf(out, size, "FILE %c   %d STARS", 'A' + index, save.stars);
            else std::snprintf(out, size, "FILE %c   NEW", 'A' + index);
        } else if (page == HOST || page == JOIN) {
            int offset = page == JOIN ? 1 : 3;
            if (page == JOIN && index == 0) std::snprintf(out, size, "ROOM CODE  %s%s", room_code, editing && row == index ? "_" : "");
            else if (index == offset) std::snprintf(out, size, "CHARACTER  < %s >", chars[character]);
            else std::snprintf(out, size, "%s", index == offset + 1 ? (online_busy?"CONNECTING...":page == HOST ? "CREATE LOBBY" : "JOIN LOBBY") :
                page == HOST && index == 5 ? "SINGLE PLAYER" : "BACK");
        } else if (page == LOBBY) {
            if(index==0) std::snprintf(out,size,"ROOM %s    %d / 7 PLAYERS",room_code,room_count);
            else if(index<=7) std::snprintf(out,size,"%d. %s",index,index<=room_count?room_members[index-1]:"OPEN SEAT");
            else std::snprintf(out,size,"%s",index==9?"LEAVE LOBBY":online_busy?"PLEASE WAIT...":room_host?"START GAME":"WAITING FOR HOST");
        } else if (page == OPTIONS) {
            static const char* categories[] = { "PLAYER", "CAMERA", "CONTROLS", "DISPLAY", "SOUND", "MISC", "SAVE AND BACK" };
            std::snprintf(out, size, "%s", categories[index]);
        } else if (page == MODS) {
            if(index<shown_mods()) {
                int i=mod_page*4+index;
                std::snprintf(out,size,"%s < %s >",mod_names[i],mod_failed[i]?"ERROR":mod_enabled[i]?"ON":"OFF");
            } else if(index==shown_mods()) std::snprintf(out,size,"PAGE < %d / %d >",mod_page+1,(mod_count+3)/4>0?(mod_count+3)/4:1);
            else std::snprintf(out,size,"%s",index==shown_mods()+1?"REFRESH MODS":index==shown_mods()+2?"TEXTURE PACK TOOLS":"BACK");
        } else if(page==TEXTURES) {
            const char* labels[]={capture_textures?"STOP TEXTURE CAPTURE":"START TEXTURE CAPTURE","CREATE PACK FROM CAPTURE","OPEN TEXTURE FOLDERS","BACK"};
            std::snprintf(out,size,"%s",labels[index]);
        } else if(page==PAUSE) {
            const char* labels[]={"RESUME","OPTIONS","MODS","QUIT"};
            std::snprintf(out,size,"%s",labels[index]);
        } else if (page == EXIT_CONFIRM) std::snprintf(out, size, "%s", index == 0 ? "KEEP PLAYING" : "QUIT");
        else if (index == rows() - 1) std::snprintf(out, size, "BACK");
        else if (page == CAMERA) std::snprintf(out, size, "CAMERA         < %s >", cameras[camera]);
        else if (page == PLAYER) {
            if (index == 0) std::snprintf(out, size, "RUN MODE       < %s >", movement_names[movement]);
            else if(index==1)std::snprintf(out,size,"OPEN MODS");
            else if(index==2)std::snprintf(out, size, "PLAYER NAMES   < %s >", names ? "ON" : "OFF");
            else if(index==3)std::snprintf(out,size,"USERNAME  %s%s",username,editing?"_":"");
            else std::snprintf(out,size,"CHARACTER COLORS");
        } else if(page==CONTROLS) {
            if(index==0)std::snprintf(out,size,"ACTION  < %s >",binding_name);
            else if(index==1)std::snprintf(out,size,"SLOT < %d / 3 >  %s",binding_slot+1,binding_labels[binding_slot]);
            else {const char* labels[]={"BIND KEYBOARD KEY","BIND CONTROLLER BUTTON","CLEAR THIS SLOT","RESET ALL CONTROLS"};std::snprintf(out,size,"%s",labels[index-2]);}
        } else if(page==COLORS) {
            const char* parts[]={"HAT","SHIRT / BODY","PANTS","GLOVES","SHOES","SKIN","HAIR","EMBLEM"};
            const char* yoshi_parts[]={"SPIKES","BODY","SADDLE","","SHOES","BELLY / SNOUT","",""};
            if(index==0)std::snprintf(out,size,"CHARACTER  < %s >",chars[color_character]);
            else if(index==1)std::snprintf(out,size,"PART  < %s >",color_character==3?yoshi_parts[color_part]:parts[color_part]);
            else if(index==2)std::snprintf(out,size,"RGB HEX  %s%s",color_hex[0]?color_hex:"DEFAULT",editing?"_":"");
            else if(index<=5) {const char* channel[]={"RED","GREEN","BLUE"};unsigned rgb=color_hex[0]?static_cast<unsigned>(std::strtoul(color_hex,nullptr,16)):0xffffff;
                std::snprintf(out,size,"%s < %3u >",channel[index-3],(rgb>>(8*(5-index)))&255);}
            else std::snprintf(out,size,"%s",index==6?"USE ORIGINAL COLOR":"RESET CHARACTER PALETTE");
        } else if (page == DISPLAY) {
            if (index == 0) {
                if (fps == 0) std::snprintf(out, size, "FRAME RATE     < NATIVE >");
                else std::snprintf(out, size, "FRAME RATE     < %d >", fps);
            } else if (index == 1) std::snprintf(out, size, "SMOOTH MOTION  < %s >", smooth ? "ON" : "OFF");
            else if (index == 2) {
                static const char* backgrounds[] = { "BOB-OMB BATTLEFIELD", "CASTLE GROUNDS", "STAFF ROLL",
                    "WHOMP'S FORTRESS", "COOL COOL MOUNTAIN", "JOLLY ROGER BAY", "LETHAL LAVA LAND", "DIRE DIRE DOCKS" };
                const int custom = background - BACKGROUND_BUILTIN_COUNT;
                const char* name = background >= 0 && background < BACKGROUND_BUILTIN_COUNT ? backgrounds[background] :
                    custom >= 0 && custom < custom_background_count ? custom_backgrounds[custom] : "BOB-OMB BATTLEFIELD";
                std::snprintf(out, size, "BACKGROUND < %s >", name);
            } else if(index==3) {
                static const char* distances[]={"DS DEFAULT","NEAR","MEDIUM","FAR","UNLIMITED"};
                std::snprintf(out,size,"OBJECT DISTANCE < %s >",distances[object_distance]);
            } else std::snprintf(out, size, "%s", index == 4 ? "ADD CUSTOM BACKGROUND" : "REFRESH CUSTOM FILES");
        } else if (page == SOUND) {
            if (index == 0) std::snprintf(out, size, "VOLUME       < %d >", volume);
            else if (index == 1) {
                int custom = music - SONG_COUNT;
                const char* name = custom >= 0 && custom < custom_music_count ? custom_music[custom] : music_name(music);
                std::snprintf(out, size, "MUSIC < %s >", name);
            }
            else if (index == 2) std::snprintf(out, size, "MENU SOUNDS  < %s >", menu_sounds ? "ON" : "OFF");
            else std::snprintf(out, size, "%s", index == 3 ? "NEXT SONG" : index == 4 ? "ADD CUSTOM MUSIC" : "REFRESH CUSTOM FILES");
        }
        else if (page == MISC) std::snprintf(out, size, "CAPTURE MOUSE  < %s >", mouse_capture ? "ON" : "OFF");
        else std::snprintf(out, size, "BACK");
    }
    Action key(Key key) {
        if (editing) {
            if (key == BACK) {
                char* p = field();
                if (p) std::strcpy(p, edit_backup);
            }
            if(key==ACCEPT&&page==PLAYER&&(!username[0]||username[0]==' '||username[std::strlen(username)-1]==' ')){std::snprintf(message,sizeof message,"Use 1-16 characters, without leading/trailing spaces.");return NONE;}
            if(key==ACCEPT&&page==COLORS&&color_hex[0]&&std::strlen(color_hex)!=6){std::snprintf(message,sizeof message,"Enter 6 hex digits, e.g. ff6600.");return NONE;}
            if (key == BACK || key == ACCEPT) editing = false;
            return NONE;
        }
        if (key == BACK) {
            if(page==LOBBY) return LEAVE_ROOM;
            if(page==PAUSE)return RESUME;
            if(page==MODS){open(in_game?PAUSE:HOME);return NONE;}
            if(page==TEXTURES){open(MODS);return NONE;}
            if (page == OPTIONS || page == PLAYER || page == CAMERA ||
                page == DISPLAY || page == SOUND || page == MISC || page == CONTROLS || page == COLORS) {
                after_save = page == OPTIONS ? (in_game?PAUSE:HOME) : page==COLORS?PLAYER:OPTIONS;
                return SAVE_OPTIONS;
            }
            open(page == HOME ? EXIT_CONFIRM : page == PLAY ? HOST :
                 page == CONTROLS ? OPTIONS : in_game?PAUSE:HOME);
            return NONE;
        }
        if (key == UP || key == DOWN) {
            row = (row + rows() + (key == UP ? -1 : 1)) % rows(); return NONE;
        }
        const int delta = key == LEFT ? -1 : 1;
        const bool change = key == LEFT || key == RIGHT || key == ACCEPT;
        if (!change) return NONE;
        if (page == HOME && key == ACCEPT) {
            static const Page pages[] = { HOST, JOIN, MODS, OPTIONS, EXIT_CONFIRM };
            open(pages[row]);
        } else if(page==MODS) {
            if(row<shown_mods())return TOGGLE_MOD;
            if(row==shown_mods()) {
                int pages=(mod_count+3)/4; if(pages<1)pages=1;
                mod_page=(mod_page+pages+delta)%pages; row=shown_mods();
            } else if(key==ACCEPT) {
                if(row==shown_mods()+1)return RELOAD_MODS;
                if(row==shown_mods()+2)open(TEXTURES);else open(in_game?PAUSE:HOME);
            }
        } else if(page==TEXTURES && key==ACCEPT) {
            if(row==0)return TEXTURE_CAPTURE;
            if(row==1)return TEXTURE_PACK;
            if(row==2)return OPEN_TEXTURE_FOLDER;
            open(MODS);
        } else if(page==PAUSE && key==ACCEPT) {
            if(row==0)return RESUME;
            open(row==1?OPTIONS:row==2?MODS:EXIT_CONFIRM);
        } else if (page == PLAY) {
            if (row == 0) slot = (slot + 3 + delta) % 3;
            else if (key == ACCEPT && row == 1) {
                if (saves[slot].damaged) std::snprintf(message, sizeof message, "Save damaged. Choose another slot.");
                else return START_SOLO;
            }
            else if (key == ACCEPT) open(HOST);
        } else if (page == HOST || page == JOIN) {
            const int offset = page == JOIN ? 1 : 3;
            if (page == HOST && row < 3) {
                if (key == LEFT || key == RIGHT) row = (row + 3 + delta) % 3;
                slot = row;
                return NONE;
            }
            if (field() && key == ACCEPT) {
                std::strcpy(edit_backup, field()); editing = true; field()[0] = 0;
            }
            else if (row == offset) character = (character + 4 + delta) % 4;
            else if (key == ACCEPT && row == offset + 1) {
                if (page == HOST && saves[slot].damaged) std::snprintf(message, sizeof message, "Save damaged. Choose another slot.");
                else if (online_busy) return NONE;
                else if (page == JOIN && std::strlen(room_code)!=6) std::snprintf(message, sizeof message, "Enter the host's six-character room code.");
                else return page == HOST ? CREATE_ROOM : JOIN_ROOM;
            } else if (key == ACCEPT && page == HOST && row == 5) open(PLAY);
            else if (key == ACCEPT && row == rows() - 1) open(HOME);
        } else if(page==LOBBY && key==ACCEPT) {
            if(row==9) return LEAVE_ROOM;
            if(row==8 && room_host && !online_busy) return START_ROOM;
        } else if (page == OPTIONS) {
            if (key == ACCEPT) {
                static const Page categories[] = { PLAYER, CAMERA, CONTROLS, DISPLAY, SOUND, MISC };
                if (row == 6) { after_save = in_game?PAUSE:HOME; return SAVE_OPTIONS; }
                open(categories[row]);
            }
        } else if(page==CONTROLS) {
            if(row==0)binding_action=(binding_action+14+delta)%14;
            else if(row==1)binding_slot=(binding_slot+3+delta)%3;
            else if(key==ACCEPT){if(row==6){after_save=OPTIONS;return SAVE_OPTIONS;}const Action actions[]={BIND_KEY,BIND_PAD,CLEAR_BIND,RESET_BINDS};return actions[row-2];}
        } else if(page==COLORS) {
            if(row==0){color_character=(color_character+4+delta)%4;if(color_character==3&&(color_part==3||color_part>5))color_part=1;}
            else if(row==1){do {color_part=(color_part+8+delta)%8;}while(color_character==3&&(color_part==3||color_part>5));}
            else if(row==2&&key==ACCEPT){std::strcpy(edit_backup,color_hex);editing=true;color_hex[0]=0;}
            else if(row>=3&&row<=5){unsigned rgb=color_hex[0]?static_cast<unsigned>(std::strtoul(color_hex,nullptr,16)):0xffffff;unsigned shift=8*(5-row);
                int channel=static_cast<int>((rgb>>shift)&255)+delta*4;if(channel<0)channel=0;if(channel>255)channel=255;
                rgb=(rgb&~(255u<<shift))|(static_cast<unsigned>(channel)<<shift);std::snprintf(color_hex,sizeof color_hex,"%06x",rgb);}
            else if(key==ACCEPT){if(row==8){after_save=PLAYER;return SAVE_OPTIONS;}return row==6?DEFAULT_COLOR:RESET_COLORS;}
        } else if (page == PLAYER || page == CAMERA || page == DISPLAY || page == SOUND || page == MISC) {
            if (row == rows() - 1) {
                if (key == ACCEPT) { after_save = OPTIONS; return SAVE_OPTIONS; }
            } else if (page == CAMERA) camera = (camera + 4 + delta) % 4;
            else if (page == PLAYER) {
                if (row == 0) movement = (movement + 3 + delta) % 3;
                else if(row==1)open(MODS);
                else if(row==2) names = !names;
                else if(row==3&&key==ACCEPT){std::strcpy(edit_backup,username);editing=true;username[0]=0;}
                else if(row==4&&key==ACCEPT)open(COLORS);
            } else if (page == DISPLAY && row == 0) {
                const int rates[] = { 0, 60, 90, 120, 144, 240 };
                int index = 0;
                for (int i = 0; i < 6; ++i) if (fps >= rates[i]) index = i;
                fps = rates[(index + delta + 6) % 6];
            } else if (page == DISPLAY) {
                if (row == 1) smooth = !smooth;
                else if (row == 2) background = (background + BACKGROUND_BUILTIN_COUNT + custom_background_count + delta) %
                    (BACKGROUND_BUILTIN_COUNT + custom_background_count);
                else if(row==3)object_distance=(object_distance+5+delta)%5;
                else if (key == ACCEPT) return row == 4 ? IMPORT_BACKGROUND : RELOAD_MEDIA;
            } else if (page == SOUND) {
                if (row == 0) { volume += delta * 10; if (volume < 0) volume = 0; if (volume > 100) volume = 100; }
                else if (row == 1) music = music_step(music, delta, custom_music_count);
                else if (row == 2) menu_sounds = !menu_sounds;
                else if (key == ACCEPT) return row == 3 ? NEXT_SONG : row == 4 ? IMPORT_MUSIC : RELOAD_MEDIA;
            }
            else if (page == MISC) mouse_capture = !mouse_capture;
        } else if (page == EXIT_CONFIRM && key == ACCEPT) {
            if (row == 1) return QUIT;
            open(in_game?PAUSE:HOME);
        }
        return NONE;
    }
};
} // namespace coop_frontend
