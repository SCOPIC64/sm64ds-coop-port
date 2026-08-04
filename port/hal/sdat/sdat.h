// Native SDAT player: the host's ARM7 sound component.
//
// SEAM: the ARM9/ARM7 command queue. SM64DS's ARM9 side is fully decompiled
// and in slice_gate10 -- Snd_SendCommand builds 5-word nodes from a 256-node
// pool, func_0205b070 flushes the batch over PXI tag 7, and the ARM7 consumes
// it. Nobody has ever decompiled SM64DS's ARM7, so the host plays that role
// natively: same queue, same command ids, same SDAT bytes, a C++ sequencer
// and mixer instead of a 33MHz core.
//
// Everything ABOVE the queue stays the game's own matched code. The SDAT
// root (data_020a5bb8) is seated with a real parsed archive, so
// Sound::InfoSequenceEntry::GetWithID and its five sibling table walkers
// resolve for real, and func_0205b78c patches the SBNK/SWAR wave-link slots
// exactly as it does on hardware (those 32 bytes at +0x18 of a DATA block
// are reserved by the format for precisely that).
//
// HONESTY: anything not implemented prints once and no-ops. It never fakes.
#ifndef SM64DS_PORT_SDAT_H
#define SM64DS_PORT_SDAT_H

#include <stddef.h>

typedef unsigned char sd_u8;
typedef unsigned short sd_u16;
typedef unsigned int sd_u32;
typedef signed char sd_s8;
typedef short sd_s16;
typedef int sd_s32;

// ---- archive ------------------------------------------------------------

// INFO record indices, as the matched accessors index them (offset in the
// INFO block header = 0x08 + rec*4).
enum {
    SDAT_REC_SEQ     = 0,   // +0x08  Sound::InfoSequenceEntry::GetWithID
    SDAT_REC_SEQARC  = 1,   // +0x0c  func_02050c14
    SDAT_REC_BANK    = 2,   // +0x10  Sound::InfoInstrumentBankEntry::GetWithID
    SDAT_REC_WAVEARC = 3,   // +0x14  func_02050b4c
    SDAT_REC_PLAYER  = 4,   // +0x18  func_02050ae8
    SDAT_REC_GROUP   = 5,   // +0x1c  func_02050a84
    SDAT_REC_STRMPLR = 6,
    SDAT_REC_STRM    = 7,
    SDAT_REC_COUNT   = 8
};

struct SdatArchive {
    sd_u8 *base;        // whole file, resident and WRITABLE (func_0205b78c
                        // patches SBNK/SWAR reserved fields in place)
    size_t size;
    sd_u8 *info;        // INFO block base; record offsets are relative to it
    sd_u8 *fat;         // FAT block base; count at +8, entries at +0xc
    sd_u32 fatCount;
    sd_u8 *root;        // the 0x88-byte object data_020a5bb8 points at
};

extern SdatArchive g_sdat;

// Load extracted/dsd/files/data/sound_data.sdat, build the root object and
// seat data_020a5bb8. Pre-seats every FAT entry's runtime "loaded address"
// slot with the resident address, so the game's own residency checks pass
// and its card-load path is correctly skipped. Returns 1 on success.
int sdat_init(void);

// Table walkers, host-side mirrors of the matched accessors. Return a
// pointer into the INFO block or NULL.
sd_u8 *sdat_info_entry(int rec, sd_u32 id);

// FAT lookups.
sd_u8 *sdat_file(sd_u32 fileId);      // resident base of file, or NULL
sd_u32 sdat_file_size(sd_u32 fileId);
// Reverse map: a resident pointer back to its file id, or -1u.
sd_u32 sdat_file_id_of(const void *p);

// ---- SWAV ---------------------------------------------------------------

// Every SWAV is decoded to mono s16 once and cached, keyed by its record
// address. PCM8/PCM16 are widened/copied, IMA-ADPCM is fully decoded, so the
// mixer never carries per-format state across a loop point.
struct SdatWave {
    const sd_s16 *pcm;
    sd_u32 totalSamples;
    int loop;
    sd_u32 loopStart;       // in samples
    sd_u32 sampleRate;
};

