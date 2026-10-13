#pragma once
#include <cstddef>

namespace coop_frontend {
struct Song { int sequence; const char* name; };
// Full themes from the EU DS SDAT. Boss tracks, stingers and temporary
// power-up cues are deliberately absent from both random and manual selection.
static const Song songs[] = {
    {58, "BOB-OMB BATTLEFIELD"}, {73, "STAFF ROLL"},
    {57, "INSIDE THE CASTLE"}, {59, "DIRE DIRE DOCKS"},
    {60, "COOL COOL MOUNTAIN"}, {61, "BIG BOO'S HAUNT"},
    {62, "UNDERGROUND"}, {63, "LETHAL LAVA LAND"},
    {64, "BOWSER'S ROAD"}, {65, "SLIDER"},
    {55, "TITLE THEME"}, {56, "FILE SELECT"},
    {68, "SUNSHINE ISLE"}, {69, "GO GO MARIO"},
    {71, "OPENING"}, {72, "ENDING"},
    {5, "REC ROOM"}, {6, "WHICH WIGGLER"},
    {7, "WANTED"}, {8, "LUIGI'S CASINO"}, {9, "LUIGI POKER"},
    {10, "MINIGAME MERRY GO ROUND"}, {11, "BOB-OMB SQUAD"},
    {12, "SNOWBALL SLALOM"}, {13, "CASINO"}, {14, "COIN COLLECTOR"},
    {16, "MINIGAME GO GO MARIO"}, {17, "MAZE GAME"},
    {18, "PIRANHA MINIGAME"}, {19, "LOVES ME LOVES ME NOT"},
    {20, "PSYCHIC SAFARI"}, {76, "VS CASTLE"}, {77, "VS SLIDER"}
};
enum { SONG_COUNT = sizeof songs / sizeof songs[0], MUSIC_RANDOM = -1, MUSIC_OFF = -2,
       MUSIC_RANDOM_CUSTOM = -3, MUSIC_RANDOM_ALL = -4, MAX_CUSTOM_MEDIA = 256 };
inline const char* music_name(int choice) {
    return choice == MUSIC_RANDOM_ALL ? "SHUFFLE ALL" : choice == MUSIC_RANDOM_CUSTOM ? "SHUFFLE CUSTOM" :
        choice == MUSIC_OFF ? "OFF" : choice < 0 ? "RANDOM DS" :
        choice < SONG_COUNT ? songs[choice].name : "RANDOM";
}
inline bool music_valid(int choice) { return choice >= MUSIC_RANDOM_ALL && choice < SONG_COUNT + MAX_CUSTOM_MEDIA; }
inline int music_step(int choice, int delta, int custom_count = 0) {
    if (!music_valid(choice)) choice = MUSIC_RANDOM;
    if (custom_count < 0) custom_count = 0;
    if (custom_count > MAX_CUSTOM_MEDIA) custom_count = MAX_CUSTOM_MEDIA;
    int low = custom_count ? MUSIC_RANDOM_ALL : MUSIC_OFF;
    int count = SONG_COUNT + custom_count - low;
    if (choice < low || choice >= SONG_COUNT + custom_count) choice = MUSIC_RANDOM;
    return (choice - low + delta + count) % count + low;
}
// Choose from a small PRNG and avoid the previous track.
inline int random_item(unsigned& state, int total, int previous = -1) {
    if (total <= 1) return 0;
    if (!state) state = 0x6d2b79f5u;
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    const int count = previous >= 0 && previous < total ? total - 1 : total;
    int next = static_cast<int>(state % static_cast<unsigned>(count));
    if (previous >= 0 && previous < total && next >= previous) ++next;
    return next;
}
inline int random_song(unsigned& state, int previous = -1) { return random_item(state,SONG_COUNT,previous); }
}
