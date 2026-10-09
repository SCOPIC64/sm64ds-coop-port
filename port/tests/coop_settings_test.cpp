#include "hal/host_settings.h"
#include <windows.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// This legacy accessor is declared at its call site in walk_window.cpp.
extern "C" int host_setting_run_mode(void);

static void expect_saved(bool native)
{
    assert(host_setting_camera_mode() == 1);
    assert(host_setting_run_mode() == 2);
    assert(host_setting_frame_rate() == (native ? 0 : 144));
    assert(host_setting_smooth_motion() == (native ? 1 : 0));
    assert(host_setting_name_tags() == (native ? 0 : 1));
    assert(host_setting_volume() == 35);
    assert(host_setting_mouse_capture() == (native ? 1 : 0));
    assert(host_setting_menu_music() == (native ? -1 : 1));
    assert(host_setting_menu_background() == (native ? 0 : 2));
    assert(host_setting_menu_sounds() == (native ? 1 : 0));
}

int main(int argc, char** argv)
{
    _set_error_mode(_OUT_TO_STDERR);
#ifdef _MSC_VER
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
    assert(argc == 3);
    const bool native = !std::strcmp(argv[2], "native");
    assert(native || !std::strcmp(argv[2], "144"));
    if (!std::strcmp(argv[1], "save")) {
        // CTest places this executable and its working directory in a dedicated
        // fixture folder. Never run this test beside a player's settings file.
        FILE* file = std::fopen("settings.json", "wb");
        assert(file);
        std::fputs("{\"Keep\":\"another menu's setting\",\"VoiceChatVolume\":63,"
                   "\"CameraMode\":\"ds\",\"SmoothMotion\":false,\"NameTags\":true}", file);
        assert(std::fclose(file) == 0);
        assert(host_setting_save_frontend_media(1, 2, native ? 0 : 144,
            native ? 1 : 0, native ? 0 : 1, 35, native ? 1 : 0,
            native ? -1 : 1, native ? 0 : 2, native ? 1 : 0));
        expect_saved(native);
        assert(!host_setting_save_frontend(-1, 2, 60, 1, 0, 35, 0));
        assert(!host_setting_save_frontend(1, 3, 60, 1, 0, 35, 0));
        assert(!host_setting_save_frontend(1, 2, 30, 1, 0, 35, 0));
        assert(!host_setting_save_frontend(1, 2, 241, 1, 0, 35, 0));
        assert(!host_setting_save_frontend(1, 2, 60, 1, 0, 101, 0));
        assert(!host_setting_save_frontend_media(1,2,60,1,0,35,0,999,0,1));
        assert(!host_setting_save_frontend_media(1,2,60,1,0,35,0,-1,3,1));
        expect_saved(native);
        HANDLE locked = CreateFileA("settings.json", GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        assert(locked != INVALID_HANDLE_VALUE);
        assert(!host_setting_save_frontend(0, 0, 60, 0, 1, 90, native ? 0 : 1));
        expect_saved(native);
        CloseHandle(locked);
    } else {
        assert(!std::strcmp(argv[1], "read"));
        expect_saved(native);
    }
    FILE* file = std::fopen("settings.json", "rb");
    assert(file);
    char text[4096] = {};
    size_t size = std::fread(text, 1, sizeof text - 1, file);
    assert(size > 0 && size < sizeof text - 1);
    std::fclose(file);
    assert(std::strstr(text, "\"Keep\":\"another menu's setting\""));
    assert(std::strstr(text, "\"VoiceChatVolume\":63"));
    std::puts("PASS: settings restart, unrelated keys, bounds and failed-write preservation");
}
