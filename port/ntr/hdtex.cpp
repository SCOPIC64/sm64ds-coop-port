// The HD texture pack's configuration seam. See ntr/hdtex.h for the whole
// contract; this file is the stub that carries it until the pack loader is
// written, and it deliberately does nothing but remember its arguments.

#include "ntr/hdtex.h"

#include <string.h>

namespace ntr {

namespace {
int g_enabled;
// The directory is COPIED rather than kept as the caller's pointer. The one
// caller hands over host_settings' static buffer, which is stable for the
// run, but a copy costs one page and makes this file's lifetime rule its own
// business instead of an assumption about somebody else's storage.
char g_pack_dir[1024];
}  // namespace

void hdtex_configure(int enabled, const char *pack_dir)
{
    g_pack_dir[0] = '\0';
    if (pack_dir && *pack_dir) {
        strncpy(g_pack_dir, pack_dir, sizeof g_pack_dir - 1);
        g_pack_dir[sizeof g_pack_dir - 1] = '\0';
    }
    // A pack with nowhere to load from is not a pack, so the directory is
    // part of "on" and not a separate question.
    g_enabled = (enabled && g_pack_dir[0]) ? 1 : 0;
}

int hdtex_enabled(void) { return g_enabled; }

const char *hdtex_pack_dir(void) { return g_pack_dir; }

}  // namespace ntr
