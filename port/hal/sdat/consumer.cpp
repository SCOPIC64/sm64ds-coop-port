// The hosted ARM7: command-queue consumer.
//
// THE PROTOCOL (all ARM9 halves are matched src/ in slice_gate10):
//   Snd_SendCommand   takes a node off the free list at data_020a6484 and
//                     appends it to the pending list data_020a648c/6490.
//   func_0205b070     moves the whole pending list into ring slot
//                     data_020a64a8[data_020a649c], bumps the in-flight
//                     count data_020a64a0 and the batch tick data_020a64a4,
//                     and pokes the ARM7 with IPCSend(7, head, 0).
//   func_0205b274     when the ARM7's progress word has moved past
//                     data_020a6488, pops ring slot data_020a6498 and
//                     splices that batch back onto the free list.
//   func_0205b5d4     reads the ARM7's progress word (data_020a7fc0[0]).
//
// THE DEADLOCK, AND HOW IT IS CLOSED. func_0205b1d8 ends with
//     do { func_0205b274(1); p = func_0205adf8(); } while (p == 0);
// and func_0205b274(1) is
//     while (func_0205b5d4() == data_020a6488) {}
// -- a spin on a counter only the consumer advances. On hardware a second
// CPU advances it. Here there is no second CPU, so if the free list ever
// empties inside game code the spin is forever.
//
// The fix is not a bigger pool or a per-frame drain (game code can exhaust
// 256 nodes between two frames and never return). It is to host the seam
// where it actually is: func_0205b5d4 is "read the other core's progress",
// and on a single-threaded host, reading the other core's progress has to
// MAKE the other core run. So this file owns func_0205b5d4 (removed from
// the slice) and pumps the consumer before returning the counter.
//
// That makes termination provable. If the spin is entered, the free list is
// empty, so all 256 nodes are on the pending list or in the ring. Whichever
// they are, func_0205b1d8 has already called func_0205b070 (directly, or via
// the func_0205b274(0) drain) before the spin, so at least one batch is in
// the ring and data_020a64a4-1 > consumed. The pump therefore consumes at
// least one batch and advances the progress word, the comparison in
// func_0205b274 fails, the spin exits, the batch is reclaimed and the free
// list is non-empty. One iteration, always.
//
// The same hook makes the whole subsystem self-starting: seeding runs from
// the first func_0205b5d4, so even a sound call that arrives before the
// frame loop's tick finds a seeded pool instead of a null free list.
#include "sdat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---- the ARM9-side globals, as the matched code sees them ---------------
extern "C" {
extern void *data_020a6484;         // free list head
extern void *data_020a6494;         // free list tail NODE
extern void *data_020a648c;         // pending list head
extern void *data_020a6490;         // pending list tail
extern int   data_020a6488;         // batches reclaimed
extern int   data_020a6498;         // ring read index
extern int   data_020a649c;         // ring write index
extern int   data_020a64a0;         // batches in flight
extern int   data_020a64a4;         // next batch tick (starts at 1)
extern void *data_020a64a8[];       // 9-slot ring of batch heads
extern int   data_020a6760[];       // the 256 x 0x18 node pool
extern unsigned int *data_020a7fc0; // -> the ARM7 status block

int func_0205b070(int blocking);

/* The game's own sound init, in pieces. See sd_sound_init_host below. */
void func_0204f1e8(void);            /* three counters */
void func_02050950(void);            /* two more */
void func_0204fc40(void);            /* voice free list + 32 player records */
void func_0204f94c(void *p);         /* clear one player's voice pointer */
void func_02011a28(void *table);     /* PlayLong's 0x40-slot handle table */
void func_02048f34(void *owner);     /* 3D voice pools */
extern int data_0209b4a0[], data_0209b4b0[], data_0209b4a4[];
extern int data_0209b53c[];
extern unsigned char data_0209b4b4[];
extern unsigned char data_0209b480;  /* the master "sound effects on" flag */
}

