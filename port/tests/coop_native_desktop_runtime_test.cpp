#include "desktop_host.h"
#include "ntr/mmio.h"
#include "ntr/owned_memory.h"
#include "ntr/ppu.h"
#include "ntr/rt.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static sm64ds::desktop::Host host;
static ntr::SubFramebuffer frame;
static unsigned tick=0,presentations=0;
static void game() {
    for(unsigned i=1;i<=6;++i) {
        tick=i;
        *reinterpret_cast<volatile uint16_t*>(0x05000400)=i&1?0x001f:0x7c00;
        ntr::rt_vblank_wait();
    }
}
static bool present(uint64_t number) {
    assert(number==tick);++presentations;
    sm64ds::desktop::Input input;assert(host.poll(input)&&!input.quit);
    ntr::ppu_scanout_sub(frame);
    const uint32_t expected=tick&1?0xffff0000u:0xff0000ffu;
    for(const auto& row:frame.px)for(uint32_t pixel:row)assert(pixel==expected);
    sm64ds::desktop::Capture output;std::string error;
    assert(host.present(&frame.px[0][0],ntr::SUB_W,ntr::SUB_H,ntr::SUB_W,error,&output));
    assert(output.width==ntr::SUB_W&&output.height==ntr::SUB_H);
    for(uint32_t pixel:output.pixels)assert(pixel==expected);
    std::vector<int16_t> silent(512,0);assert(host.queue_audio(silent.data(),256));
    return true;
}
int main() {
    assert(ntr::io_init());
    assert(ntr::io_owns_range(0x02086b60,96));assert(!ntr::io_owns_range(0x01000000,96));
    *reinterpret_cast<volatile uint32_t*>(0x04001000)=1u<<16; // engine B, normal scanout, backdrop only
    sm64ds::desktop::Config config;config.hidden=true;config.width=ntr::SUB_W;config.height=ntr::SUB_H;
    std::string error;assert(host.open(config,error));assert(host.open_audio(32768,error));
    assert(ntr::rt_run(game,present)==6&&tick==6&&presentations==6);
    host.close();std::cout<<"Actual Linux32 DS frame runtime -> PPU -> SDL desktop pixels/audio PASS (ROM-free fixture, not full game boot)\n";
}