// Resolve wave `index` inside a resident SWAR image. Returns 1 on success.
int sdat_swar_wave(const sd_u8 *swar, sd_u32 index, SdatWave *out);

// ---- SBNK ---------------------------------------------------------------

struct SdatNote {
    sd_u16 swav, swar;
    sd_u8 baseNote, attack, decay, sustain, release, pan;
};

// Resolve program `prog` + note `note` in a resident SBNK to its sample and
// envelope. Walks single/drumset/keysplit records. Returns 1 on success.
// `swarOut` receives the linked SWAR image read from the SBNK's reserved
// wave-link slots (SBNK+0x18+i*8), which func_0205b78c fills in.
int sdat_bank_note(const sd_u8 *sbnk, int prog, int note,
                   SdatNote *out, const sd_u8 **swarOut);

// Fill any UNSET wave-link slot in a resident SBNK (SBNK+0x18+i*8) with its
// SWAR's resident address. In the running game func_0205b78c does this; the
// call here is idempotent and only writes empty slots, so it is a backstop
// for paths that reach a bank without going through the loader (the smoke
// test does exactly that). Returns the number of slots it filled.
int sdat_link_bank_waves(sd_u8 *sbnk);

// ---- mixer --------------------------------------------------------------

enum { SD_CHANNELS = 16, SD_MIX_RATE = 32768 };

struct SdatChanKey { int player; int track; int note; };

// Allocate a hardware-style channel. Returns channel index or -1.
int sd_mix_alloc(int priority);
void sd_mix_start(int ch, const SdatWave *w, const SdatNote *n,
                  int volume_db10, int pan, double rate, int priority);
void sd_mix_release(int ch);          // enter the release phase
void sd_mix_kill(int ch);             // immediate stop
int  sd_mix_active(int ch);
void sd_mix_set(int ch, int volume_db10, int pan, double rate);
void sd_mix_set_pan(int ch, int pan);   // retune a voice already sounding
void sd_mix_frame(void);              // advance every envelope one 192Hz frame
void sd_mix_render(sd_s16 *dst, int frames);   // stereo interleaved
void sd_mix_reset(void);

// dB conversion shared by the sequencer and the mixer: 0..127 -> tenths of
// a dB in -723..0, the DS's own volume range.
int sd_cnv_vol(int v);

// ---- sequencer ----------------------------------------------------------

// Start a sequence. `seqData` points at the SSEQ bytecode (SSEQ base +
// *(u32*)(base+0x18)) -- exactly what command 0 carries. `sbnk` is the
// resident bank image. Returns 1 if a player slot was taken.
int sd_seq_start(int player, const sd_u8 *seqData, const sd_u8 *sbnk);
void sd_seq_stop(int player);
void sd_seq_set_volume(int player, int vol);      // 0..127
void sd_seq_set_pan(int player, int pan);         // 0..127, 64 centre
void sd_seq_frame(void);                          // one 192Hz sequencer frame
void sd_seq_reset(void);
int  sd_seq_active(int player);

// ---- output device ------------------------------------------------------

// winmm waveOut, loaded dynamically. SM64DS_NO_AUDIO=1 skips the device and
// the mixer still runs (and still feeds the .wav dump).
int  sd_out_open(void);
void sd_out_close(void);
void sd_out_push(void);       // render + submit whatever the ring wants

// SM64DS_WAV_DUMP=<path> captures the mixed stream to a RIFF/WAVE file.
void sd_wav_open(const char *path);
void sd_wav_write(const sd_s16 *pcm, int frames);
void sd_wav_close(void);

// ---- consumer -----------------------------------------------------------

// Seed the command pool and status block. Idempotent.
void sd_consumer_init(void);
// One hosted ARM7 tick: flush the ARM9 pending list, execute every queued
// command, return the nodes, advance the progress counter.
void sd_consumer_tick(void);

// THE call site. Safe before init, safe with no audio device.
extern "C" void sdat_host_tick(void);

#endif
