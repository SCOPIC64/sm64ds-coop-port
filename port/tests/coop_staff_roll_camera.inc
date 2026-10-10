// The isolated menu renderer uses the original DS credits bytecode and paths.
#include "hal/coop_staff_roll.h"

static bool coop_staff_roll_child() {
    static const bool on=getenv("SM64DS_MENU_BACKDROP_CHILD") && getenv("SM64DS_MENU_NATIVE_STAFF_ROLL");
    return on;
}
extern "C" {
    extern unsigned char data_02087c00[];
    extern unsigned char data_02087858[];
    extern int data_0209fc4c;
}
static void coop_staff_roll_queue() {
    // Loop after the original final course panorama, before the ending scene.
    // Only this renderer's private RAM changes; the ROM and saves stay intact.
    const unsigned first=(unsigned)(size_t)data_02087c00;
    data_02087858[0x2c]=6;
    data_02087858[0x2d]=0;
    data_02087858[0x2e]=1;
    memcpy(data_02087858+0x2f,&first,sizeof first);
    data_0209fc4c=(int)first;
}
static unsigned g_staff_camera_frames;
static int g_staff_eye[3],g_staff_at[3];
static bool coop_staff_roll_camera(void* cam) {
    // Diagnostics only. The matched Camera actor owns the view matrix.
    if(coop_staff_roll_child() && cam && data_0209fc48) {
        memcpy(g_staff_eye,static_cast<char*>(cam)+0x8c,sizeof g_staff_eye);
        ++g_staff_camera_frames;
    }
    return false;
}
static void coop_staff_roll_skybox(void* stage,void* cam) {
    (void)cam;
    port_stage_render_skybox(stage);
}
