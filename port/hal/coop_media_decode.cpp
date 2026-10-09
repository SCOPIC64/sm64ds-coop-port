#include "coop_media_decode.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define MA_NO_DEVICE_IO
#define MA_NO_THREADING
#define MA_NO_ENGINE
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_GENERATION
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "../third_party/miniaudio/miniaudio.h"

#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_ONLY_BMP
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_WINDOWS_UTF8
#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb/stb_image.h"

namespace coop_media {
struct MusicDecoder::Impl { ma_decoder decoder; bool ready = false, end = true; };
MusicDecoder::MusicDecoder() : impl_(new (std::nothrow) Impl()) {}
MusicDecoder::~MusicDecoder() { close(); delete impl_; }
void MusicDecoder::close() {
    if (impl_ && impl_->ready) ma_decoder_uninit(&impl_->decoder);
    if (impl_) { impl_->ready = false; impl_->end = true; }
}
bool MusicDecoder::open(const char* path) {
    close();
    if (!impl_ || !path || !*path) return false;
    ma_decoder_config config = ma_decoder_config_init(ma_format_s16, 2, 32768);
    impl_->ready = ma_decoder_init_file(path, &config, &impl_->decoder) == MA_SUCCESS;
    impl_->end = !impl_->ready;
    return impl_->ready;
}
unsigned MusicDecoder::read(short* stereo, unsigned frames) {
    if (!stereo || !frames) return 0;
    std::memset(stereo, 0, static_cast<size_t>(frames)*2*sizeof(short));
    if (!impl_ || !impl_->ready || impl_->end) return 0;
    ma_uint64 got = 0;
    ma_result result = ma_decoder_read_pcm_frames(&impl_->decoder, stereo, frames, &got);
    if (got < frames || (result != MA_SUCCESS && result != MA_AT_END)) impl_->end = true;
    return static_cast<unsigned>(got);
}
bool MusicDecoder::ended() const { return !impl_ || impl_->end; }

bool image(const char* path, std::vector<uint32_t>& pixels, int width, int height) {
    int w = 0, h = 0, channels = 0;
    if (!path || width < 1 || height < 1 || width > 4096 || height > 4096 ||
        !stbi_info(path,&w,&h,&channels) || w < 1 || h < 1 || w > 8192 || h > 8192 ||
        static_cast<uint64_t>(w)*h > 16777216u) return false;
    unsigned char* data = stbi_load(path,&w,&h,&channels,4);
    if (!data) return false;
    std::vector<uint32_t> result(static_cast<size_t>(width)*height);
    // Fill the viewport while preserving the picture's aspect, cropping centrally.
    double scale = std::max(static_cast<double>(width)/w, static_cast<double>(height)/h);
    double left = (w-width/scale)/2, top = (h-height/scale)/2;
    for (int y=0; y<height; ++y) for (int x=0; x<width; ++x) {
        double sx = std::max(0.0,left+(x+0.5)/scale-0.5);
        double sy = std::max(0.0,top+(y+0.5)/scale-0.5);
        int ax=std::min(w-1,static_cast<int>(sx)), ay=std::min(h-1,static_cast<int>(sy));
        int bx=std::min(w-1,ax+1), by=std::min(h-1,ay+1);
        double fx=sx-ax, fy=sy-ay;
        uint32_t value=0xff000000u;
        for (int c=0; c<3; ++c) {
            double a=data[(ay*w+ax)*4+c]*(1-fx)+data[(ay*w+bx)*4+c]*fx;
            double b=data[(by*w+ax)*4+c]*(1-fx)+data[(by*w+bx)*4+c]*fx;
            value |= static_cast<uint32_t>(a*(1-fy)+b*fy+0.5) << (16-c*8);
        }
        result[static_cast<size_t>(y)*width+x]=value;
    }
    stbi_image_free(data); pixels.swap(result); return true;
}
}
