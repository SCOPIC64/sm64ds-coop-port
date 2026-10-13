#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coop_sm64 {
struct Lakitu {
    bool live=false,zoomed=false,mario=false,inspecting=false;
    unsigned held=0;
    short yaw=0,saved_yaw=0;
    int side=0,inspect_pitch=0,inspect_yaw=0;
    float distance=800,pan=0,look_x=0,look_y=0;
    float focus[3]={},eye[3]={};
    enum { LEFT=1,RIGHT=2,UP=4,DOWN=8,BEHIND=16,CANCEL=32,TOGGLE=64 };
    void seed(const int* position,const int* camera_eye,short facing) {
        for(int axis=0;axis<3;++axis){focus[axis]=position[axis]/4096.0f;eye[axis]=camera_eye[axis]/4096.0f;}
        yaw=static_cast<short>(std::atan2(eye[0]-focus[0],eye[2]-focus[2])*10430.37835f);
        if(eye[0]==focus[0] && eye[2]==focus[2])yaw=static_cast<short>(facing+0x8000);
        focus[1]+=125;side=0;pan=0;distance=800;zoomed=mario=inspecting=false;
        held=0;look_x=look_y=0;live=true;
    }
    void end_inspection() { inspecting=false;yaw=saved_yaw;look_x=look_y=0; }
    void input(unsigned buttons,short facing,bool can_inspect=true) {
        unsigned pressed=buttons&~held;held=buttons;
        if(inspecting) {
            if(!can_inspect || pressed&(LEFT|RIGHT|DOWN|CANCEL|TOGGLE|BEHIND))end_inspection();
            else return;
        }
        if(pressed&TOGGLE)mario=!mario;
        if(pressed&LEFT)side=-0x1000;
        if(pressed&RIGHT)side=0x1000;
        if(pressed&UP) {
            if(zoomed)zoomed=false;
            else if(can_inspect) {
                saved_yaw=yaw;inspecting=true;inspect_pitch=inspect_yaw=0;
                side=0;pan=0;
            }
        }
        if(pressed&DOWN)zoomed=true;
        if(pressed&BEHIND){yaw=static_cast<short>(facing+0x8000);side=0;}
    }
    void look(float x,float y) { look_x=std::clamp(x,-1.0f,1.0f);look_y=std::clamp(y,-1.0f,1.0f); }
    void step(const int* position,short facing,bool moving) {
        // N64 camera.c: C-Up orbits at 250 units, with a 125-unit height,
        // pitch -45..80 degrees and yaw +/-120 degrees. Keep Mario visible.
        if(inspecting) {
            inspect_pitch=std::clamp(inspect_pitch+(int)(look_y*800),-0x2000,0x38e3);
            inspect_yaw=std::clamp(inspect_yaw-(int)(look_x*800),-0x5555,0x5555);
            yaw=static_cast<short>(facing+inspect_yaw+0x8000);
            const float angle=yaw/10430.37835f,pitch=inspect_pitch/10430.37835f;
            float target[3]={position[0]/4096.0f+250*std::cos(pitch)*std::sin(angle),
                position[1]/4096.0f+125+250*std::sin(pitch),
                position[2]/4096.0f+250*std::cos(pitch)*std::cos(angle)};
            for(int axis=0;axis<3;++axis) {
                focus[axis]=position[axis]/4096.0f+(axis==1?125:0);
                eye[axis]+=(target[axis]-eye[axis])*0.25f;
            }
            return;
        }
        // Default Lakitu constants from camera.c: C-side movement over 16
        // frames, 800/1200 zoom, reduced follow yaw while moving, 0.025 pan.
        if(side){yaw=static_cast<short>(yaw+(side>0?0x200:-0x200));side+=side>0?-0x100:0x100;}
        else {
            int difference=static_cast<short>(static_cast<short>(facing+0x8000)-yaw);
            int rate=mario?0x200:moving?0x20:0x100;
            yaw=static_cast<short>(yaw+std::clamp(difference,-rate,rate));
        }
        float target=mario?(zoomed?1400.0f:350.0f):(zoomed?1200.0f:800.0f);
        distance+=std::clamp(target-distance,-30.0f,30.0f);
        float angle=yaw/10430.37835f;
        float side_target=std::sin(static_cast<short>(facing-yaw)/10430.37835f)*0.2902847f*distance;
        pan+=(side_target-pan)*0.025f;
        focus[0]=position[0]/4096.0f+pan*std::cos(angle);
        focus[2]=position[2]/4096.0f-pan*std::sin(angle);
        float height=position[1]/4096.0f+125;
        focus[1]+=(height-focus[1])*0.2f;
        eye[0]=position[0]/4096.0f+std::sin(angle)*distance;
        eye[2]=position[2]/4096.0f+std::cos(angle)*distance;
        eye[1]+=(position[1]/4096.0f+200-eye[1])*0.2f;
    }
};
}
