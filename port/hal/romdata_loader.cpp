// ROM-CLEAN boot loader (2026-08-10).
//
// walk_window.exe embeds ZERO Nintendo ROM bytes. Every ROM-derived table that
// romdata.py / ovdata.py used to bake into the binary is now a ZEROED array in
// the generated C, and the real bytes travel in one runtime file,
// build/assets/romdata.bin, produced from the user's own ROM at first run (and
// from the repo's extracted/ at dev-build time). port_romdata_load() reads that
// file, verifies it against build/assets/romdata.manifest, and memcpys each
// slice into its array via the generated dispatch (port_romdata_load_all).
//
// SEAM / ORDER: this runs at the very top of the game's boot, right after the
// asset root is resolvable and BEFORE the first read of any ROM table
// (port_ov002_patch, the sinits, Heap::SetupRootHeap). The pointer-rebase passes
// that follow (port_ov002_patch, port_cross_patch, port_ovNNN_[syms_]patch, the
// hand seats) run over the freshly-loaded bytes exactly as they did when the
// bytes were a linker initialiser -- the change is only WHERE the bytes live.
//
// LOUD TRAP (release discipline): a missing, short, or wrong romdata.bin, or any
// manifest disagreement (whole-file sha, a part sha, or a symbol size), is a
// FATAL abort that NAMES the file. A ROM-clean exe with no romdata.bin cannot
// run the game -- it must say so, not limp along on zeroed tables.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;
typedef unsigned int u32;
typedef unsigned long long u64;

#ifndef PORT_REPO_ROOT
#define PORT_REPO_ROOT "."
#endif

// The generated dispatch (build/port/host-src/romdata_dispatch.c).
extern "C" void port_romdata_load_all(const u8 *blob);

// ---- a self-contained SHA-256 (no external crypto dependency) --------------
// Standard FIPS-180-4. Kept local so the exe carries no DLL/crypto import.
namespace {

struct Sha256 {
    u32 h[8];
    u64 len;
    u8 buf[64];
    unsigned n;
};

const u32 K256[64] = {
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,
    0x923f82a4u,0xab1c5ed5u,0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
    0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,0xe49b69c1u,0xefbe4786u,
    0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,
    0x06ca6351u,0x14292967u,0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
    0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,0xa2bfe8a1u,0xa81a664bu,
    0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,
    0x5b9cca4fu,0x682e6ff3u,0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
    0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u,
};

inline u32 ror(u32 x, unsigned r) { return (x >> r) | (x << (32 - r)); }

void sha_init(Sha256 *s) {
    static const u32 iv[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u,
    };
    memcpy(s->h, iv, sizeof iv);
    s->len = 0;
    s->n = 0;
}

void sha_block(Sha256 *s, const u8 *p) {
    u32 w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (u32(p[i*4]) << 24) | (u32(p[i*4+1]) << 16) |
               (u32(p[i*4+2]) << 8) | u32(p[i*4+3]);
    for (int i = 16; i < 64; ++i) {
        u32 s0 = ror(w[i-15],7) ^ ror(w[i-15],18) ^ (w[i-15] >> 3);
        u32 s1 = ror(w[i-2],17) ^ ror(w[i-2],19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    u32 a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],
        e=s->h[4],f=s->h[5],g=s->h[6],hh=s->h[7];
    for (int i = 0; i < 64; ++i) {
        u32 S1 = ror(e,6) ^ ror(e,11) ^ ror(e,25);
        u32 ch = (e & f) ^ (~e & g);
        u32 t1 = hh + S1 + ch + K256[i] + w[i];
        u32 S0 = ror(a,2) ^ ror(a,13) ^ ror(a,22);
        u32 maj = (a & b) ^ (a & c) ^ (b & c);
        u32 t2 = S0 + maj;
        hh=g; g=f; f=e; e=d+t1; d=c; c=b; b=a; a=t1+t2;
    }
    s->h[0]+=a; s->h[1]+=b; s->h[2]+=c; s->h[3]+=d;
    s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=hh;
}

void sha_update(Sha256 *s, const u8 *p, size_t n) {
    s->len += n;
    while (n) {
        unsigned take = 64 - s->n;
        if (take > n) take = (unsigned)n;
        memcpy(s->buf + s->n, p, take);
        s->n += take; p += take; n -= take;
        if (s->n == 64) { sha_block(s, s->buf); s->n = 0; }
    }
}

void sha_final(Sha256 *s, char out[65]) {
    u64 bits = s->len * 8;
    u8 pad = 0x80;
    sha_update(s, &pad, 1);
    u8 zero = 0;
    while (s->n != 56) sha_update(s, &zero, 1);
    u8 lenbe[8];
    for (int i = 0; i < 8; ++i) lenbe[i] = (u8)(bits >> (56 - i*8));
    sha_update(s, lenbe, 8);
    for (int i = 0; i < 8; ++i)
        snprintf(out + i*8, 9, "%08x", s->h[i]);
    out[64] = 0;
}

void sha256_hex(const u8 *p, size_t n, char out[65]) {
    Sha256 s; sha_init(&s); sha_update(&s, p, n); sha_final(&s, out);
}

const char *asset_root() {
    const char *env = getenv("SM64DS_ASSET_ROOT");
    return env ? env : PORT_REPO_ROOT;
}

// Read an entire file. Returns malloc'd bytes + length, or NULL on any error.
u8 *read_all(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return 0; }
    fseek(f, 0, SEEK_SET);
    u8 *buf = (u8 *)malloc((size_t)sz ? (size_t)sz : 1);
    size_t got = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { free(buf); return 0; }
    *out_len = (size_t)sz;
    return buf;
}

void fatal_missing(const char *path) {
    fprintf(stderr,
        "FATAL: ROM data file missing or unreadable: %s\n"
        "       The port is ROM-clean: this file carries the ROM tables the\n"
        "       exe no longer embeds. Run the first-run extraction on your own\n"
        "       Super Mario 64 DS ROM to produce build/assets/romdata.bin:\n"
        "         python port/tools/romextract.py <your.nds> --root <dir>\n"
        "       (the launcher does this for you on first run).\n", path);
    abort();
}

} // namespace

