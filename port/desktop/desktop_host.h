#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace sm64ds::desktop {
struct Config {
    const char* title="SM64DS Co-op";
    int width=1024,height=768;
    bool hidden=false,fullscreen=false;
};
struct Pad {
    unsigned buttons=0;
    int lx=0,ly=0,rx=0,ry=0,lt=0,rt=0;
};
struct Input {
    // Legacy PC key values keep existing portable three-slot bindings usable.
    std::array<bool,256> keys{};
    Pad pad;
    std::string text;
    bool focused=false,quit=false,resized=false,mouse_valid=false;
    int width=0,height=0,mouse_x=-1,mouse_y=-1;
};
struct Capture {
    int width=0,height=0;
    std::vector<uint32_t> pixels;
};
// Own and call this adapter on the main/UI thread (required by macOS).
// It does not depend on the game's 32-bit object or virtual-call ABI.
class Host {
    struct Impl;
    std::unique_ptr<Impl> impl_;
public:
    Host();
    ~Host();
    Host(const Host&)=delete;
    Host& operator=(const Host&)=delete;
    bool open(const Config&,std::string& error);
    void close();
    bool poll(Input&);
    bool present(const uint32_t* pixels,int width,int height,int stride,
                 std::string& error,Capture* capture=nullptr);
    bool fullscreen(bool enabled,std::string& error);
    bool text_input(bool enabled);
    bool open_audio(int sample_rate,std::string& error);
    bool queue_audio(const int16_t* stereo,unsigned frames);
    bool pause_audio(bool paused);
    unsigned queued_audio_frames() const;
    unsigned window_id() const;
};
}
