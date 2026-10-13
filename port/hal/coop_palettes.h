#pragma once
#include <array>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cctype>

namespace coop_palettes {
using Colors = std::array<std::string,8>;
inline Colors stock(int character) {
    static const char* values[4][8] = {
        {"ff0000","ff0000","0000ff","ffffff","721c0e","fec179","730600","ffffff"},
        {"00b000","00b000","0000ff","ffffff","721c0e","fec179","730600","ffffff"},
        {"ffff00","ffff00","8000ff","ffffff","209020","fec179","402000","6040c0"},
        {"ff0000","00b000","ff0000","ffffff","ff8000","ffffff","000000","ffffff"}
    };
    Colors result; for(int p=0;p<8;++p) result[p]=values[character>=0&&character<4?character:0][p]; return result;
}
inline std::string trim(std::string value) {
    size_t a=value.find_first_not_of(" \t\r\n"),b=value.find_last_not_of(" \t\r\n");
    return a==std::string::npos?"":value.substr(a,b-a+1);
}
// The official CoopDX [PALETTE] INI format. Pants and overalls share one part;
// our existing wire order stays unchanged, so importing never swaps body parts.
inline bool read(const std::string& text, Colors& result) {
    if(text.size()>65536)return false;
    const char* parts[]={"CAP","SHIRT","PANTS","GLOVES","SHOES","SKIN","HAIR","EMBLEM"};
    bool seen[8][3]={},section=false;unsigned rgb[8][3]={};std::istringstream input(text);std::string line;
    while(std::getline(input,line)) {
        line=trim(line);if(line.empty()||line[0]=='#'||line[0]==';')continue;
        if(line[0]=='['){section=line=="[PALETTE]";continue;}if(!section)continue;
        size_t equal=line.find('=');if(equal==std::string::npos)return false;
        std::string key=trim(line.substr(0,equal)),value=trim(line.substr(equal+1));
        size_t comment=value.find_first_of(";#");if(comment!=std::string::npos)value=trim(value.substr(0,comment));
        bool found=false;
        for(int p=0;p<8;++p)for(int c=0;c<3;++c)if(key==std::string(parts[p])+"_"+"RGB"[c]) {
            if(seen[p][c]||value.empty()||value.size()>3)return false;
            for(char n:value)if(n<'0'||n>'9')return false;
            unsigned n=static_cast<unsigned>(std::strtoul(value.c_str(),nullptr,10));if(n>255)return false;
            rgb[p][c]=n;seen[p][c]=true;found=true;
        }
        (void)found; // Future metadata is harmless; required channels remain strict.
    }
    Colors out;for(int p=0;p<8;++p){for(int c=0;c<3;++c)if(!seen[p][c])return false;
        char hex[7];std::snprintf(hex,sizeof hex,"%02x%02x%02x",rgb[p][0],rgb[p][1],rgb[p][2]);out[p]=hex;}
    result=out;return true;
}
inline std::string write(const Colors& colors,int character) {
    const char* parts[]={"CAP","SHIRT","PANTS","GLOVES","SHOES","SKIN","HAIR","EMBLEM"};
    auto defaults=stock(character);std::ostringstream out;out<<"[PALETTE]\n";
    for(int p=0;p<8;++p){unsigned rgb=static_cast<unsigned>(std::strtoul((colors[p].empty()?defaults[p]:colors[p]).c_str(),nullptr,16));
        for(int c=0;c<3;++c)out<<parts[p]<<"_"<<"RGB"[c]<<" = "<<((rgb>>(16-8*c))&255)<<"\n";}
    return out.str();
}
}