// ---- the boot entry point --------------------------------------------------
extern "C" void port_romdata_load(void) {
    static int done;
    if (done) return;
    done = 1;

    char bin_path[1024], man_path[1024];
    snprintf(bin_path, sizeof bin_path, "%s/build/assets/romdata.bin",
             asset_root());
    snprintf(man_path, sizeof man_path, "%s/build/assets/romdata.manifest",
             asset_root());

    size_t blob_len = 0, man_len = 0;
    u8 *blob = read_all(bin_path, &blob_len);
    if (!blob) fatal_missing(bin_path);
    u8 *man = read_all(man_path, &man_len);
    if (!man) fatal_missing(man_path);

    // Header: "# romdata-manifest v1 <sha> <len>"
    char whole_sha[128]; unsigned long long want_len = 0;
    if (sscanf((char *)man, "# romdata-manifest v1 %127s %llu",
               whole_sha, &want_len) != 2) {
        fprintf(stderr, "FATAL: %s: unrecognised manifest header\n", man_path);
        abort();
    }
    if ((unsigned long long)blob_len != want_len) {
        fprintf(stderr, "FATAL: %s is %zu bytes; manifest says %llu -- "
                "wrong or truncated extraction\n", bin_path, blob_len, want_len);
        abort();
    }
    char got[65];
    sha256_hex(blob, blob_len, got);
    if (strcmp(got, whole_sha) != 0) {
        fprintf(stderr, "FATAL: %s sha256 %s != manifest %s -- corrupt or "
                "wrong-ROM extraction\n", bin_path, got, whole_sha);
        abort();
    }

    // Verify each part sha and each symbol size against the blob. Walk the
    // manifest line by line; any disagreement is fatal and names the offender.
    char *line = strtok((char *)man, "\n");
    while (line) {
        if (strncmp(line, "part\t", 5) == 0) {
            char tag[128], psha[128];
            unsigned long long base = 0, size = 0;
            if (sscanf(line, "part\t%127[^\t]\t%llu\t%llu\t%127s",
                       tag, &base, &size, psha) == 4) {
                if (base + size > blob_len) {
                    fprintf(stderr, "FATAL: %s: part %s runs past the blob\n",
                            man_path, tag);
                    abort();
                }
                char c[65];
                sha256_hex(blob + base, (size_t)size, c);
                if (strcmp(c, psha) != 0) {
                    fprintf(stderr, "FATAL: %s: part %s sha mismatch\n",
                            man_path, tag);
                    abort();
                }
            }
        } else if (strncmp(line, "sym\t", 4) == 0) {
            char tag[128], name[256], ssha[128];
            unsigned long long off = 0, sz = 0;
            if (sscanf(line, "sym\t%127[^\t]\t%255[^\t]\t%llu\t%llu\t%127s",
                       tag, name, &off, &sz, ssha) == 5) {
                if (off + sz > blob_len) {
                    fprintf(stderr, "FATAL: %s: symbol %s runs past the blob\n",
                            man_path, name);
                    abort();
                }
            }
        }
        line = strtok(0, "\n");
    }
    free(man);

    // Verified. Fill every zeroed array from its slice.
    port_romdata_load_all(blob);
    // The arrays now hold their own copies; the blob buffer is no longer needed.
    // (The dispatch memcpys into the arrays, so the loaded file can be freed.)
    free(blob);

    fprintf(stderr, "[romdata] loaded %zu bytes from %s\n", blob_len, bin_path);
}
