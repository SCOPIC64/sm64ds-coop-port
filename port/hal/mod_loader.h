#pragma once

#include <string>

namespace sm64ds::mods {

bool load_all(const std::string &mods_directory, std::string &error);
bool installed(const std::string &mods_directory, const std::string &name);

// Requested startup config set by Lua mods (sm64ds.set_*). -1 = untouched,
// so walk_window can overlay them onto its persisted settings after load_all.
int requested_character(void);   // 0 Mario, 1 Luigi, 2 Wario, 3 Yoshi, -1 unset
int requested_speed_pct(void);   // e.g. 150, -1 unset
int requested_jump_pct(void);    // e.g. 130, -1 unset
int requested_sub_scale(void);   // 1..4, -1 unset
int requested_widescreen(void);  // 0/1, -1 unset
int requested_arena(void);       // 0/1, -1 unset
int requested_camera(void);      // 0 analog, 1 freecam, 2 DS-exact, -1 unset
int requested_color(void);       // 0..7 outfit tint, -1 unset

// Native-backed mod registry. A mod is INSTALLED when its folder exists
// under mods/ and ENABLED when it is not off_-prefixed; toggling renames
// the folder, so mods are real removable units (delete one and its menu
// rows go dim). Works with or without the Lua runtime. If the mods
// directory does not exist at all, everything reads enabled (old kits).
struct ModInfo {
    const char *id;
    const char *title;
};
int mod_count(void);
ModInfo mod_info(int i);
bool mod_enabled(const char *id);
bool mod_set_enabled(const std::string &mods_directory, const char *id,
                     bool on);
void mod_rescan(const std::string &mods_directory);

}
