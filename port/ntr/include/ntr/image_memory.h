#pragma once
#if defined(_WIN32)
#include <windows.h>
#include <stdint.h>
namespace ntr {
// Adopt ONLY the executable's purpose-built zero-data sections. Never overwrite
// a DLL, heap or arbitrary private reservation that happens to occupy a DS address.
inline bool image_owns_ds_range(uintptr_t address,size_t size) {
    const char* name=address==0x02000000u&&size==0x400000u?".dsram":
        address==0x027ff000u&&size==0x1000u?".dssys":nullptr;
    if(!name)return false;
    auto image=reinterpret_cast<const unsigned char*>(GetModuleHandleW(nullptr));if(!image)return false;
    auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return false;
    auto nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE)return false;
    auto section=IMAGE_FIRST_SECTION(nt);bool found=false;
    for(unsigned i=0;i<nt->FileHeader.NumberOfSections;++i){
        const auto& row=section[i];bool same=true;
        for(unsigned n=0;n<8;++n){char expected=n<6?name[n]:0;if(row.Name[n]!=expected){same=false;break;}}
        if(same&&reinterpret_cast<uintptr_t>(image)+row.VirtualAddress==address&&row.Misc.VirtualSize==(address==0x02000000u?0x7ff000u:size)&&
            row.SizeOfRawData==0&&(row.Characteristics&IMAGE_SCN_MEM_WRITE)&&!(row.Characteristics&IMAGE_SCN_MEM_EXECUTE))found=true;
    }
    if(!found)return false;
    uintptr_t cursor=address,end=address+size;
    while(cursor<end){
        MEMORY_BASIC_INFORMATION mbi={};if(!VirtualQuery(reinterpret_cast<void*>(cursor),&mbi,sizeof mbi))return false;
        DWORD protect=mbi.Protect&0xff;
        if(mbi.AllocationBase!=image||mbi.Type!=MEM_IMAGE||mbi.State!=MEM_COMMIT||
            (protect!=PAGE_READWRITE&&protect!=PAGE_WRITECOPY))return false;
        uintptr_t stop=reinterpret_cast<uintptr_t>(mbi.BaseAddress)+mbi.RegionSize;if(stop<=cursor)return false;cursor=stop;
    }
    return true;
}
}
#endif
