#pragma once
#include <cstdint>
#include <vector>

namespace coop_media {
// These decoders do not open audio devices or use the game's memory/ABI.
class MusicDecoder {
    struct Impl;
    Impl* impl_;
public:
    MusicDecoder();
    ~MusicDecoder();
    MusicDecoder(const MusicDecoder&) = delete;
    MusicDecoder& operator=(const MusicDecoder&) = delete;
    bool open(const char* utf8_path);
    void close();
    unsigned read(short* stereo, unsigned frames);
    bool ended() const;
};
bool image(const char* utf8_path, std::vector<uint32_t>& pixels, int width, int height);
}
