#include "desktop_storage.h"
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <atomic>
#include <limits>

namespace sm64ds::desktop {
namespace {
bool valid(const std::string& name) {
    if(name.empty() || name.size()>128 || name.front()=='.') return false;
    for(unsigned char c:name)
        if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.')) return false;
    return true;
}
bool failure(std::string& error,const char* operation) {
    error=std::string(operation)+": "+std::strerror(errno); return false;
}
}
Storage::~Storage() { if(directory_>=0) ::close(directory_); }
bool Storage::open(const std::string& folder,std::string& error) {
    int fd=::open(folder.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
    if(fd<0) return failure(error,"Open storage directory");
    if(directory_>=0) ::close(directory_);
    directory_=fd; error.clear(); return true;
}
bool Storage::read(const std::string& name,std::vector<uint8_t>& data,size_t limit,std::string& error) const {
    if(directory_<0 || !valid(name)) { error="Invalid storage entry"; return false; }
    int fd=::openat(directory_,name.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK);
    if(fd<0) return failure(error,"Open storage entry");
    struct stat info{};
    if(::fstat(fd,&info)<0) { failure(error,"Inspect storage entry"); ::close(fd); return false; }
    if(!S_ISREG(info.st_mode) || info.st_size<0 || uint64_t(info.st_size)>limit) {
        error="Storage entry is not a bounded regular file"; ::close(fd); return false;
    }
    std::vector<uint8_t> candidate(static_cast<size_t>(info.st_size));
    size_t offset=0;
    while(offset<candidate.size()) {
        ssize_t count=::read(fd,candidate.data()+offset,candidate.size()-offset);
        if(count<0 && errno==EINTR) continue;
        if(count<=0) { error="Storage read failed or file changed"; ::close(fd); return false; }
        offset+=static_cast<size_t>(count);
    }
    uint8_t extra=0; ssize_t count;
    do { count=::read(fd,&extra,1); } while(count<0 && errno==EINTR);
    ::close(fd);
    if(count!=0) { error="Storage entry changed during read"; return false; }
    data.swap(candidate); error.clear(); return true;
}
bool Storage::write(const std::string& name,const void* data,size_t size,std::string& error) const {
    if(directory_<0 || !valid(name) || (!data && size) || size>64*1024*1024) {
        error="Invalid storage write"; return false;
    }
    static std::atomic<unsigned long> sequence{0};
    std::string temporary; int fd=-1;
    for(unsigned attempt=0;attempt<32;++attempt) {
        temporary=".save-"+std::to_string(::getpid())+"-"+std::to_string(sequence.fetch_add(1));
        fd=::openat(directory_,temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
        if(fd>=0 || errno!=EEXIST) break;
    }
    if(fd<0) return failure(error,"Create temporary save");
    bool ok=true; size_t offset=0;
    while(offset<size) {
        ssize_t count=::write(fd,static_cast<const uint8_t*>(data)+offset,size-offset);
        if(count<0 && errno==EINTR) continue;
        if(count<=0) { failure(error,"Write temporary save"); ok=false; break; }
        offset+=static_cast<size_t>(count);
    }
    if(ok && ::fsync(fd)<0) { failure(error,"Flush temporary save"); ok=false; }
    if(::close(fd)<0 && ok) { failure(error,"Close temporary save"); ok=false; }
    if(ok && ::renameat(directory_,temporary.c_str(),directory_,name.c_str())<0) {
        failure(error,"Replace save"); ok=false;
    }
    if(!ok) { ::unlinkat(directory_,temporary.c_str(),0); return false; }
    // Some filesystems don't support directory fsync. Replacement already
    // committed; report success rather than encourage a duplicate write.
    (void)::fsync(directory_);
    error.clear(); return true;
}
}
