#include "hal/sdat/sdat.h"

// The front end uses the native ARM7 player directly. The cartridge's ARM9
// sound state is initialized later by the game, not by this menu.
class CoopMenuAudio {
    enum { MUSIC_PLAYER = 15, EFFECT_PLAYER = 14 };
    bool ready_ = false;
    int choice_ = -999, current_ = -1;
    unsigned random_ = GetTickCount() ^ GetCurrentProcessId();
    CoopMediaLibrary& library_;
    coop_media::MusicDecoder custom_;
    bool custom_playing_ = false;
    int volume_ = 80;
    static CoopMenuAudio* instance_;
    static void mix_custom(sd_s16* dst, int frames) {
        CoopMenuAudio* self=instance_; if(!self || !self->custom_playing_)return;
        short pcm[2048];
        for(int pos=0;pos<frames;pos+=1024) {
            int n=frames-pos<1024?frames-pos:1024; self->custom_.read(pcm,static_cast<unsigned>(n));
            for(int i=0;i<n*2;++i) {
                int v=dst[pos*2+i]+static_cast<int>(pcm[i])*self->volume_/100;
                dst[pos*2+i]=static_cast<short>(v<-32768?-32768:v>32767?32767:v);
            }
        }
    }
    static unsigned short u16(const sd_u8* p) { return p[0] | (p[1] << 8); }
    static unsigned u32(const sd_u8* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (static_cast<unsigned>(p[3]) << 24); }
    bool start(int player, const sd_u8* code, unsigned offset, unsigned bank, int volume) {
        sd_u8* info = sdat_info_entry(SDAT_REC_BANK, bank);
        sd_u8* image = info ? sdat_file(u32(info)) : 0;
        if (!image || memcmp(image, "SBNK", 4)) return false;
        sdat_link_bank_waves(image);
        sd_seq_stop(player);
        if (!sd_seq_start(player, code, offset, image)) return false;
        sd_seq_set_volume(player, volume);
        return true;
    }
    bool song(int index) {
        sd_u8* entry = sdat_info_entry(SDAT_REC_SEQ, coop_frontend::songs[index].sequence);
        if (!entry) return false;
        sd_u8* seq = sdat_file(u32(entry));
        unsigned size = sdat_file_size(u32(entry));
        if (!seq || size < 0x1c || memcmp(seq, "SSEQ", 4)) return false;
        unsigned offset = u32(seq + 0x18);
        if (offset >= size) return false;
        return start(MUSIC_PLAYER, seq + offset, 0, u16(entry + 4), entry[6]);
    }
public:
    explicit CoopMenuAudio(CoopMediaLibrary& library) : library_(library) {
        instance_=this; sd_mix_set_aux_render(mix_custom);
    }
    ~CoopMenuAudio() {
        sd_mix_set_aux_render(0); instance_=0; custom_.close();
        if (ready_) { sd_seq_stop(EFFECT_PLAYER); sd_seq_stop(MUSIC_PLAYER); sd_mix_reset(); sd_seq_reset(); }
    }
    bool init() { if (!ready_) ready_ = sdat_init() != 0; return ready_; }
    void invalidate() { choice_=-999; }
    void next(coop_frontend::Menu& menu) {
        choice_ = menu.music;
        sd_seq_stop(MUSIC_PLAYER); custom_.close(); custom_playing_=false; menu.now_playing[0]=0;
        if(menu.music==coop_frontend::MUSIC_OFF)return;
        int first=menu.music==coop_frontend::MUSIC_RANDOM_CUSTOM?coop_frontend::SONG_COUNT:0;
        int count=menu.music==coop_frontend::MUSIC_RANDOM?coop_frontend::SONG_COUNT:
            menu.music==coop_frontend::MUSIC_RANDOM_CUSTOM?static_cast<int>(library_.music.size()):
            coop_frontend::SONG_COUNT+static_cast<int>(library_.music.size());
        if(count<=0) { snprintf(menu.message,sizeof menu.message,"Add custom songs first.");return; }
        current_=menu.music<0?first+coop_frontend::random_item(random_,count,current_-first):menu.music;
        if(current_>=coop_frontend::SONG_COUNT) {
            int custom=current_-coop_frontend::SONG_COUNT;
            if(custom<static_cast<int>(library_.music.size()) && custom_.open(library_.music[custom].utf8.c_str())) {
                custom_playing_=true; snprintf(menu.now_playing,sizeof menu.now_playing,"%.47s",library_.music[custom].label.c_str());return;
            }
            snprintf(menu.message,sizeof menu.message,"Cannot play custom song. Using DS music.");
            menu.music=coop_frontend::MUSIC_RANDOM; choice_=menu.music;
            current_=coop_frontend::random_song(random_);
        }
        if(init() && song(current_))snprintf(menu.now_playing,sizeof menu.now_playing,"%s",coop_frontend::songs[current_].name);
    }
    bool effect(int entry) {
        if (!init()) return false;
        sd_u8* info = sdat_info_entry(SDAT_REC_SEQARC, 2);
        sd_u8* arc = info ? sdat_file(u32(info)) : 0;
        unsigned size = info ? sdat_file_size(u32(info)) : 0;
        if (!arc || size < 0x20 || memcmp(arc, "SSAR", 4)) return false;
        unsigned count = u32(arc + 0x1c), data = u32(arc + 0x18);
        if (entry < 0 || static_cast<unsigned>(entry) >= count ||
            0x20u + static_cast<unsigned>(entry) * 12 + 12 > size || data >= size) return false;
        const sd_u8* rec = arc + 0x20 + entry * 12;
        unsigned offset = u32(rec);
        if (offset >= size - data) return false;
        return start(EFFECT_PLAYER, arc + data, offset, u16(rec + 4), rec[6]);
    }
    void tick(coop_frontend::Menu& menu, int sound) {
        volume_=menu.volume;
        out_set_volume_pct(menu.volume);
        if (choice_ != menu.music) {
            choice_ = menu.music;
            sd_seq_stop(MUSIC_PLAYER); menu.now_playing[0] = 0;
            next(menu);
        } else if (choice_ != coop_frontend::MUSIC_OFF &&
            (custom_playing_?custom_.ended():!sd_seq_active(MUSIC_PLAYER))) next(menu);
        if (sound >= 0 && menu.menu_sounds) effect(sound);
        sd_out_push();
    }
    bool test() {
        if (!init()) return false;
        out_set_volume_pct(100);
        bool ok = true;
        sd_s16 pcm[2048];
        for (int i = 0; i < coop_frontend::SONG_COUNT + 3; ++i) {
            sd_seq_reset(); sd_mix_reset();
            const int effects[] = {0, 30, 123};
            bool started = i < coop_frontend::SONG_COUNT ? song(i) : effect(effects[i - coop_frontend::SONG_COUNT]);
            int peak = 0;
            for (int block = 0; started && block < 96; ++block) {
                sd_mix_render(pcm, 1024);
                for (int n = 0; n < 2048; ++n) { int v = pcm[n]; if (v < 0) v = -v; if (v > peak) peak = v; }
            }
            bool pass = started && peak > 32;
            fprintf(stderr, "[coop-menu] audio %s: %s peak=%d\n", pass ? "PASS" : "FAIL",
                i < coop_frontend::SONG_COUNT ? coop_frontend::songs[i].name : "MENU SFX", peak);
            ok = ok && pass;
        }
        return ok;
    }
    bool test_custom(coop_frontend::Menu& menu) {
        bool ok=true; volume_=100;
        for(size_t i=0;i<library_.music.size();++i) {
            menu.music=coop_frontend::SONG_COUNT+static_cast<int>(i); next(menu);
            sd_s16 pcm[2048]; int peak=0;
            for(int block=0;block<96;++block) {
                sd_mix_render(pcm,1024);
                for(int n=0;n<2048;++n){int v=pcm[n];if(v<0)v=-v;if(v>peak)peak=v;}
            }
            bool pass=custom_playing_ && peak>32; ok=ok&&pass;
            fprintf(stderr,"[coop-menu] custom audio %s: %s peak=%d\n",pass?"PASS":"FAIL",library_.music[i].label.c_str(),peak);
        }
        return ok;
    }
};
CoopMenuAudio* CoopMenuAudio::instance_=0;
