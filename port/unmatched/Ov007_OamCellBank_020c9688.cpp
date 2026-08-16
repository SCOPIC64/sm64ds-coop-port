/* HOST TRANSCRIPTION of func_ov007_020c9688, THE SCENE 1 INTERIM.
 * ov007 0x020c9688, 0x300 bytes (768, 192 ARM instructions).
 *
 * ============================ WHAT THIS IS ================================
 *
 * INTERIM. There is no C for this address anywhere in the tree and there is no
 * delink block covering it: config/arm9/overlays/ov007/delinks.txt runs
 * src/func_ov007_020c95fc.c to end:0x020c9688 and the next block starts at
 * src/func_ov007_020c9988.c, so the 768 bytes between them are a hole. This
 * file fills the hole for the PORT ONLY. It is derived from the ROM's own
 * instructions at that address and it is not a byte-match attempt, is not
 * scored by match.py, and is not counted by port/tools/linkage.py (its stem is
 * deliberately not func_ov007_020c9688, so port/tools/objsrc_check.py cannot
 * read it as the matched TU either).
 *
 * THE CRACK SIDE OWNS THE MATCH. The title-key residual (div=10, notes 6bg) is
 * the decomp lane's problem and nothing here helps it. When src/ gains a body
 * for 0x020c9688, THIS FILE RETIRES AUTOMATICALLY: the CMake block that adds it
 * is guarded on the src TU not existing (port/CMakeLists.txt, SC1_INTERIM_
 * SOURCES), so the matched body wins the day it lands and the interim leaves
 * the binary in the same configure. Deleting the file is then a cleanup, not a
 * fix, and the guard means a stale interim cannot quietly outlive its match.
 *
 * WHAT IT REPLACES. hal/scene_boot.cpp stood an L2_UNMATCHED trap here that
 * named itself and returned 0. Its caller src/func_ov007_020b6d40.c stored the
 * 0 into six slots and src/func_ov007_020ade58.c dereferenced one at +0xc:
 * FAULT c0000005 at 0x0000000c, which is the whole of scene 1's block. The trap
 * was the right answer while nobody had read the ROM at that address. It is not
 * the right answer once somebody has.
 *
 * ============================ PROVENANCE ==================================
 *
 * Read straight off extracted/overlays/overlay_0007.bin with capstone, at the
 * CONFIG-ALIGNED base 0x020ad660 (the minimum section start in
 * config/arm9/overlays/ov007/delinks.txt: .text starts there and .bss ends at
 * 0x02103340, a span of 0x55ce0 which is exactly the file's length, so file
 * offset = address - 0x020ad660 across the whole image). NOT the dsd export:
 * that export is short by 1248 bytes for this overlay (the ROM table declares
 * 0x55ce0), a defect the port-catchup lane raised and its reviewer confirmed as
 * a hard size defect, and lane A1 hit the same staleness in 2897 bytes of the
 * ov085 export.
 *
 * RELOCATIONS: FOUR, all of them arm_call, all four to bodies that are already
 * matched. There is not one relocated DATA word in these 768 bytes, so the
 * "does this word carry a DS address, and is it dereferenced" question that
 * romdata pointer tables force has no instances here. The four are
 *
 *   from:0x020c9698 -> 0x020c9a2c  src/func_ov007_020c9a2c.c   the header walk
 *   from:0x020c96bc -> 0x020c12ac  src/func_ov007_020c12ac.c   the bank alloc
 *   from:0x020c9718 -> 0x020c3df4  src/func_ov007_020c3df4.cpp the allocator
 *   from:0x020c9918 -> 0x020c13a4  src/func_ov007_020c13a4.c   the group alloc
 *
 * LITERAL POOL: ONE WORD, and it is a constant rather than an address.
 * 0x020C96E0 `ldr fp, [pc, #0x29c]` resolves to 0x020C9984, the last word of
 * the function, and that word reads 0x000001ff. No relocation row exists for
 * 0x020c9984, so nothing rewrites it at load time: it is the nine-bit mask the
 * two coordinate reads use, and it is spelled 0x1ff below.
 *
 * THE ARGUMENT is a pointer into ov007 .data, and IT IS DEREFERENCED, so the
 * mount has to have delivered a host address for it. It does: the sole caller
 * passes data_ov007_020d7e50[i] for i in 0..5, six words at 0x020d7e50 that
 * each carry a `kind:load` relocation into ov007 .data (0x020d9794, 0x020da744,
 * 0x020db0ac, 0x020d9ec0, 0x020d87b8, 0x020d8914). They are container files and
 * func_ov007_020c9a2c walks their headers. This is not taken on trust: the two
 * loops immediately ABOVE this call in func_ov007_020b6d40 already run the same
 * shape through func_ov007_020c99d8 and func_ov007_020c9988 against the sibling
 * tables data_ov007_020d7e28 and data_ov007_020d7e38, and they completed in
 * every run the seating lane made. The trap firing exactly six times is the
 * measurement that says the run reached this loop and no earlier one.
 *
 * ============================ THE SHAPE ===================================
 *
 * NOTHING HERE IS INVENTED and the frame proves it. The function opens
 * `push {r4-r8,sb,sl,fp,lr}` then `sub sp, sp, #0xb4`, hands `sp + 0x30` to
 * func_ov007_020c9a2c, and 0x30 + 0x84 = 0xb4 exactly. 0x84 is the size both
 * matched siblings give that same buffer (`char buf[0x84]` in
 * src/func_ov007_020c99d8.c, `struct Local { char b[0x84]; }` in
 * src/func_ov007_020c9988.c), so the top of this frame IS their local and the
 * bottom 0x30 bytes are this function's own scratch. The two reads at sp+0x74
 * and sp+0x80 are that buffer's +0x44 and +0x50, and +0x44 is the offset
 * func_ov007_020c9988 already spells as the pointer to the container's FIRST
 * data block. +0x50 is three int slots further on, so it is the FOURTH.
 *
 * func_ov007_020c9a2c is the NitroSDK-style container header walk: it takes the
 * file pointer, reads the block count from *(u16 *)(file + 0xe), and for each
 * block writes the block pointer at o[1 + i] and the block's DATA pointer
 * (block + 8) at o[0x11 + i], stepping by the block's own size at block + 4.
 * o[0x11] is block 0's data and o[0x14] is block 3's.
 *
 * SO: block 0 is a bank of OAM cells and block 3 is a bank of groups over them.
 * Both open with a u32 count. That reading is derived, not assumed, and the bit
 * layout below is where it comes from.
 *
 * ---- the packed 32-bit word, and why it is DS OAM attributes 0 and 1 -------
 *
 * Each 0x14-byte source record produces six output bytes: a 32-bit word stored
 * as two halfwords, then one more halfword. The 32-bit word is assembled at
 * 0x020C9748 through 0x020C9854 out of twelve fields, and every one of them
 * lands where the DS object attribute of that number lives:
 *
 *   bits  0..7   rec+2 low byte (ldrsh then AND 0xff)      attr0 Y
 *   bits  8..9   rec[8]                                    attr0 rot/scale,
 *                                                          double-size
 *   bits 10..11  rec[9]                                    attr0 OBJ mode
 *   bit  12      rec[10]                                   attr0 mosaic
 *   bit  13      rec[11]                                   attr0 colour mode
 *   bits 14..15  rec[12]                                   attr0 shape
 *   bits 16..24  rec+0 AND 0x1ff (the pool word)           attr1 X, nine bits
 *   bits 25..    rec[5]                                    attr1 rot/scale
 *                                                          parameter
 *   bit  28      rec[6]                                    attr1 H flip
 *   bit  29      rec[7]                                    attr1 V flip
 *   bits 30..31  rec[13]                                   attr1 size
 *
 * and the two branch conditions are the two places where those fields overlap
 * on real hardware, which is what makes the reading more than a coincidence:
 *
 *   THE OUTER TEST at 0x020C9774 and 0x020C97A0 compares the assembled
 *   (rec[8] << 8) | (rec[6] << 28) | (rec[7] << 29) against 0x100 and 0x300.
 *   Both values require rec[6] and rec[7] to be ZERO, so the test passes only
 *   when rot/scale is on AND neither flip bit is set. rec[5] << 25 is applied
 *   in exactly that case and in no other, and attr1 bits 9..13 are the
 *   rot/scale parameter when rot/scale is on and the flip flags when it is off.
 *   The ROM is refusing to write a parameter into bits that mean flip.
 *
 *   THE INNER TEST compares rec[9], the OBJ mode, against 3. Mode 3 is bitmap
 *   OBJ, where attr0 bit 13 is not a colour-mode selector, and rec[11] << 13 is
 *   dropped in exactly that case.
 *
 * The third halfword at 0x020C9858 is attr2: *(u16 *)(rec + 16) for the tile
 * name in bits 0..9, rec[14] << 10 for priority, rec[15] << 12 for the palette
 * row. Three attributes, six bytes, and the buffer is allocated as n * 6 at
 * 0x020C9710. That is the whole argument for the naming, and the BODY DOES NOT
 * DEPEND ON IT: the transcription below packs the same bits in the same order
 * whatever the fields are called.
 *
 * ---- block boundaries, each with the ROM address it came from --------------
 *
 *   0x020C9688  prologue, `sp + 0x30` handed to the header walk
 *   0x020C969C  the two block data pointers and the two counts
 *   0x020C96BC  func_ov007_020c12ac builds the bank
 *   0x020C96C4  cell loop setup, the unsigned `bls` early-out on count 0
 *   0x020C96F0  cell loop head: 4-byte group header, n * 6 buffer, count store
 *   0x020C973C  record loads (twelve fields out of 0x14 bytes)
 *   0x020C979C  the four packing branches (0x97A8, 0x97D8, 0x9808, 0x9834)
 *   0x020C9858  attr2 and the three halfword stores, cursor + 6
 *   0x020C98AC  cell loop tail, entry cursor + 8
 *   0x020C98D0  group loop setup
 *   0x020C98F4  group loop head: 12-byte header, func_ov007_020c13a4
 *   0x020C9928  group loop inner: cell back-pointer and one u16, stride 0x18
 *   0x020C9974  return the bank
 *
 * ============================ HONESTY =====================================
 *
 * NOT byte-verified and it never will be from here: the ROM never built from
 * this source, and a host transcription cannot be scored against ARM bytes. It
 * is behaviourally faithful to the disassembly, which is a weaker claim than a
 * match and a much stronger one than a guess. It carries no
 * "recovered from vtable slot identity" marker because it is not that kind of
 * body, so port/tools/inferred_stub_guard.py neither counts it nor should.
 *
 * WHAT IS PROVEN, and it is coarse. A 300-frame scene-1 run with this file in
 * enters no trap for 0x020c9688 (the counter is per name and the run prints
 * every entry), completes the whole
 * func_ov007_020b7138 -> func_ov007_020b68e8 -> func_ov007_020b6d40 subtree
 * including all six container builds and the nine func_ov007_020ade58 objects
 * that used to dereference the trap's zero, and returns from it into the next
 * call of dScDSMT_c::InitResources. That says the shape is right and the
 * pointers land somewhere usable. It does not say a single OAM bit is right,
 * because nothing has drawn one yet.
 *
 * WHAT IS NOT PROVEN.
 *   1. THE BITS. No frame has been rendered from this data, so the packing is
 *      verified against the ROM's instructions and against nothing else.
 *   2. THE BRANCHES. The two predicates are exercised only by whatever the six
 *      title-screen container files actually contain; a path none of them takes
 *      is transcribed but unrun.
 *   3. THE FOURTH BLOCK. The ROM reads o[0x14] unconditionally, and
 *      func_ov007_020c9a2c only writes o[0x11 + i] for i below the container's
 *      own block count. A file with fewer than four blocks makes the ROM read
 *      an uninitialised stack word, and this reads one too, on purpose: the
 *      alternative is a validity check the ROM does not have. Nothing here
 *      validates the container at all, and a malformed block count walks off
 *      the same way the ROM walks off.
 */

