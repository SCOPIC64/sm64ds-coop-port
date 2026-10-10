#include <vector>

struct CoopSceneFrame {
    volatile LONG lock;
    unsigned serial;
    unsigned cinematic_frames;
    int camera_eye[3];
    unsigned credits_script;
    unsigned credits_cursor;
    int credits_level;
    int credits_shot;
    uint32_t pixels[512 * 384];
};
static bool coop_backdrop_child() { return getenv("SM64DS_MENU_BACKDROP_CHILD") != 0; }

// A hidden instance of this SAME executable renders the actual DS level.
// It has isolated saves, no network, no audio and no input. A kill-on-close
// job guarantees that closing/crashing the menu also closes its renderer.
class CoopBackdrop {
    HANDLE mapping_ = 0, job_ = 0, process_ = 0;
    CoopSceneFrame* shared_ = 0;
    std::vector<uint32_t> pixels_;
    int choice_ = -1, level_ = -1;
    unsigned last_serial_ = 0;
    DWORD retry_ = 0;
    char save_[MAX_PATH] = "";
    CoopMediaLibrary& library_;
    int tour_ = -1;
    unsigned scenes_ = 0;
    bool visible_ = false;
    unsigned cinematic_frames_ = 0;
    unsigned credits_visits_ = 0, last_credits_script_ = 0;
    int camera_eye_[3] = {};
    unsigned credits_script_=0,credits_cursor_=0;
    int credits_level_=-1;
    struct Env {
        const char* name; char old[2048]; bool present;
        Env(const char* key, const char* value) : name(key) {
            DWORD n = GetEnvironmentVariableA(key, old, sizeof old);
            present = n > 0 && n < sizeof old;
            SetEnvironmentVariableA(key, value);
        }
        ~Env() { SetEnvironmentVariableA(name, present ? old : 0); }
    };
public:
    explicit CoopBackdrop(CoopMediaLibrary& library) : library_(library) {}
    void invalidate() { choice_=-1; }
    ~CoopBackdrop() { stop(); }
    void stop() {
        if (job_) CloseHandle(job_);
        job_ = 0;
        if (process_) { WaitForSingleObject(process_, 1000); CloseHandle(process_); }
        process_ = 0;
        if (shared_) UnmapViewOfFile(shared_);
        shared_ = 0;
        if (mapping_) CloseHandle(mapping_);
        mapping_ = 0;
        if (save_[0]) { DeleteFileA(save_); char temp[MAX_PATH + 8]; snprintf(temp,sizeof temp,"%s.tmp",save_); DeleteFileA(temp); save_[0]=0; }
        last_serial_ = 0;
        last_credits_script_=0;
        credits_script_=credits_cursor_=0;credits_level_=-1;
        visible_ = false;
        cinematic_frames_ = 0;
    }
    bool start(int level, int shot = -1) {
        stop(); level_ = level;
        char exe[MAX_PATH], name[96], lv[16], shot_text[16];
        DWORD n = GetModuleFileNameA(0, exe, sizeof exe);
        if (!n || n >= sizeof exe) return false;
        snprintf(name,sizeof name,"Local\\SM64DSMenu-%lu-%lu",GetCurrentProcessId(),GetTickCount());
        mapping_=CreateFileMappingA(INVALID_HANDLE_VALUE,0,PAGE_READWRITE,0,sizeof(CoopSceneFrame),name);
        if (!mapping_) return false;
        shared_=static_cast<CoopSceneFrame*>(MapViewOfFile(mapping_,FILE_MAP_ALL_ACCESS,0,0,sizeof(CoopSceneFrame)));
        if (!shared_) { stop(); return false; }
        memset(shared_,0,sizeof *shared_);
        job_=CreateJobObjectA(0,0);
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits={};
        limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!job_ || !SetInformationJobObject(job_,JobObjectExtendedLimitInformation,&limits,sizeof limits)) { stop(); return false; }
        const char* root=getenv("SM64DS_ASSET_ROOT");
        snprintf(save_,sizeof save_,"%s/build/menu-backdrop-%lu-%lu.sav",root?root:".",GetCurrentProcessId(),GetTickCount());
        if (GetFileAttributesA(save_) != INVALID_FILE_ATTRIBUTES) { save_[0]=0; stop(); return false; }
        snprintf(lv,sizeof lv,"%d",level);
        snprintf(shot_text,sizeof shot_text,"%d",shot);
        Env child("SM64DS_MENU_BACKDROP_CHILD","1"), map("SM64DS_MENU_BACKDROP_MAP",name), level_env("SM64DS_LEVEL",lv);
        Env cinematic("SM64DS_MENU_NATIVE_STAFF_ROLL",shot>=0?"1":0), old_cinematic("SM64DS_MENU_STAFF_ROLL_SHOT",0), actors("SM64DS_NO_ACTORS",0);
        Env frames("SM64DS_WINDOW_SELFTEST","2000000000"), idle("SM64DS_SELFTEST_IDLE","1"), orbit("SM64DS_SELFTEST_ORBIT",0);
        Env character("SM64DS_CHARACTER","0"), camera_test("SM64DS_GAMEPLAY_PROBE",0);
        Env aspect("SM64DS_ASPECT","0"), scale("SM64DS_RENDER_SCALE","2"), rate("SM64DS_FRAME_RATE","0");
        Env save("SM64DS_SAVE_PATH",save_), audio("SM64DS_NO_AUDIO","1"), log("SM64DS_NO_PLAYLOG","1");
        Env focus("SM64DS_NO_FOCUS","1"), freecam("SM64DS_FREECAM",0);
        Env panel("SM64DS_SUB_PANEL","0");
        Env boot("SM64DS_BOOT_FILE",0), scene("SM64DS_SCENE",0), classic("SM64DS_CLASSIC_MENU",0), menu_test("SM64DS_MENU_SELFTEST",0);
        Env audio_test("SM64DS_MENU_AUDIO_SELFTEST",0), bg_test("SM64DS_MENU_BACKDROP_SELFTEST",0), wav("SM64DS_WAV_DUMP",0);
        Env role("SM64DS_COMMS_ROLE",0), party("SM64DS_PARTY",0), vs("SM64DS_VS_MAP",0), no_dialog("SM64DS_NO_DIALOG","1");
        SECURITY_ATTRIBUTES security={sizeof security,0,TRUE};
        const char* trace=getenv("SM64DS_MENU_BACKDROP_TRACE");
        HANDLE sink=CreateFileA(trace?trace:"NUL",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
            &security,trace?CREATE_ALWAYS:OPEN_EXISTING,0,0);
        STARTUPINFOA si={}; si.cb=sizeof si; si.dwFlags=STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES; si.wShowWindow=SW_HIDE;
        si.hStdInput=si.hStdOutput=si.hStdError=sink;
        PROCESS_INFORMATION pi={}; char command[MAX_PATH+4]; snprintf(command,sizeof command,"\"%s\"",exe);
        BOOL created=CreateProcessA(exe,command,0,0,TRUE,CREATE_SUSPENDED|CREATE_NO_WINDOW,0,root,&si,&pi);
        if (sink!=INVALID_HANDLE_VALUE) CloseHandle(sink);
        if (!created) { stop(); return false; }
        if (!AssignProcessToJobObject(job_,pi.hProcess)) {
            TerminateProcess(pi.hProcess,2); CloseHandle(pi.hThread); CloseHandle(pi.hProcess); stop(); return false;
        }
        process_=pi.hProcess; ResumeThread(pi.hThread); CloseHandle(pi.hThread);
        ++scenes_;
        return true;
    }
    void tick(int choice) {
        DWORD now=GetTickCount();
        bool changed=choice!=choice_;
        if (changed) { choice_=choice; retry_=now; tour_=-1; visible_=false; }
        if(choice>=coop_frontend::BACKGROUND_BUILTIN_COUNT) {
            if(changed) {
                stop();
                int index=choice-coop_frontend::BACKGROUND_BUILTIN_COUNT;
                if(index<static_cast<int>(library_.backgrounds.size()) &&
                    !coop_media::image(library_.backgrounds[index].utf8.c_str(),pixels_,512,384))
                    fprintf(stderr,"[coop-menu] custom background unavailable\n");
            }
            return;
        }
        const int levels[]={6,1,1,7,10,8,14,18};
        int level=choice>=0 && choice<coop_frontend::BACKGROUND_BUILTIN_COUNT?levels[choice]:6;
        if (choice==2) { level=6; if(changed)tour_=-1; }
        bool exited=process_ && WaitForSingleObject(process_,0)==WAIT_OBJECT_0;
        if ((changed || !process_ || exited || level!=level_) && static_cast<LONG>(now-retry_)>=0) {
            if (!start(level,choice==2?0:-1)) fprintf(stderr,"[coop-menu] backdrop could not start; menu stays available\n");
            retry_=now+3000;
        }
        if (shared_ && shared_->serial!=last_serial_ && InterlockedCompareExchange(&shared_->lock,1,0)==0) {
            if (shared_->serial) {
                if (pixels_.empty()) pixels_.resize(512*384);
                memcpy(pixels_.data(),shared_->pixels,sizeof shared_->pixels); last_serial_=shared_->serial;
                cinematic_frames_=shared_->cinematic_frames;
                credits_script_=shared_->credits_script;credits_cursor_=shared_->credits_cursor;
                credits_level_=shared_->credits_level;
                if(choice==2) {
                    tour_=shared_->credits_shot;
                    if(shared_->credits_script && shared_->credits_script!=last_credits_script_) {
                        last_credits_script_=shared_->credits_script; ++credits_visits_;
                    }
                }
                memcpy(camera_eye_,shared_->camera_eye,sizeof camera_eye_);
                if(!visible_) {
                    visible_=true;
                }
            }
            InterlockedExchange(&shared_->lock,0);
        }
    }
    const uint32_t* image() const { return pixels_.empty()?0:pixels_.data(); }
    unsigned credits_script() const { return credits_script_; }
    unsigned credits_cursor() const { return credits_cursor_; }
    int credits_level() const { return credits_level_; }
    unsigned credits_visits() const { return credits_visits_; }
    unsigned serial() const { return last_serial_; }
    unsigned scenes() const { return scenes_; }
    int shot() const { return tour_; }
    unsigned cinematic_frames() const { return cinematic_frames_; }
    const int* camera_eye() const { return camera_eye_; }
};

