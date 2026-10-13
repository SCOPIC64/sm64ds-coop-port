#include "hal/coop_palettes.h"
class CoopPaletteLibrary {
    struct Preset { std::string name; coop_palettes::Colors colors; };
    std::vector<Preset> presets;
public:
    static std::string folder() { const char* app=getenv("APPDATA");return app?std::string(app)+"\\SM64DS-Coop\\palettes":""; }
    void refresh() {
        presets.clear();presets.push_back({"Original",{}});
        const char* names[]={"Mario","Luigi","Wario","Yoshi"};
        for(int c=0;c<4;++c)presets.push_back({names[c],coop_palettes::stock(c)});
        auto mint=coop_palettes::stock(0);mint[0]=mint[1]="2ae4ad";mint[2]="183e68";presets.push_back({"Mint",mint});
        auto night=coop_palettes::stock(0);night[0]=night[1]="272d46";night[2]="735bd1";presets.push_back({"Midnight",night});
        WIN32_FIND_DATAA data;HANDLE find=FindFirstFileA((folder()+"\\*.ini").c_str(),&data);
        if(find!=INVALID_HANDLE_VALUE){do {
            if((data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||data.nFileSizeHigh||data.nFileSizeLow>65536||presets.size()>=128)continue;
            coop_palettes::Colors colors;
            if(coop_palettes::read(coop_online::read_file(folder()+"\\"+data.cFileName),colors)) {
                std::string name=data.cFileName;name.erase(name.size()-4);presets.push_back({name,colors});
            }
        }while(FindNextFileA(find,&data));FindClose(find);}
    }
    void tick(coop_frontend::Menu& menu) {
        if(presets.empty())refresh();menu.palette_count=static_cast<int>(presets.size());
        if(menu.palette_index>=menu.palette_count)menu.palette_index=0;
        snprintf(menu.palette_selected,sizeof menu.palette_selected,"%.63s",presets[menu.palette_index].name.c_str());
    }
    static std::string safe_name(std::string name) {
        name=coop_palettes::trim(name);if(name.empty()||name.size()>32||name.back()=='.')return "";
        for(char c:name)if(c<32||c>126||std::strchr("\\/:*?\"<>|",c))return "";
        std::string upper=name;for(char& c:upper)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if(upper=="CON"||upper=="PRN"||upper=="AUX"||upper=="NUL"||
           (upper.size()==4&&(upper.substr(0,3)=="COM"||upper.substr(0,3)=="LPT")&&upper[3]>='1'&&upper[3]<='9'))return "";
        return name;
    }
    static bool save(const std::string& path,const std::string& text) {
        std::string tmp=path+".tmp";std::ofstream f(tmp,std::ios::binary|std::ios::trunc);if(!f)return false;
        f<<text;f.close();bool ok=f&&MoveFileExA(tmp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
        if(!ok)DeleteFileA(tmp.c_str());return ok;
    }
    bool action(coop_frontend::Action action,coop_frontend::Menu& menu,coop_prefs::Preferences& prefs) {
        using namespace coop_frontend;
        if(action!=IMPORT_PALETTE&&action!=EXPORT_PALETTE&&action!=SAVE_PALETTE&&action!=APPLY_PALETTE)return false;
        tick(menu);auto& colors=prefs.colors[menu.color_character];
        if(action==APPLY_PALETTE){colors=presets[menu.palette_index].colors;return true;}
        std::string name=safe_name(menu.palette_name);
        if(action==SAVE_PALETTE) {
            if(name.empty()){snprintf(menu.message,sizeof menu.message,"Use a name of 1-32 characters without file-path symbols.");return true;}
            const std::string dir=folder();CreateDirectoryA(dir.substr(0,dir.find_last_of('\\')).c_str(),nullptr);CreateDirectoryA(dir.c_str(),nullptr);
            bool ok=save(dir+"\\"+name+".ini",coop_palettes::write(colors,menu.color_character));
            snprintf(menu.message,sizeof menu.message,"%s",ok?"Preset saved.":"Could not save preset.");refresh();
            for(size_t n=0;n<presets.size();++n)if(presets[n].name==name)menu.palette_index=static_cast<int>(n);return true;
        }
        HMODULE dll=LoadLibraryW(L"comdlg32.dll");
        using Pick=BOOL(WINAPI*)(LPOPENFILENAMEW);auto pick=dll?reinterpret_cast<Pick>(GetProcAddress(dll,action==IMPORT_PALETTE?"GetOpenFileNameW":"GetSaveFileNameW")):nullptr;
        if(!pick){if(dll)FreeLibrary(dll);snprintf(menu.message,sizeof menu.message,"File picker unavailable.");return true;}
        wchar_t path[MAX_PATH]={};if(action==EXPORT_PALETTE)MultiByteToWideChar(CP_UTF8,0,(name.empty()?"My palette.ini":name+".ini").c_str(),-1,path,MAX_PATH);
        wchar_t initial[MAX_PATH]={};const char* app=getenv("APPDATA");
        if(app)MultiByteToWideChar(CP_UTF8,0,(std::string(app)+"\\sm64coopdx\\palettes").c_str(),-1,initial,MAX_PATH);
        OPENFILENAMEW options={};options.lStructSize=sizeof options;options.hwndOwner=g_entry_hwnd;
        options.lpstrFile=path;options.nMaxFile=MAX_PATH;options.lpstrFilter=L"CoopDX color preset (.ini)\0*.ini\0\0";
        options.lpstrDefExt=L"ini";options.lpstrInitialDir=initial;options.lpstrTitle=action==IMPORT_PALETTE?L"Import a SM64CoopDX color preset":L"Export a CoopDX color preset";
        options.Flags=OFN_EXPLORER|OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(action==IMPORT_PALETTE?OFN_FILEMUSTEXIST:OFN_OVERWRITEPROMPT);
        bool picked=pick(&options)!=FALSE;FreeLibrary(dll);if(!picked)return true;
        const std::string file=CoopMediaLibrary::utf8(path);
        if(action==EXPORT_PALETTE){snprintf(menu.message,sizeof menu.message,"%s",save(file,coop_palettes::write(colors,menu.color_character))?"Preset exported.":"Could not export preset.");return true;}
        WIN32_FILE_ATTRIBUTE_DATA attributes={};coop_palettes::Colors imported;
        if(!GetFileAttributesExW(path,GetFileExInfoStandard,&attributes)||attributes.nFileSizeHigh||attributes.nFileSizeLow>65536||
           !coop_palettes::read(coop_online::read_file(file),imported)){snprintf(menu.message,sizeof menu.message,"Invalid preset: expected all 24 RGB channels in [PALETTE].");return true;}
        colors=imported;std::string base=file.substr(file.find_last_of("\\/")+1);base=base.substr(0,base.find_last_of('.'));
        name=safe_name(base.substr(0,32));if(name.empty())name="Imported palette";
        snprintf(menu.palette_name,sizeof menu.palette_name,"%s",name.c_str());
        const std::string dir=folder();CreateDirectoryA(dir.substr(0,dir.find_last_of('\\')).c_str(),nullptr);CreateDirectoryA(dir.c_str(),nullptr);
        bool stored=save(dir+"\\"+name+".ini",coop_palettes::write(imported,menu.color_character));refresh();
        snprintf(menu.message,sizeof menu.message,"%s",stored?"Imported and saved. Esc returns to the color editor.":"Imported colors; preset file could not be saved.");return true;
    }
};
static CoopPaletteLibrary g_coop_palettes;
