#include "hal/coop_staff_roll.h"

static bool coop_staff_roll_child() {
    static const bool on=getenv("SM64DS_MENU_BACKDROP_CHILD") && getenv("SM64DS_MENU_STAFF_ROLL_SHOT");
    return on;
}
static unsigned g_staff_camera_frames;
static int g_staff_eye[3],g_staff_at[3];
static bool coop_staff_roll_camera(void* cam) {
    if(!coop_staff_roll_child() || !cam)return false;
    static const DWORD started=GetTickCount();
    static const int shot=atoi(getenv("SM64DS_MENU_STAFF_ROLL_SHOT"));
    static const unsigned duration=getenv("SM64DS_MENU_TOUR_TEST")?6000:coop_staff_roll::DURATION_MS;
    float t=static_cast<float>(coop_staff_roll::elapsed(GetTickCount(),started))/duration;
    coop_staff_roll::pose(shot,t,g_staff_eye,g_staff_at);
    fc_push_view(cam,g_staff_eye,g_staff_at);
    ++g_staff_camera_frames;
    return true;
}
static void coop_staff_roll_skybox(void* stage,void* cam) {
    if(!coop_staff_roll_child() || !cam){port_stage_render_skybox(stage);return;}
    // Skybox placement reads the Camera actor, while geometry reads the view.
    // Seat it on the cinematic eye only for this draw, then restore the actor.
    int* eye=reinterpret_cast<int*>(static_cast<char*>(cam)+0x8c);
    int saved[3];memcpy(saved,eye,sizeof saved);memcpy(eye,g_staff_eye,sizeof saved);
    port_stage_render_skybox(stage);memcpy(eye,saved,sizeof saved);
}
