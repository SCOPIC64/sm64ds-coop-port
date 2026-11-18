#pragma once
#include <cmath>
#include <cstddef>
namespace ntr {
struct SmoothClipPoint {float x,y,z,w;};
// Only the near and screen-side planes used by the existing raster path.
// No far-plane rejection: the hosted raster does not promise that clip.
inline unsigned smooth_clip_mask(const SmoothClipPoint& p) {
    if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(p.w))return 0;
    const float epsilon=0.001f+std::fabs(p.w)*0.01f;
    unsigned mask=0;
    if(p.z+p.w < -epsilon)mask|=1;
    if(p.x+p.w < -epsilon)mask|=2;
    if(p.w-p.x < -epsilon)mask|=4;
    if(p.y+p.w < -epsilon)mask|=8;
    if(p.w-p.y < -epsilon)mask|=16;
    return mask;
}
inline bool smooth_hull_outside(const SmoothClipPoint* points,std::size_t count,unsigned candidates=31) {
    if(!points||!count)return false;
    for(std::size_t i=0;i<count&&candidates;++i)candidates&=smooth_clip_mask(points[i]);
    return candidates!=0;
}
}
