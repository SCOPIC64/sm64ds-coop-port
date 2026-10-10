#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace coop_sm64 {
struct Lakitu {
    bool live=false,zoomed=false;
    unsigned held=0;
    short yaw=0;
    int side=0;
    float distance=800,pan=0;
    float focus[3]={},eye[3]={};
    enum { LEFT=1,RIGHT=2,UP=4,DOWN=8,BEHIND=16 };
    void seed(const int* position,const int* camera_eye,short facing) {
        for(int axis=0;axis<3;++axis){focus[axis]=position[axis]/4096.0f;eye[axis]=camera_eye[axis]/4096.0f;}
        yaw=static_cast<short>(std::atan2(eye[0]-focus[0],eye[2]-focus[2])*10430.37835f);
        if(eye[0]==focus[0] && eye[2]==focus[2])yaw=static_cast<short>(facing+0x8000);
        focus[1]+=125;side=0;pan=0;distance=800;zoomed=false;held=0;live=true;
    }
    void input(unsigned buttons,short facing) {
        unsigned pressed=buttons&~held;held=buttons;
        if(pressed&LEFT)side=-0x1000;
        if(pressed&RIGHT)side=0x1000;
        if(pressed&UP)zoomed=false;
        if(pressed&DOWN)zoomed=true;
        if(pressed&BEHIND){yaw=static_cast<short>(facing+0x8000);side=0;}
    }
    void step(const int* position,short facing,bool moving) {
        // Default Lakitu constants from camera.c: C-side movement over 16
        // frames, 800/1200 zoom, reduced follow yaw while moving, 0.025 pan.
        if(side){yaw=static_cast<short>(yaw+(side>0?0x200:-0x200));side+=side>0?-0x100:0x100;}
        else {
            int difference=static_cast<short>(static_cast<short>(facing+0x8000)-yaw);
            int rate=moving?0x20:0x100;
            yaw=static_cast<short>(yaw+std::clamp(difference,-rate,rate));
        }
        distance+=std::clamp((zoomed?1200.0f:800.0f)-distance,-30.0f,30.0f);
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
