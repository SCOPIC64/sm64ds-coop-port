/* HOST COPIES of src/func_ov002_020bbd5c.cpp and src/func_ov002_020bbda4.cpp
 * -- the sign's own five-state machine, with the mwcc pointer-to-member pairs
 * read as plain function pointers and seated with host addresses.
 *
 * data_ov002_0210e084 is five {Init, Main} PMF pairs, written by
 * __sinit_ov002_02101738 out of ten statics at ov002 0x02109a64..0x02109ab4.
 * All ten are the nonvirtual { function, 0 } form:
 *
 *     0  planted   init 020bba24 (empty)   main 020bb9fc  talk/grab checks
 *     1  read      init 020bb9f0           main 020bb614  UNMATCHED
 *     2  carried   init 020bbd50           main 020bbcb8
 *     3  thrown    init 020bbc78           main 020bbb14
 *     4  dropped   init 020bbac8           main 020bba28
 *
 * Two host-only steps, the same pair OneUpMushroom_Behavior.cpp needs.
 *
 * MSVC forms a pointer-to-member of an INCOMPLETE class as the most general
 * representation (four words, not one), which would stride the table at 0x20
 * instead of 0x10 and dispatch a neighbour's Init as a Main. Both source TUs
 * declare `struct C;` before the typedef, so both hit it.
 *
 * And the words the sinit copied are the ov002 image's own -- DS CODE
 * ADDRESSES, which is the ovdata contract. The seat below rewrites each with
 * its host body after checking the stored word against the ROM address that
 * body was compiled from, so a mount pointing at the wrong bytes says so
 * instead of calling into the overlay image.
 *
 * STATE 1 IS SEATED WITH A NAMED ABORT. Its Main is ov002 0x020bb614, a
 * 0x3dc-byte hole in the delink table with no C at all -- the sign's read
 * loop, which drives the Message box. The only way into it is
 * func_ov002_020bb520 -> Player::StartTalk returning 1, and the port declines
 * that talk (hal/actor_vtables.cpp). If a future gate hosts Message, the guard
 * comes off and this state needs its Main matched first; until then the abort
 * is the loud version of "that path is closed".
 */
#include <cstdio>
#include <cstdlib>

extern "C" {

void func_ov002_020bba24(void *);   /* 0 Init  */
void func_ov002_020bb9fc(void *);   /* 0 Main  */
void func_ov002_020bb9f0(void *);   /* 1 Init  */
void func_ov002_020bbd50(void *);   /* 2 Init  */
void func_ov002_020bbcb8(void *);   /* 2 Main  */
void func_ov002_020bbc78(void *);   /* 3 Init  */
void func_ov002_020bbb14(void *);   /* 3 Main  */
void func_ov002_020bbac8(void *);   /* 4 Init  */
void func_ov002_020bba28(void *);   /* 4 Main  */

struct PortStatePair { unsigned fn; int delta; };
struct PortSignPostState { PortStatePair init, main_; };
extern PortSignPostState data_ov002_0210e084[];

}  /* extern "C" */

enum { PORT_SIGNPOST_STATES = 5 };

static void port_signpost_read_main(void *)
{
    std::fprintf(stderr, "FATAL: SignPost state 1 (read) is not hosted -- "
                 "ov002 0x020bb614 is unmatched and its body is the Message "
                 "box. Player::StartTalk is supposed to decline.\n");
    std::abort();
}

static const struct { unsigned rom; void (*host)(void *); } g_states[10] = {
    {0x020bba24, func_ov002_020bba24}, {0x020bb9fc, func_ov002_020bb9fc},
    {0x020bb9f0, func_ov002_020bb9f0}, {0x020bb614, port_signpost_read_main},
    {0x020bbd50, func_ov002_020bbd50}, {0x020bbcb8, func_ov002_020bbcb8},
    {0x020bbc78, func_ov002_020bbc78}, {0x020bbb14, func_ov002_020bbb14},
    {0x020bbac8, func_ov002_020bbac8}, {0x020bba28, func_ov002_020bba28},
};

extern "C" void port_sign_post_states_seat(void)
{
    PortStatePair *p = (PortStatePair *)data_ov002_0210e084;
    for (int i = 0; i < 10; ++i) {
        if (p[i].fn != g_states[i].rom) {
            std::fprintf(stderr, "FATAL: SignPost state %d %s: the sinit left "
                         "%08x, the ROM's own table says %08x -- WRONG "
                         "BYTES\n", i / 2, (i & 1) ? "Main" : "Init", p[i].fn,
                         g_states[i].rom);
            std::abort();
        }
        p[i].fn = (unsigned)(size_t)g_states[i].host;
        p[i].delta = 0;
    }
}

/* func_ov002_020bbd5c: enter state `i` -- store it, then run its Init. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" void func_ov002_020bbd5c(void *selfv, int i)
{
    char *c = (char *)selfv;
    *(int *)(c + 0x354) = i;
    if ((unsigned)i >= PORT_SIGNPOST_STATES) {
        std::fprintf(stderr, "FATAL: SignPost state %d out of range\n", i);
        std::abort();
    }
    ((void (*)(void *))(size_t)data_ov002_0210e084[i].init.fn)(c);
}

/* func_ov002_020bbda4: run the current state's Main. */
// PORT_HOST_ABI: mwcc pointer-to-member dispatch (MSVC widens PMF over an incomplete class).
extern "C" void func_ov002_020bbda4(void *selfv)
{
    char *c = (char *)selfv;
    int i = *(int *)(c + 0x354);
    if ((unsigned)i >= PORT_SIGNPOST_STATES) {
        std::fprintf(stderr, "FATAL: SignPost state %d out of range\n", i);
        std::abort();
    }
    ((void (*)(void *))(size_t)data_ov002_0210e084[i].main_.fn)(c);
}
