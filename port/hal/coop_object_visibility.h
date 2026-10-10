#pragma once
#include <cstdint>

namespace coop_objects {
// These are rendering limits, in DS world units. They never gate Behavior.
inline int limit(int preference) {
    const int limits[]={-1,2000,6000,16000,0};
    return preference>=0 && preference<5?limits[preference]:-1;
}
inline bool within(int preference,const int* position,const int* eye) {
    const int distance=limit(preference);
    if(distance<=0)return true;
    double squared=0;
    for(int axis=0;axis<3;++axis) {
        double delta=(static_cast<double>(position[axis])-eye[axis])/4096.0;
        squared+=delta*delta;
    }
    return squared<=static_cast<double>(distance)*distance;
}
}
