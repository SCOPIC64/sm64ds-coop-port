#include "desktop_host.h"
#include <SDL.h>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iostream>
#include <thread>

using namespace sm64ds::desktop;
static void focus(Host& host,bool gained) {
    SDL_Event event{};event.type=SDL_WINDOWEVENT;event.window.windowID=host.window_id();
    event.window.event=gained?SDL_WINDOWEVENT_FOCUS_GAINED:SDL_WINDOWEVENT_FOCUS_LOST;
    assert(SDL_PushEvent(&event)==1);
}
static void key(unsigned id,SDL_Keycode symbol,bool down,bool repeat=false) {
    SDL_Event event{};event.type=down?SDL_KEYDOWN:SDL_KEYUP;
    event.key.windowID=id;event.key.keysym.sym=symbol;event.key.repeat=repeat?1:0;
    assert(SDL_PushEvent(&event)==1);
}
int main() {
    // The hidden fixture has no OS keyboard focus; input remains gated by the
    // host's injected focus events rather than SDL's physical desktop window.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    Host host;Config config;config.width=256;config.height=192;config.hidden=true;
    std::string error;assert(host.open(config,error));assert(error.empty());
    Input input;assert(host.poll(input));assert(!input.quit);
    focus(host,true);key(host.window_id(),SDLK_w,true);key(host.window_id()+1,SDLK_a,true);
    assert(host.poll(input)&&input.focused&&input.keys['W']&&!input.keys['A']);
    key(host.window_id(),SDLK_LSHIFT,true);key(host.window_id(),SDLK_RSHIFT,true);
    key(host.window_id(),SDLK_LSHIFT,false);assert(host.poll(input)&&input.keys[16]&&input.keys[161]&&!input.keys[160]);
    assert(host.text_input(true));SDL_Event text{};text.type=SDL_TEXTINPUT;text.text.windowID=host.window_id();
    SDL_strlcpy(text.text.text,"chat \xc3\xa9",sizeof text.text.text);assert(SDL_PushEvent(&text)==1);
    assert(host.poll(input)&&input.text=="chat \xc3\xa9");assert(host.poll(input)&&input.text.empty());
    focus(host,false);key(host.window_id(),SDLK_w,true,true);
    assert(host.poll(input)&&!input.focused&&!input.keys['W']&&!input.keys[16]);
    focus(host,true);key(host.window_id(),SDLK_w,true,true);assert(host.poll(input)&&!input.keys['W']);
    key(host.window_id(),SDLK_w,false);key(host.window_id(),SDLK_w,true);assert(host.poll(input)&&input.keys['W']);

    // Actual SDL controller mapping/axis state, using an isolated virtual device.
    int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,SDL_CONTROLLER_AXIS_MAX,SDL_CONTROLLER_BUTTON_MAX,0);
    assert(device>=0);char guid[33];SDL_JoystickGetGUIDString(SDL_JoystickGetDeviceGUID(device),guid,sizeof guid);
    std::string mapping=std::string(guid)+",SM64DS isolated test,a:b0,b:b1,x:b2,y:b3,back:b4,start:b6,leftstick:b7,rightstick:b8,leftshoulder:b9,rightshoulder:b10,dpup:b11,dpdown:b12,dpleft:b13,dpright:b14,leftx:a0,lefty:a1,rightx:a2,righty:a3,lefttrigger:a4,righttrigger:a5,";
    assert(SDL_GameControllerAddMapping(mapping.c_str())>=0);
    SDL_Joystick* stick=SDL_JoystickOpen(device);assert(stick);
    assert(SDL_JoystickSetVirtualButton(stick,SDL_CONTROLLER_BUTTON_A,1)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_LEFTX,12345)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_LEFTY,-8192)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_TRIGGERLEFT,-32768)==0);
    assert(SDL_JoystickSetVirtualAxis(stick,SDL_CONTROLLER_AXIS_TRIGGERRIGHT,32767)==0);
    SDL_JoystickUpdate();assert(host.poll(input));
    assert((input.pad.buttons&0x1000)&&input.pad.lx==12345&&input.pad.ly==8192&&input.pad.lt==0&&input.pad.rt==255);
    focus(host,false);assert(host.poll(input)&&input.pad.buttons==0&&input.pad.lx==0);
    assert(SDL_JoystickDetachVirtual(device)==0);SDL_JoystickClose(stick);assert(host.poll(input)&&input.pad.buttons==0);

    const int stride=265;std::vector<uint32_t> frame(stride*192,0xff112233u);
    for(int y=0;y<192;++y)for(int x=0;x<256;++x)frame[y*stride+x]=y<96?0xffff0000u:0xff0000ffu;
    Capture capture;assert(host.present(frame.data(),256,192,stride,error,&capture));
    assert(capture.width==256&&capture.height==192);
    for(int y=0;y<192;++y)for(int x=0;x<256;++x)assert(capture.pixels[y*256+x]==(y<96?0xffff0000u:0xff0000ffu));
    assert(!host.present(frame.data(),256,192,255,error));assert(!error.empty());
    SDL_Window* window=SDL_GetWindowFromID(host.window_id());assert(window);
    SDL_SetWindowSize(window,384,192);assert(host.poll(input)&&input.resized&&input.width==384&&input.height==192);
    assert(host.present(frame.data(),256,192,stride,error,&capture));
    // A stale 4:3 frame is fitted, never stretched. The engine uses the reported
    // drawable aspect to expand its camera/framebuffer in the full-game adapter.
    assert(capture.pixels[0]==0xff000000u&&capture.pixels[64]==0xffff0000u&&capture.pixels[383]==0xff000000u);
    frame.assign(384*192,0xff00ff00u);assert(host.present(frame.data(),384,192,384,error,&capture));
    assert(std::all_of(capture.pixels.begin(),capture.pixels.end(),[](uint32_t pixel){return pixel==0xff00ff00u;}));
    assert(host.fullscreen(true,error));assert(host.fullscreen(false,error));

    assert(host.open_audio(32768,error));std::vector<int16_t> silent(512,0);
    int writes=0;while(host.queue_audio(silent.data(),256)){++writes;assert(writes<32);}
    assert(writes>0&&host.queued_audio_frames()<=32768u*80/1000);
    assert(!host.queue_audio(silent.data(),~0u));assert(host.pause_audio(false));assert(host.pause_audio(true));
    bool foreign_result=true;std::thread foreign([&]{Input other;foreign_result=host.poll(other);});foreign.join();assert(!foreign_result);
    SDL_Event quit{};quit.type=SDL_WINDOWEVENT;quit.window.windowID=host.window_id();quit.window.event=SDL_WINDOWEVENT_CLOSE;
    assert(SDL_PushEvent(&quit)==1);assert(host.poll(input)&&input.quit);
    host.close();assert(host.window_id()==0);assert(host.open(config,error));assert(host.poll(input)&&!input.quit);
    host.close();std::cout<<"Native SDL desktop adapter PASS: exact framebuffer pixels/stride, resize aspect, focus-safe keys, UTF-8 text, virtual controller/hotplug, fullscreen, bounded PCM queue, UI thread and restart\n";
}
