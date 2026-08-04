// smoke_sdat: play a known SDAT sequence headless and report on the audio.
//
// This links ONLY the SDAT layer (parser, sequencer, mixer) -- no game code,
// no command queue -- so a failure here is a failure in the player itself
// rather than anywhere in the 7000-file slice. The queue and the front doors
// are exercised by the live walk_window run instead.
//
//   smoke_sdat                       BGM 58 (NCS_BGM_CHIJOU, castle grounds)
//   smoke_sdat --seq 58              a SEQ table entry by id
//   smoke_sdat --sfx 0:1             SEQARC 0 entry 1 (NCS_SE_PT_JUMP_N)
//   smoke_sdat --seconds 4           how long to render
//   SM64DS_WAV_DUMP=out.wav          write the mix to a RIFF/WAVE file
//
// Exit code is 0 only if the render was audible (peak above a floor), so CI
// can tell "played" from "parsed and produced silence".
#include "../hal/sdat/sdat.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// sdat_init seats the SDAT root here. In the game this symbol lives in
// hal/actor_vtables.cpp and every matched table walker reads it; this test
// links no game code, so it owns the storage itself.
extern "C" void *data_020a5bb8;
void *data_020a5bb8;

static inline sd_u16 rd16(const sd_u8 *p) { return (sd_u16)(p[0] | (p[1] << 8)); }
static inline sd_u32 rd32(const sd_u8 *p)
{ return (sd_u32)p[0] | ((sd_u32)p[1] << 8) | ((sd_u32)p[2] << 16) | ((sd_u32)p[3] << 24); }

// Resolve a SEQ table entry: fileId, bank, volume.
static int start_seq(int id)
{
    sd_u8 *e = sdat_info_entry(SDAT_REC_SEQ, (sd_u32)id);
    if (!e) { fprintf(stderr, "no SEQ entry %d\n", id); return 0; }
    sd_u32 fileId = rd32(e);
    sd_u16 bankId = rd16(e + 4);
    int vol = e[6];

    sd_u8 *sseq = sdat_file(fileId);
    if (!sseq || memcmp(sseq, "SSEQ", 4) != 0) {
        fprintf(stderr, "SEQ %d: file %u is not an SSEQ\n", id, fileId);
        return 0;
    }
    const sd_u8 *bytecode = sseq + rd32(sseq + 0x18);

    sd_u8 *be = sdat_info_entry(SDAT_REC_BANK, bankId);
    if (!be) { fprintf(stderr, "SEQ %d: no BANK %u\n", id, bankId); return 0; }
    sd_u8 *sbnk = sdat_file(rd32(be));
    if (!sbnk || memcmp(sbnk, "SBNK", 4) != 0) {
        fprintf(stderr, "SEQ %d: bank %u is not an SBNK\n", id, bankId);
        return 0;
    }
    int linked = sdat_link_bank_waves(sbnk);

    printf("SEQ %d: file=%u bank=%u (file %u, %d wavearcs linked) vol=%d\n",
           id, fileId, bankId, rd32(be), linked, vol);
    if (!sd_seq_start(0, bytecode, sbnk)) return 0;
    sd_seq_set_volume(0, vol);
    return 1;
}

