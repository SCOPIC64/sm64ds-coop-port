// CoopDX's menu positions, adapted to the collision floors in the DS courses.
// Reference: coop-deluxe/sm64coopdx, src/game/level_update.c, update_menu_level.
// The original DS credits keep their own players, scripts and camera lifecycle.
extern "C" {
    extern void (*port_menu_player_preview_hook)(void*, bool);
    extern unsigned char data_ov002_02110154[];
    void func_ov002_020e444c(void*);
}
struct CoopMenuPose {
    int level, position[3], eye[3];
};
static const CoopMenuPose coop_menu_poses[]={
    {1, {-1328,260,4664}, {-1328,390,6064}},
    {6, {7008,864,1943}, {7909,1064,2843}},
    {7, {-2904,2560,-327}, {-4504,2760,-777}},
    {8, {3639,1536,6202}, {5039,1736,6402}},
    {10, {-1127,-3580,6162}, {-1330,-2830,9099}},
    {14, {-2376,638,956}, {-3576,938,1576}}
};
static bool coop_menu_pose_ready;
static int coop_menu_position[3],coop_menu_eye[3];
static short coop_menu_yaw;
static bool coop_menu_player_child() {
    return getenv("SM64DS_MENU_BACKDROP_CHILD") && !coop_staff_roll_child();
}
static void coop_menu_player_prepare(void* player,bool before) {
    if(!coop_menu_player_child() || !player || !g_mc)return;
    char* p=static_cast<char*>(player);
    if(!coop_menu_pose_ready) {
        const CoopMenuPose* preset=nullptr;
        for(const auto& pose:coop_menu_poses)if(pose.level==data_0209f2f8)preset=&pose;
        for(int axis=0;axis<3;++axis) {
            coop_menu_position[axis]=preset?preset->position[axis]*4096:((int*)(p+0x5c))[axis];
            coop_menu_eye[axis]=preset?preset->eye[axis]*4096:coop_menu_position[axis];
        }
        // DDD has no CoopDX menu preset: use its DS entrance ledge.
        if(!preset){coop_menu_eye[1]+=250*4096;coop_menu_eye[2]+=1400*4096;}
        int floor=0;
        if(hal_ground_ray(g_mc,coop_menu_position[0],coop_menu_position[1]+400*4096,
                          coop_menu_position[2],2400*4096,&floor)) {
            const int shift=floor-coop_menu_position[1];
            coop_menu_position[1]=floor;
            coop_menu_eye[1]+=shift;
        }
        // Some DS hills extend into the N64 camera position. Keep the eye
        // above the DS floor and pull it in if terrain blocks Mario's face.
        if(hal_ground_ray(g_mc,coop_menu_eye[0],coop_menu_eye[1]+600*4096,
                          coop_menu_eye[2],1600*4096,&floor))
            coop_menu_eye[1]=(std::max)(coop_menu_eye[1],floor+160*4096);
        int focus[3],hit[3];memcpy(focus,coop_menu_position,sizeof focus);focus[1]+=125*4096;
        if(hal_line_ray(g_mc,focus,coop_menu_eye,hit)) {
            double length=0;
            for(int axis=0;axis<3;++axis){double d=(double)hit[axis]-focus[axis];length+=d*d;}
            length=std::sqrt(length);
            const double fraction=length>0?(std::max)(0.0,(length-80*4096)/length):0;
            if(length>240*4096)
                for(int axis=0;axis<3;++axis)coop_menu_eye[axis]=focus[axis]+(int)(((double)hit[axis]-focus[axis])*fraction);
        }
        coop_menu_yaw=(short)(std::atan2((double)coop_menu_eye[0]-coop_menu_position[0],
            (double)coop_menu_eye[2]-coop_menu_position[2])*10430.378350470453);
        coop_menu_pose_ready=true;
        fprintf(stderr,"[coop-menu] Mario preview level=%d pos=%d,%d,%d eye=%d,%d,%d yaw=%d\n",
            data_0209f2f8,coop_menu_position[0]/4096,coop_menu_position[1]/4096,coop_menu_position[2]/4096,
            coop_menu_eye[0]/4096,coop_menu_eye[1]/4096,coop_menu_eye[2]/4096,(int)coop_menu_yaw);
    }
    memcpy(p+0x5c,coop_menu_position,sizeof coop_menu_position);
    memcpy(p+0x68,coop_menu_position,sizeof coop_menu_position);
    // Use the DS collision query's scripted-position mode. Its floor/wall
    // queries still run, but cannot sweep Mario back toward his entrance.
    *(unsigned*)(p+0x390)|=0x2010u;
    *(short*)(p+0x8c)=0;*(short*)(p+0x8e)=coop_menu_yaw;*(short*)(p+0x90)=0;
    *(short*)(p+0x94)=coop_menu_yaw;
    *(int*)(p+0x98)=0;memset(p+0xa4,0,3*sizeof(int));
    *(unsigned*)(p+0xb0)&=~(0x800000u|0x10u);
    *(unsigned char*)(p+0x6f5)=31;*(unsigned short*)(p+0x6a0)=0;
    // No course-entry dialogue or input in this private menu renderer.
    // Actual gameplay retains the DS introduction and saved hint flags.
    *(unsigned char*)(p+0x71e)=0;*(unsigned char*)(p+0x71f)=0;
    data_0209b454[0]&=~0x800000;
    if(before && *(void**)(p+0x370)!=data_ov002_02110154)
        _ZN6Player11ChangeStateERNS_5StateE(player,data_ov002_02110154);
    if(!before) {
        // Recompose BOTH native body/head matrices after the DS state tick.
        // Behavior already registered the shadow. The native no-shadow byte
        // prevents a second registration (which would cycle its linked list).
        const unsigned char shadow=*(unsigned char*)(p+0x717);
        *(unsigned char*)(p+0x717)=1;
        func_ov002_020e444c(player);
        *(unsigned char*)(p+0x717)=shadow;
    }
}
static bool coop_menu_camera(void* cam) {
    if(!coop_menu_player_child() || !cam || !coop_menu_pose_ready)return false;
    int focus[3];memcpy(focus,coop_menu_position,sizeof focus);focus[1]+=125*4096;
    memcpy((char*)cam+0x8c,coop_menu_eye,sizeof coop_menu_eye);
    memcpy((char*)cam+0x80,focus,sizeof focus);
    fc_push_view(cam,coop_menu_eye,focus);
    return true;
}
