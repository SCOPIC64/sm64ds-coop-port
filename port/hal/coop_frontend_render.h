#pragma once
#include "coop_frontend.h"
#include <cstdint>
#include <cmath>

namespace coop_frontend {
struct Canvas {
    uint32_t* pixels;
    int width, height, stride;
    const unsigned char (*font)[8];
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
        shadow((width - static_cast<int>(std::strlen(str)) * 6 * size) / 2, y, str, color, size);
    }
};
struct ButtonRect { int x, y, w, h; };
inline int panel_width(const Canvas& c, const Menu& m) { return m.page == HOME ? c.width * 46 / 100 : c.width * 90 / 100; }
inline int panel_left(const Canvas& c, const Menu& m) { return m.page == HOME ? c.width / 30 : (c.width - panel_width(c,m)) / 2; }
inline int button_left(const Canvas& c, const Menu& m) { return panel_left(c,m) + c.width / 30; }
inline int button_width(const Canvas& c, const Menu& m) { return panel_width(c,m) - c.width / 15; }
inline int row_top(const Canvas& c, const Menu& m) { return c.height * (m.page == HOME ? 40 : m.page == CONTROLS ? 77 : 27) / 100; }
inline int row_height(const Canvas& c) { int h = c.height * 7 / 100; return h > 1 ? h : 2; }
inline ButtonRect button_rect(const Canvas& c, const Menu& m, int row) {
    ButtonRect r = {button_left(c,m), row_top(c,m) + row * row_height(c), button_width(c,m), row_height(c) * 4 / 5};
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
    c.shadow(panel_left(c,m)+(panel_width(c,m)-static_cast<int>(std::strlen(str))*6*size)/2,y,str,color,size);
}
inline void hand(const Canvas& c, int x, int y, int s) {
    static const unsigned char rows[] = { 0x30,0x38,0xfc,0xfe,0xff,0xfe,0x7c,0x38 };
    for (int yy=0; yy<8; ++yy) for (int xx=0; xx<8; ++xx)
        if (rows[yy] & (0x80>>xx)) c.box(x+xx*s+s,y+yy*s+s,s,s,0xff101020u);
    for (int yy=0; yy<8; ++yy) for (int xx=0; xx<8; ++xx)
        if (rows[yy] & (0x80>>xx)) c.box(x+xx*s,y+yy*s,s,s,0xffffffffu);
}
inline void draw(const Canvas& c, const Menu& menu, unsigned frame, const char* loading=nullptr,
                 const uint32_t* backdrop=nullptr, int bw=512, int bh=384) {
    if (!c.valid()) return;
    const int s=c.scale();
    for (int y=0; y<c.height; ++y) for (int x=0; x<c.width; ++x) {
        uint32_t v;
        if (backdrop && bw>0 && bh>0) v=backdrop[(y*bh/c.height)*bw+x*bw/c.width];
        else {
            // The menu remains usable before ROM import or while a scene loads.
            unsigned red=50+static_cast<unsigned>(y*30/c.height), green=110+static_cast<unsigned>(y*35/c.height);
            v=0xff000000u | (red<<16) | (green<<8) | 190;
        }
        c.pixels[y*c.stride+x]=v;
    }
    const int px=panel_left(c,menu), pw=panel_width(c,menu);
    c.shade(px,c.height/12,pw,c.height*79/100,165);
    if (menu.page==HOME) {
        const char* title="SUPER MARIO"; const int size=s*2;
        const uint32_t colors[]={0xfff34b46u,0xff62caffu,0xffffdc47u,0xff70df6bu};
        int x=px+(pw-static_cast<int>(std::strlen(title))*6*size)/2;
        for (int i=0; title[i]; ++i) { char one[]={title[i],0}; c.shadow(x,c.height*16/100,one,colors[i%4],size); x+=6*size; }
        panel_text(c,menu,c.height*24/100,"64 DS CO-OP",0xffffffffu,size);
        panel_text(c,menu,c.height*32/100,"MAIN MENU",0xffffdf60u,s);
    } else panel_text(c,menu,c.height*16/100,menu.title(),0xffffdf60u,s*2);
    if (loading) {
        panel_text(c,menu,c.height*45/100,loading,0xffffffffu,s);
        panel_text(c,menu,c.height*57/100,"PLEASE WAIT...",0xffeeeeeeu,s);
    } else {
        if (menu.page==CONTROLS) {
            const char* lines[]={"WASD / ARROWS: MOVE   SPACE: JUMP","X: ATTACK   CTRL: CROUCH   SHIFT: RUN",
                "Q/E: TURN   R/F: TILT CAMERA","RIGHT MOUSE: LOOK   WHEEL: ZOOM","LEFT STICK: MOVE   RIGHT STICK: CAMERA",
                "A: JUMP   B: ATTACK   RT: CROUCH","F5: GAME OPTIONS   F12: FULLSCREEN"};
            for (int i=0;i<7;++i) panel_text(c,menu,c.height*(29+i*6)/100,lines[i],0xffffffffu,s);
        }
        for (int i=0;i<menu.rows();++i) {
            ButtonRect r=button_rect(c,menu,i); bool selected=i==menu.row;
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
            if(selected) { c.shade(r.x,r.y,r.w,r.h,50); hand(c,r.x+2*s,r.y+(r.h-8*s)/2,s); }
            char label[80]; menu.label(i,label,sizeof label);
            int size=s*2; while(size>1 && static_cast<int>(std::strlen(label))*6*size>r.w-22*s) --size;
            const int x=menu.page==HOME?r.x+20*s:r.x+(r.w-static_cast<int>(std::strlen(label))*6*size)/2;
            c.shadow(x,r.y+(r.h-8*size)/2,label,selected?0xffffdf60u:0xffffffffu,size);
        }
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

