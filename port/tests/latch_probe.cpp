/* Proof rig for the crash-report latch. Not part of the gate build; compiled
   by hand (see the report) to answer one question:

   Does an exception that is CAUGHT AND HANDLED -- exactly what the per-actor
   quarantine in port/unmatched/func_02043fdc_hostcopy.cpp does on every frozen actor --
   burn port_crash_write_file's one-shot `once` latch, so that a LATER genuinely
   fatal crash writes no crash.txt and no rolling dump?

   Sequence:
     1. install the probe the shipped walk_window installs
     2. raise an access violation INSIDE a __try/__except that swallows it,
        the way port_dispatch_guarded swallows a quarantined actor's fault
     3. delete any artifact step 2 produced
     4. raise a SECOND access violation and let the UEF take it -- the real
        crash the player would be reporting
     5. report whether crash.txt came back

   PASS (bug absent) = crash.txt exists after step 4.
   FAIL (bug present) = crash.txt absent: the handled fault consumed the latch.

   NOT WIRED INTO CMake ON PURPOSE: the last thing it does is crash, so a gate
   that runs every smoke and expects exit 0 would go red on a rig that is
   working correctly. Build and run it by hand:

     cl /nologo /EHa /Zi /Od /Fe:latch_probe.exe port/tests/latch_probe.cpp \
        /link /SUBSYSTEM:CONSOLE

   Measured on daeeb9b29, the 0.2.4 game tip:
     before the fix  step 2 "written", step 5 ABSENT, and the rolling dump in
                     %TEMP%\sm64ds-crashes named the HANDLED fault (+0000af0e).
                     One dump for the run, describing a fault nobody died of.
     after the fix   step 2 "absent", step 5 PRESENT, and the rolling dump names
                     the FATAL fault (+0000aede). */

#include <stdio.h>
#include <stdlib.h>
#include "fault_probe.h"

static int touch_null(void)
{
    volatile int *p = (volatile int *)0x14;
    return *p;                    /* c0000005, like a stomped actor receiver */
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

    /* step 2: the quarantined fault. The VEH sees it first-chance; the
       __except swallows it and the "frame" continues, exactly as a frozen
       actor's fault does in the shipped game. */
    __try {
        touch_null();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        printf("[rig] handled fault swallowed (this is a quarantine)\n");
    }
    printf("[rig] crash.txt after the HANDLED fault: %s\n",
           artifact_present() ? "written" : "absent");

    /* step 3: clear it, so step 5 can only see a NEW write */
    artifact_clear();
    printf("[rig] cleared. now raising the fatal crash the player reports.\n");
    fflush(stdout);

    /* step 4: the real crash. Nothing catches this one. */
    touch_null();

    printf("[rig] unreachable\n");
    return 0;
}
