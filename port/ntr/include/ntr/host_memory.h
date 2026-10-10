#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace ntr { namespace host_memory {
inline size_t page_size() {
#if defined(_WIN32)
    SYSTEM_INFO info; GetSystemInfo(&info); return info.dwPageSize;
#else
    const long size=sysconf(_SC_PAGESIZE);
    return size>0?static_cast<size_t>(size):0;
#endif
}
inline bool span(uintptr_t address,size_t bytes,size_t page,uintptr_t& begin,size_t& length) {
    const uintptr_t maximum=(std::numeric_limits<uintptr_t>::max)();
    if(!address || !bytes || !page || (page&(page-1)) || bytes>maximum-address)return false;
    begin=address&~static_cast<uintptr_t>(page-1);
    const uintptr_t end=address+bytes;
    if(end>maximum-(page-1))return false;
    length=static_cast<size_t>(((end+page-1)&~static_cast<uintptr_t>(page-1))-begin);
    return length!=0;
}
// Never replace somebody else's mapping. On kernels that ignore NOREPLACE,
// or systems without it (including Apple), verify the returned address and
// release an unwanted hint allocation. Runtime page size also covers 16 KB.
inline void* map_at(uintptr_t address,size_t bytes) {
    uintptr_t begin=0;size_t length=0;
    if(!span(address,bytes,page_size(),begin,length))return nullptr;
#if defined(_WIN32)
    SYSTEM_INFO info;GetSystemInfo(&info);
    if(begin%info.dwAllocationGranularity)return nullptr;
    void* result=VirtualAlloc(reinterpret_cast<void*>(begin),length,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
#else
    int flags=MAP_PRIVATE|MAP_ANONYMOUS;
#if defined(MAP_FIXED_NOREPLACE) && !defined(NTR_TEST_MMAP_HINT)
    flags|=MAP_FIXED_NOREPLACE;
#endif
    void* result=mmap(reinterpret_cast<void*>(begin),length,PROT_READ|PROT_WRITE,flags,-1,0);
    if(result==MAP_FAILED)return nullptr;
    if(reinterpret_cast<uintptr_t>(result)!=begin){munmap(result,length);return nullptr;}
#endif
    return result?reinterpret_cast<void*>(address):nullptr;
}
inline bool unmap(void* address,size_t bytes) {
    uintptr_t begin=0;size_t length=0;
    if(!span(reinterpret_cast<uintptr_t>(address),bytes,page_size(),begin,length))return false;
#if defined(_WIN32)
    return VirtualFree(reinterpret_cast<void*>(begin),0,MEM_RELEASE)!=0;
#else
    return munmap(reinterpret_cast<void*>(begin),length)==0;
#endif
}
} }
