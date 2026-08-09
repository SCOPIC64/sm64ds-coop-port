/* SOUND_OBJECT's (daSoundObj_c, actor 359) seven-cell PMF dispatch seat and
 * its Behavior host copy, the Cap_StateDispatch treatment applied to the
 * positional sound emitter -- gate 190's fix round.
 *
 * WHY A HOST COPY. src/_ZN11SoundObject8BehaviorEv.cpp dispatches
 * `(c->*data_ov002_0211110c[sel])(cc + 0xdc)` through a REAL C++
 * pointer-to-member on an INCOMPLETE class -- mwcc's PMF there is the 8-byte
 * {function, delta} pair, which is both the table's ROM stride and the
 * sinit's copy unit. MSVC's PMF for an incomplete class is the
 * unknown-inheritance representation (a different size entirely), so the
 * matched TU cannot even index the table correctly on the host, let alone
 * dispatch it. The matched src stays in src/ as the byte proof; this file is
 * the port body.
 *
 * THE TABLE. data_ov002_0211110c is 7 bss cells x 8 bytes (0x0211110c..
 * 0x02111144, the next-symbol delta -- sel 0..6; InitResources accepts param
 * <= 7 but the ROM table has no entry 7, a latent off-table dispatch on the
 * DS too, so sel 7 traps loudly here instead of silently reading the
 * neighbouring cell). __sinit_ov002_02107f88 (linked at gate 10, runs with
 * ov002's boot sinits) copies the seven {function, 0} source records at
 * data_ov002_0210c010..0210c040 into the cells -- DS CODE ADDRESSES, the
 * ovdata contract. port_sound_object_states_seat rewrites each cell's fn with
 * its host body after checking it against the ROM address that body was
 * compiled from (the Cap seat shape: a mount pointing at the wrong bytes
 * aborts instead of calling into the overlay image). All seven are the
 * nonvirtual {function, 0} form; the raw ROM bytes at 0x0210c010..0x0210c048
 * confirm every delta word is 0.
 *
 * THE FIVE DISTINCT TARGETS, all matched src, all the same
 * `int fn(self, u16 *counter)` C shape:
 *   [0] 0x0200f8f8 Sound::PlaySecretSound       (arm9, slice_gate32)
 *   [1] 0x0200f874 Sound::PlaySmallSecretSound  (arm9, slice_gate41)
 *   [2] 0x0200f7f0 func_0200f7f0                (arm9, joins slice_gate190)
 *   [3] 0x020f9468 func_ov002_020f9468          (ov002, joins slice_gate190)
 *   [4] 0x020f93a8 func_ov002_020f93a8          (ov002, joins slice_gate190)
 *   [5] 0x020f9468 (shares [3])
 *   [6] 0x020f9468 (shares [3])   <- the Actor::SpawnSoundObj(6) path
 */
#include <cstdio>
#include <cstdlib>

extern "C" {
int _ZN5Sound7PlaySubEjjj5Fix12IiEb(unsigned soundID, unsigned vol,
                                    unsigned pan, int dist, int loop);
void _ZN9ActorBase18MarkForDestructionEv(void *self);
int _ZN5Sound15PlaySecretSoundEP5ActorPt(void *self, unsigned short *counter);
int _ZN5Sound20PlaySmallSecretSoundEP5ActorPt(void *self,
                                              unsigned short *counter);
int func_0200f7f0(void *self, unsigned short *counter);
int func_ov002_020f9468(char *self, unsigned short *counter);
int func_ov002_020f93a8(char *self, unsigned short *counter);

extern unsigned char data_0208e430[4];   /* current music id, romdata */
extern int data_0209b49c[];              /* auto_bss */
extern int data_0209b490[];              /* auto_bss */

struct PortSoundObjPair { unsigned fn; int delta; };
extern PortSoundObjPair data_ov002_0211110c[];   /* the 7 dispatch cells */
}

enum { PORT_SOUND_OBJ_CELLS = 7 };

typedef int (*PortSoundObjFn)(char *, unsigned short *);

static const struct { unsigned rom; PortSoundObjFn host; }
g_sound_obj_states[PORT_SOUND_OBJ_CELLS] = {
    {0x0200f8f8, (PortSoundObjFn)_ZN5Sound15PlaySecretSoundEP5ActorPt},
    {0x0200f874, (PortSoundObjFn)_ZN5Sound20PlaySmallSecretSoundEP5ActorPt},
    {0x0200f7f0, (PortSoundObjFn)func_0200f7f0},
    {0x020f9468, func_ov002_020f9468},
    {0x020f93a8, func_ov002_020f93a8},
    {0x020f9468, func_ov002_020f9468},
    {0x020f9468, func_ov002_020f9468},
};

extern "C" void port_sound_object_states_seat(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    PortSoundObjPair *p = data_ov002_0211110c;
    for (int i = 0; i < PORT_SOUND_OBJ_CELLS; ++i) {
        if (p[i].fn != g_sound_obj_states[i].rom || p[i].delta != 0) {
            std::fprintf(stderr, "FATAL: SoundObject cell %d: the sinit left "
                         "%08x/%d, the ROM's own records say %08x/0 -- WRONG "
                         "BYTES\n", i, p[i].fn, p[i].delta,
                         g_sound_obj_states[i].rom);
            std::abort();
        }
        p[i].fn = (unsigned)(size_t)g_sound_obj_states[i].host;
    }
}

/* The Behavior, translated field-for-field from the matched src (the PMF
 * dispatch replaced with the seated plain-function call; deltas are 0, so
 * `this` needs no adjustment). Object fields per include/daSoundObj_c.h:
 * +0xd4 s32 sound id, +0xd8 s32 volume floor, +0xdc u16 counter (the arg the
 * dispatch passes by address), +0xde u16 target. +8 is the actor's param word
 * (the sel). */
extern "C" int _ZN11SoundObject8BehaviorEv(char *cc)
{
    int sel = *(int *)(cc + 8);
    if (sel < 0 || sel >= PORT_SOUND_OBJ_CELLS) {
        /* sel 7 passes InitResources' `> 7` gate but has no table entry --
           off-table on the DS as well (it would dispatch the neighbouring
           class's cell bytes). Loudly decline and self-destruct rather than
           mimic a garbage dispatch. */
        std::fprintf(stderr, "SOUND_OBJECT: param %d has no dispatch cell "
                     "(table is 0..6) -- destroying the emitter\n", sel);
        _ZN9ActorBase18MarkForDestructionEv(cc);
        return 1;
    }
    int r = ((PortSoundObjFn)(size_t)data_ov002_0211110c[sel].fn)(
        cc, (unsigned short *)(cc + 0xdc));
    int music = *(int *)data_0208e430;
    if (!(r == 0 && *(int *)(cc + 0xd4) == music
          && (*(unsigned short *)(cc + 0xdc) <= 0xau
              || data_0209b49c[0] > 0x7f))) {
        _ZN9ActorBase18MarkForDestructionEv(cc);
        if (music != 0x22)
            _ZN5Sound7PlaySubEjjj5Fix12IiEb((unsigned)music, 0x7f, 0,
                                            0x7f000, 0);
    }
    if (sel != 6) {
        if (data_0209b490[0] < *(int *)(cc + 0xd8))
            *(unsigned short *)(cc + 0xdc) = *(unsigned short *)(cc + 0xde);
    }
    return 1;
}
