#include "hal/coop_staff_roll.h"
#include "hal/coop_sm64_camera.h"
#include "hal/coop_sm64_movement.h"
#include "hal/coop_object_visibility.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>
int main() {
    // Reference checkpoints from the N64 formulas; runtime tests separately
    // exercise the DS adapters, camera collisions and all 20 credits scripts.
    assert(coop_sm64::walk_speed(0,4096,true,0)==4505);
    assert(coop_sm64::walk_speed(40*4096,4096,true,0)==39*4096);
    assert(coop_sm64::walk_speed(40*4096,4096,false,0)==40*4096);
    assert(coop_sm64::walk_speed(4096,0,true,0)==0);
    assert(coop_sm64::turn(0,0x4000)==0x800);
    assert(coop_sm64::turn(32767,-32767)==-32767);
    assert(coop_sm64::jump_launch(0,32*4096)==50*4096);
    assert(coop_sm64::jump_launch(1,32*4096)==60*4096);
    assert(coop_sm64::jump_launch(2,32*4096)==69*4096);
    int origin[3]={},position[3]={2000*4096,0,0};
    assert(coop_objects::within(1,position,origin));
    ++position[0];assert(!coop_objects::within(1,position,origin));
    assert(coop_objects::within(4,position,origin));
    assert(coop_objects::within(0,position,origin));
    coop_sm64::Lakitu camera;int eye[3]={0,200*4096,800*4096};
    camera.seed(origin,eye,0);camera.input(coop_sm64::Lakitu::RIGHT,0);
    for(int tick=0;tick<16;++tick){camera.input(coop_sm64::Lakitu::RIGHT,0);camera.step(origin,0,false);}
    assert(camera.yaw==0x2000 && camera.side==0);
    camera.input(coop_sm64::Lakitu::DOWN,0);
    for(int tick=0;tick<20;++tick)camera.step(origin,0,false);
    assert(camera.distance==1200);
    camera.input(coop_sm64::Lakitu::UP,0);
    for(int tick=0;tick<20;++tick)camera.step(origin,0,false);
    assert(camera.distance==800);
    camera.input(0,0);camera.input(coop_sm64::Lakitu::UP,0);assert(camera.inspecting);
    camera.look(1,1);for(int i=0;i<100;++i)camera.step(origin,0,false);
    assert(camera.inspect_pitch==0x38e3 && camera.inspect_yaw==-0x5555);
    camera.look(-1,-1);for(int i=0;i<100;++i)camera.step(origin,0,false);
    assert(camera.inspect_pitch==-0x2000 && camera.inspect_yaw==0x5555);
    camera.input(coop_sm64::Lakitu::CANCEL,0);assert(!camera.inspecting);
    camera.input(0,0);camera.input(coop_sm64::Lakitu::TOGGLE,0);assert(camera.mario);
    for(int i=0;i<40;++i){camera.step(origin,0,false);}assert(camera.distance==350);
    camera.input(coop_sm64::Lakitu::DOWN,0);
    for(int i=0;i<40;++i){camera.step(origin,0,false);}assert(camera.distance==1400);
    camera.input(0,0);camera.input(coop_sm64::Lakitu::UP,0,false);assert(!camera.inspecting);
    for(int shot=0;shot<20;++shot)assert(coop_staff_roll::levels[shot]>0 && coop_staff_roll::levels[shot]<52);
    assert(coop_staff_roll::levels[0]==6 && coop_staff_roll::levels[19]==10);
    std::puts("PASS: movement reference checkpoints, discrete Lakitu input, object limits and original credits order");
}
