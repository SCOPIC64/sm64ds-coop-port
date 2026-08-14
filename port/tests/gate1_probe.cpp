/* GATE 1 rig: the shipped crash path, driven the way a player drives it.
   Not part of the gate build; hand-compiled (see the build line below).

   latch_probe.cpp proves the latch mechanism with a bare __try/__except stand-
   in. This one removes the stand-in. It links the REAL quarantine translation
   unit (port/unmatched/func_02043fdc.cpp) and drives the REAL list walker
   func_02043fdc, whose port_dispatch_guarded wraps each actor tick in the
   __try/__except that actually ships, and it raises the fault through the REAL
   entry point port_actor_slot_decline_for -- the same call an unhosted vtable
   slot makes when a player punches, ground-pounds or head-bonks a piece of
   level furniture. Nothing here is synthetic:

     - port_fault_synthetic is never set. It is set ONLY by the
       SM64DS_TEST_QUARANTINE hook, which is PORT_TEST_HOOKS-only and which
       deliberately suppresses every disk artifact AND deliberately leaves the
       latch unclaimed. That hook therefore cannot exercise this bug at all,
       which is a large part of why the bug survived: the one rig built to
       prove quarantine behaviour is blind to exactly this failure.
     - PORT_TEST_HOOKS is left OFF, so the binary is shaped like the shipped
       one.
     - PORT_FAULT_PROBE_DEFINE_EXPORTS is defined here exactly as
       walk_window.cpp defines it, so the quarantine filter reaches the real
       port_rich_dump_ex and the real %TEMP%\sm64ds-crashes sink rather than
       the weak stubs a bare smoke falls back to.

   Sequence, which is one ordinary play session in miniature:
     1. install the probe walk_window installs
     2. walk a one-node actor list whose phase callback declines an unhosted
        slot: the real AV, caught by the real filter, actor frozen, walk
        continues. The player notices nothing.
     3. delete anything step 2 left next to the exe
     4. take a genuinely fatal access violation, the crash being reported
     5. report whether crash.txt came back

   PASS (bug absent) = crash.txt exists after step 4.
   FAIL (bug present) = crash.txt absent: the survivable fault consumed the
   one-shot latch and the real crash cannot be reported.

   Build (32-bit, matching the shipped exe):
     cl /nologo /EHa /O2 /Fe:gate1_probe.exe port/tests/gate1_probe.cpp \
        port/unmatched/func_02043fdc.cpp /I port/tests /link /SUBSYSTEM:CONSOLE
*/

#include <stdio.h>
#include <stdlib.h>
#define PORT_FAULT_PROBE_DEFINE_EXPORTS
#include "fault_probe.h"

extern "C" {
void *func_02043fdc(void *listv);
void port_actor_slot_decline_for(void *actor, const char *what);
int port_quarantine_is_frozen(void *actor);
}
/* The engine globals the walker parks its cursor in live in gate1_globals.c,
   not here. fault_probe.h declares data_020a4b68 as `int *` (it reads the
   published cursor and dereferences it) while the walker declares the same
   symbol `int []` (it stores the node pointer into element 0). The two agree
   in memory and disagree in C++ type, so they cannot both be visible in one
   TU. The shipped build never hits this because walk_window.cpp includes the
   header and func_02043fdc.cpp does not. A separate plain-C TU keeps the rig
   in the same position. */

/* One "actor". The walker only ever hands this pointer back to the callback
   and to the freeze set, so a plain object is enough. */
static int g_actor[8];

/* The phase callback. This is the shipped shape of a trapped slot: it names
   its receiver and declines, which raises the catchable AV that
   port_dispatch_guarded's __except is there to catch. */
static int declining_tick(void *self)
{
    port_actor_slot_decline_for(self, "gate1 unhosted slot");
    return 0;                       /* never reached: the decline raises */
}

static int touch_null(void)
{
    volatile int *p = (volatile int *)0x14;
    return *p;
}

static void artifact_path(char *out)
{
    DWORD n = GetModuleFileNameA(0, out, MAX_PATH);
    while (n && out[n - 1] != '\\') --n;
    lstrcpyA(out + n, "crash.txt");
}

static int artifact_present(void)
{
    char p[MAX_PATH + 16];
    artifact_path(p);
    return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES;
}

static void artifact_clear(void)
{
    char p[MAX_PATH + 16];
    artifact_path(p);
    DeleteFileA(p);
}

int main(void)
{
    PORT_INSTALL_FAULT_PROBE();
    printf("[gate1] sink = %s\n", port_crash_dir_get());

    /* step 2: the survivable fault, through the real walker.
       list = {head, tail, callback, 0}; node = {prev, next, owner, ...} */
    {
        int node[4];
        int list[4];
        node[0] = 0;
        node[1] = 0;                                   /* single node */
        node[2] = (int)(size_t)&g_actor[0];            /* the owner/actor */
        node[3] = 0;
        list[0] = (int)(size_t)&node[0];
        list[1] = 0;
        list[2] = (int)(size_t)&declining_tick;
        list[3] = 0;
        func_02043fdc(&list[0]);
    }
    printf("[gate1] actor frozen by the real quarantine: %s\n",
           port_quarantine_is_frozen(&g_actor[0]) ? "yes" : "NO (rig broken)");
    printf("[gate1] crash.txt after the SURVIVABLE fault: %s\n",
           artifact_present() ? "written" : "absent");

    artifact_clear();
    /* The rolling dump is named crash-<YYYYMMDD-HHMMSS>-<pid>.txt, which is one
       second of resolution and no uniquifier, so two dumps from the same
       process in the same second land on the same path and CREATE_ALWAYS makes
       the second silently overwrite the first. Wait past the second boundary so
       the quarantine dump and the crash dump are separately visible and this
       rig measures the latch, not the filename collision. The collision is a
       real defect in its own right and is reported separately. */
    Sleep(1100);
    printf("[gate1] cleared. now taking the fatal crash the player reports.\n");
    fflush(stdout);

    /* step 4: the real crash. Nothing catches this one. */
    touch_null();

    printf("[gate1] unreachable\n");
    return 0;
}
