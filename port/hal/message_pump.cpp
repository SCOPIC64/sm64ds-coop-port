// The message-box PUMP, host side.
//
// WHAT WAS MISSING. The dialogue box's whole state machine is matched src and
// already links: Message::UpdateWindow (src/_ZN7Message12UpdateWindowEv.cpp,
// the box open/close animation and its WIN0 register writes) and Message::Update
// (src/_ZN7Message6UpdateEv.cpp, the per-frame box logic, cursor OAM and the
// AddChar glyph stream). Nothing TICKED them, so the box never advanced.
//
// On the ROM the tick is Stage::Behavior's, in exactly one place. When
// data_0209f2d8 != 1 (i.e. not the VS split-screen mode) Stage::Behavior calls
// Stage::UpdateMessage (src/_ZN5Stage13UpdateMessageEv.cpp), whose body is:
//
//     void Stage::UpdateMessage() {
//         if (data_0209d660 == 0) return;              // no message active
//         if (Message::UpdateWindow()) {               // box fully open?
//             if (data_0209d654 == 0) { Message::Update(); return; }  // normal
//             ... save-screen countdown ...            // data_0209d654 != 0
//         }
//         ... save-screen close ...
//     }
//
// The Stage runs on the DS's behaviour PROCESSING LIST at priority 3, and
// slot 7 (Scene::BeforeBehavior) clears its pause bits so slot 6
// (Stage::Behavior) runs every frame. The port does not run the Stage as a
// list actor yet -- hal/stage_bridges.cpp drives its Render and texture-anim
// halves by hand and traps every vtable slot -- so Stage::Behavior, and with it
// UpdateMessage, never ran.
//
// WHY A HOST COPY AND NOT THE MATCHED FILE. src/_ZN5Stage13UpdateMessageEv.cpp
// is a STUB: it defines its own empty Message::UpdateWindow / Message::Update /
// Message::DisplaySaving / SaveData::SaveCurrentFile locally and calls those,
// so linking it would (a) collide with the real Message symbols already in the
// slice and (b) tick nothing. And Stage::Behavior itself pulls in the entire
// pause/VS/level-change machinery (PS_Update, VE_Init, Scene::SetSceneToSpawn,
// data_0209f5bc->v5 ...), none of which the port hosts. So this file is the ONE
// statement of Stage::Behavior that owns the message box -- UpdateMessage's own
// body, calling the REAL matched Message methods -- and nothing else. It is the
// same treatment the +0x13 pause-bit clear and the LC save-prompt clear already
// get in stage_bridges.cpp / level_boot.cpp: a stand-in for one line of the
// Stage's own Behavior, to be retired the day the Stage runs as a real actor.
//
// THE SAVE-SCREEN ARM (data_0209d654 != 0) is deliberately NOT reproduced: it
// drives SaveData::SaveCurrentFile and Message::DisplaySaving, the save prompt,
// which is not a dialogue box and is not reachable from an in-world talk. Only
// the data_0209d654 == 0 path -- Message::Update -- is the dialogue pump. If a
// message ever comes up with data_0209d654 set, this logs once and skips it
// rather than pretend to host the save flow.
#include <cstdio>
#include <cstdlib>

/* Message::UpdateWindow is a real C++ static method (int Message::UpdateWindow()
   in src/_ZN7Message12UpdateWindowEv.cpp), so it is reached through the class,
   not an itanium C name -- MSVC mangles the static to ?UpdateWindow@Message@@SAHXZ
   and only the qualified call spells it. Message::Update is a non-static method
   faced in hal/reverse_bridges.cpp as the C entry _ZN7Message6UpdateEv(void*);
   it reads only globals, so the self pointer is unused. */
struct Message {
    static int UpdateWindow();
};

