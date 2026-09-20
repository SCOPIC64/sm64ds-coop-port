// HOST COPY of src/func_0201f32c.c: the message-box Show without the text
// engine.
//
// The ROM version draws the typewriter text box into VRAM, pumps it with
// Message::Update until the counter reaches 4, and parks completion in
// data_0209d660 (=0 when finished) and data_0209d6bc/6ac. The text engine
// behind it (Message::LoadTextVS and friends) is not hosted -- see the
// LoadFont3D stub in hal/sub_screen.cpp -- and Show faults on entry reading
// the message data pointer.
//
// Nothing reaches it while Mario walks alone, which is why this slept until
// character select: Yoshi's entrance cutscene (func_ov002_020c4188 case 2)
// shows a text, and case 3 waits on data_0209d660 == 0 before the entrance
// can finish and hand over to Walk.
//
// The host version logs the id and reports instant completion with the same
// net state Show ends in (not-showing, counter done), so every waiter --
// the entrance cutscene, Talk, the owl ride -- proceeds without text. Talk
// boxes auto-advance instead of hanging; that is the documented degradation
// until the text engine is hosted.
#include <cstdio>

extern "C" {

extern int data_0209d660[];
extern int data_0209d6d4[];
extern int data_0209d6bc[];
extern int data_0209d6ac[];

int g_last_msg_id = -1;

void func_0201f32c(int msgID)
{
    static int seen[256];
    const unsigned id = (unsigned)(msgID & 0xff);
    g_last_msg_id = msgID;
    if (!seen[id]) {
        seen[id] = 1;
        std::printf("[msg] Show(%d): text engine unhosted, auto-advanced\n",
                    msgID);
    }
    data_0209d6d4[0] = (data_0209d6d4[0] & ~0xffff) | (msgID & 0xffff);
    /* not showing: the entrance cutscene's case-3 gate reads exactly this */
    ((unsigned char *)data_0209d660)[0] = 0;
    /* typewriter done: any waiter on the update counter falls through */
    ((unsigned char *)data_0209d6bc)[0] = 4;
    ((unsigned char *)data_0209d6ac)[0] = 4;
}

}  // extern "C"