namespace {

// A queue node: next, then the five words Snd_SendCommand writes.
struct Node {
    Node *next;
    int op, a, b, c, d;
};

enum { NODES = 256, RING = 9, STATUS_WORDS = 0x280 / 4 };

unsigned int g_status[STATUS_WORDS];   // the ARM7 status block we publish
int g_seeded;
int g_pumping;                          // reentrancy guard for the hook
unsigned g_consumed;                    // batches this consumer has executed
int g_readIdx;                          // our own cursor into the ring
int g_trace;                            // SM64DS_SND_TRACE=1
sd_u8 g_sawOp[256];

const char *op_name(int op)
{
    switch (op) {
    case 0x00: return "START_SEQ";
    case 0x01: return "STOP_SEQ";
    case 0x02: return "PREPARE_SEQ";
    case 0x03: return "PLAYER_PARAM";
    case 0x04: return "TRACK_PARAM";
    case 0x05: return "MUTE_TRACKS";
    case 0x09: return "SETUP_CAPTURE";
    case 0x0a: return "START_CHANNEL";
    case 0x0b: return "CHANNEL_SETUP";
    case 0x0e: return "CHANNEL_FLAGS";
    case 0x0f: return "CALLBACK_REG";
    case 0x11: return "SHARED_WORK";
    case 0x13: return "STOP_UNUSED";
    case 0x15: return "STRM_SETUP";
    case 0x16: return "STRM_PARAM";
    case 0x17: return "STRM_PARAM2";
    case 0x19: return "SET_STATUS_BLOCK";
    case 0x1b: return "LOAD_SEQ";
    case 0x1c: return "LOAD_BANK";
    case 0x1d: return "LOAD_WAVEARC";
    default:   return "?";
    }
}

// An opcode this consumer handles can still carry a PARAMETER it does not.
// Those would otherwise vanish without a word, which is the one thing the
// port is not allowed to do quietly.
void note_param(int op, int param)
{
    static sd_u8 seen[2][256];
    int row = (op == 3) ? 0 : 1;
    if (param < 0 || param > 255 || seen[row][param]) return;
    seen[row][param] = 1;
    fprintf(stderr, "[snd] command 0x%02x parameter 0x%02x not implemented "
            "-- ignored\n", op, param);
}

void exec(const Node *n)
{
    int op = n->op;
    if (g_trace)
        fprintf(stderr, "[snd] op %02x %-16s a=%08x b=%08x c=%08x d=%08x\n",
                op, op_name(op), (unsigned)n->a, (unsigned)n->b,
                (unsigned)n->c, (unsigned)n->d);

    switch (op) {
    case 0x00: {                        // START_SEQ
        // a = voice id (the byte at voice+0x3c), b = sequence data base,
        // c = offset of this entry within it, d = resident SBNK.
        //
        // b + c is the SEQARC case: func_02051a98 passes the SSAR's DATA
        // base in b and the entry's own offset in c, so a sound effect is
        // one entry inside a packed archive. Music passes c = 0, so the
        // same addition covers both.
        int slot = n->a & 31;
        const sd_u8 *seq = (const sd_u8 *)(size_t)(unsigned)(n->b + n->c);
        const sd_u8 *bnk = (const sd_u8 *)(size_t)(unsigned)n->d;
        if (!seq || !bnk) break;
        // func_0205b78c has normally already patched the bank's wave-link
        // slots by now; this fills any that are still empty and is a no-op
        // otherwise.
        sdat_link_bank_waves((sd_u8 *)bnk);
        sd_seq_start(slot, seq, bnk);
        break;
    }
    case 0x01:                          // STOP_SEQ
        sd_seq_stop(n->a & 31);
        break;
    case 0x03: {                        // PLAYER_PARAM: b = param, c = value
        int slot = n->a & 31;
        if (n->b == 4) sd_seq_set_volume(slot, n->c);
        else note_param(3, n->b);
        break;
    }
    case 0x04: {                        // TRACK_PARAM
        // a = voice id | (mode << 24), b = track mask, c = param, d = value.
        // Param 9 is PAN, and the value is SIGNED: func_02048d80 derives it
        // from the listener-relative X as (dx >> 12) / 2 clamped to
        // -0x40..0x3f and hands it straight to func_0204f7cc, so 0 means
        // centre. Reading it as an absolute 0..127 put every centred sound
        // hard left.
        int slot = (n->a & 0xffffff) & 31;
        if (n->c == 9) sd_seq_set_pan(slot, 64 + (int)(signed char)n->d);
        else note_param(4, n->c);
        break;
    }
    case 0x1b: case 0x1c: case 0x1d:
        // Load commands. Never expected: sdat_init pre-seats every FAT
        // residency slot with the resident address, so the game correctly
        // finds nothing to load. If one appears, the archive is already in
        // memory and there is nothing for the consumer to do.
        break;
    default:
        if (!g_sawOp[op & 0xff]) {
            g_sawOp[op & 0xff] = 1;
            fprintf(stderr, "[snd] command 0x%02x %s not implemented "
                    "(a=%08x b=%08x c=%08x d=%08x) -- skipped\n",
                    op, op_name(op), (unsigned)n->a, (unsigned)n->b,
                    (unsigned)n->c, (unsigned)n->d);
        }
        break;
    }
}

// Execute every batch the ARM9 has flushed but the consumer has not run.
void drain(void)
{
    while (g_consumed < (unsigned)(data_020a64a4 - 1)) {
        Node *head = (Node *)data_020a64a8[g_readIdx];
        if (++g_readIdx > 8) g_readIdx = 0;
        for (Node *n = head; n; n = n->next) exec(n);
        g_consumed++;
        g_status[0] = g_consumed;       // the word func_0205b5d4 reads
    }
}

// The game's own sound init, minus the four things that are hardware.
//
// func_020133bc is Sound::Init on the DS. It is not in any slice, and it
// cannot be called wholesale here, so this runs the six matched sub-inits
// that carry real state and skips the rest deliberately:
//
//   RUN  func_0204f1e8, func_02050950   counter resets
//   RUN  func_0204fc40                  builds the 16 voice records (the
//                                       +0x3c byte it stores is the voice id
//                                       every command carries) and the 32
//                                       player records with their default
//                                       playable-sequence limit of 1
//   RUN  func_0204f94c x3               clears the music, sub-music and SFX
//                                       player objects
//   RUN  func_02011a28                  Sound::PlayLong's handle table
//   RUN  func_02048f34                  the 3D voice pools
//   SET  data_0209b480 = 1              the master SFX flag. Without it
//                                       Player_PlaySoundEffect returns at its
//                                       first line and NOTHING makes a sound.
//
//   SKIP func_0205a82c   seeds the command pool via func_0205b358, which
//                        depends on three DS symbol adjacencies (see
//                        sd_consumer_init); this file seeds it instead.
//   SKIP func_02050f34   opens the SDAT off the card into a 1MB sound heap;
//                        hal/sdat/sdat.cpp seats an equivalent root already.
//   SKIP func_020134d8   loads group 1 into that heap; residency is
//                        pre-seated, so there is nothing to load.
//   SKIP func_020506fc   starts the ARM9 sound THREAD that would drain the
//                        queue. This consumer is that drain.
//
// If a sound plays that this init did not prepare for, the failure is a
// missing voice or a skipped command, both of which print -- not silence
// that pretends to be working.
void sd_sound_init_host(void)
{
    func_0204f1e8();
    func_02050950();
    func_0204fc40();
    func_0204f94c(&data_0209b4a0);
    func_0204f94c(&data_0209b4b0);
    func_0204f94c(&data_0209b4a4);
    func_02011a28(data_0209b53c);
    func_02048f34(data_0209b4b4);
    data_0209b480 = 1;
    fprintf(stderr, "[snd] sound init: 16 voices, 32 players, SFX enabled\n");
}

}  // namespace

