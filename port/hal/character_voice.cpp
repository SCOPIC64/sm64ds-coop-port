#include "../waluigi.h"

#include <cstdio>
#include <cstdlib>
#include <windows.h>

struct Vector3 { int x, y, z; };

extern "C" unsigned int _ZN5Sound4PlayEjjRK7Vector3(
    unsigned int kind, unsigned int id, const Vector3 &position);
extern "C" int port_character_voice_logical(unsigned int resource_character);
extern "C" unsigned char data_02075250[];

namespace {
typedef BOOL (WINAPI *PlaySoundFn)(LPCSTR, HMODULE, DWORD);

bool play_imported_voice(unsigned logical, unsigned id) {
    const char *root = std::getenv("SM64DS_CHARACTER_PACK_DIR");
    if (!root || !*root || logical >= PORT_CHARACTER_COUNT) return false;
    static const char *names[PORT_CHARACTER_COUNT] = {
        "mario", "luigi", "wario", "yoshi", "waluigi", "toad"
    };
    char pattern[MAX_PATH];
    std::snprintf(pattern, sizeof pattern, "%s\\%s\\voices\\%02X_*.wav",
                  root, names[logical], id & 0xff);
    WIN32_FIND_DATAA found{};
    HANDLE find = FindFirstFileA(pattern, &found);
    if (find == INVALID_HANDLE_VALUE) return false;
    FindClose(find);
    char path[MAX_PATH];
    std::snprintf(path, sizeof path, "%s\\%s\\voices\\%s",
                  root, names[logical], found.cFileName);
    HMODULE winmm = LoadLibraryA("winmm.dll");
    if (!winmm) return false;
    PlaySoundFn play = reinterpret_cast<PlaySoundFn>(GetProcAddress(winmm, "PlaySoundA"));
    const bool ok = play && play(path, nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
    FreeLibrary(winmm);
    return ok;
}
} // namespace

extern "C" unsigned int _ZN5Sound13PlayCharVoiceEjjRK7Vector3(
    unsigned int resource_character, unsigned int voice_id,
    const Vector3 &position) {
    const int logical = port_character_voice_logical(resource_character);
    if (logical >= 0 && play_imported_voice((unsigned)logical, voice_id)) return 1;
    return _ZN5Sound4PlayEjjRK7Vector3(
        1, voice_id + data_02075250[resource_character], position);
}
