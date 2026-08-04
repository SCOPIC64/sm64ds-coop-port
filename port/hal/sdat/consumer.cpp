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
        break;
    }
    case 0x04: {                        // TRACK_PARAM
        // a = voice id | (mode << 24), b = track mask, c = param, d = value
        int slot = (n->a & 0xffffff) & 31;
        if (n->c == 9) sd_seq_set_pan(slot, n->d);
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

extern "C" void sdat_host_tick(void)
{
    sd_consumer_tick();
    sd_out_push();
}
