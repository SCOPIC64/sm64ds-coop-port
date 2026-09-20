// HOST COPY of three sound-group loaders (src/_ZN5Sound19LoadGroupAndSetBankEii.c,
// src/func_02011ee4.c, src/func_02011f7c.c) plus the unlinked
// src/_ZN5Sound21ResetPlayerVoiceGroupEv.c: the group switch without the DS
// heap-state RESTORE.
//
// The ROM wraps every group switch in a solid-heap snapshot
// (func_0205117c) and restores it on the way out (func_020510a4), walking
// nested-heap metadata. On the host the sound heap is malloc-backed file
// images behind the fs seam, not DS heap metadata, so the restore's walk
// derails -- Previous on a null iterator -- and faults. Nothing reaches it
// while Mario is the only voice that ever loads, which is why this slept
// until character select: a non-Mario boot reaches func_ov002_020e6330 ->
// func_02011f7c (per-character voice group) and faults at frame ~28.
//
// The host versions load the group files (func_020134d8, the fs seam, which
// works) and update the same bookkeeping words, but never touch the snapshot
// stack at all -- neither SAVE nor RESTORE. The stack's words
// (data_0209b4a8/484/488) stay at their zero boot state: on the host the
// sound-heap handle itself (data_0209b498) is unset, so even the save walk
// dereferences null. Cost: old group bytes stay allocated, a small leak per
// group switch -- which only happens on a character switch, not per frame.
#include <cstdio>

extern "C" {

int func_0203d974(void);
int func_020134d8(int a, int b);
void func_0203d7d4(void);

extern int data_0209b498[];            /* sound heap handle */
extern unsigned char data_0208e428[];  /* split BSS: byte stores below */
extern unsigned char data_0209b47c[];
extern unsigned char data_0209b478[];

void _ZN5Sound19LoadGroupAndSetBankEii(int a, int b)
{
    if (func_0203d974() != 0) {
        if (a != 0x2f) return;
        func_020134d8(a, data_0209b498[0]);
        func_0203d7d4();
        data_0208e428[0] = (unsigned char)b;
        data_0209b47c[0] = (unsigned char)a;
        return;
    }
    if (a == 0) {
        func_020134d8(a, data_0209b498[0]);
    } else if (data_0209b47c[0] != (unsigned char)a) {
        /* no func_020510a4 restore, no func_0205117c snapshot (see above) */
        func_020134d8(a, data_0209b498[0]);
    }
    data_0208e428[0] = (unsigned char)b;
    data_0209b47c[0] = (unsigned char)a;
    data_0209b478[0] = 0;
}

void _ZN5Sound21ResetPlayerVoiceGroupEv(void)
{
    /* same restore skipped for the same reason; the flag is the part the
       sequencer actually reads */
    static int said;
    if (!said++) std::printf("[snd] ResetPlayerVoiceGroup: host no-op\n");
    data_0209b478[0] = 0;
}

/* func_02011ee4(c): the cap/hat group load through func_ov002_020e6350 */
int func_02011ee4(void *c)
{
    int x = func_0203d974();
    if (x != 0) return x;
    /* no func_020510a4 restore: same fault (see above) */
    return func_020134d8((int)(size_t)c, data_0209b498[0]);
}

/* func_02011f7c(self): the per-character voice group load through
   func_ov002_020e6330 -- THE character-select call */
void func_02011f7c(void *self)
{
    if (!func_0203d974()) {
        /* no func_020510a4 restore, no func_0205117c snapshot (see above) */
        func_020134d8((int)(size_t)self, data_0209b498[0]);
    }
    data_0209b478[0] = (unsigned char)(int)(size_t)self;
}

}  // extern "C"
