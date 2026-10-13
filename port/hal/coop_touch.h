#pragma once
#include <cstdint>
#include <cmath>

// Platform-neutral multi-touch state for the mobile host seam. OS adapters must
// supply stable finger IDs and call cancel() on focus loss, pause and rotation.
// This module does not imply that the DS engine has an Android/iOS host yet.
namespace coop_frontend {
struct TouchState {
    float move_x = 0, move_y = 0, look_x = 0, look_y = 0;
    unsigned buttons = 0;
};
struct TouchControls {
    enum Zone { UNUSED, MOVE, LOOK, JUMP, ATTACK, CROUCH, START };
    struct Finger { bool down = false; int64_t id = 0; Zone zone = UNUSED;
        float x = 0, y = 0, origin_x = 0, origin_y = 0; };
    Finger fingers[10];
    int width = 0, height = 0, left = 0, top = 0;

    void cancel() { for (auto& finger : fingers) finger = Finger{}; }
    void resize(int w, int h, int inset_left = 0, int inset_top = 0,
                int inset_right = 0, int inset_bottom = 0) {
        cancel();
        left = inset_left; top = inset_top;
        width = w - inset_left - inset_right;
        height = h - inset_top - inset_bottom;
        if (width < 1 || height < 1) width = height = 0;
    }
    static Zone hit(float x, float y) {
        if (x < 0 || x > 1 || y < 0 || y > 1) return UNUSED;
        if (x > .86f && y < .19f) return START;
        if (y < .40f) return UNUSED;
        if (x < .43f) return MOVE;
        if (x < .72f) return LOOK;
        if (y < .58f) return CROUCH;
        return x > .86f ? JUMP : ATTACK;
    }
    void down(int64_t id, float px, float py) {
        if (!width || !height || !std::isfinite(px) || !std::isfinite(py)) return;
        for (auto& f : fingers) if (f.down && f.id == id) { move(id, px, py); return; }
        float x = (px - left) / width, y = (py - top) / height;
        Zone zone = hit(x, y);
        if (zone == UNUSED) return;
        // One finger owns each stick. Buttons allow overlapping fingers and
        // remain pressed until the last owner releases.
        if (zone == MOVE || zone == LOOK)
            for (const auto& f : fingers) if (f.down && f.zone == zone) return;
        for (auto& f : fingers) if (!f.down) {
            f.down = true; f.id = id; f.zone = zone;
            f.x = f.origin_x = px; f.y = f.origin_y = py;
            return;
        }
    }
    void move(int64_t id, float px, float py) {
        if (!std::isfinite(px) || !std::isfinite(py)) return;
        for (auto& f : fingers) if (f.down && f.id == id) { f.x = px; f.y = py; }
    }
    void up(int64_t id) {
        for (auto& f : fingers) if (f.down && f.id == id) f = Finger{};
    }
    TouchState state() const {
        TouchState result;
        if (!width || !height) return result;
        float radius = (width < height ? width : height) * .18f;
        for (const auto& f : fingers) if (f.down) {
            if (f.zone == MOVE || f.zone == LOOK) {
                float x = (f.x - f.origin_x) / radius;
                float y = (f.origin_y - f.y) / radius;
                float distance = std::sqrt(x * x + y * y);
                if (distance < .12f) x = y = 0;
                else {
                    float scale = distance > 1 ? 1 / distance : (distance - .12f) / (.88f * distance);
                    x *= scale; y *= scale;
                }
                if (f.zone == MOVE) { result.move_x = x; result.move_y = y; }
                else { result.look_x = x; result.look_y = y; }
            } else {
                // A finger may slide off a button without holding it stuck.
                if (hit((f.x - left) / width, (f.y - top) / height) != f.zone) continue;
                if (f.zone == JUMP) result.buttons |= 0x1000;
                if (f.zone == ATTACK) result.buttons |= 0x2000;
                if (f.zone == CROUCH) result.buttons |= 0x20000;
                if (f.zone == START) result.buttons |= 0x10;
            }
        }
        return result;
    }
};
} // namespace coop_frontend