void sd_consumer_init(void)
{
    if (g_seeded) return;
    g_seeded = 1;

    g_trace = getenv("SM64DS_SND_TRACE") != 0;

    // Seed the free list. This is func_0205b358's data effect, written out
    // by hand on purpose: that function is unusable on the host because it
    // depends on three symbols being ADJACENT in DS memory --
    //   data_020a6760 (pool) + 0x1800 == data_020a7f60 (callback table),
    //   data_020a7760 + 0x7e8      == the pool's LAST node, and
    //   data_020a7f48              == that same last node,
    // which it uses to zero pool[255].next and to seat the tail pointer.
    // Host symbols are separate objects, so running it would write past two
    // of them. The three lines below are what it means, not where it wrote.
    Node *pool = (Node *)data_020a6760;
    for (int i = 0; i < NODES - 1; i++) pool[i].next = &pool[i + 1];
    pool[NODES - 1].next = 0;
    data_020a6484 = &pool[0];
    data_020a6494 = &pool[NODES - 1];

    data_020a648c = 0;
    data_020a6490 = 0;
    data_020a64a0 = 0;
    data_020a6498 = 0;
    data_020a649c = 0;
    data_020a64a4 = 1;
    data_020a6488 = 0;

    memset(g_status, 0, sizeof g_status);
    data_020a7fc0 = g_status;
    g_consumed = 0;
    g_readIdx = 0;

    // func_0205b358 would now send command 0x19 to tell the ARM7 where the
    // status block is. There is no message to send: this consumer owns the
    // block, so the handshake is a no-op rather than a fake.

    sdat_init();
    sd_mix_reset();
    sd_seq_reset();
    sd_sound_init_host();

    const char *wav = getenv("SM64DS_WAV_DUMP");
    if (wav) sd_wav_open(wav);

    fprintf(stderr, "[snd] hosted ARM7: %d-node command pool seeded, "
            "status block at %p\n", NODES, (void *)g_status);
}

