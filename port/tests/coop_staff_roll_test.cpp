#include "hal/coop_staff_roll.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
int main() {
    bool seen[32]={};
    for(int shot=0;shot<coop_staff_roll::SHOT_COUNT;++shot) {
        int level=coop_staff_roll::shots[shot].level;
        assert(level>0 && level<32 && !seen[level]);seen[level]=true;
        int first[3],first_at[3];coop_staff_roll::pose(shot,0,first,first_at);
        int prior[3]={first[0],first[1],first[2]};
        for(int tick=0;tick<=360;++tick) {
            int eye[3],at[3];coop_staff_roll::pose(shot,tick/360.0f,eye,at);
            double distance=0;
            for(int axis=0;axis<3;++axis) {
                assert(std::abs(eye[axis])<15000*4096 && std::abs(at[axis])<15000*4096);
                assert(std::abs(eye[axis]-prior[axis])<150*4096);prior[axis]=eye[axis];
                double delta=(eye[axis]-at[axis])/4096.0;distance+=delta*delta;
            }
            assert(distance>1000*1000);
        }
        assert(first[0]!=prior[0] || first[1]!=prior[1] || first[2]!=prior[2]);
        int eye[3],at[3];coop_staff_roll::pose(shot,-1,eye,at);assert(eye[0]==first[0]);
        coop_staff_roll::pose(shot,std::numeric_limits<float>::quiet_NaN(),eye,at);assert(eye[0]==first[0]);
    }
    assert(coop_staff_roll::index(coop_staff_roll::SHOT_COUNT)==0);
    assert(coop_staff_roll::index(-1)==coop_staff_roll::SHOT_COUNT-1);
    assert(coop_staff_roll::elapsed(20,0xfffffff0u)==36);
    assert(coop_staff_roll::opacity(0,12000)==0);
    assert(coop_staff_roll::opacity(500,12000)==255);
    assert(coop_staff_roll::opacity(6000,12000)==255);
    assert(coop_staff_roll::opacity(12000,12000)==0);
    for(unsigned ms=0;ms<12000;++ms)assert(coop_staff_roll::opacity(ms,12000)<=255);
    std::puts("PASS: cinematic paths, smooth movement, loop wrap, fades and timer overflow");
}
