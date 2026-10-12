// Windows menu font rasterized once into grayscale glyph atlases. Rendering
// stays in the framebuffer, including screenshots; there is no font download.
class CoopMenuFont {
    struct Face { int height=0,cell=0;std::array<int,95> advance{};std::vector<uint32_t> mask; };
    std::array<Face,9> faces;
    HMODULE dll=nullptr;
    decltype(&CreateCompatibleDC) make_dc=nullptr;decltype(&CreateFontW) make_font=nullptr;
    decltype(&CreateDIBSection) make_dib=nullptr;decltype(&SelectObject) select=nullptr;
    decltype(&DeleteObject) remove=nullptr;decltype(&DeleteDC) remove_dc=nullptr;
    decltype(&SetTextColor) text_color=nullptr;decltype(&SetBkColor) back_color=nullptr;
    decltype(&TextOutA) text_out=nullptr;decltype(&GetTextExtentPoint32A) extent=nullptr;
    decltype(&GdiFlush) flush=nullptr;
    Face& face(int size) {
        size=(std::max)(1,(std::min)(8,size));Face& f=faces[size];if(f.height)return f;
        f.height=9*size+2;f.cell=f.height+8;
        if(!dll){dll=LoadLibraryW(L"gdi32.dll");
#define COOP_GDI(member,name) member=reinterpret_cast<decltype(member)>(GetProcAddress(dll,name))
            if(dll){COOP_GDI(make_dc,"CreateCompatibleDC");COOP_GDI(make_font,"CreateFontW");COOP_GDI(make_dib,"CreateDIBSection");
                COOP_GDI(select,"SelectObject");COOP_GDI(remove,"DeleteObject");COOP_GDI(remove_dc,"DeleteDC");
                COOP_GDI(text_color,"SetTextColor");COOP_GDI(back_color,"SetBkColor");COOP_GDI(text_out,"TextOutA");COOP_GDI(extent,"GetTextExtentPoint32A");COOP_GDI(flush,"GdiFlush");}
#undef COOP_GDI
        }
        if(!make_dc||!make_font||!make_dib||!select||!remove||!remove_dc||!text_color||!back_color||!text_out||!extent)return f;
        HDC dc=make_dc(nullptr);HFONT font=make_font(f.height,0,0,0,FW_MEDIUM,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
        BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=f.cell*95;
        info.bmiHeader.biHeight=-f.height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        void* bits=nullptr;HBITMAP bitmap=make_dib(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
        if(dc&&font&&bitmap&&bits) {
            auto old_font=select(dc,font),old_bitmap=select(dc,bitmap);
            std::memset(bits,0,static_cast<size_t>(f.cell)*95*f.height*4);text_color(dc,RGB(255,255,255));back_color(dc,RGB(0,0,0));
            for(int n=0;n<95;++n){char ch=static_cast<char>(n+32);SIZE sz={};extent(dc,&ch,1,&sz);f.advance[n]=(std::max)(1,static_cast<int>(sz.cx));text_out(dc,n*f.cell,0,&ch,1);}
            if(flush)flush();
            f.mask.assign(static_cast<uint32_t*>(bits),static_cast<uint32_t*>(bits)+static_cast<size_t>(f.cell)*95*f.height);
            select(dc,old_font);select(dc,old_bitmap);
        }
        if(bitmap)remove(bitmap);if(font)remove(font);if(dc)remove_dc(dc);return f;
    }
public:
    static CoopMenuFont& get(){static CoopMenuFont font;return font;}
    static int measure(const char* text,int size) {
        Face& f=get().face(size);int width=0;for(;*text;++text) {unsigned ch=static_cast<unsigned char>(*text);width+=ch>=32&&ch<=126?f.advance[ch-32]:0;}return width;
    }
    static void draw(uint32_t* pixels,int width,int height,int stride,int x,int y,const char* text,uint32_t color,int size) {
        Face& f=get().face(size);if(f.mask.empty())return;
        for(;*text;++text){unsigned ch=static_cast<unsigned char>(*text);if(ch<32||ch>126)continue;int glyph=ch-32,advance=f.advance[glyph];
            for(int py=0;py<f.height&&py+y<height;++py)for(int px=0;px<(std::min)(f.cell,advance+2)&&px+x<width;++px){
                if(x+px<0||y+py<0)continue;unsigned a=f.mask[py*f.cell*95+glyph*f.cell+px]&255;if(!a)continue;
                uint32_t& old=pixels[(y+py)*stride+x+px];old=0xff000000u|
                    ((((old>>16&255)*(255-a)+(color>>16&255)*a)/255)<<16)|((((old>>8&255)*(255-a)+(color>>8&255)*a)/255)<<8)|(((old&255)*(255-a)+(color&255)*a)/255);
            }x+=advance;
        }
    }
};
static void coop_canvas_prepare(coop_frontend::Canvas& canvas) {
    canvas.smooth_text=CoopMenuFont::draw;canvas.text_width=CoopMenuFont::measure;
    static std::vector<uint32_t> logo;static int width=0,height=0;static bool loaded=false;
    if(!loaded){loaded=true;HRSRC resource=FindResourceW(nullptr,MAKEINTRESOURCEW(210),MAKEINTRESOURCEW(10));
        if(resource){HGLOBAL memory=LoadResource(nullptr,resource);auto bytes=static_cast<const unsigned char*>(LockResource(memory));
            coop_media::image_memory(bytes,static_cast<int>(SizeofResource(nullptr,resource)),logo,width,height);}}
    if(!logo.empty()){canvas.logo=logo.data();canvas.logo_width=width;canvas.logo_height=height;}
}
