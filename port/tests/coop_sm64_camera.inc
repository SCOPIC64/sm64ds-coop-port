extern "C" int port_sm64_can_inspect(const void* player);
static bool coop_sm64_camera_draw(void* cam,char* player) {
    if(cam_mode!=CAM_SM64 || !cam || !player || data_0209fc48)return false;
    const int* position=(const int*)(player+0x5c);
    static int last_tick=-1;
    if(!sm64_camera.live){sm64_camera.seed(position,(const int*)((char*)cam+0x8c),*(short*)(player+0x8e));last_tick=-1;}
    const int tick=port_rom_frame();
    const bool stepped=tick!=last_tick;
    if(stepped){sm64_camera.step(position,*(short*)(player+0x8e),*(int*)(player+0x98)!=0);last_tick=tick;}
    if(stepped && getenv("SM64DS_GAMEPLAY_PROBE") && tick%30==0)
        fprintf(stderr,"[sm64-cam] tick=%d yaw=%d side=%d distance=%.1f mario=%d c_up=%d pitch=%d look=%d\n",tick,
            (int)sm64_camera.yaw,sm64_camera.side,sm64_camera.distance,sm64_camera.mario,sm64_camera.inspecting,sm64_camera.inspect_pitch,sm64_camera.inspect_yaw);
    int eye[3],focus[3],hit[3];
    for(int axis=0;axis<3;++axis){eye[axis]=(int)(sm64_camera.eye[axis]*4096);focus[axis]=(int)(sm64_camera.focus[axis]*4096);}
    // Adapt wall/floor avoidance to DS collision geometry. The original N64
    // level-specific camera volumes have no matching DS data and remain DS-owned.
    if(g_mc && hal_line_ray(g_mc,focus,eye,hit)) {
        double length=0;
        for(int axis=0;axis<3;++axis){double delta=(double)hit[axis]-focus[axis];length+=delta*delta;}
        length=std::sqrt(length);
        double fraction=length>0?(std::max)(0.0,(length-80*4096)/length):0;
        for(int axis=0;axis<3;++axis)eye[axis]=focus[axis]+(int)(((double)hit[axis]-focus[axis])*fraction);
    }
    int floor=0;
    if(g_mc && hal_ground_ray(g_mc,eye[0],eye[1]+200*4096,eye[2],1024*4096,&floor))
        eye[1]=(std::max)(eye[1],floor+(sm64_camera.inspecting?40:125)*4096);
    memcpy((char*)cam+0x8c,eye,sizeof eye);memcpy((char*)cam+0x80,focus,sizeof focus);
    fc_yaw=sm64_camera.yaw;
    fc_push_view(cam,eye,focus);
    return true;
}
