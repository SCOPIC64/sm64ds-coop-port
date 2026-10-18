#ifndef NTR_FRAME_CONTEXT_H
#define NTR_FRAME_CONTEXT_H

#include <stddef.h>

namespace ntr {

// A game stack suspended at VBlank, on the same OS thread as its presenter.
// Destroying a suspended stack abandons it, like Windows DeleteFiber; callers
// must keep host resources outside that stack and release them themselves.
struct FrameContext;
enum class FrameStep { yielded, completed, failed };

FrameContext *frame_context_create(void (*entry)(void *), void *argument,
                                  size_t stack_size = 256 * 1024);
FrameStep frame_context_resume(FrameContext *context);
bool frame_context_yield(FrameContext *context);
void frame_context_destroy(FrameContext *context);

} // namespace ntr
#endif