extern "C" {

void _ZN7Message6UpdateEv(void *self);     /* Message::Update, faced in reverse_bridges */

extern unsigned char data_0209d660;   /* message-active flag */
extern unsigned char data_0209d654;   /* save-screen flag (0 for a dialogue) */
extern unsigned char data_0209d698;   /* box target screen: 0 engine A, 1/2 sub */
extern unsigned char data_0209d6bc;   /* Message::Update state var */
extern short         data_0209d6d4;   /* current message id (-1 = none) */

/* THE ENGINE-A DISPLAY SYNC, the piece the port skips. On the DS func_02019144
   runs once per frame and, among other things, copies the software BG-offset
   shadows and the BG-enable layer mask into the engine-A hardware registers.
   SetBg3Offset (src/SetBg3Offset.c) writes data_0209d48c/d490 -- shadows, NOT
   hardware -- and func_0201f32c scrolls BG3 through it so the box content (which
   sits at the TOP of the BG3 tilemap) lines up under the WIN0 window lower down
   the screen. Without the flush the hardware BG3 offset stays 0, the content
   never scrolls under the window, and the compositor reads only the empty fill
   tile inside the window (the 2026-08-08 "box active but 0 px composited"
   finding).

   hal/sub_screen.cpp already reproduces func_02019144's OTHER beats -- the sub
   DISPCNT mask publish, the minimap affine callback, OAM Load. This is the
   engine-A remainder those did not cover: the engine-A DISPCNT layer-mask
   publish (data_0209d45c) and the four engine-A BG offset registers, byte for
   byte as func_02019144 lines 46,49,53,54,55 write them. Reached only while a
   message is up, so it touches nothing when no box is open. */
extern unsigned char data_0209d45c;   /* engine-A BG-enable layer mask */
extern short data_0209d48c, data_0209d490;   /* BG3 hofs/vofs shadows */

/* THE MESSAGE-BOX INPUT PUBLISH, the piece the port skips. On the DS the pad
   record data_020a0e58 (PadData[4], stride 4: {u16 held @ +0, u16 pressed @ +2})
   is filled by the pad read every frame, and data_020a0e5a is the SAME memory --
   pad 0's pressed halfword -- read by IsButtonInputValid (src/IsButtonInputValid.c)
   and Message::Update (src/_ZN7Message6UpdateEv.cpp) to advance and DISMISS the
   box. The port splits those two into separate auto_bss symbols, and its input
   layer publishes the face buttons into the game's remapped Ctrl block
   (data_0209f49c held / data_0209f49e pressed) directly rather than through the
   pad record, so nothing ever reaches data_020a0e5a. IsButtonInputValid then
   reads 0 every frame, Message::Update never leaves its wait state, the box
   cannot be dismissed and the player is softlocked (the castle-grounds door
   bug -- the box opens and shows text, but there is no way out).

   This restores the DS's pad aliasing for the one consumer that needs it: while a
   message is active it republishes the local player's pressed face buttons into
   the pad record (data_020a0e58's pressed halfword) and its overlay symbol
   (data_020a0e5a), taking the press edge from the Ctrl block the input layer has
   already filled this frame. The Ctrl-block pressed word carries A in bit 0 and B
   in bit 1 (StartTalk's own gate reads data_0209f49e & 3), which is the same low
   two bits IsButtonInputValid's mode-0 test (data_020a0e5a & 0xf) reads for A/B,
   so the two low bits carry straight across with no remap. Reached only while a
   message is up, so it touches the pad record for nothing else. */
extern unsigned char data_020a0e40;          /* the local player index */
extern unsigned short data_020a0e58[];       /* PadData[4]: [i*2] held, [i*2+1] pressed */
extern unsigned short data_020a0e5a[];        /* pad0.pressed overlay the box reads */
extern unsigned short data_0209f49e[];        /* Ctrl block: pressed-this-frame */

static void port_message_publish_input(void)
{
    int idx = (int)data_020a0e40;
    if (idx < 0 || idx > 3)
        idx = 0;
    /* A (bit0) and B (bit1) pressed this frame, from the remapped Ctrl block
       (data_0209f49e is already a one-frame edge: the input layer writes it as
       btn & ~btn_was), folded onto whatever the pad record's own pressed edge
       carries (the D-pad, written fresh each frame by the input layer). */
    unsigned short pressed = (unsigned short)(data_020a0e58[idx * 2 + 1] |
                                              (data_0209f49e[0] & 0x3));
    data_020a0e58[idx * 2 + 1] = pressed;
    /* data_020a0e5a is the DS overlay of the pressed halfword -- the same memory
       on hardware, split into its own symbol on the port and never otherwise
       written. ASSIGN (not OR) so it tracks the pad record's per-frame edge
       exactly, instead of latching a stale press that would dismiss every future
       box on its first frame. */
    data_020a0e5a[idx * 2] = pressed;
}

static void port_message_flush_engine_a_regs(void)
{
    typedef volatile unsigned int vu32;
    /* engine-A DISPCNT layer-mask publish (func_02019144 line 46): puts BG3 on
       (data_0209d45c bit 3, which func_0201f32c set), so the compositor's
       BG-enable read sees the box layer. */
    *(vu32 *)0x4000000 = (*(vu32 *)0x4000000 & ~0x1f00u) | (data_0209d45c << 8);
    /* BG3 offset (func_02019144 line 55): the box's own scroll, so the content
       at the top of the BG3 tilemap lines up under the WIN0 window. This is the
       one offset the box actually uses; BG0/1/2's offset shadows are not seated
       on the port and the box does not scroll them, so only BG3 is flushed. */
    *(vu32 *)0x400001c = (data_0209d48c & 0x1ff) | (0x1ff0000u & (data_0209d490 << 16));
}

/* Reproduces Stage::UpdateMessage's dialogue arm, called once per frame. This
   is the exact tick Stage::Behavior gives the box when data_0209f2d8 != 1. */
void port_message_pump(void)
{
    /* diagnostic: report the box state each frame it is active for the first
       ~30 active frames, so a headless run can watch UpdateWindow animate the
       box open (d658/d64c grow to d6d0/d6c8) and Message::Update advance d6bc. */
    if (std::getenv("SM64DS_MSG_PUMP_DEBUG") && data_0209d660) {
        extern unsigned char data_0209d658, data_0209d64c, data_0209d6d0,
            data_0209d6c8, data_0209d650, data_0209d6cc, data_0209d670;
        static int n;
        if (n < 30) {
            ++n;
            std::fprintf(stderr, "[msgpump] active: d698=%u d6bc=%u id=%d "
                         "cur(w=%u h=%u) max(w=%u h=%u) min(w=%u h=%u) close=%u\n",
                         (unsigned)data_0209d698, (unsigned)data_0209d6bc,
                         (int)data_0209d6d4, (unsigned)data_0209d658,
                         (unsigned)data_0209d64c, (unsigned)data_0209d6d0,
                         (unsigned)data_0209d6c8, (unsigned)data_0209d650,
                         (unsigned)data_0209d6cc, (unsigned)data_0209d670);
        }
    }

    if (data_0209d660 == 0)
        return;                        /* no message active: same first line */

    /* restore the DS pad aliasing the box's input readers depend on, so a real
       A/B press can advance and dismiss the box (without it the box softlocks --
       the castle-grounds door bug). Reached only while a message is active. */
    port_message_publish_input();

    if (Message::UpdateWindow()) {     /* box fully open */
        if (data_0209d654 == 0) {
            _ZN7Message6UpdateEv(0);   /* the dialogue tick */
            port_message_flush_engine_a_regs();
            return;
        }
        /* save-screen arm: not a dialogue box, not hosted */
        static int said_save;
        if (!said_save) {
            said_save = 1;
            std::fprintf(stderr, "[msg] pump: save-screen arm reached "
                         "(data_0209d654=%u); not hosted, skipping\n",
                         (unsigned)data_0209d654);
        }
    }
}

} /* extern "C" */
