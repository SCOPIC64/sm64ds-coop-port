#include "ntr/frame_context.h"

#define MINICORO_IMPL
#include "../third_party/minicoro/minicoro.h"

#include <cstdlib>

namespace ntr {

struct FrameContext {
    mco_coro *coroutine;
    void (*entry)(void *);
    void *argument;
};

static void frame_entry(mco_coro *coroutine) {
    FrameContext *context = static_cast<FrameContext *>(mco_get_user_data(coroutine));
    context->entry(context->argument);
}

FrameContext *frame_context_create(void (*entry)(void *), void *argument,
                                  size_t stack_size) {
    if (!entry || stack_size < MCO_MIN_STACK_SIZE) return nullptr;
    FrameContext *context = static_cast<FrameContext *>(std::calloc(1, sizeof(FrameContext)));
    if (!context) return nullptr;
    context->entry = entry;
    context->argument = argument;
    mco_desc description = mco_desc_init(frame_entry, stack_size);
    description.user_data = context;
    if (mco_create(&context->coroutine, &description) != MCO_SUCCESS) {
        std::free(context);
        return nullptr;
    }
    return context;
}

FrameStep frame_context_resume(FrameContext *context) {
    if (!context) return FrameStep::failed;
    if (mco_status(context->coroutine) == MCO_DEAD) return FrameStep::completed;
    if (mco_resume(context->coroutine) != MCO_SUCCESS) return FrameStep::failed;
    return mco_status(context->coroutine) == MCO_DEAD
        ? FrameStep::completed : FrameStep::yielded;
}

bool frame_context_yield(FrameContext *context) {
    if (!context || mco_running() != context->coroutine) return false;
    return mco_yield(context->coroutine) == MCO_SUCCESS;
}

void frame_context_destroy(FrameContext *context) {
    if (!context) return;
    // A running context owns its stack; it cannot destroy that stack itself.
    if (mco_status(context->coroutine) == MCO_RUNNING ||
        mco_status(context->coroutine) == MCO_NORMAL) return;
    if (mco_destroy(context->coroutine) != MCO_SUCCESS) return;
    std::free(context);
}

} // namespace ntr
