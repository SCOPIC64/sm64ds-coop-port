// The model smoothing configuration seam. See ntr/smooth.h for the contract;
// this file is the stub that carries it until the subdivision is written, and
// it deliberately does nothing but remember its argument.

#include "ntr/smooth.h"

namespace ntr {

namespace {
int g_level;
}  // namespace

void smooth_configure(int level)
{
    // Clamped at the point of use, the rule ntr::configure_aspect follows:
    // host_settings has already sanitised this, but the value is about to
    // size per-polygon work and does not get to trust its caller.
    if (level < 0) level = 0;
    if (level > 3) level = 3;
    g_level = level;
}

int smooth_level(void) { return g_level; }

}  // namespace ntr