static void coop_backdrop_publish(const ntr::Framebuffer& fb) {
    if (!coop_backdrop_child()) return;
    static HANDLE mapping=0;
    static CoopSceneFrame* shared=0;
    if (!mapping) {
        const char* name=getenv("SM64DS_MENU_BACKDROP_MAP");
        if (!name || strncmp(name,"Local\\SM64DSMenu-",sizeof("Local\\SM64DSMenu-")-1)) return;
        mapping=OpenFileMappingA(FILE_MAP_ALL_ACCESS,FALSE,name);
        if (mapping) shared=static_cast<CoopSceneFrame*>(MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(CoopSceneFrame)));
    }
    if (shared && InterlockedCompareExchange(&shared->lock,1,0)==0) {
        for (int y=0;y<384;++y) for(int x=0;x<512;++x)
            shared->pixels[y*512+x]=fb.px[y*ntr::active_h/384][x*ntr::active_w/512];
        shared->cinematic_frames=g_staff_camera_frames;
        memcpy(shared->camera_eye,g_staff_eye,sizeof shared->camera_eye);
        shared->credits_script=(unsigned)data_0209fc48;
        shared->credits_cursor=data_0209b274;
        shared->credits_level=data_0209f2f8;
        const unsigned char* script=(const unsigned char*)(size_t)data_0209fc48;
        shared->credits_shot=coop_staff_roll_child() && script && script[0]==8 && script[1]==0x2e?script[7]-1:-1;
        ++shared->serial; InterlockedExchange(&shared->lock,0);
    }
    // Limit this preview renderer even though the engine's test loop is unpaced.
    static DWORD last=GetTickCount(); DWORD now=GetTickCount(), elapsed=now-last;
    if(!getenv("SM64DS_MENU_CREDITS_FAST") && elapsed<33) Sleep(33-elapsed);
    last=GetTickCount();
}
