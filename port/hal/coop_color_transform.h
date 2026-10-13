#pragma once
#include "vs_char_palette_gen.h"
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <cstdlib>

namespace coop_colors {
using Palette=std::array<std::string,8>;
inline bool parse(const std::string& text,Palette& result) {
    Palette out;size_t start=0;
    for(int p=0;p<8;++p){size_t end=text.find(':',start);
        if((p<7&&end==std::string::npos)||(p==7&&end!=std::string::npos))return false;
        out[p]=text.substr(start,end==std::string::npos?end:end-start);
        if(!out[p].empty()){if(out[p].size()!=6)return false;for(char c:out[p])if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;}
        start=end+1;
    }result=out;return true;
}
inline unsigned long long hash(const Palette& p,int character){
    unsigned long long h=1469598103934665603ull;bool any=false;
    for(const auto& part:p){for(unsigned char c:part){h=(h^c)*1099511628211ull;any=true;}h=(h^':')*1099511628211ull;}
    return any?(h^static_cast<unsigned>(character+1)):0;
}
// Classify the stock texture before transforming it. Eyes keep their whites
// and pupils; cap emblems are separate from teeth and the main cap color.
inline int part(int character,const char* texture,int x,int y,int w,int h,uint32_t rgb) {
    int r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
    int hi=(std::max)(r,(std::max)(g,b)),lo=(std::min)(r,(std::min)(g,b));
    bool eye=std::strstr(texture,"eye")!=nullptr,body=std::strstr(texture,"body")!=nullptr;
    float u=static_cast<float>(x)/w,v=static_cast<float>(y)/h;
    bool skin=r>g*1.08f&&g>b*1.15f&&r>140&&g>85&&b>45;
    if(character==3){
        if(g>r*1.2f&&g>b*1.2f)return 1; // body / head, including eyelids
        if(eye)return -1;
        if(hi-lo<35&&hi>110)return 5; // white belly / snout
        if(r>g*1.4f&&g>b*1.4f&&g>45)return 4; // orange shoes
        if(r>g*1.8f&&r>b*1.8f){if(u<.26f&&v<.6f)return 0;if(u>.65f&&v>.25f&&v<.7f)return 2;}
        return -1;
    }
    if(eye){if(skin)return 5;if(r>g*1.25f&&g>b&&hi<150)return 6;return -1;}
    // The body atlas uses warm browns for shoes as well as skin. Separate
    // their stock UV regions before the hue test, including the dark soles.
    if(body&&character==0&&v>=.75f){if(u<.1875f)return 5;if(u<.625f)return 4;return 3;}
    if(body&&character==1&&u>=.65f&&v>=.32f&&v<.68f)return 4;
    if(skin&&!(body&&character==1))return 5;
    if(!body){
        bool emblem=character!=2||(u<.27f&&v<.38f);
        if(hi-lo<45&&lo>130&&emblem)return 7;
        if(character==2&&b>r*1.15f&&b>g*1.3f&&emblem)return 7;
        if(character==0&&r>g*1.5f&&r>b*1.5f)return 0;
        if(character==1&&g>r*1.15f&&g>b*1.15f)return 0;
        if(character==2&&r>b*1.8f&&g>b*1.8f&&g>r*.55f)return 0;
        if(hi<70||(r>g*1.15f&&g>=b&&hi<200))return 6;
        return -1;
    }
    if(hi-lo<40&&hi>130)return 3;
    if(character<2&&b>r*1.2f&&b>g*1.08f)return 2;
    if(character==2&&r>g*1.4f&&b>g*1.2f)return 2;
    if(character==0&&r>g*1.7f&&r>b*1.7f)return 1;
    if(character==1&&g>r*1.25f&&g>b*1.25f)return 1;
    if(character==2&&r>b*1.7f&&g>b*1.7f&&g>r*.6f)return 1;
    if(hi<90||(r>g*1.12f&&g>b*1.12f)||(character==2&&g>r*1.3f&&g>b*1.3f))return 4;
    return -1;
}
inline void recolor(std::vector<uint32_t>& pixels,int w,int h,int character,const char* texture,const Palette& p) {
    if(!hash(p,character)||w<=0||h<=0||pixels.size()!=static_cast<size_t>(w)*h)return;
    std::array<std::vector<int>,8> groups;std::vector<vspal::u16> colors(pixels.size());
    for(size_t i=0;i<pixels.size();++i){uint32_t c=pixels[i];colors[i]=static_cast<vspal::u16>(((c>>19)&31)|(((c>>11)&31)<<5)|(((c>>3)&31)<<10));
        if((c>>24)==0)continue;int k=part(character,texture,static_cast<int>(i%w),static_cast<int>(i/w),w,h,c);
        if(k>=0&&!p[k].empty())groups[k].push_back(static_cast<int>(i));
    }
    for(int k=0;k<8;++k)if(!groups[k].empty()){
        vspal::u8 target[3];for(int j=0;j<3;++j)target[j]=static_cast<vspal::u8>(std::strtoul(p[k].substr(j*2,2).c_str(),nullptr,16));
        vspal::char_recolor_palette(colors.data(),static_cast<int>(colors.size()),groups[k].data(),static_cast<int>(groups[k].size()),target);
        for(int i:groups[k]){unsigned c=colors[i];pixels[i]=(pixels[i]&0xff000000u)|((c&31)*255/31<<16)|(((c>>5)&31)*255/31<<8)|(((c>>10)&31)*255/31);}
    }
}
}
