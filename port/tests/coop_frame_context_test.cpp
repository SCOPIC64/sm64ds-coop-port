#include "ntr/frame_context.h"

#include <cassert>
#include <cstdio>
#include <thread>

struct Game {
    ntr::FrameContext *context = nullptr;
    int ticks = 0;
    int returned = 0;
    std::thread::id owner;
};

static void nested_tick(Game *game, int depth) {
    // Callee stack storage must survive host presentation and the next resume.
    volatile unsigned char saved[4096];
    for (unsigned i = 0; i < sizeof(saved); ++i)
        saved[i] = static_cast<unsigned char>(i + depth + game->ticks);
    const int tick = game->ticks;
    if (depth) nested_tick(game, depth - 1);
    else {
        ++game->ticks;
        assert(std::this_thread::get_id() == game->owner);
        assert(ntr::frame_context_yield(game->context));
        assert(std::this_thread::get_id() == game->owner);
    }
    for (unsigned i = 0; i < sizeof(saved); ++i)
        assert(saved[i] == static_cast<unsigned char>(i + depth + tick));
}

static void run_game(void *argument) {
    Game *game = static_cast<Game *>(argument);
    for (int i = 0; i < 600; ++i) nested_tick(game, 3);
    ++game->returned;
}

int main() {
    assert(!ntr::frame_context_create(nullptr, nullptr));
    assert(!ntr::frame_context_create(run_game, nullptr, 1));
    assert(ntr::frame_context_resume(nullptr) == ntr::FrameStep::failed);
    assert(!ntr::frame_context_yield(nullptr));
    ntr::frame_context_destroy(nullptr);

    Game first, second;
    first.owner = second.owner = std::this_thread::get_id();
    first.context = ntr::frame_context_create(run_game, &first);
    second.context = ntr::frame_context_create(run_game, &second);
    assert(first.context && second.context);
    assert(!ntr::frame_context_yield(first.context));
    for (int frame = 1; frame <= 600; ++frame) {
        assert(ntr::frame_context_resume(first.context) == ntr::FrameStep::yielded);
        assert(first.ticks == frame && second.ticks == frame - 1);
        assert(first.returned == 0);
        assert(ntr::frame_context_resume(second.context) == ntr::FrameStep::yielded);
        assert(second.ticks == frame && second.returned == 0);
    }
    assert(ntr::frame_context_resume(first.context) == ntr::FrameStep::completed);
    assert(ntr::frame_context_resume(second.context) == ntr::FrameStep::completed);
    assert(first.returned == 1 && second.returned == 1);
    assert(ntr::frame_context_resume(first.context) == ntr::FrameStep::completed);
    ntr::frame_context_destroy(first.context);
    ntr::frame_context_destroy(second.context);

    // Stopping a frame loop must discard the suspended game, then allow restart.
    Game stopped;
    stopped.owner = std::this_thread::get_id();
    stopped.context = ntr::frame_context_create(run_game, &stopped);
    assert(stopped.context);
    assert(ntr::frame_context_resume(stopped.context) == ntr::FrameStep::yielded);
    ntr::frame_context_destroy(stopped.context);
    assert(stopped.ticks == 1 && stopped.returned == 0);
    stopped.context = ntr::frame_context_create(run_game, &stopped);
    assert(stopped.context);
    assert(ntr::frame_context_resume(stopped.context) == ntr::FrameStep::yielded);
    assert(stopped.ticks == 2);
    ntr::frame_context_destroy(stopped.context);
    std::puts("Native frame contexts passed: nested yields, thread ownership, 1200 frames, stop/restart");
}