void sd_consumer_tick(void)
{
    sd_consumer_init();
    // The ARM9 half: publish whatever is pending. Non-blocking (0) -- with
    // 0 this cannot re-enter func_0205b274, so it cannot re-enter the pump.
    func_0205b070(0);
    drain();
}

// The seam. See the header comment for why this, and not a bigger pool.
extern "C" unsigned int func_0205b5d4(void)
{
    sd_consumer_init();
    if (!g_pumping) {
        g_pumping = 1;
        drain();
        g_pumping = 0;
    }
    return g_status[0];
}

extern "C" void _ZN5Sound22LoadAndSetMusic_Layer1Ei(int seqId);
extern "C" void func_0205a8c4(void *c);   /* Snd_SendCommand(0x13, c, 0,0,0) */

extern "C" void sdat_host_tick(void)
{
    sd_consumer_tick();

    // SM64DS_SND_MUSIC=<seq id> starts a BGM through the game's OWN front
    // door on the first tick. The walking harness never reaches the level
    // boot's music call, so without this there is no way to exercise
    // LoadAndSetMusic_Layer1 -> func_02011dcc -> func_02051fb4 ->
    // func_02051bd0 -> START in the live binary. 58 is NCS_BGM_CHIJOU, the
    // castle grounds theme.
    static int musicDone;
    if (!musicDone) {
        musicDone = 1;

        // SM64DS_SND_QUEUE_STRESS=N pushes N commands in one burst, with no
        // tick in between, which is exactly the state that used to deadlock:
        // past 256 the free list is empty, func_0205b1d8 falls into
        //     do { func_0205b274(1); p = func_0205adf8(); } while (!p);
        // and func_0205b274(1) spins on a counter only the consumer moves.
        // If this returns for N > 256, the spin terminates. It hangs forever
        // if the func_0205b5d4 hook is ever removed.
        const char *stress = getenv("SM64DS_SND_QUEUE_STRESS");
        if (stress) {
            int n = atoi(stress);
            fprintf(stderr, "[snd] queue stress: pushing %d commands with no "
                            "consumer tick...\n", n);
            for (int i = 0; i < n; i++) func_0205a8c4((void *)(size_t)i);
            fprintf(stderr, "[snd] queue stress: all %d pushed, no deadlock "
                            "(pool is %d nodes)\n", n, NODES);
        }
        const char *m = getenv("SM64DS_SND_MUSIC");
        if (m) {
            fprintf(stderr, "[snd] SM64DS_SND_MUSIC=%s: "
                    "Sound::LoadAndSetMusic_Layer1(%d)\n", m, atoi(m));
            _ZN5Sound22LoadAndSetMusic_Layer1Ei(atoi(m));
            sd_consumer_tick();
        }
    }

    sd_out_push();
}
