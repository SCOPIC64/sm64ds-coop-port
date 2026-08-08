// Audio output: winmm waveOut, loaded dynamically.
//
// Dynamic loading follows walk_window's rule for user32/gdi32 -- resolve the
// library only after ntr::io_init has reserved the fixed DS regions, so the
// loader cannot place a DLL at 0x04000000..0x07ffffff and take an address
// the game needs. sd_out_open is reached from the first hosted ARM7 tick,
// which is well after io_init.
//
// A ring of four 1024-frame headers is 125 ms at 32768 Hz. The device is
// asked for 32768 Hz first (the DS's own mixing rate, so no resampling); if
// it refuses, 48000 Hz with linear interpolation at the output stage.
//
// SM64DS_NO_AUDIO=1 skips the device entirely. The mixer and sequencer still
// run -- clocked off the video frame instead of the device -- so a WAV dump
// and every "did that command do anything" check still work.
#include "sdat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {

enum { OUT_FRAMES = 1024, NBUF = 4, MIX_MAX = 2048 };

sd_s16 g_mix[MIX_MAX * 2];      // scratch at SD_MIX_RATE, stereo interleaved

int g_opened;                   // 0 untried, 1 device live, -1 no device
int g_devRate = SD_MIX_RATE;

// MUTED BY DEFAULT. The device still opens and the mixer still clocks off it,
// so the sound engine's timing is identical either way -- the output buffers
// are just zeroed on the way to the speaker. SM64DS_SOUND=1 unmutes (the
// debug switch); SM64DS_NO_AUDIO=1 remains the stronger "no device at all".
int g_muted = -1;               // -1 unread, 0 sound on, 1 muted

int out_muted(void)
{
    if (g_muted < 0)
        g_muted = getenv("SM64DS_SOUND") == 0;
    return g_muted;
}

#if defined(_WIN32)
typedef MMRESULT (WINAPI *pfnOpen)(HWAVEOUT *, UINT, const WAVEFORMATEX *,
                                   DWORD_PTR, DWORD_PTR, DWORD);
typedef MMRESULT (WINAPI *pfnHdr)(HWAVEOUT, WAVEHDR *, UINT);
typedef MMRESULT (WINAPI *pfnDev)(HWAVEOUT);

HMODULE  g_lib;
pfnOpen  p_Open;
pfnHdr   p_Prepare, p_Unprepare, p_Write;
pfnDev   p_Reset, p_Close;

HWAVEOUT g_dev;
WAVEHDR  g_hdr[NBUF];
sd_s16  *g_buf[NBUF];

// Resampler state, only used when the device would not take 32768 Hz.
double  g_phase;
sd_s16  g_last[2];

int open_at(int rate)
{
    WAVEFORMATEX wf;
    memset(&wf, 0, sizeof wf);
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.nSamplesPerSec = (DWORD)rate;
    wf.wBitsPerSample = 16;
    wf.nBlockAlign = 4;
    wf.nAvgBytesPerSec = (DWORD)rate * 4;
    return p_Open(&g_dev, WAVE_MAPPER, &wf, 0, 0, CALLBACK_NULL)
           == MMSYSERR_NOERROR;
}
#endif

// ---- wav dump -----------------------------------------------------------
FILE *g_wav;
sd_u32 g_wavFrames;

void wav_hdr(FILE *f, sd_u32 frames)
{
    sd_u32 dataBytes = frames * 4;
    sd_u32 riff = 36 + dataBytes;
    sd_u32 fmtLen = 16, rate = SD_MIX_RATE, byteRate = SD_MIX_RATE * 4;
    sd_u16 fmt = 1, ch = 2, bits = 16, align = 4;
    fwrite("RIFF", 1, 4, f);      fwrite(&riff, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);  fwrite(&fmtLen, 4, 1, f);
    fwrite(&fmt, 2, 1, f);        fwrite(&ch, 2, 1, f);
    fwrite(&rate, 4, 1, f);       fwrite(&byteRate, 4, 1, f);
    fwrite(&align, 2, 1, f);      fwrite(&bits, 2, 1, f);
    fwrite("data", 1, 4, f);      fwrite(&dataBytes, 4, 1, f);
}

// Render `frames` at SD_MIX_RATE into g_mix and feed the dump.
void render_mix(int frames)
{
    if (frames > MIX_MAX) frames = MIX_MAX;
    sd_mix_render(g_mix, frames);
    sd_wav_write(g_mix, frames);
}

}  // namespace

// ---- wav ----------------------------------------------------------------

void sd_wav_open(const char *path)
{
    if (g_wav) return;
    g_wav = fopen(path, "wb");
    if (!g_wav) { fprintf(stderr, "[sdat] cannot write %s\n", path); return; }
    g_wavFrames = 0;
    wav_hdr(g_wav, 0);          // patched on close
    // walk_window exits straight out of its frame loop, so without this the
    // RIFF sizes stay at zero and the file reads as empty even though every
    // sample is in it.
    atexit(sd_wav_close);
    fprintf(stderr, "[sdat] wav dump -> %s\n", path);
}

void sd_wav_write(const sd_s16 *pcm, int frames)
{
    if (!g_wav || frames <= 0) return;
    fwrite(pcm, sizeof(sd_s16) * 2, (size_t)frames, g_wav);
    g_wavFrames += (sd_u32)frames;
}

void sd_wav_close(void)
{
    if (!g_wav) return;
    fseek(g_wav, 0, SEEK_SET);
    wav_hdr(g_wav, g_wavFrames);
    fclose(g_wav);
    g_wav = 0;
    fprintf(stderr, "[sdat] wav dump closed: %u frames (%.2f s)\n",
            g_wavFrames, (double)g_wavFrames / SD_MIX_RATE);
}