extern "C" {

/* The four callees, spelled the way their matched TUs spell them. All flat C,
   all __cdecl on the host, so no receiver rides anywhere here. */
void  func_ov007_020c9a2c(int *o, int x);
int   func_ov007_020c12ac(int cellCount, int groupCount);
int   func_ov007_020c3df4(int zero, int size);
void  func_ov007_020c13a4(char *group, int count, short a, short b);

int   func_ov007_020c9688(int fileAddr);

}

/* The bank func_ov007_020c12ac returns. Named for what its own matched TU
   (src/func_ov007_020c12ac.c, `struct S`) stores in each word: the two counts
   and the two zeroed arrays it allocates for them, 8 bytes per cell and 0x10
   bytes per group. Spelled locally rather than shared, because nothing else in
   the port has a declaration of it to agree with. */
struct Ov007CellBank {
    int   cellCount;    /* +0x00, the a it was called with            */
    char *cells;        /* +0x04, func_ov007_020c3df4(0, a << 3)      */
    int   groupCount;   /* +0x08, the b it was called with            */
    char *groups;       /* +0x0c, func_ov007_020c3df4(0, b << 4)      */
};

/* One cell: the OAM buffer this function allocates for it and how many entries
   went in. The ROM writes both through the bank's cells array at a byte cursor
   that steps by 8, and reads the pointer back out of it on every entry rather
   than keeping it in a register, so this does the same. */
