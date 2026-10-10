#include "ntr/host_memory.h"
#include <cassert>
#include <cstdio>
#include <limits>

int main() {
    using namespace ntr::host_memory;
    uintptr_t begin=0;size_t length=0;
    assert(span(0x027ff000,4096,4096,begin,length) && begin==0x027ff000 && length==4096);
    assert(span(0x027ff000,4096,16384,begin,length) && begin==0x027fc000 && length==16384);
    assert(!span(1,1,3,begin,length));
    assert(!span((std::numeric_limits<uintptr_t>::max)()-10,32,4096,begin,length));
    assert(!map_at(0,4096) && !map_at(0x70000000,0));
    const uintptr_t base=sizeof(uintptr_t)>4?static_cast<uintptr_t>(0x200000000ULL):0x70000000;
    const size_t bytes=page_size()+1;
    unsigned char* memory=static_cast<unsigned char*>(map_at(base,bytes));
    assert(memory && reinterpret_cast<uintptr_t>(memory)==base);
    memory[0]=0xa5;memory[bytes-1]=0x5a;
    assert(!map_at(base,bytes));
    assert(!map_at(base+page_size(),1));
    assert(memory[0]==0xa5 && memory[bytes-1]==0x5a);
    assert(unmap(memory,bytes));
    memory=static_cast<unsigned char*>(map_at(base,bytes));
    assert(memory && memory[0]==0 && memory[bytes-1]==0);
    assert(unmap(memory,bytes));
    std::puts("PASS: native memory reservation, overlap preservation, release and 4/16 KB spans");
}
