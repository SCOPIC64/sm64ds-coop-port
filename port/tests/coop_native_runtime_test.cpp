#include "ntr/mmio.h"
#include "ntr/rt.h"

#include <cassert>
#include <cstdio>
#include <cstdint>

extern "C" {
void _ZN3IRQ13SetIRQHandlerEjPFvvE(unsigned, void (*)());
unsigned _ZN3IRQ10EnableIRQsEj(unsigned);
int port_irqcb_band_ok();
extern char dsstate_lo, dsstate_hi;
extern unsigned char port_timer0_saved[];
}

static int ticks, returned, presentations, hblanks;
static int frame_limit;

static volatile uint32_t &reg32(uintptr_t address) {
    return *reinterpret_cast<volatile uint32_t *>(address);
}
static volatile uint16_t &reg16(uintptr_t address) {
    return *reinterpret_cast<volatile uint16_t *>(address);
}

static void on_hblank() { ++hblanks; }

static bool on_frame(uint64_t frame) {
    ++presentations;
    assert(frame == static_cast<uint64_t>(presentations));
    assert(ticks == presentations);
    assert(hblanks == 263 * presentations);
    assert(reg32(0x04000214) & ntr::IRQ_VBLANK);
    return !frame_limit || presentations < frame_limit;
}

static void nested_vblank(int depth) {
    volatile int local[64];
    for (int i = 0; i < 64; ++i) local[i] = ticks + depth + i;
    const int before = ticks;
    if (depth) nested_vblank(depth - 1);
    else {
        ++ticks;
        ntr::rt_vblank_wait();
        assert(!(reg32(0x04000214) & ntr::IRQ_VBLANK));
    }
    for (int i = 0; i < 64; ++i) assert(local[i] == before + depth + i);
}

static void game() {
    // The singleton runtime must reject reentry without replacing the game stack.
    assert(ntr::rt_run(game, on_frame, 1) == 0);
    for (int i = 0; i < 16; ++i) nested_vblank(3);
    ++returned;
}

static void reset() {
    ticks = returned = presentations = hblanks = frame_limit = 0;
    _ZN3IRQ13SetIRQHandlerEjPFvvE(ntr::IRQ_HBLANK, on_hblank);
    _ZN3IRQ10EnableIRQsEj(ntr::IRQ_HBLANK);
    reg16(0x04000004) |= 0x10;
}

int main() {
    assert(ntr::io_init());
    assert(port_irqcb_band_ok());
    const uintptr_t low = reinterpret_cast<uintptr_t>(&dsstate_lo);
    const uintptr_t high = reinterpret_cast<uintptr_t>(&dsstate_hi);
    const uintptr_t timer = reinterpret_cast<uintptr_t>(port_timer0_saved);
    assert(high > low && timer > low && timer + 20 <= high);
    assert(ntr::rt_run(nullptr, on_frame, 1) == 0);

    reset();
    assert(ntr::rt_run(game, on_frame) == 16);
    assert(returned == 1 && ticks == 16);
    reset();
    assert(ntr::rt_run(game, on_frame, 4) == 4);
    assert(returned == 0 && ticks == 4);
    reset();
    frame_limit = 7;
    assert(ntr::rt_run(game, on_frame) == 7);
    assert(returned == 0 && ticks == 7);
    reset();
    assert(ntr::rt_run(game, on_frame) == 16);
    assert(returned == 1 && ticks == 16);
    std::puts("Native DS runtime passed: fixed I/O, state section, IRQ bank, nested VBlank, 11309 HBlank deliveries, stop/restart");
}