// Resolve one entry of a SEQARC. This is the shape SM64DS's sound effects
// take: func_02051a98 passes the archive's DATA base and the entry's own
// offset separately, and the consumer adds them.
static int start_sfx(int arc, int entry)
{
    sd_u8 *ae = sdat_info_entry(SDAT_REC_SEQARC, (sd_u32)arc);
    if (!ae) { fprintf(stderr, "no SEQARC %d\n", arc); return 0; }
    sd_u8 *ssar = sdat_file(rd32(ae));
    if (!ssar || memcmp(ssar, "SSAR", 4) != 0) {
        fprintf(stderr, "SEQARC %d: file %u is not an SSAR\n", arc, rd32(ae));
        return 0;
    }
    sd_u32 dataOff = rd32(ssar + 0x18);
    sd_u32 count = rd32(ssar + 0x1c);
    if ((sd_u32)entry >= count) {
        fprintf(stderr, "SEQARC %d has %u entries, asked for %d\n",
                arc, count, entry);
        return 0;
    }
    const sd_u8 *rec = ssar + 0x20 + entry * 12;
    sd_u32 off = rd32(rec);
    sd_u16 bankId = rd16(rec + 4);
    int vol = rec[6];

    sd_u8 *be = sdat_info_entry(SDAT_REC_BANK, bankId);
    if (!be) { fprintf(stderr, "SEQARC entry: no BANK %u\n", bankId); return 0; }
    sd_u8 *sbnk = sdat_file(rd32(be));
    if (!sbnk || memcmp(sbnk, "SBNK", 4) != 0) {
        fprintf(stderr, "SEQARC entry: bank %u is not an SBNK\n", bankId);
        return 0;
    }
    int linked = sdat_link_bank_waves(sbnk);

    printf("SEQARC %d entry %d: dataOff=%u entryOff=%u bank=%u "
           "(%d wavearcs linked) vol=%d\n",
           arc, entry, dataOff, off, bankId, linked, vol);
    if (!sd_seq_start(0, ssar + dataOff + off, sbnk)) return 0;
    sd_seq_set_volume(0, vol);
    return 1;
}

int main(int argc, char **argv)
{
    int seq = 58, arc = -1, entry = 0;
    double seconds = 4.0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seq") && i + 1 < argc) {
            seq = atoi(argv[++i]); arc = -1;
        } else if (!strcmp(argv[i], "--sfx") && i + 1 < argc) {
            const char *a = argv[++i];
            const char *c = strchr(a, ':');
            arc = atoi(a);
            entry = c ? atoi(c + 1) : 0;
        } else if (!strcmp(argv[i], "--seconds") && i + 1 < argc) {
            seconds = atof(argv[++i]);
        } else {
            fprintf(stderr, "usage: %s [--seq N | --sfx ARC:ENTRY] "
                            "[--seconds S]\n", argv[0]);
            return 2;
        }
    }

    if (!sdat_init()) return 1;
    sd_mix_reset();
    sd_seq_reset();

    const char *wav = getenv("SM64DS_WAV_DUMP");
    if (wav) sd_wav_open(wav);

    int ok = (arc >= 0) ? start_sfx(arc, entry) : start_seq(seq);
    if (!ok) { sd_wav_close(); return 1; }

    // Render in blocks; sd_mix_render drives the 192 Hz sequencer clock.
    enum { BLK = 1024 };
    static sd_s16 buf[BLK * 2];
    long total = (long)(seconds * SD_MIX_RATE);
    long done = 0;
    double sumsq = 0;
    int peak = 0;
    long nonzero = 0;
    long firstSound = -1;

    while (done < total) {
        int n = (int)((total - done) < BLK ? (total - done) : BLK);
        sd_mix_render(buf, n);
        sd_wav_write(buf, n);
        for (int i = 0; i < n * 2; i++) {
            int v = buf[i];
            if (v) {
                nonzero++;
                if (firstSound < 0) firstSound = done + i / 2;
            }
            if (v < 0) v = -v;
            if (v > peak) peak = v;
            sumsq += (double)buf[i] * buf[i];
        }
        done += n;
    }
    sd_wav_close();

    double rms = sqrt(sumsq / (double)(total * 2));
    printf("rendered %.2f s at %d Hz: peak=%d (%.1f%% FS) rms=%.1f "
           "(%.1f%% FS) nonzero=%.1f%%\n",
           seconds, SD_MIX_RATE, peak, 100.0 * peak / 32767.0, rms,
           100.0 * rms / 32767.0, 100.0 * nonzero / (double)(total * 2));
    if (firstSound >= 0)
        printf("first non-zero sample at %.3f s\n",
               (double)firstSound / SD_MIX_RATE);
    else
        printf("no non-zero sample in the whole render\n");

    if (peak < 256) {
        printf("FAIL: output is silent (peak %d)\n", peak);
        return 1;
    }
    printf("OK\n");
    return 0;
}
