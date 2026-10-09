#include "hal/coop_media_decode.h"
#include <commdlg.h>
#include <algorithm>
#include <string>
#include <vector>

class CoopMediaLibrary {
public:
    struct File { std::wstring path, name; std::string utf8, id, label; };
    std::vector<File> music, backgrounds;
    static std::string utf8(const std::wstring& value) {
        int size=WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,0,0,0,0);
        if(size<=1) return std::string();
        std::vector<char> out(size); WideCharToMultiByte(CP_UTF8,0,value.c_str(),-1,out.data(),size,0,0);
        return std::string(out.data());
    }
    static std::wstring directory(bool songs) {
        wchar_t exe[MAX_PATH]; DWORD count=GetModuleFileNameW(0,exe,MAX_PATH);
        if(!count || count>=MAX_PATH) return std::wstring();
        std::wstring root(exe); size_t slash=root.find_last_of(L"\\/");
        if(slash==std::wstring::npos) return std::wstring();
        return root.substr(0,slash)+(songs?L"\\menu\\music":L"\\menu\\backgrounds");
    }
    static bool extension(const std::wstring& path, bool songs) {
        size_t dot=path.find_last_of(L'.'); if(dot==std::wstring::npos) return false;
        const wchar_t* ext=path.c_str()+dot;
        return songs ? (!_wcsicmp(ext,L".wav") || !_wcsicmp(ext,L".mp3") || !_wcsicmp(ext,L".flac")) :
            (!_wcsicmp(ext,L".png") || !_wcsicmp(ext,L".jpg") || !_wcsicmp(ext,L".jpeg") || !_wcsicmp(ext,L".bmp"));
    }
    static void scan(bool songs, std::vector<File>& files) {
        files.clear(); std::wstring folder=directory(songs); if(folder.empty()) return;
        WIN32_FIND_DATAW data; HANDLE find=FindFirstFileW((folder+L"\\*").c_str(),&data);
        if(find==INVALID_HANDLE_VALUE) return;
        do {
            if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY || !extension(data.cFileName,songs) ||
               data.nFileSizeHigh || !data.nFileSizeLow || data.nFileSizeLow>(songs?512u:32u)*1024u*1024u) continue;
            File file; file.name=data.cFileName; file.path=folder+L"\\"+file.name;
            file.utf8=utf8(file.path); file.id=utf8(file.name);
            file.label=file.id.substr(0,file.id.find_last_of('.'));
            for(char& c:file.label) if(static_cast<unsigned char>(c)<32 || static_cast<unsigned char>(c)>126) c='?';
            files.push_back(file);
        } while(FindNextFileW(find,&data));
        FindClose(find);
        std::sort(files.begin(),files.end(),[](const File& a,const File& b){return _wcsicmp(a.name.c_str(),b.name.c_str())<0;});
        if(files.size()>coop_frontend::MAX_CUSTOM_MEDIA) files.resize(coop_frontend::MAX_CUSTOM_MEDIA);
    }
    void refresh(coop_frontend::Menu& menu) {
        scan(true,music); scan(false,backgrounds);
        menu.custom_music_count=static_cast<int>(music.size()); menu.custom_background_count=static_cast<int>(backgrounds.size());
        for(size_t i=0;i<music.size();++i) snprintf(menu.custom_music[i],48,"%.47s",music[i].label.c_str());
        for(size_t i=0;i<backgrounds.size();++i) snprintf(menu.custom_backgrounds[i],48,"%.47s",backgrounds[i].label.c_str());
        if(menu.music>=coop_frontend::SONG_COUNT) {
            int match=-1; for(size_t i=0;i<music.size();++i) if(music[i].id==menu.music_file) match=static_cast<int>(i);
            menu.music=match>=0?coop_frontend::SONG_COUNT+match:coop_frontend::MUSIC_RANDOM;
        }
        if(menu.background>=coop_frontend::BACKGROUND_BUILTIN_COUNT) {
            int match=-1; for(size_t i=0;i<backgrounds.size();++i) if(backgrounds[i].id==menu.background_file) match=static_cast<int>(i);
            menu.background=match>=0?coop_frontend::BACKGROUND_BUILTIN_COUNT+match:0;
        }
        if(music.empty() && menu.music<coop_frontend::MUSIC_OFF) menu.music=coop_frontend::MUSIC_RANDOM;
    }
    void selection(coop_frontend::Menu& menu) const {
        int song=menu.music-coop_frontend::SONG_COUNT, bg=menu.background-coop_frontend::BACKGROUND_BUILTIN_COUNT;
        snprintf(menu.music_file,sizeof menu.music_file,"%s",song>=0 && song<static_cast<int>(music.size())?music[song].id.c_str():"");
        snprintf(menu.background_file,sizeof menu.background_file,"%s",bg>=0 && bg<static_cast<int>(backgrounds.size())?backgrounds[bg].id.c_str():"");
    }
    bool add(bool songs, HWND owner, coop_frontend::Menu& menu) {
        HMODULE dialog=LoadLibraryW(L"comdlg32.dll");
        typedef BOOL(WINAPI *Open)(LPOPENFILENAMEW); Open open=dialog?reinterpret_cast<Open>(GetProcAddress(dialog,"GetOpenFileNameW")):0;
        if(!open) { if(dialog)FreeLibrary(dialog); snprintf(menu.message,sizeof menu.message,"File picker unavailable."); return false; }
        wchar_t path[MAX_PATH]={}; OPENFILENAMEW options={}; options.lStructSize=sizeof options; options.hwndOwner=owner;
        options.lpstrFile=path; options.nMaxFile=MAX_PATH;
        options.lpstrFilter=songs?L"Music (MP3, FLAC, WAV)\0*.mp3;*.flac;*.wav\0\0":L"Pictures (PNG, JPEG, BMP)\0*.png;*.jpg;*.jpeg;*.bmp\0\0";
        options.lpstrTitle=songs?L"Add custom menu music":L"Add custom menu background";
        options.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
        bool chosen=open(&options)!=FALSE; FreeLibrary(dialog); if(!chosen)return false;
        std::wstring source(path); std::string input=utf8(source);
        WIN32_FILE_ATTRIBUTE_DATA attributes={};
        if(!extension(source,songs) || !GetFileAttributesExW(path,GetFileExInfoStandard,&attributes) ||
           attributes.nFileSizeHigh || !attributes.nFileSizeLow || attributes.nFileSizeLow>(songs?512u:32u)*1024u*1024u) {
            snprintf(menu.message,sizeof menu.message,"Unsupported file or file too large.");return false;
        }
        if(songs) { coop_media::MusicDecoder decoder; if(!decoder.open(input.c_str())) {
            snprintf(menu.message,sizeof menu.message,"Cannot decode this music file.");return false; } }
        else { std::vector<uint32_t> test; if(!coop_media::image(input.c_str(),test,512,384)) {
            snprintf(menu.message,sizeof menu.message,"Cannot decode this picture.");return false; } }
        std::wstring folder=directory(songs); size_t slash=folder.find_last_of(L'\\');
        CreateDirectoryW(folder.substr(0,slash).c_str(),0); CreateDirectoryW(folder.c_str(),0);
        std::wstring name=source.substr(source.find_last_of(L"\\/")+1), dest=folder+L"\\"+name;
        const std::vector<File>& current=songs?music:backgrounds;
        bool existing=false;
        for(size_t i=0;i<current.size();++i) if(!_wcsicmp(current[i].name.c_str(),name.c_str())) existing=true;
        if(current.size()>=coop_frontend::MAX_CUSTOM_MEDIA && !existing) {
            snprintf(menu.message,sizeof menu.message,"Custom library limit reached (256 files).");return false;
        }
        if(_wcsicmp(source.c_str(),dest.c_str()) && !CopyFileW(source.c_str(),dest.c_str(),TRUE)) {
            snprintf(menu.message,sizeof menu.message,"File already exists or folder is not writable.");return false;
        }
        refresh(menu);
        const std::vector<File>& files=songs?music:backgrounds;
        int found=-1; for(size_t i=0;i<files.size();++i)if(!_wcsicmp(files[i].name.c_str(),name.c_str()))found=static_cast<int>(i);
        if(found<0) { snprintf(menu.message,sizeof menu.message,"Custom library limit reached (256 files).");return false; }
        if(songs)menu.music=coop_frontend::SONG_COUNT+found; else menu.background=coop_frontend::BACKGROUND_BUILTIN_COUNT+found;
        selection(menu); snprintf(menu.message,sizeof menu.message,"Added. Back saves your choice."); return true;
    }
};
