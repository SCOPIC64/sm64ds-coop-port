#pragma once
#include "coop_frontend.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

namespace coop_frontend {
inline uint32_t composite_logo(uint32_t source, uint32_t destination) {
    const unsigned alpha=source>>24, inverse=255-alpha;
    if(!alpha)return destination;
    return 0xff000000u|
        (((((source>>16)&255)*alpha+((destination>>16)&255)*inverse+127)/255)<<16)|
        (((((source>>8)&255)*alpha+((destination>>8)&255)*inverse+127)/255)<<8)|
        (((source&255)*alpha+(destination&255)*inverse+127)/255);
}
struct Canvas {
    uint32_t* pixels;
    int width, height, stride;
    const unsigned char (*font)[8];
    void (*smooth_text)(uint32_t*,int,int,int,int,int,const char*,uint32_t,int)=nullptr;
    int (*text_width)(const char*,int)=nullptr;
    const uint32_t* logo=nullptr;
    int logo_width=0,logo_height=0;
    bool valid() const { return pixels && font && width > 0 && height > 0 && stride >= width; }
    int scale() const { int s = height / 256; if (s > width / 384) s = width / 384; return s < 1 ? 1 : s; }
    void box(int x, int y, int w, int h, uint32_t color) const {
        for (int py = y < 0 ? 0 : y; py < y + h && py < height; ++py)
            for (int px = x < 0 ? 0 : x; px < x + w && px < width; ++px) pixels[py * stride + px] = color;
    }
    void shade(int x, int y, int w, int h, unsigned opacity) const {
        for (int py = y < 0 ? 0 : y; py < y + h && py < height; ++py)
            for (int px = x < 0 ? 0 : x; px < x + w && px < width; ++px) {
                uint32_t v = pixels[py * stride + px]; unsigned a = 255 - opacity;
                pixels[py * stride + px] = 0xff000000u |
                    ((((v >> 16) & 255) * a / 255) << 16) | ((((v >> 8) & 255) * a / 255) << 8) | ((v & 255) * a / 255);
            }
    }
    void text(int x, int y, const char* str, uint32_t color, int size) const {
        if(smooth_text){smooth_text(pixels,width,height,stride,x,y,str,color,size);return;}
        for (; *str; ++str, x += 6 * size) {
            unsigned char ch = static_cast<unsigned char>(*str);
            if (ch < 32 || ch > 126) continue;
            for (int r = 0; r < 8; ++r) for (int col = 0; col < 8; ++col)
                if (font[ch - 32][r] & (0x80 >> col)) box(x + col * size, y + r * size, size, size, color);
        }
    }
    void shadow(int x, int y, const char* str, uint32_t color, int size) const {
        text(x + size, y + size, str, 0xff101020u, size); text(x, y, str, color, size);
    }
    void centered(int y, const char* str, uint32_t color, int size) const {
        shadow((width - measure(str,size)) / 2, y, str, color, size);
    }
    int measure(const char* str,int size) const {return text_width?text_width(str,size):static_cast<int>(std::strlen(str))*6*size;}
    void rounded(int x,int y,int w,int h,int radius,uint32_t color,unsigned opacity=255) const {
        if(radius>h/2)radius=h/2;
        if(radius>w/2)radius=w/2;
        for(int py=(std::max)(0,y);py<(std::min)(height,y+h);++py)for(int px=(std::max)(0,x);px<(std::min)(width,x+w);++px) {
            int dx=px<x+radius?x+radius-px:px>=x+w-radius?px-(x+w-radius-1):0;
            int dy=py<y+radius?y+radius-py:py>=y+h-radius?py-(y+h-radius-1):0;
            if(dx*dx+dy*dy>radius*radius)continue;
            uint32_t old=pixels[py*stride+px];unsigned a=opacity;
            pixels[py*stride+px]=0xff000000u|((((old>>16&255)*(255-a)+(color>>16&255)*a)/255)<<16)|
                ((((old>>8&255)*(255-a)+(color>>8&255)*a)/255)<<8)|(((old&255)*(255-a)+(color&255)*a)/255);
        }
    }
};
struct ButtonRect { int x, y, w, h; };
inline int panel_width(const Canvas& c, const Menu& m) { return m.page == HOME ? c.width * 46 / 100 : c.width * 90 / 100; }
inline int panel_left(const Canvas& c, const Menu& m) { return m.page == HOME ? c.width / 30 : (c.width - panel_width(c,m)) / 2; }
inline int button_left(const Canvas& c, const Menu& m) { return panel_left(c,m) + c.width / 30; }
inline int button_width(const Canvas& c, const Menu& m) { return panel_width(c,m) - c.width / 15; }
inline int row_top(const Canvas& c, const Menu& m) { return c.height * (m.page == HOME ? 40 : m.page==COLORS?24:m.page==CONTROLS?25:27) / 100; }
inline int row_height(const Canvas& c) { int h = c.height * 7 / 100; return h > 1 ? h : 2; }
inline const char* control_name(int row) {
    static const char* names[]={"Up","Down","Left","Right","Jump (DS B)","Attack (DS A)","Crouch","Run (DS Y)","Start / Pause","Select","Look (DS X)","Camera Left","Camera Right","Chat"};
    return row>=0&&row<14?names[row]:"";
}
inline ButtonRect control_cell(const Canvas& c,const Menu& m,int action,int slot) {
    int left=button_left(c,m),width=button_width(c,m),rh=c.height*4/100;
    if(rh<9)rh=9;
    int label=width*31/100,cell=(width-label)/3;
    return {left+label+slot*cell,c.height*25/100+action*rh,cell-2,rh-2};
}
inline int hit_control_slot(const Canvas& c,const Menu& m,int row,int x,int y) {
    for(int s=0;s<3;++s){auto a=control_cell(c,m,row,s);if(x>=a.x&&x<a.x+a.w&&y>=a.y&&y<a.y+a.h)return s;}return -1;
}
inline ButtonRect button_rect(const Canvas& c, const Menu& m, int row) {
    ButtonRect r = {button_left(c,m), row_top(c,m) + row * row_height(c), button_width(c,m), row_height(c) * 4 / 5};
    if(m.page==CONTROLS) {
        int rh=(std::max)(9,c.height*4/100);
        r.y=c.height*25/100+row*rh;r.h=rh-2;
    }
    if(m.page==LOBBY) { r.y=c.height*27/100+row*c.height*5/100; r.h=c.height*4/100; }
    if(m.page==COLORS) { r.y=c.height*24/100+row*c.height*5/100; r.h=c.height*4/100; }
    if (m.page == HOST) {
        if (row < 3) {
            const int cell = button_width(c,m) / 3;
            r.x += row * cell; r.y = c.height * 27 / 100; r.w = cell - c.scale() * 3; r.h = c.height * 16 / 100;
        } else { r.y = c.height * 49 / 100 + (row-3) * row_height(c); }
    }
    return r;
}
inline int hit_row(const Canvas& c, const Menu& m, int x, int y) {
    if (!c.valid() || x < 0 || y < 0 || x >= c.width || y >= c.height) return -1;
    for (int row=0; row<m.rows(); ++row) {
        ButtonRect r=button_rect(c,m,row);
        if (x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h) return row;
    }
    return -1;
}
inline void panel_text(const Canvas& c, const Menu& m, int y, const char* str, uint32_t color, int size) {
    c.shadow(panel_left(c,m)+(panel_width(c,m)-c.measure(str,size))/2,y,str,color,size);
}
inline void hand(const Canvas& c, int x, int y, int s) {
    static const unsigned char rows[] = { 0x30,0x38,0xfc,0xfe,0xff,0xfe,0x7c,0x38 };
    for (int yy=0; yy<8; ++yy) for (int xx=0; xx<8; ++xx)
        if (rows[yy] & (0x80>>xx)) c.box(x+xx*s+s,y+yy*s+s,s,s,0xff101020u);
    for (int yy=0; yy<8; ++yy) for (int xx=0; xx<8; ++xx)
        if (rows[yy] & (0x80>>xx)) c.box(x+xx*s,y+yy*s,s,s,0xffffffffu);
}
inline void draw(const Canvas& c, const Menu& menu, unsigned frame, const char* loading=nullptr,
                 const uint32_t* backdrop=nullptr, int bw=512, int bh=384, int bs=0) {
    if (!c.valid()) return;
    const int s=c.scale();
    for (int y=0; y<c.height; ++y) for (int x=0; x<c.width; ++x) {
        uint32_t v;
        if (backdrop && bw>0 && bh>0) v=backdrop[(y*bh/c.height)*(bs?bs:bw)+x*bw/c.width];
        else {
            // The menu remains usable before ROM import or while a scene loads.
            unsigned red=50+static_cast<unsigned>(y*30/c.height), green=110+static_cast<unsigned>(y*35/c.height);
            v=0xff000000u | (red<<16) | (green<<8) | 190;
        }
        c.pixels[y*c.stride+x]=v;
    }
    const int px=panel_left(c,menu), pw=panel_width(c,menu);
    c.rounded(px,c.height/12,pw,c.height*79/100,12*s,0xff101724u,205);
    if (menu.page==HOME) {
      if(c.logo && c.logo_width>0 && c.logo_height>0) {
        int h=c.height*27/100,w=h*c.logo_width/c.logo_height;
        if(w>pw*90/100){w=pw*90/100;h=w*c.logo_height/c.logo_width;}
        int left=px+(pw-w)/2,top=c.height*9/100;
        for(int y=0;y<h;++y)for(int x=0;x<w;++x) {
            uint32_t& destination=c.pixels[(top+y)*c.stride+left+x];
            destination=composite_logo(c.logo[(y*c.logo_height/h)*c.logo_width+x*c.logo_width/w],destination);
        }
      } else {
        const char* title="SUPER MARIO"; const int size=s*2;
        const uint32_t colors[]={0xfff34b46u,0xff62caffu,0xffffdc47u,0xff70df6bu};
        int x=px+(pw-static_cast<int>(std::strlen(title))*6*size)/2;
        for (int i=0; title[i]; ++i) { char one[]={title[i],0}; c.shadow(x,c.height*16/100,one,colors[i%4],size); x+=6*size; }
        panel_text(c,menu,c.height*24/100,"64 DS CO-OP",0xffffffffu,size);
        panel_text(c,menu,c.height*32/100,"MAIN MENU",0xffffdf60u,s);
      }
    } else panel_text(c,menu,c.height*16/100,menu.title(),0xffffdf60u,s*2);
    if (loading) {
        panel_text(c,menu,c.height*45/100,loading,0xffffffffu,s);
        panel_text(c,menu,c.height*57/100,"PLEASE WAIT...",0xffeeeeeeu,s);
    } else {
        static Page last_page=HOME;static int last_height=0;static double focus_y=0;
        ButtonRect focus=button_rect(c,menu,menu.row);
        if(last_page!=menu.page||last_height!=c.height){focus_y=focus.y;last_page=menu.page;last_height=c.height;}
        else focus_y+=(focus.y-focus_y)*0.28;
        for (int i=0;i<menu.rows();++i) {
            ButtonRect r=button_rect(c,menu,i); bool selected=i==menu.row;
            if(menu.page==CONTROLS&&i<14) {
                int size=(std::max)(1,(r.h-2)/11);
                c.text(r.x+4,r.y,control_name(i),0xffe7ecf4u,size);
                for(int slot=0;slot<3;++slot) {
                    auto cell=control_cell(c,menu,i,slot);
                    bool cell_focus=selected&&slot==menu.binding_slot;
                    c.rounded(cell.x,cell.y,cell.w,cell.h,2,cell_focus?0xff426f96u:0xff293449u);
                    if(cell_focus)c.box(cell.x,cell.y,cell.w,1,0xff5ae1c1u);
                    const char* name=menu.control_labels[i][slot];
                    int text_size=size;while(text_size>1&&c.measure(name,text_size)>cell.w-6)--text_size;
                    c.text(cell.x+(cell.w-c.measure(name,text_size))/2,cell.y,name,cell_focus?0xffffffffu:0xffced9eau,text_size);
                }
                continue;
            }
            if(menu.page==CONTROLS) {
                c.rounded(r.x,r.y,r.w,r.h,2,selected?0xff426f96u:0xff293449u);
                const char* text=i==14?"Reset Binds":"Back";
                c.text(r.x+(r.w-c.measure(text,s))/2,r.y,text,0xffffffffu,s);continue;
            }
            if (menu.page==HOST && i<3) {
                c.shade(r.x,r.y,r.w,r.h,selected?95:60);
                uint32_t border=i==menu.slot?0xffffd84du:0xffbbbbc9u;
                c.box(r.x,r.y,r.w,s,border); c.box(r.x,r.y+r.h-s,r.w,s,border);
                c.box(r.x,r.y,s,r.h,border); c.box(r.x+r.w-s,r.y,s,r.h,border);
                char file[12]; std::snprintf(file,sizeof file,"FILE %c",'A'+i);
                c.shadow(r.x+8*s,r.y+8*s,file,selected?0xffffdf60u:0xffffffffu,s);
                const SavePreview& save=menu.saves[i]; char detail[32];
                if(save.damaged) std::snprintf(detail,sizeof detail,"CHECK SAVE");
                else if(save.exists) std::snprintf(detail,sizeof detail,"%d STARS",save.stars);
                else std::snprintf(detail,sizeof detail,"NEW GAME");
                c.shadow(r.x+8*s,r.y+r.h/2,detail,save.damaged?0xffff8585u:0xffffffffu,s);
                if(i==menu.slot) c.shadow(r.x+8*s,r.y+r.h-12*s,"SELECTED",0xffffdf60u,s);
                continue;
            }
            c.rounded(r.x,r.y,r.w,r.h,5*s,selected?0xff345171u:0xff1b273bu,selected?235:160);
            if(selected)c.rounded(r.x,static_cast<int>(focus_y),3*s,r.h,s,0xff5ae1c1u);
            char label[80]; menu.label(i,label,sizeof label);
            if(menu.page==COLORS&&i>=3&&i<=5) {
                const char* channels[]={"Red","Green","Blue"};unsigned rgb=static_cast<unsigned>(std::strtoul(menu.color_hex,nullptr,16));
                int value=(rgb>>(8*(5-i)))&255,bx=r.x+r.w*27/100,bar_width=r.w*55/100,by=r.y+r.h/2;
                c.text(r.x+8*s,r.y+(r.h-10*s)/2,channels[i-3],0xffe7ecf4u,s);
                c.rounded(bx,by-2*s,bar_width,4*s,2*s,0xff51627au);
                const uint32_t colors[]={0xffff6f7du,0xff58dba5u,0xff65b9ffu};
                c.rounded(bx,by-2*s,bar_width*value/255,4*s,2*s,colors[i-3]);
                c.rounded(bx+bar_width*value/255-3*s,by-5*s,6*s,10*s,3*s,0xfff6f9ffu);
                char number[8];std::snprintf(number,sizeof number,"%d",value);c.text(r.x+r.w*86/100,r.y+(r.h-10*s)/2,number,0xffffffffu,s);
                continue;
            }
            int size=(menu.page==COLORS||menu.page==LOBBY)?s:s*2; while(size>1 && (c.measure(label,size)>r.w-22*s||10*size>r.h)) --size;
            const int x=menu.page==HOME?r.x+12*s:r.x+(r.w-c.measure(label,size))/2;
            c.text(x,r.y+(r.h-10*size)/2,label,selected?0xffffffffu:0xffced9eau,size);
        }
    }
    const char* detail=nullptr;
    if(menu.page==MODS && menu.row<menu.shown_mods())detail=menu.mod_descriptions[menu.selected_mod()];
    if(menu.page==TEXTURES)detail="CAPTURE WHILE PLAYING, THEN EDIT THE PNGS";
    if(menu.page==CONTROLS)detail="ENTER / CLICK: BIND   DELETE: CLEAR   LEFT / RIGHT: SLOT";
    if(menu.page==COLORS) {
        detail="DRAG RGB SLIDERS - LEFT/RIGHT: ADJUST - ENTER: HEX";
        unsigned rgb=menu.color_hex[0]?static_cast<unsigned>(std::strtoul(menu.color_hex,nullptr,16)):0xffffff;
        int sw=c.width/20,sh=c.height/22;
        c.rounded(c.width*85/100-s,c.height*16/100-s,sw+2*s,sh+2*s,3*s,0xffffffffu);
        c.rounded(c.width*85/100,c.height*16/100,sw,sh,2*s,0xff000000u|rgb);
    }
    if(menu.page==CAMERA && menu.camera==3)detail="Q/E: TURN  R: C-UP  F: OUT  V: MARIO/LAKITU";
    if(menu.page==DISPLAY && menu.row==3)detail="NEAR: FEWER OBJECTS   FAR: MORE OBJECTS";
    if(detail) {
        char line[76];std::snprintf(line,sizeof line,"%.75s",detail);
        panel_text(c,menu,c.height*(menu.page==MODS?22:menu.page==CONTROLS?21:80)/100,line,0xffddddddU,s);
    }
    if(menu.now_playing[0]) {
        char now[64]; std::snprintf(now,sizeof now,"MUSIC: %s",menu.now_playing);
        c.shadow(c.width*52/100,c.height*85/100,now,0xffffffffu,s);
    }
    const char* footer=menu.message[0]?menu.message:menu.editing?"TYPE VALUE - ENTER: DONE":
#ifdef SM64DS_HANDHELD
        "D-PAD: CHOOSE   A: SELECT   B: BACK";
#else
        "ARROWS: CHOOSE   ENTER: SELECT   ESC: BACK";
#endif
    c.centered(c.height*91/100,footer,menu.message[0]?0xffffad8eu:0xffffffffu,s);
    c.centered(c.height*96/100,"SM64DS CO-OP / WINDOWS PREVIEW",0xffddddddU,s);
    (void)frame;
}
}

