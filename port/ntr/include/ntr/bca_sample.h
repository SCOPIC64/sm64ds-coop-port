#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace ntr {
// BCA selectors follow the cartridge's func_020457f0/func_020456a0:
// constant, per-frame, or decimated keys followed by an uncompressed tail.
template<class T>
inline float bca_sample(const std::vector<uint8_t>& bytes, uint32_t pool,
                        uint16_t first, unsigned shift, bool animated,
                        unsigned frames, unsigned frame, bool rotation=false) {
    auto value=[&](unsigned index) {
        const uint64_t at=uint64_t(pool)+(uint64_t(first)+index)*sizeof(T);
        T result{};if(at+sizeof(T)<=bytes.size())std::memcpy(&result,bytes.data()+size_t(at),sizeof(T));
        return int(result);
    };
    if(!animated||!frames)return float(value(0));
    frame=(std::min)(frame,frames-1);
    if(!shift)return float(value(frame));
    if(shift>7)return float(value(0));
    const unsigned tail=((frames-1)>>shift)<<shift,index=frame>>shift;
    if(frame>=tail)return float(value(index+frame-tail));
    const unsigned remainder=frame-(index<<shift);
    int a=value(index),b=value(index+1);
    if(rotation&&std::abs(a-b)>=0x800){if(a>b)b+=0x1000;else a+=0x1000;}
    return float(a)+(b-a)*(float(remainder)/float(1u<<shift));
}
}
