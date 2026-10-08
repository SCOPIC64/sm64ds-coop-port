#pragma once

// Port-owned front end. No Nintendo assets, platform calls or game-state writes.
#include <cstdio>
#include <cstring>
#include <cstdlib>

namespace coop_frontend {
enum Page { HOME, PLAY, HOST, JOIN, OPTIONS, PLAYER, CAMERA, CONTROLS,
            DISPLAY, SOUND, MISC, EXIT_CONFIRM };
enum Action { NONE, START_SOLO, START_HOST, START_JOIN, SAVE_OPTIONS, QUIT };
enum Key { UP, DOWN, LEFT, RIGHT, ACCEPT, BACK };

inline bool valid_address(const char* text) {
    // Deliberately IPv4 only, matching comms_loopback's supported transport.
    if (!text || !*text) return false;
    const char* p = text;
    for (int part = 0; part < 4; ++part) {
        int value = 0, digits = 0;
        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p++ - '0');
            if (++digits > 3 || value > 255) return false;
        }
        if (!digits) return false;
        if (part < 3) { if (*p++ != '.') return false; }
        else if (*p) return false;
    }
    return true;
}

inline bool valid_port(const char* text) {
    if (!text || !*text) return false;
    unsigned value = 0;
    for (const char* p = text; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
        value = value * 10 + (*p - '0');
        if (value > 65534) return false;
    }
    return value >= 1024;
}

struct Menu {
    Page page = HOME;
    Page after_save = HOME;
    int row = 0, slot = 0, character = 0;
    int camera = 0, movement = 1, fps = 0, volume = 80;
    bool smooth = true, names = true, mouse_capture = false, editing = false;
    char address[16] = "192.168.1.2", port[6] = "51765";
    char edit_backup[16] = "";
    char message[128] = "";

