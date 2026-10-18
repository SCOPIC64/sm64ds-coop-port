#include "desktop_storage.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/stat.h>
int main() {
    char fixture[]="/tmp/sm64ds-storage-XXXXXX";
    char* folder=::mkdtemp(fixture); assert(folder);
    std::string error;
    {
        sm64ds::desktop::Storage storage; assert(storage.open(folder,error));
        std::vector<uint8_t> first(8192,17),second(8192,42),readback{99};
        assert(storage.write("sm64ds.sav",first.data(),first.size(),error));
        assert(storage.read("sm64ds.sav",readback,8192,error) && readback==first);
        struct stat info{}; auto file=std::string(folder)+"/sm64ds.sav";
        assert(::stat(file.c_str(),&info)==0 && (info.st_mode&0777)==0600);
        assert(!storage.write("../escape",first.data(),first.size(),error));
        assert(!storage.write("empty.sav",nullptr,1,error));
        assert(!storage.read("sm64ds.sav",readback,8191,error) && readback==first);
        assert(::symlink("sm64ds.sav",(std::string(folder)+"/linked.sav").c_str())==0);
        assert(!storage.read("linked.sav",readback,8192,error));
        assert(storage.write("linked.sav",second.data(),second.size(),error));
        assert(storage.read("sm64ds.sav",readback,8192,error) && readback==first);
        assert(storage.read("linked.sav",readback,8192,error) && readback==second);
        assert(::mkfifo((std::string(folder)+"/pipe.sav").c_str(),0600)==0);
        assert(!storage.read("pipe.sav",readback,8192,error));
        std::atomic<bool> done{false};
        std::thread writer([&]{ std::string own_error; for(int i=0;i<50;++i) {
            const auto& bytes=(i%2)?first:second;
            assert(storage.write("sm64ds.sav",bytes.data(),bytes.size(),own_error));
        } done=true; });
        do { assert(storage.read("sm64ds.sav",readback,8192,error));
             assert(readback==first || readback==second); } while(!done);
        writer.join();
        for(const auto& entry:std::filesystem::directory_iterator(folder))
            assert(entry.path().filename().string().find(".save-")!=0);
    }
    std::filesystem::remove_all(folder);
}
