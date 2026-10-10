#include "coop_texture_tools.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstring>
namespace sm64ds::textures {
namespace fs=std::filesystem;
bool create_pack(const std::string& capture,const std::string& packs,
                 std::string& created,std::string& error) try {
    created.clear(); error.clear(); std::vector<fs::path> files;
    const fs::path input=fs::u8path(capture),output=fs::u8path(packs);
    std::error_code ec;
    if(!fs::is_directory(input,ec)){error="Start capture and play a level first.";return false;}
    for(const auto& entry:fs::directory_iterator(input)) {
        const std::string stem=entry.path().stem().u8string();
        if(entry.is_symlink() || !entry.is_regular_file() || entry.path().extension()!=".png" ||
            stem.size()!=16 || stem.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)continue;
        unsigned char header[24]={}; std::ifstream image(entry.path(),std::ios::binary);
        image.read(reinterpret_cast<char*>(header),24);
        const unsigned char signature[]={137,80,78,71,13,10,26,10};
        if(!image || std::memcmp(header,signature,8) || std::memcmp(header+12,"IHDR",4) || fs::file_size(entry.path())>64*1024*1024) {
            error="A captured PNG is invalid: "+entry.path().filename().u8string();return false;
        }
        if(files.size()==4096){error="Capture exceeds 4096 textures.";return false;}
        files.push_back(entry.path());
    }
    if(files.empty()){error="No captured textures yet. Resume and visit a level.";return false;}
    std::sort(files.begin(),files.end()); fs::create_directories(output);
    fs::path dest; bool reserved=false;
    for(int i=1;i<=999;++i){char name[40];std::snprintf(name,sizeof name,"my-texture-pack-%03d",i);
        dest=output/name;if(fs::create_directory(dest)){reserved=true;break;}}
    if(!reserved){error="No unused texture pack name.";return false;}
    fs::create_directory(dest/"assets");
    for(const auto& file:files)fs::copy_file(file,dest/"assets"/file.filename());
    {
        std::ofstream readme(dest/"README.txt",std::ios::binary);
        readme<<"Edit or upscale assets/*.png without changing their filenames.\n"
            "Restart the game to load this resource pack. Rename the folder to off_... to disable it.\n"
            "Captured original textures are local working files; publish only your own replacement art.\n";
        readme.close();if(!readme){error="Cannot write pack instructions.";return false;}
    }
    // Publish the manifest last, so an interrupted copy cannot activate a partial pack.
    const fs::path temporary=dest/"pack.lua.tmp";
    std::ofstream script(temporary,std::ios::binary);
    script<<"-- name: My Texture Pack\n-- Generated from local texture capture.\n";
    for(const auto& file:files)script<<"sm64ds.texture { target = '"<<file.stem().u8string()
        <<"', source = 'assets/"<<file.filename().u8string()<<"' }\n";
    script.close();if(!script){error="Cannot write texture manifest.";return false;}
    fs::rename(temporary,dest/"pack.lua");created=dest.u8string();return true;
} catch(const std::exception& e){error=e.what();return false;}
}