    int rows() const {
        switch (page) {
        case HOME: return 4;
        case PLAY: return 3;
        case HOST: return 5;
        case JOIN: return 5;
        case OPTIONS: return 7;
        case PLAYER: case DISPLAY: return 3;
        case CAMERA: case SOUND: case MISC: return 2;
        case CONTROLS: return 1;
        case EXIT_CONFIRM: return 2;
        }
        return 0;
    }
    const char* title() const {
        switch (page) {
        case HOME: return "";
        case PLAY: return "SINGLE PLAYER";
        case HOST: return "HOST A GAME";
        case JOIN: return "JOIN A GAME";
        case OPTIONS: return "OPTIONS";
        case PLAYER: return "PLAYER";
        case CAMERA: return "CAMERA";
        case DISPLAY: return "DISPLAY";
        case SOUND: return "SOUND";
        case MISC: return "MISC";
        case CONTROLS: return "CONTROLS";
        case EXIT_CONFIRM: return "QUIT THE GAME?";
        }
        return "";
    }
    void open(Page target) { page = target; row = 0; editing = false; message[0] = 0; }
    char* field() {
        if (page == JOIN && row == 0) return address;
        if ((page == HOST && row == 0) || (page == JOIN && row == 1)) return port;
        return nullptr;
    }
    void type(char ch) {
        char* p = editing ? field() : nullptr;
        if (!p) return;
        size_t size = p == address ? sizeof address : sizeof port;
        size_t n = std::strlen(p);
        if (ch == '\b') { if (n) p[n - 1] = 0; }
        else if (n + 1 < size && ((ch >= '0' && ch <= '9') ||
                                  (p == address && ch == '.'))) {
            p[n] = ch; p[n + 1] = 0;
        }
    }
    void label(int index, char* out, size_t size) const {
        static const char* home[] = { "HOST", "JOIN", "OPTIONS", "QUIT" };
        static const char* chars[] = { "MARIO", "LUIGI", "WARIO", "YOSHI" };
        static const char* cameras[] = { "ANALOG", "FREE", "DS" };
        static const char* movement_names[] = { "BUTTON", "ANALOG", "AUTO" };
        out[0] = 0;
        if (index < 0 || index >= rows()) return;
        if (page == HOME) std::snprintf(out, size, "%s", home[index]);
        else if (page == PLAY) {
            if (index == 0) std::snprintf(out, size, "SAVE FILE    < %c >", 'A' + slot);
            else std::snprintf(out, size, "%s", index == 1 ? "START GAME" : "BACK");
        } else if (page == HOST || page == JOIN) {
            int offset = page == JOIN ? 1 : 0;
            if (page == JOIN && index == 0) std::snprintf(out, size, "ADDRESS    %s%s", address, editing && row == index ? "_" : "");
            else if (index == offset) std::snprintf(out, size, "PORT       %s%s", port, editing && row == index ? "_" : "");
            else if (index == offset + 1) std::snprintf(out, size, "CHARACTER  < %s >", chars[character]);
            else std::snprintf(out, size, "%s", index == offset + 2 ? (page == HOST ? "START HOSTING" : "CONNECT") :
                page == HOST && index == 3 ? "SINGLE PLAYER" : "BACK");
        } else if (page == OPTIONS) {
            static const char* categories[] = { "PLAYER", "CAMERA", "CONTROLS", "DISPLAY", "SOUND", "MISC", "SAVE AND BACK" };
            std::snprintf(out, size, "%s", categories[index]);
        } else if (page == EXIT_CONFIRM) std::snprintf(out, size, "%s", index == 0 ? "KEEP PLAYING" : "QUIT");
        else if (index == rows() - 1) std::snprintf(out, size, "BACK");
        else if (page == CAMERA) std::snprintf(out, size, "CAMERA         < %s >", cameras[camera]);
        else if (page == PLAYER) {
            if (index == 0) std::snprintf(out, size, "MOVEMENT       < %s >", movement_names[movement]);
            else std::snprintf(out, size, "PLAYER NAMES   < %s >", names ? "ON" : "OFF");
        } else if (page == DISPLAY) {
            if (index == 0) {
                if (fps == 0) std::snprintf(out, size, "FRAME RATE     < NATIVE >");
                else std::snprintf(out, size, "FRAME RATE     < %d >", fps);
            } else std::snprintf(out, size, "SMOOTH MOTION  < %s >", smooth ? "ON" : "OFF");
        } else if (page == SOUND) std::snprintf(out, size, "VOLUME         < %d >", volume);
        else if (page == MISC) std::snprintf(out, size, "CAPTURE MOUSE  < %s >", mouse_capture ? "ON" : "OFF");
        else std::snprintf(out, size, "BACK");
    }
    Action key(Key key) {
        if (editing) {
            if (key == BACK) {
                char* p = field();
                if (p) std::strcpy(p, edit_backup);
            }
            if (key == BACK || key == ACCEPT) editing = false;
            return NONE;
        }
        if (key == BACK) {
            if (page == OPTIONS || page == PLAYER || page == CAMERA ||
                page == DISPLAY || page == SOUND || page == MISC) {
                after_save = page == OPTIONS ? HOME : OPTIONS;
                return SAVE_OPTIONS;
            }
            open(page == HOME ? EXIT_CONFIRM : page == PLAY ? HOST :
                 page == CONTROLS ? OPTIONS : HOME);
            return NONE;
        }
        if (key == UP || key == DOWN) {
            row = (row + rows() + (key == UP ? -1 : 1)) % rows(); return NONE;
        }
        const int delta = key == LEFT ? -1 : 1;
        const bool change = key == LEFT || key == RIGHT || key == ACCEPT;
        if (!change) return NONE;
        if (page == HOME && key == ACCEPT) {
            static const Page pages[] = { HOST, JOIN, OPTIONS, EXIT_CONFIRM };
            open(pages[row]);
        } else if (page == PLAY) {
            if (row == 0) slot = (slot + 3 + delta) % 3;
            else if (key == ACCEPT && row == 1) return START_SOLO;
            else if (key == ACCEPT) open(HOST);
        } else if (page == HOST || page == JOIN) {
            const int offset = page == JOIN ? 1 : 0;
            if (field() && key == ACCEPT) {
                std::strcpy(edit_backup, field()); editing = true; field()[0] = 0;
            }
            else if (row == offset + 1) character = (character + 4 + delta) % 4;
            else if (key == ACCEPT && row == offset + 2) {
                if (!valid_port(port)) std::snprintf(message, sizeof message, "Use a port from 1024 to 65534.");
                else if (page == JOIN && !valid_address(address)) std::snprintf(message, sizeof message, "Enter the host's IPv4 address.");
                else return page == HOST ? START_HOST : START_JOIN;
            } else if (key == ACCEPT && page == HOST && row == 3) open(PLAY);
            else if (key == ACCEPT && row == rows() - 1) open(HOME);
        } else if (page == OPTIONS) {
            if (key == ACCEPT) {
                static const Page categories[] = { PLAYER, CAMERA, CONTROLS, DISPLAY, SOUND, MISC };
                if (row == 6) { after_save = HOME; return SAVE_OPTIONS; }
                open(categories[row]);
            }
        } else if (page == PLAYER || page == CAMERA || page == DISPLAY || page == SOUND || page == MISC) {
            if (row == rows() - 1) {
                if (key == ACCEPT) { after_save = OPTIONS; return SAVE_OPTIONS; }
            } else if (page == CAMERA) camera = (camera + 3 + delta) % 3;
            else if (page == PLAYER) {
                if (row == 0) movement = (movement + 3 + delta) % 3;
                else names = !names;
            } else if (page == DISPLAY && row == 0) {
                const int rates[] = { 0, 60, 90, 120, 144, 240 };
                int index = 0;
                for (int i = 0; i < 6; ++i) if (fps >= rates[i]) index = i;
                fps = rates[(index + delta + 6) % 6];
            } else if (page == DISPLAY) smooth = !smooth;
            else if (page == SOUND) { volume += delta * 10; if (volume < 0) volume = 0; if (volume > 100) volume = 100; }
            else if (page == MISC) mouse_capture = !mouse_capture;
        } else if (page == EXIT_CONFIRM && key == ACCEPT) {
            if (row == 1) return QUIT;
            open(HOME);
        } else if (page == CONTROLS && key == ACCEPT) open(OPTIONS);
        return NONE;
    }
};
} // namespace coop_frontend