struct Ov007Cell {
    char     *oam;      /* +0x00, func_ov007_020c3df4(0, n * 6) */
    unsigned  count;    /* +0x04 */
};

/* 0x020C9688. The sole caller (src/func_ov007_020b6d40.c) declares this
   `int func_ov007_020c9688(int a)` and stores the result as a pointer, so the
   int spelling is kept here rather than improved on: both are one 32-bit word
   in EAX under __cdecl, and matching the caller's own declaration is what
   guarantees there is no seam to argue about. */
int func_ov007_020c9688(int fileAddr)
{
    /* sp+0x30. 0x21 ints is 0x84 bytes, the size both matched siblings give
       this buffer, and int rather than char so the callee's word writes are
       aligned. */
    int hdr[0x21];
    char *h = (char *)hdr;

    func_ov007_020c9a2c(hdr, fileAddr);                            /* 0x9698 */

    /* 0x969C. +0x44 and +0x50 in the buffer, which are container blocks 0 and
       3. Each block's first word is its entry count. */
    const unsigned char *cellBlock  = *(const unsigned char **)(h + 0x44);
    const unsigned char *groupBlock = *(const unsigned char **)(h + 0x50);

    const unsigned cellCount  = *(const unsigned *)cellBlock;      /* sp+0x08 */
    const unsigned groupCount = *(const unsigned *)groupBlock;     /* sp+0x0c */

    /* 0x96BC. sp+0x00 holds the bank for the rest of the body and is what the
       function returns. */
    Ov007CellBank *bank =
        (Ov007CellBank *)func_ov007_020c12ac((int)cellCount, (int)groupCount);

    /* ---- 0x96C4: block 0, the cells ----------------------------------- */
    {
        /* r5 walks the block as one stream: a 4-byte group header, then that
           header's own count of 0x14-byte records, then the next header. */
        const unsigned char *rec = cellBlock + 4;
        unsigned cellOff = 0;                                    /* sp+0x1c */

        for (unsigned i = 0; i < cellCount; ++i) {                /* sp+0x04 */
            const unsigned n = rec[0];                           /* sp+0x10 */
            rec += 4;

            Ov007Cell *cell = (Ov007Cell *)(bank->cells + cellOff);
            cell->oam   = (char *)func_ov007_020c3df4(0, (int)(n * 6)); /* 0x9718 */
            cell->count = n;

            unsigned cursor = 0;                                 /* sl, +6 */
            for (unsigned j = 0; j < n; ++j) {                   /* 0x973C */
                /* Every field is unsigned before it is shifted. rec[13] << 30
                   and rec[5] << 25 overflow a signed int, and the ARM lsl
                   simply drops what leaves the top, which is what an unsigned
                   shift does in C and what a signed one does not promise. */
                const unsigned f5  = rec[5];
                const unsigned f6  = rec[6];
                const unsigned f7  = rec[7];
                const unsigned f8  = rec[8];
                const unsigned f9  = rec[9];
                const unsigned f10 = rec[10];
                const unsigned f11 = rec[11];
                const unsigned f12 = rec[12];
                const unsigned f13 = rec[13];
                const unsigned f14 = rec[14];
                const unsigned f15 = rec[15];
                const unsigned f16 = *(const unsigned short *)(rec + 16);
                /* both read with ldrsh, and both masked afterwards, so the
                   sign extension changes nothing that survives the mask */
                const unsigned y = (unsigned)*(const short *)(rec + 2);
                const unsigned x = (unsigned)*(const short *)(rec + 0);

                const unsigned shapeSize = (f12 << 14) | (f13 << 30);   /* ip */
                const unsigned tag = (f8 << 8) | (f6 << 28) | (f7 << 29); /* lr */

                rec += 0x14;                                     /* 0x9778 */

                unsigned word = tag | shapeSize
                              | (y & 0xff)
                              | (f9  << 10)
                              | (f10 << 12)
                              | ((x & 0x1ff) << 16);
                /* 0x9774 / 0x97A0: rot/scale on and neither flip bit set */
                if (tag == 0x100 || tag == 0x300)
                    word |= f5 << 25;
                /* 0x97A8 / 0x9808: OBJ mode 3 is bitmap, no colour-mode bit */
                if (f9 != 3)
                    word |= f11 << 13;

                const unsigned attr2 = f16 | (f14 << 10) | (f15 << 12);

                /* 0x9874 onward. Three halfword stores, and the 32-bit word
                   goes out as two of them because the cursor steps by 6 and
                   every other entry is therefore 2-mod-4. The pointer is
                   re-read from the cell on each store, as the ROM does. */
                {
                    char *out = ((Ov007Cell *)(bank->cells + cellOff))->oam;
                    *(unsigned short *)(out + cursor + 0) =
                        (unsigned short)word;
                    out = ((Ov007Cell *)(bank->cells + cellOff))->oam;
                    *(unsigned short *)(out + cursor + 2) =
                        (unsigned short)(word >> 16);
                    out = ((Ov007Cell *)(bank->cells + cellOff))->oam;
                    *(unsigned short *)(out + cursor + 4) =
                        (unsigned short)attr2;
                }
                cursor += 6;
            }
            cellOff += 8;                                        /* 0x98C4 */
        }
    }

    /* ---- 0x98D0: block 3, the groups ---------------------------------- */
    {
        /* Same one-stream shape, different strides: a 12-byte group header
           carrying its count at +8, then that many 0x18-byte records of which
           this function reads the first four bytes. */
        const unsigned char *q = groupBlock + 4;
        unsigned groupOff = 0;                                   /* sl, +0x10 */

        for (unsigned i = 0; i < groupCount; ++i) {              /* r6 */
            const unsigned count = q[8];                         /* r4 */
            char *group = bank->groups + groupOff;               /* r5 */
            q += 0xc;

            /* 0x9918. Allocates count * 4 pointers at group+0x00 and count * 2
               halfwords at group+0x04, and stores count at group+0x0c. */
            func_ov007_020c13a4(group, (int)count, 0, 0);

            for (unsigned k = 0; k < count; ++k) {               /* 0x9928 */
                const unsigned idx = *(const unsigned short *)(q + 0);
                const unsigned short w = *(const unsigned short *)(q + 2);
                q += 0x18;

                /* a back-pointer to the cell, by index, into the array the
                   first half of this function just filled */
                ((char **)*(char **)(group + 0))[k] = bank->cells + idx * 8;
                ((unsigned short *)*(char **)(group + 4))[k] = w;
            }
            groupOff += 0x10;
        }
    }

    return (int)bank;                                            /* 0x9974 */
}
