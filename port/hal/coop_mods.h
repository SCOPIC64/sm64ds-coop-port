#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace sm64ds::mods {
enum class Event { before_update, after_update, walk, state };
struct PlayerState {
    int character = 0, action = 0, jump_stage = 0;
    int facing_yaw = 0, desired_yaw = 0, previous_yaw = 0;
    float horizontal_speed = 0, vertical_speed = 0, gravity = 0;
    float terminal_velocity = 0, stick = 0, floor_normal = 1, sink_depth = 0;
    float launch_speed = 0;
    bool airborne = false, mega = false, wings = false, no_control = false;
    bool multiplayer = false, cutscene = false, jump_flag = false;
};
struct Info {
    std::string id, name, description, error;
    bool enabled = false, failed = false;
};
// Scripts receive copied, bounded player fields; no native pointers escape.
bool load(const std::string& root, bool migrate_sm64_movement, std::string& error);
void clear();
std::size_t count();
Info info(std::size_t index);
bool set_enabled(std::size_t index, bool enabled, std::string& error);
bool enabled(const char* id);
bool dispatch(Event event, PlayerState& player);
const char* event_name(Event event);
}
