#include "desktop_host.h"
#include <SDL.h>
#include <algorithm>
#include <cassert>
#include <limits>

namespace sm64ds::desktop {
namespace {
int key_code(SDL_Keycode key) {
    if(key>=SDLK_a&&key<=SDLK_z)return 'A'+static_cast<int>(key-SDLK_a);
    if(key>=SDLK_0&&key<=SDLK_9)return '0'+static_cast<int>(key-SDLK_0);
    if(key>=SDLK_F1&&key<=SDLK_F12)return 112+static_cast<int>(key-SDLK_F1);
    switch(key) {
    case SDLK_BACKSPACE:return 8;case SDLK_TAB:return 9;case SDLK_RETURN:return 13;
    case SDLK_ESCAPE:return 27;case SDLK_SPACE:return 32;
    case SDLK_PAGEUP:return 33;case SDLK_PAGEDOWN:return 34;case SDLK_END:return 35;case SDLK_HOME:return 36;
    case SDLK_LEFT:return 37;case SDLK_UP:return 38;case SDLK_RIGHT:return 39;case SDLK_DOWN:return 40;
    case SDLK_INSERT:return 45;case SDLK_DELETE:return 46;
    case SDLK_LSHIFT:return 160;case SDLK_RSHIFT:return 161;case SDLK_LCTRL:return 162;case SDLK_RCTRL:return 163;
    case SDLK_LALT:return 164;case SDLK_RALT:return 165;case SDLK_LGUI:return 91;case SDLK_RGUI:return 92;
    case SDLK_SEMICOLON:return 186;case SDLK_EQUALS:return 187;case SDLK_COMMA:return 188;
    case SDLK_MINUS:return 189;case SDLK_PERIOD:return 190;case SDLK_SLASH:return 191;case SDLK_BACKQUOTE:return 192;
    case SDLK_LEFTBRACKET:return 219;case SDLK_BACKSLASH:return 220;case SDLK_RIGHTBRACKET:return 221;case SDLK_QUOTE:return 222;
    default:return 0;
    }
}
int mouse_code(Uint8 button) {
    switch(button){case SDL_BUTTON_LEFT:return 1;case SDL_BUTTON_RIGHT:return 2;
    case SDL_BUTTON_MIDDLE:return 4;case SDL_BUTTON_X1:return 5;case SDL_BUTTON_X2:return 6;default:return 0;}
}
}
struct Host::Impl {
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    SDL_Texture* texture=nullptr;
    SDL_GameController* controller=nullptr;
    SDL_AudioDeviceID audio=0;
    SDL_threadID owner=SDL_ThreadID();
    Uint32 initialized=0;
    int texture_w=0,texture_h=0,sample_rate=0;
    SDL_Rect viewport{};
    Input input;
    bool text=false;
    bool on_thread() const {return owner==SDL_ThreadID();}
    void clear_input(){input.keys.fill(false);input.pad={};input.text.clear();input.mouse_valid=false;}
    void controller_open(int index) {
        if(!controller&&SDL_IsGameController(index))controller=SDL_GameControllerOpen(index);
    }
    ~Impl() {
        assert(on_thread());
        if(text)SDL_StopTextInput();
        if(audio)SDL_CloseAudioDevice(audio);
        if(controller)SDL_GameControllerClose(controller);
        if(texture)SDL_DestroyTexture(texture);
        if(renderer)SDL_DestroyRenderer(renderer);
        if(window)SDL_DestroyWindow(window);
        if(initialized)SDL_QuitSubSystem(initialized);
    }
};
Host::Host()=default;
Host::~Host()=default;
void Host::close(){if(impl_&&impl_->on_thread())impl_.reset();}
bool Host::open(const Config& config,std::string& error) {
    error.clear();
    if(impl_&&!impl_->on_thread()){error="Desktop host must run on its UI thread";return false;}
    if(config.width<1||config.height<1||config.width>8192||config.height>8192||!config.title){error="Invalid desktop window configuration";return false;}
    close();SDL_SetMainReady();
    auto next=std::make_unique<Impl>();
    const Uint32 systems=SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER;
    if(SDL_InitSubSystem(systems)){error=SDL_GetError();return false;}
    next->initialized=systems;
    Uint32 flags=SDL_WINDOW_RESIZABLE|SDL_WINDOW_ALLOW_HIGHDPI;
    if(config.hidden)flags|=SDL_WINDOW_HIDDEN;
    if(config.fullscreen)flags|=SDL_WINDOW_FULLSCREEN_DESKTOP;
    next->window=SDL_CreateWindow(config.title,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,config.width,config.height,flags);
    if(!next->window){error=SDL_GetError();return false;}
    // Presentation never waits for vsync: the game's frame scheduler owns timing.
    next->renderer=SDL_CreateRenderer(next->window,-1,SDL_RENDERER_ACCELERATED);
    if(!next->renderer)next->renderer=SDL_CreateRenderer(next->window,-1,SDL_RENDERER_SOFTWARE);
    if(!next->renderer){error=SDL_GetError();return false;}
    SDL_GetRendererOutputSize(next->renderer,&next->input.width,&next->input.height);
    next->input.focused=(SDL_GetWindowFlags(next->window)&SDL_WINDOW_INPUT_FOCUS)!=0;
    for(int i=0;i<SDL_NumJoysticks();++i)next->controller_open(i);
    SDL_StopTextInput();impl_=std::move(next);return true;
}
unsigned Host::window_id() const {return impl_&&impl_->on_thread()?SDL_GetWindowID(impl_->window):0;}
bool Host::fullscreen(bool enabled,std::string& error) {
    error.clear();if(!impl_||!impl_->on_thread()){error="Desktop host is not available on this thread";return false;}
    if(SDL_SetWindowFullscreen(impl_->window,enabled?SDL_WINDOW_FULLSCREEN_DESKTOP:0)){error=SDL_GetError();return false;}return true;
}
bool Host::text_input(bool enabled) {
    if(!impl_||!impl_->on_thread())return false;
    impl_->text=enabled;if(enabled&&impl_->input.focused)SDL_StartTextInput();else SDL_StopTextInput();return true;
}
bool Host::poll(Input& result) {
    if(!impl_||!impl_->on_thread())return false;
    auto& p=*impl_;auto& input=p.input;input.text.clear();input.resized=false;
    const Uint32 id=SDL_GetWindowID(p.window);SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT:input.quit=true;break;
        case SDL_WINDOWEVENT:
            if(event.window.windowID!=id)break;
            if(event.window.event==SDL_WINDOWEVENT_CLOSE)input.quit=true;
            if(event.window.event==SDL_WINDOWEVENT_FOCUS_LOST){input.focused=false;p.clear_input();SDL_StopTextInput();}
            if(event.window.event==SDL_WINDOWEVENT_FOCUS_GAINED){input.focused=true;if(p.text)SDL_StartTextInput();}
            break;
        case SDL_KEYDOWN:case SDL_KEYUP:
            if(event.key.windowID==id&&input.focused&&!event.key.repeat) {
                int code=key_code(event.key.keysym.sym);if(code)input.keys[static_cast<size_t>(code)]=event.type==SDL_KEYDOWN;
                if(event.type==SDL_KEYDOWN&&event.key.keysym.sym==SDLK_F11) {
                    std::string ignored;fullscreen(!(SDL_GetWindowFlags(p.window)&SDL_WINDOW_FULLSCREEN_DESKTOP),ignored);
                }
            }
            break;
        case SDL_MOUSEBUTTONDOWN:case SDL_MOUSEBUTTONUP:
            if(event.button.windowID==id&&input.focused) {
                int code=mouse_code(event.button.button);if(code)input.keys[static_cast<size_t>(code)]=event.type==SDL_MOUSEBUTTONDOWN;
            }
            break;
        case SDL_TEXTINPUT:
            if(event.text.windowID==id&&input.focused&&p.text&&input.text.size()+SDL_strlen(event.text.text)<=1024)input.text+=event.text.text;
            break;
        case SDL_CONTROLLERDEVICEADDED:p.controller_open(event.cdevice.which);break;
        case SDL_CONTROLLERDEVICEREMOVED:
            if(p.controller&&SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(p.controller))==event.cdevice.which) {
                SDL_GameControllerClose(p.controller);p.controller=nullptr;input.pad={};
                for(int i=0;i<SDL_NumJoysticks();++i)p.controller_open(i);
            }
            break;
        default:break;
        }
    }
    input.keys[16]=input.keys[160]||input.keys[161];input.keys[17]=input.keys[162]||input.keys[163];input.keys[18]=input.keys[164]||input.keys[165];
    int width=0,height=0;SDL_GetRendererOutputSize(p.renderer,&width,&height);
    input.resized=width!=input.width||height!=input.height;input.width=width;input.height=height;
    input.pad={};
    if(input.focused&&p.controller&&SDL_GameControllerGetAttached(p.controller)) {
        const std::pair<SDL_GameControllerButton,unsigned> buttons[]={
            {SDL_CONTROLLER_BUTTON_DPAD_UP,1},{SDL_CONTROLLER_BUTTON_DPAD_DOWN,2},{SDL_CONTROLLER_BUTTON_DPAD_LEFT,4},{SDL_CONTROLLER_BUTTON_DPAD_RIGHT,8},
            {SDL_CONTROLLER_BUTTON_START,0x10},{SDL_CONTROLLER_BUTTON_BACK,0x20},{SDL_CONTROLLER_BUTTON_LEFTSTICK,0x40},{SDL_CONTROLLER_BUTTON_RIGHTSTICK,0x80},
            {SDL_CONTROLLER_BUTTON_LEFTSHOULDER,0x100},{SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,0x200},
            {SDL_CONTROLLER_BUTTON_A,0x1000},{SDL_CONTROLLER_BUTTON_B,0x2000},{SDL_CONTROLLER_BUTTON_X,0x4000},{SDL_CONTROLLER_BUTTON_Y,0x8000}};
        for(auto button:buttons)if(SDL_GameControllerGetButton(p.controller,button.first))input.pad.buttons|=button.second;
        input.pad.lx=SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_LEFTX);
        input.pad.ly=(std::min)(32767,-int(SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_LEFTY)));
        input.pad.rx=SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_RIGHTX);
        input.pad.ry=(std::min)(32767,-int(SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_RIGHTY)));
        input.pad.lt=(std::max)(0,int(SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_TRIGGERLEFT)))*255/32767;
        input.pad.rt=(std::max)(0,int(SDL_GameControllerGetAxis(p.controller,SDL_CONTROLLER_AXIS_TRIGGERRIGHT)))*255/32767;
    }
    int mx=0,my=0,ww=0,wh=0;SDL_GetMouseState(&mx,&my);SDL_GetWindowSize(p.window,&ww,&wh);
    input.mouse_valid=false;input.mouse_x=input.mouse_y=-1;
    if(input.focused&&ww>0&&wh>0&&p.viewport.w>0&&p.viewport.h>0) {
        int x=int(int64_t(mx)*width/ww)-p.viewport.x,y=int(int64_t(my)*height/wh)-p.viewport.y;
        if(x>=0&&y>=0&&x<p.viewport.w&&y<p.viewport.h) {
            input.mouse_valid=true;input.mouse_x=int(int64_t(x)*p.texture_w/p.viewport.w);input.mouse_y=int(int64_t(y)*p.texture_h/p.viewport.h);
        }
    }
    result=input;return true;
}
bool Host::present(const uint32_t* pixels,int width,int height,int stride,std::string& error,Capture* capture) {
    error.clear();if(!impl_||!impl_->on_thread()){error="Desktop host is not available on this thread";return false;}
    if(!pixels||width<1||height<1||width>4096||height>4096||stride<width||stride>(std::numeric_limits<int>::max)()/4){error="Invalid framebuffer";return false;}
    auto& p=*impl_;
    if(!p.texture||p.texture_w!=width||p.texture_h!=height) {
        SDL_Texture* next=SDL_CreateTexture(p.renderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,width,height);
        if(!next){error=SDL_GetError();return false;}
        SDL_SetTextureBlendMode(next,SDL_BLENDMODE_NONE);
        if(p.texture)SDL_DestroyTexture(p.texture);
        p.texture=next;p.texture_w=width;p.texture_h=height;
    }
    if(SDL_UpdateTexture(p.texture,nullptr,pixels,stride*4)){error=SDL_GetError();return false;}
    int ow=0,oh=0;if(SDL_GetRendererOutputSize(p.renderer,&ow,&oh)||ow<1||oh<1){error=SDL_GetError();return false;}
    SDL_Rect rect{0,0,ow,int(int64_t(ow)*height/width)};
    if(rect.h>oh){rect.h=oh;rect.w=int(int64_t(oh)*width/height);}
    rect.x=(ow-rect.w)/2;rect.y=(oh-rect.h)/2;p.viewport=rect;
    // The engine must expand its camera/framebuffer to the drawable aspect for
    // fullscreen outside-view. Never stretch a stale 4:3 frame after resizing.
    SDL_SetRenderDrawColor(p.renderer,0,0,0,255);
    if(SDL_RenderClear(p.renderer)||SDL_RenderCopy(p.renderer,p.texture,nullptr,&rect)){error=SDL_GetError();return false;}
    if(capture) {
        if(int64_t(ow)*oh>16777216){error="Capture exceeds pixel limit";return false;}
        capture->width=ow;capture->height=oh;capture->pixels.resize(static_cast<size_t>(ow)*oh);
        if(SDL_RenderReadPixels(p.renderer,nullptr,SDL_PIXELFORMAT_ARGB8888,capture->pixels.data(),ow*4)){error=SDL_GetError();return false;}
    }
    SDL_RenderPresent(p.renderer);return true;
}
bool Host::open_audio(int rate,std::string& error) {
    error.clear();if(!impl_||!impl_->on_thread()||rate<8000||rate>96000){error="Invalid audio configuration";return false;}
    auto& p=*impl_;if(p.audio){SDL_CloseAudioDevice(p.audio);p.audio=0;}
    if(!(p.initialized&SDL_INIT_AUDIO)){if(SDL_InitSubSystem(SDL_INIT_AUDIO)){error=SDL_GetError();return false;}p.initialized|=SDL_INIT_AUDIO;}
    SDL_AudioSpec desired{},obtained{};desired.freq=rate;desired.channels=2;desired.format=AUDIO_S16SYS;desired.samples=512;
    p.audio=SDL_OpenAudioDevice(nullptr,0,&desired,&obtained,0);
    if(!p.audio){error=SDL_GetError();return false;}
    p.sample_rate=rate;return true;
}
bool Host::queue_audio(const int16_t* stereo,unsigned frames) {
    if(!impl_||!impl_->on_thread()||!impl_->audio||!stereo||!frames)return false;
    auto& p=*impl_;const unsigned limit=static_cast<unsigned>(p.sample_rate)*80/1000;
    if(frames>limit||SDL_GetQueuedAudioSize(p.audio)/4>limit-frames)return false;
    return SDL_QueueAudio(p.audio,stereo,frames*4)==0;
}
bool Host::pause_audio(bool paused) {
    if(!impl_||!impl_->on_thread()||!impl_->audio)return false;
    SDL_PauseAudioDevice(impl_->audio,paused?1:0);return true;
}
unsigned Host::queued_audio_frames() const {return impl_&&impl_->on_thread()&&impl_->audio?SDL_GetQueuedAudioSize(impl_->audio)/4:0;}
}