// ---- device -------------------------------------------------------------

int sd_out_open(void)
{
    if (g_opened) return g_opened > 0;
    g_opened = -1;

    if (getenv("SM64DS_NO_AUDIO")) {
        fprintf(stderr, "[sdat] SM64DS_NO_AUDIO=1: no output device, "
                        "mixer runs silently\n");
        return 0;
    }
#if defined(_WIN32)
    g_lib = LoadLibraryA("winmm.dll");
    if (!g_lib) {
        fprintf(stderr, "[sdat] winmm.dll not available -- silent\n");
        return 0;
    }
    p_Open      = (pfnOpen)GetProcAddress(g_lib, "waveOutOpen");
    p_Prepare   = (pfnHdr) GetProcAddress(g_lib, "waveOutPrepareHeader");
    p_Unprepare = (pfnHdr) GetProcAddress(g_lib, "waveOutUnprepareHeader");
    p_Write     = (pfnHdr) GetProcAddress(g_lib, "waveOutWrite");
    p_Reset     = (pfnDev) GetProcAddress(g_lib, "waveOutReset");
    p_Close     = (pfnDev) GetProcAddress(g_lib, "waveOutClose");
    if (!p_Open || !p_Prepare || !p_Unprepare || !p_Write || !p_Reset || !p_Close) {
        fprintf(stderr, "[sdat] winmm waveOut entry points missing -- silent\n");
        return 0;
    }

    g_devRate = SD_MIX_RATE;
    if (!open_at(SD_MIX_RATE)) {
        g_devRate = 48000;
        if (!open_at(48000)) {
            fprintf(stderr, "[sdat] waveOutOpen failed at 32768 and 48000 Hz "
                            "-- silent\n");
            return 0;
        }
    }
    for (int i = 0; i < NBUF; i++) {
        g_buf[i] = (sd_s16 *)calloc(OUT_FRAMES * 2, sizeof(sd_s16));
        memset(&g_hdr[i], 0, sizeof g_hdr[i]);
        g_hdr[i].lpData = (LPSTR)g_buf[i];
        g_hdr[i].dwBufferLength = OUT_FRAMES * 2 * sizeof(sd_s16);
        p_Prepare(g_dev, &g_hdr[i], sizeof(WAVEHDR));
        g_hdr[i].dwFlags |= WHDR_DONE;      // free to fill
    }
    g_opened = 1;
    fprintf(stderr, "[sdat] waveOut open at %d Hz, %d x %d frames (%.0f ms)%s\n",
            g_devRate, NBUF, OUT_FRAMES,
            1000.0 * NBUF * OUT_FRAMES / g_devRate,
            out_muted() ? " -- MUTED (SM64DS_SOUND=1 for sound)" : "");
    return 1;
#else
    fprintf(stderr, "[sdat] no audio backend on this platform -- silent\n");
    return 0;
#endif
}

void sd_out_close(void)
{
#if defined(_WIN32)
    if (g_opened > 0) {
        p_Reset(g_dev);
        for (int i = 0; i < NBUF; i++) {
            p_Unprepare(g_dev, &g_hdr[i], sizeof(WAVEHDR));
            free(g_buf[i]);
            g_buf[i] = 0;
        }
        p_Close(g_dev);
        g_dev = 0;
    }
    if (g_lib) { FreeLibrary(g_lib); g_lib = 0; }
#endif
    g_opened = -1;
    sd_wav_close();
}

void sd_out_push(void)
{
    if (!g_opened) sd_out_open();

#if defined(_WIN32)
    if (g_opened > 0) {
        for (int i = 0; i < NBUF; i++) {
            if (!(g_hdr[i].dwFlags & WHDR_DONE)) continue;
            if (g_devRate == SD_MIX_RATE) {
                render_mix(OUT_FRAMES);
                memcpy(g_buf[i], g_mix, OUT_FRAMES * 2 * sizeof(sd_s16));
            } else {
                // Linear interpolation up to the device rate. g_phase and
                // g_last carry across blocks so the seam is continuous.
                double ratio = (double)SD_MIX_RATE / g_devRate;
                int need = (int)(OUT_FRAMES * ratio) + 2;
                render_mix(need);
                sd_s16 *o = g_buf[i];
                for (int k = 0; k < OUT_FRAMES; k++, o += 2) {
                    int idx = (int)g_phase;
                    double f = g_phase - idx;
                    if (idx >= need - 1) idx = need - 2, f = 1.0;
                    for (int c = 0; c < 2; c++) {
                        double a = g_mix[idx * 2 + c], b = g_mix[(idx + 1) * 2 + c];
                        o[c] = (sd_s16)(a + (b - a) * f);
                    }
                    g_phase += ratio;
                }
                g_phase -= need;
                if (g_phase < 0) g_phase = 0;
                g_last[0] = g_mix[(need - 1) * 2];
                g_last[1] = g_mix[(need - 1) * 2 + 1];
            }
            if (out_muted())
                memset(g_buf[i], 0, OUT_FRAMES * 2 * sizeof(sd_s16));
            g_hdr[i].dwFlags &= ~WHDR_DONE;
            g_hdr[i].dwBufferLength = OUT_FRAMES * 2 * sizeof(sd_s16);
            p_Write(g_dev, &g_hdr[i], sizeof(WAVEHDR));
        }
        return;
    }
#endif
    // No device: clock the mixer off the video frame so state still advances
    // (envelopes decay, sequences end) and the dump still fills.
    render_mix(SD_MIX_RATE / 60);
}
