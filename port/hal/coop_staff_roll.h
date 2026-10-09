#pragma once
#include <cstdint>

// Port-owned course flyovers. World coordinates, not DS scene coordinates.
// Like CoopDX's menu credits, this loops scenic cameras with no player/HUD.
namespace coop_staff_roll {
struct Vec { float x,y,z; };
struct Shot { int level; Vec eye[4],at[4]; };
static const Shot shots[] = {
    {6, {{-6000,5200,7800},{-2000,6000,7200},{3000,5700,5800},{6500,4800,2800}},
        {{0,1600,0},{500,1900,-500},{1000,2100,-1200},{1800,2400,-2200}}},
    {7, {{-6500,5400,6000},{-4000,6100,6200},{1800,5900,5600},{5600,5100,2400}},
        {{0,2000,0},{0,2400,0},{500,2400,-500},{500,2100,-1000}}},
    {10,{{-6500,6500,6500},{-4000,7000,5500},{2200,6200,5500},{6000,4800,3500}},
        {{0,2200,0},{0,2300,-500},{0,1700,-1200},{0,800,-1800}}},
    {8, {{-1000,5200,7000},{500,6000,8500},{4000,5500,9000},{7500,4400,7000}},
        {{4000,800,1800},{4400,1000,2000},{4800,900,2200},{5200,800,2500}}},
    {14,{{-6500,4300,6500},{-2500,4800,7000},{2500,4200,5500},{6500,3300,1500}},
        {{0,100,0},{500,200,-500},{1000,300,-1200},{1500,500,-1800}}},
    {18,{{-9000,3000,-1800},{-8500,2900,-1900},{-7300,2800,-1700},{-6200,2800,-1500}},
        {{-7000,1800,500},{-6500,1700,750},{-6000,1600,900},{-5500,1500,1300}}},
    {1, {{-6500,4300,9000},{-3800,5200,9200},{3800,5100,8500},{6500,4000,6000}},
        {{0,1400,-1000},{0,1800,-1000},{0,1800,-1000},{0,1600,-1000}}}
};
enum { SHOT_COUNT=sizeof shots/sizeof shots[0], DURATION_MS=12000, FADE_MS=500 };
inline int index(int n) { int i=n%SHOT_COUNT;return i<0?i+SHOT_COUNT:i; }
inline Vec curve(const Vec p[4],float t) {
    float a=1-t, b=a*a*a,c=3*a*a*t,d=3*a*t*t,e=t*t*t;
    return {b*p[0].x+c*p[1].x+d*p[2].x+e*p[3].x,
            b*p[0].y+c*p[1].y+d*p[2].y+e*p[3].y,
            b*p[0].z+c*p[1].z+d*p[2].z+e*p[3].z};
}
inline void pose(int shot,float t,int eye[3],int at[3]) {
    if(!(t>=0))t=0;
    if(t>1)t=1;
    const Shot& s=shots[index(shot)];Vec e=curve(s.eye,t),a=curve(s.at,t);
    eye[0]=static_cast<int>(e.x*4096);eye[1]=static_cast<int>(e.y*4096);eye[2]=static_cast<int>(e.z*4096);
    at[0]=static_cast<int>(a.x*4096);at[1]=static_cast<int>(a.y*4096);at[2]=static_cast<int>(a.z*4096);
}
inline unsigned elapsed(uint32_t now,uint32_t start) { return now-start; }
inline unsigned opacity(unsigned age,unsigned duration) {
    if(age>=duration)return 0;
    unsigned edge=age<duration-age?age:duration-age;
    return edge<FADE_MS?edge*255/FADE_MS:255;
}
}
