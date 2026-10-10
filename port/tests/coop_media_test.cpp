#include "hal/coop_media_decode.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#ifndef COOP_MEDIA_FIXTURES
#define COOP_MEDIA_FIXTURES "."
#endif
int main() {
    coop_media::MusicDecoder decoder;
    for(const char* name:{"menu-tone.wav","menu-tone.mp3","menu-tone.flac"}) {
        std::string path=std::string(COOP_MEDIA_FIXTURES)+"/"+name;
        assert(decoder.open(path.c_str()));
        short pcm[2048]; unsigned total=0; int peak=0;
        for(int block=0;block<64 && !decoder.ended();++block) {
            total+=decoder.read(pcm,1024);
            for(int i=0;i<2048;++i){int v=pcm[i];if(v<0)v=-v;if(v>peak)peak=v;}
        }
        assert(decoder.ended() && total>4096 && total<32000 && peak>1000);
        // A selected individual song can be restarted after reaching its end.
        assert(decoder.open(path.c_str()) && !decoder.ended());
        assert(decoder.read(pcm,1024)>0);
        std::printf("PASS: %s decoded/resampled, %u frames, peak %d\n",name,total,peak);
    }
    assert(!decoder.open("missing-custom-music.wav") && decoder.ended());
    std::string unicode=std::string(COOP_MEDIA_FIXTURES)+"/menu-tone-\xc3\xa9.wav";
    assert(decoder.open(unicode.c_str()));
    short unicode_pcm[2048]; assert(decoder.read(unicode_pcm,1024)>0);
    decoder.close();
    assert(!decoder.open("missing-custom-music.wav"));
    short silence[8]={1,1,1,1,1,1,1,1}; assert(decoder.read(silence,4)==0);
    for(short v:silence)assert(v==0);
    std::vector<uint32_t> pixels;
    for(const char* name:{"menu-colors.png","menu-colors.jpg","menu-colors.bmp"}) {
        std::string path=std::string(COOP_MEDIA_FIXTURES)+"/"+name;
        assert(coop_media::image(path.c_str(),pixels,64,64));
        assert(pixels.size()==4096 && pixels.front()!=pixels.back());
        for(uint32_t color:pixels)assert((color>>24)==255);
        std::printf("PASS: %s decoded/cropped\n",name);
    }
    std::vector<uint32_t> prior=pixels;
    assert(!coop_media::image("missing-picture.png",pixels,64,64) && pixels==prior);
    assert(!coop_media::image("missing-picture.png",pixels,0,64));
    std::puts("PASS: custom audio/image formats, streaming end/restart, bounds and missing files");
}
