#pragma once
#include "coop_online.h"
#include <array>

namespace coop_prefs {
enum Action { UP,DOWN,LEFT,RIGHT,JUMP,ATTACK,CROUCH,RUN,START,SELECT,LOOK,CAMERA_LEFT,CAMERA_RIGHT,CHAT,COUNT };
inline const char* action_name(int a) {
    static const char* n[]={"MOVE UP","MOVE DOWN","MOVE LEFT","MOVE RIGHT","JUMP / DS B","ATTACK / DS A","CROUCH","RUN / DS Y","PAUSE / START","SELECT","LOOK / DS X","CAMERA LEFT","CAMERA RIGHT","CHAT"};
    return a>=0&&a<COUNT?n[a]:"";
}
struct Preferences {
    std::string name="Player",id,path;
    std::array<std::array<int,3>,COUNT> bindings{};
    bool custom_controls=false;
    // Empty colors preserve the cartridge palette. Per-character RGB parts.
    std::array<std::array<std::string,8>,4> colors{};
    void defaults() {
        const int keys[COUNT][3]={{'W',VK_UP,-1},{'S',VK_DOWN,-2},{'A',VK_LEFT,-4},{'D',VK_RIGHT,-8},
            {VK_SPACE,0,-0x1000},{'X',0,-0x2000},{VK_CONTROL,0,-0x20000},{VK_SHIFT,0,-0x4000},
            {VK_RETURN,0,-0x10},{VK_BACK,0,0},{'Z',0,-0x8000},{'Q',0,-0x100},{'E',0,-0x200},{'T',0,0}};
        for(int a=0;a<COUNT;++a)for(int s=0;s<3;++s)bindings[a][s]=keys[a][s];
    }
    static bool valid_name(const std::string& n) {
        if(n.empty()||n.size()>16||n.front()==' '||n.back()==' ')return false;
        for(unsigned char c:n)if(c<32||c>126||c==','||c=='|')return false;
        return true;
    }
    static bool color(const std::string& s) {
        if(s.empty())return true;if(s.size()!=6)return false;
        for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return true;
    }
    Preferences(){defaults();}
    void load() {
        const char* app=getenv("APPDATA");if(!app||!*app)throw std::runtime_error("Windows AppData unavailable");
        path=std::string(app)+"\\SM64DS-Coop\\player.json";
        std::string text=coop_online::read_file(path);
        if(!text.empty())try {
            const auto j=coop_online::Parser(text).parse();
            if(valid_name(j.at("username").str()))name=j.at("username").str();
            id=j.at("player_id").str();
            auto b=j.at("bindings");if(b.array.size()==COUNT) {
                bool valid=true;auto tmp=bindings;
                for(int a=0;a<COUNT;++a) {
                    if(b.array[a].array.size()!=3){valid=false;break;}
                    for(int s=0;s<3;++s) {
                        const auto& v=b.array[a].array[s];int n=v.integer();
                        if(v.kind!=coop_online::Json::NUMBER||n>255||n< -0x3ffff){valid=false;break;}tmp[a][s]=n;
                    }
                }
                if(valid){bindings=tmp;custom_controls=true;}
            }
            const auto& palettes=j.at("palettes").array;
            if(palettes.size()==4)for(int c=0;c<4;++c)if(palettes[c].array.size()==8)
                for(int p=0;p<8;++p)if(color(palettes[c].array[p].str()))colors[c][p]=palettes[c].array[p].str();
        }catch(const std::exception&){/* Keep usable defaults if the preference file is damaged. */}
        bool valid=id.size()==32;for(char c:id)valid=valid&&((c>='0'&&c<='9')||(c>='a'&&c<='f'));
        if(!valid){coop_online::Identity random;BYTE bytes[16];random.random(bytes,16);id.clear();for(BYTE b:bytes){char v[3];std::snprintf(v,sizeof v,"%02x",b);id+=v;}}
    }
    bool save() {
        if(path.empty())return false;
        auto slash=path.find_last_of('\\');CreateDirectoryA(path.substr(0,slash).c_str(),nullptr);
        std::string tmp=path+"."+std::to_string(GetCurrentProcessId())+".tmp";
        std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f)return false;
        f<<"{\"username\":"<<coop_online::quote(name)<<",\"player_id\":"<<coop_online::quote(id);
        if(custom_controls){f<<",\"bindings\":[";for(int a=0;a<COUNT;++a){if(a)f<<",";f<<"["<<bindings[a][0]<<","<<bindings[a][1]<<","<<bindings[a][2]<<"]";}f<<"]";}
        f<<",\"palettes\":[";for(int c=0;c<4;++c){if(c)f<<",";f<<"[";for(int p=0;p<8;++p){if(p)f<<",";f<<coop_online::quote(colors[c][p]);}f<<"]";}f<<"]}";f.close();
        if(!f){DeleteFileA(tmp.c_str());return false;}
        bool ok=MoveFileExA(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
        if(!ok)DeleteFileA(tmp.c_str());return ok;
    }
    std::string palette(int chr) const {std::string out;for(int p=0;p<8;++p){if(p)out+=':';out+=colors[chr][p];}return out;}
};
inline Preferences& get(){static Preferences prefs;static bool loaded=false;if(!loaded){prefs.load();loaded=true;}return prefs;}
}
