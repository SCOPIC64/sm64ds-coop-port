#pragma once
#include <cstdio>
#include <cstring>
#include <cstddef>

namespace coop_frontend {
struct SavePreview { bool exists = false, damaged = false; int stars = 0, character = 3; };
inline unsigned save_checksum(const unsigned char* payload) {
    unsigned sum = 0;
    const char* tag = "ds mario";
    for (int i = 0; i < 8; ++i) sum += static_cast<unsigned char>(tag[i]);
    for (int i = 0; i < 0x44; ++i) sum = (((sum << 1) | (sum >> 15)) ^ payload[i]) & 0xffff;
    return sum;
}
inline SavePreview preview_save(const unsigned char* chip, size_t size, int slot) {
    SavePreview result;
    if (!chip || slot < 0 || slot >= 3) return result;
    if (size != 8192) { result.damaged = true; return result; }
    bool erased = true;
    for (int copy = 0; copy < 2; ++copy) {
        const unsigned char* record = chip + copy * 4096 + slot * 128;
        for (int i = 0; i < 0x44 + 10; ++i) if (record[i] != 255) erased = false;
        if (std::memcmp(record + 2, "ds mario", 8)) continue;
        const unsigned char* data = record + 10;
        if (static_cast<unsigned>(record[0] | (record[1] << 8)) != save_checksum(data)) continue;
        result.exists = true;
        result.character = data[0x41] < 4 ? data[0x41] : 3;
        // IsStarCollected reads the 30 course bytes at FileSaveData + 0x14.
        for (int course = 0; course < 30; ++course) {
            unsigned bits = data[0x14 + course];
            while (bits) { result.stars += bits & 1; bits >>= 1; }
        }
        return result;
    }
    result.damaged = !erased;
    return result;
}
// Read only. A missing save remains a new slot; never repair or create data here.
inline void read_save_previews(const char* path, SavePreview (&out)[3]) {
    for (int i = 0; i < 3; ++i) out[i] = SavePreview();
    FILE* f = std::fopen(path, "rb"); if (!f) return;
    unsigned char chip[8193];
    size_t size = std::fread(chip, 1, sizeof chip, f); std::fclose(f);
    for (int i = 0; i < 3; ++i) out[i] = preview_save(chip, size, i);
}
}
