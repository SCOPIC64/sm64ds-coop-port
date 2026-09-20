#include "lobby.h"

#include <cstdio>
#include <cstring>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace {
int received;

void on_state(const char *, unsigned sequence, int area, int character,
              int x, int y, int z, int yaw)
{
    if (sequence == 77 && area == 3 && character == 2 &&
        x == 0x12345 && y == -0x23456 && z == 0x34567 && yaw == -1234)
        received = 1;
}
}

int main(int argc, char **argv)
{
    if (argc < 2 || (strcmp(argv[1], "host") && strcmp(argv[1], "join"))) {
        std::fprintf(stderr, "usage: lobby_state_probe host|join [address]\n");
        return 2;
    }
    sm64ds::lobby::NetEvents events = {};
    events.state = on_state;
    sm64ds::lobby::set_events(&events);
    const bool host = !strcmp(argv[1], "host");
    sm64ds::lobby::set_name(host ? "ProbeHost" : "ProbeJoin");
    if (host)
        sm64ds::lobby::host_start();
    else
        sm64ds::lobby::join(argc > 2 ? argv[2] : "127.0.0.1");

    int sent = 0;
    for (int tick = 0; tick < 500 && !received; ++tick) {
        sm64ds::lobby::poll();
        if (!sent && sm64ds::lobby::peer_count() > 0) {
            sm64ds::lobby::send_state(77, 3, 2, 0x12345, -0x23456,
                                      0x34567, -1234);
            sent = 1;
        }
        Sleep(10);
    }
    sm64ds::lobby::leave();
    if (!received) {
        std::fprintf(stderr, "gameplay snapshot was not received\n");
        return 1;
    }
    std::printf("gameplay snapshot relay: PASS\n");
    return 0;
}
