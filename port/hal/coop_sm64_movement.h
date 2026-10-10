#pragma once
#include <algorithm>
#include <cstdint>

namespace coop_sm64 {
// Adapted to the DS fixed-point world from n64decomp/sm64:
// mario_actions_moving.c:update_walking_speed and mario.c airborne launches.
inline int walk_speed(int speed,int magnitude,bool flat,int sink_depth) {
    float velocity=speed/4096.0f;
    float target=32.0f*std::clamp(magnitude,0,4096)/4096.0f;
    if(sink_depth>10*4096)target*=6.25f/(sink_depth/4096.0f);
    if(!magnitude)velocity=(std::max)(0.0f,velocity-1.0f);
    else if(velocity<=0)velocity+=1.1f;
    else if(velocity<=target)velocity+=1.1f-velocity/43.0f;
    else if(flat)velocity-=1.0f;
    return static_cast<int>((std::min)(velocity,48.0f)*4096.0f);
}
inline short turn(short facing,short intended) {
    int difference=static_cast<short>(intended-facing);
    return static_cast<short>(facing+std::clamp(difference,-0x800,0x800));
}
inline int jump_launch(int stage,int speed) {
    if(stage==2)return 69*4096;
    return (stage==1?52:42)*4096+speed/4;
}
}
