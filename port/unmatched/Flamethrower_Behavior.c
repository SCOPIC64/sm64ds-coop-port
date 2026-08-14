/* HOST COPY of _ZN12Flamethrower8BehaviorEv (ov095 0x021368f0, 0x470 bytes),
 * the flamethrower's per-frame spine -- slot 6 of _ZTV12Flamethrower, the one
 * method of the class that has NO matched TU in src/.
 *
 * WHY A HOST COPY AND NOT A SLICE LINE: there is nothing to slice. The other
 * four methods of the class ARE matched and go in slice_w6f.txt
 * (Flamethrower_Spawn, InitResources, D1, D0); 0x021368f0 alone is
 * undecompiled -- config/arm9/overlays/ov095/delinks.txt names four
 * Flamethrower TUs and this address is not among them, and the near-miss DB's
 * best attempt for ov095:0x021368f0 still stands at 154 divergences
 * (config/match_attempts.jsonl). Registering the class without this body would
 * be a stub, not ROM behaviour, which is exactly why lane w5-C refused the row.
 *
 * PROVENANCE, so the next reader can re-derive the trust rather than take it:
 * this text is a per-instruction transcription written from the ROM listing,
 * not from the near-miss draft (the draft was read for orientation only and
 * diverges from the ROM in a third of its instructions). The listing comes from
 * extracted/overlays/overlay_0095.bin at file offset (addr - 0x02135700), with
 * config/arm9/overlays/ov095/relocs.txt applied. The span is 0x470 bytes = 284
 * words: 276 ARM instructions plus the 8-word literal pool at 0x02136d40. Every
 * ROM address below is a real label in that listing, and every block comment
 * quotes the instructions it stands for. The pool is
 *
 *     02136d40  0209f2f8   data_0209f2f8 (arm9 bss, the current level)
 *     02136d44  00000465   the mLength member offset
 *     02136d48  00000466   the mTimer member offset
 *     02136d4c  00000434   the rotation matrix offset
 *     02136d50  02136fb0   data_ov095_02136fb0  (12 x s32, segment offsets)
 *     02136d54  02082214   data_02082214        (arm9 s16 [sin, cos] pairs)
 *     02136d58  02136f80   data_ov095_02136f80  (12 x s16, emitter arg 4)
 *     02136d5c  02136f98   data_ov095_02136f98  (12 x s16, emitter arg 5)
 *
 * The three ov095 tables ride this lane's ov095_syms.txt append; the two arm9
 * symbols were already mounted (data_0209f2f8 via cxx_aliases.cpp, the trig
 * table as `short data_02082214[]`, the spelling hal/player_bridges.cpp and
 * unmatched/Actor_ClosestPlayer_OverlayReaders.cpp already use).
 *
 * CALL TARGETS: thirteen distinct, every one resolved BY ADDRESS out of
 * relocs.txt and every one already linked into walk_window before this lane
 * except the four particle emitters, which this lane's slice adds:
 *
 *     0203adbc x2  DecIfAbove0_Short
 *     02012328     _ZN5Sound8PlayLongEjjjRK7Vector3j
 *     0203be9c     Matrix4x3_FromRotationXYZExt
 *     02052858     MulVec3Mat4x3
 *     0203d340 x2  Vec3_Add
 *     02010f3c     _ZN5Actor10FindWithIDEj
 *     020d57d8     _ZN6Player4BurnEv          (ov002, linked by gate 175)
 *     02022774     func_02022774  \  Particle::System::New wrappers, effect
 *     020226fc     func_020226fc  |  ids 0x4f / 0x50 / 0x51 / 0x52. Added to
 *     02022864     func_02022864  |  the build by slice_w6f.txt; their own
 *     020227ec     func_020227ec  /  closure was already linked.
 *     02015024     _ZN12CylinderClsn5ClearEv
 *     02014ff0     _ZN12CylinderClsn6UpdateEv
 *
 * No alias is introduced by this file: every name below is the ROM symbol at
 * C linkage and resolves to a definition already in the binary.
 *
 * THE OBJECT, as the listing uses it (alloc 0x46c, from Flamethrower_Spawn):
 *     +0x008  u32       spawn param word; the low byte selects the variant
 *     +0x05c  Vector3   the emitter's own position (x/y/z at 5c/60/64)
 *     +0x074  Vector3   the position Sound::PlayLong is given
 *     +0x08c  s16 x3    the three Euler angles
 *     +0x0b0  u32       Actor flags; bit 3 short-circuits the frame
 *     +0x0d4  CylinderClsnWithPos[12], stride 0x3c  (Spawn builds these)
 *               element +0x24  u32 the actor id it is touching
 *               element +0x30  Vector3 the collider's position
 *     +0x3a4  Vector3[12], stride 0xc  (the flame segment positions)
 *     +0x434  Matrix4x3 the rotation InitResources and this both build
 *     +0x464  u8        state:  0 = retracting, 1 = extending
 *     +0x465  u8        length: how many segments are live this frame
 *     +0x466  u16       the state timer
 *     +0x468  u32       the Sound::PlayLong handle
 *
 * When the decomp lands a byte match for 0x021368f0 that TU replaces this file
 * via slice_w6f.txt, the UpDownLiftBbh_Behavior / Boo_Behavior arrangement.
 */

typedef int s32;
typedef short s16;
typedef unsigned int u32;
typedef unsigned short u16;
typedef signed char s8;
typedef unsigned char u8;

typedef struct { s32 x, y, z; } Vector3;

/* ---- the ROM symbols this body reaches ---------------------------------- */
extern s8 data_0209f2f8;              /* arm9 bss: the current level */
extern short data_02082214[];         /* arm9: s16 [sin, cos] pairs, 4096 = 1 */
extern s32 data_ov095_02136fb0[];     /* 12 x s32, the per-segment reach */
extern short data_ov095_02136f80[];   /* 12 x s16 */
extern short data_ov095_02136f98[];   /* 12 x s16 */

extern u16 DecIfAbove0_Short(u16 *p);
extern s32 _ZN5Sound8PlayLongEjjjRK7Vector3j(u32 handle, u32 a, u32 b,
                                             Vector3 *pos, short e);
extern void Matrix4x3_FromRotationXYZExt(void *m, s32 x, s32 y, s32 z);
extern void MulVec3Mat4x3(Vector3 *in, void *m, Vector3 *out);
extern void Vec3_Add(Vector3 *out, Vector3 *a, Vector3 *b);
extern void *_ZN5Actor10FindWithIDEj(u32 id);
extern void _ZN6Player4BurnEv(void *player);
extern void func_02022774(s32 x, s32 y, s32 z, s16 a, s16 b);
extern void func_020226fc(s32 x, s32 y, s32 z, s16 a, s16 b);
extern void func_02022864(s32 x, s32 y, s32 z, s16 a, s16 b);
extern void func_020227ec(s32 x, s32 y, s32 z, s16 a, s16 b);
extern void _ZN12CylinderClsn5ClearEv(void *c);
extern void _ZN12CylinderClsn6UpdateEv(void *c);

s32 _ZN12Flamethrower8BehaviorEv(void *self);

s32 _ZN12Flamethrower8BehaviorEv(void *self)
{
    char *thiz = (char *)self;

    /* 021368f8..02136920: the segment count. Twelve everywhere except level 12,
       where it is nine, and then clamped into [1, 12] -- the clamp is dead for
       both live values but it is in the ROM, so it is here.
         ldrsb r0,[data_0209f2f8] / mov r8,#0xc / cmp r0,#0xc / moveq r8,#9
         cmp r8,#1 / movle r8,#1 / cmp r8,#0xc / movge r8,#0xc            */
    s32 n = 0xc;
    if (data_0209f2f8 == 0xc)
        n = 9;
    if (n <= 1)
        n = 1;
    if (n >= 0xc)
        n = 0xc;

    /* 02136924..02136934: the two-state machine, dispatched by a compare chain
       (not a table): 0 -> 02136938, 1 -> 02136980, anything else falls to the
       common tail at 021369c4. */
    {
        u8 state = *(u8 *)(thiz + 0x464);

        if (state == 0) {
            /* 02136938..0213697c -- RETRACTING. One segment dies per frame
               while any is left, and when the timer runs out the thrower flips
               to extending with a fresh 60-frame budget.
                 ldrb r0,[sb,#0x465] / cmp r0,#0
                 ldrne..strbne       the decrement, re-read through (this+0x465)
                 bl DecIfAbove0_Short(this+0x466) / cmp r0,#0 / bne 021369c4
                 mov r0,#1 / strb r0,[sb,#0x464]
                 add r0,sb,#0x400 / mov r1,#0x3c / strh r1,[r0,#0x66]   */
            if (*(u8 *)(thiz + 0x465) != 0)
                *(u8 *)(thiz + 0x465) = (u8)(*(u8 *)(thiz + 0x465) - 1);
            if (DecIfAbove0_Short((u16 *)(thiz + 0x466)) == 0) {
                *(u8 *)(thiz + 0x464) = 1;
                *(u16 *)(thiz + 0x466) = 0x3c;
            }
        } else if (state == 1) {
            /* 02136980..021369c0 -- EXTENDING. The mirror: one segment grows
               per frame until n-1, and the timer flips back to retracting.
               Note the growth bound is a SIGNED compare against n-1, and the
               transition here is the eq case of the same DecIfAbove0_Short.
                 ldrb r1,[sb,#0x465] / sub r0,r8,#1 / cmp r1,r0
                 ldrlt..strblt       the increment
                 bl DecIfAbove0_Short(this+0x466) / cmp r0,#0
                 moveq/strbeq r0,#0 -> state / strheq #0x3c -> timer        */
            if ((s32)*(u8 *)(thiz + 0x465) < n - 1)
                *(u8 *)(thiz + 0x465) = (u8)(*(u8 *)(thiz + 0x465) + 1);
            if (DecIfAbove0_Short((u16 *)(thiz + 0x466)) == 0) {
                *(u8 *)(thiz + 0x464) = 0;
                *(u16 *)(thiz + 0x466) = 0x3c;
            }
        }
    }

    /* 021369c4..021369ec: the looping flame sound, kept alive only while at
       least one segment is out. The handle is read back into +0x468.
         ldrb r0,[sb,#0x465] / cmp r0,#0 / beq 021369f0
         mov r0,#0 / str r0,[sp]        the stacked 5th argument
         ldr r0,[sb,#0x468] / add r3,sb,#0x74 / mov r1,#3 / mov r2,#0x180 */
    if (*(u8 *)(thiz + 0x465) != 0)
        *(u32 *)(thiz + 0x468) = (u32)_ZN5Sound8PlayLongEjjjRK7Vector3j(
            *(u32 *)(thiz + 0x468), 3, 0x180, (Vector3 *)(thiz + 0x74), 0);

    /* 021369f0..02136a10: the early out. `ands r0,r0,#8` then the conditional
       epilogue -- the ROM returns 1 without touching geometry or colliders. */
    if ((*(u32 *)(thiz + 0xb0) & 8) != 0)
        return 1;

    /* 02136a14..02136a48: rebuild the rotation matrix, but skip it for the
       0xff variant unless this is level 12. The listing spells the condition as
       two branches, which is why it reads inverted here:
         ldr r0,[sb,#8] / and r0,#0xff / cmp r0,#0xff / bne 02136a34 (do it)
         ldrsb data_0209f2f8 / cmp r0,#0xc / bne 02136a4c (skip it)        */
    if ((*(u32 *)(thiz + 8) & 0xff) != 0xff || data_0209f2f8 == 0xc)
        Matrix4x3_FromRotationXYZExt(thiz + 0x434,
                                     *(s16 *)(thiz + 0x8c),
                                     *(s16 *)(thiz + 0x8e),
                                     *(s16 *)(thiz + 0x90));

    /* 02136a4c..02136cf0: the per-segment pass. Two guards before it
       (02136a54 length == 0, 02136a60 n <= 0) send the frame straight to the
       collider pass, and the loop head re-reads the length every iteration
       (02136a88 `cmp r7,r0 / beq 02136cf4`) so a mid-frame shrink is seen. */
    if (*(u8 *)(thiz + 0x465) != 0 && n > 0) {
        /* the three cursors the ROM keeps instead of indexing: r6 walks the
           Vector3[12] at +0x3a4 (stride 0xc), r4 walks the
           CylinderClsnWithPos[12] at +0xd4 (stride 0x3c, biased so the element
           is at r4+0xd4), r5 counts 1.0 in Fix12 per segment. */
        char *seg = thiz;          /* r6 */
        char *col = thiz;          /* r4 */
        s32 rot = 0;               /* r5 */
        s32 i;

        for (i = 0; i < n; i++) {
            if (i == (s32)*(u8 *)(thiz + 0x465))
                break;

            /* 02136a94..02136ab0: the same variant gate as the matrix, guarding
               the whole positioning block (jump to 02136bdc skips it). */
            if ((*(u32 *)(thiz + 8) & 0xff) != 0xff || data_0209f2f8 == 0xc) {
                /* 02136ab4..02136af0: this segment's offset is a pure +Z reach
                   from data_ov095_02136fb0[i] pushed through the rotation.
                   Both vectors are zeroed first, the ROM's `str r0` x6 from the
                   zero it parked in sp[4]; only in.z is then set. */
                Vector3 in;
                Vector3 dst;
                Vector3 sum;
                s32 im1 = i - 1;

                in.x = 0; in.y = 0; in.z = 0;
                dst.x = 0; dst.y = 0; dst.z = 0;
                in.z = data_ov095_02136fb0[i];
                MulVec3Mat4x3(&in, thiz + 0x434, &dst);

                /* 02136af4..02136b54: segment 0 hangs off the thrower's own
                   position (+0x5c); every later one hangs off its predecessor,
                   which is what makes the flame a chain. */
                if (im1 < 0)
                    Vec3_Add(&sum, (Vector3 *)(thiz + 0x5c), &dst);
                else
                    Vec3_Add(&sum, (Vector3 *)(thiz + 0x3a4 + im1 * 0xc), &dst);
                *(s32 *)(seg + 0x3a4) = sum.x;
                *(s32 *)(seg + 0x3a8) = sum.y;
                *(s32 *)(seg + 0x3ac) = sum.z;

                /* 02136b58..02136bc0: level 12 only -- the vertical wobble.
                   The index is the low 16 bits of the Fix12 counter shifted
                   down 4 (the ROM's lsl16/asr16/lsl16/lsr16/asr4 chain), the
                   table is the arm9 [sin, cos] pairs so the element is idx*2,
                   and the scale is the signed 64-bit Fix12 multiply the port
                   already spells this way elsewhere:
                     umull/mla/mla for the full signed product, then
                     (product + 0x800) >> 12 taken as 32 bits.
                   -0xaa000 is -170.0 in Fix12. */
                if (data_0209f2f8 == 0xc) {
                    u16 a16 = (u16)(s16)rot;
                    s32 idx = (s32)(a16 >> 4);
                    s16 sinv = data_02082214[idx * 2];
                    s32 dy = (s32)((((long long)sinv * (long long)-0xaa000)
                                    + 0x800) >> 12);
                    *(s32 *)(seg + 0x3a8) = *(s32 *)(thiz + 0x60) + dy;
                }

                /* 02136bc4..02136bd8: publish the segment position into this
                   segment's collider (element +0x30 .. +0x38). */
                *(s32 *)(col + 0x104) = *(s32 *)(seg + 0x3a4);
                *(s32 *)(col + 0x108) = *(s32 *)(seg + 0x3a8);
                *(s32 *)(col + 0x10c) = *(s32 *)(seg + 0x3ac);
            }

            /* 02136bdc..02136c0c: whatever the collider touched last frame
               (element +0x24 holds the actor id) gets burned, but only if it is
               the player. The ROM materialises the 1 and the 0 of that test in
               two stack slots (sp[8] = 1, sp[0xc] = 0) and selects between them
               with ldreq/ldrne on `cmp r1,#0xbf`; the value is then tested
               against zero. 0xbf = 191 = PLAYER, and the pointer Burn is given
               is the one FindWithID returned. */
            {
                u32 other = *(u32 *)(col + 0xf8);
                if (other != 0) {
                    void *found = _ZN5Actor10FindWithIDEj(other);
                    if (found != 0 && *(u16 *)((char *)found + 0xc) == 0xbf)
                        _ZN6Player4BurnEv(found);
                }
            }

            /* 02136c10..02136cd8: the particle pair. The emit point is the
               segment position raised by 0x32000 (50.0 in Fix12) in Y --
               written into the slot twice, the ROM stores y then overwrites it
               with y + 0x32000. Which pair of emitters runs is the variant's
               low byte, and both calls in a pair share the SAME
               data_ov095_02136f80[i] as their 4th argument (the ROM keeps it in
               sl across both). The 5th argument of the first call is the zero
               the ROM parked in sp[0x10] / sp[0x14] before the loop; of the
               second, data_ov095_02136f98[i]. */
            {
                Vector3 at;
                s16 arg4;

                at.x = *(s32 *)(seg + 0x3a4);
                at.y = *(s32 *)(seg + 0x3a8);
                at.z = *(s32 *)(seg + 0x3ac);
                at.y = at.y + 0x32000;

                if ((*(u32 *)(thiz + 8) & 0xff) != 0xff) {
                    arg4 = data_ov095_02136f80[i];
                    func_02022774(at.x, at.y, at.z, arg4, 0);
                    func_020226fc(at.x, at.y, at.z, arg4,
                                  data_ov095_02136f98[i]);
                } else {
                    arg4 = data_ov095_02136f80[i];
                    func_02022864(at.x, at.y, at.z, arg4, 0);
                    func_020227ec(at.x, at.y, at.z, arg4,
                                  data_ov095_02136f98[i]);
                }
            }

            /* 02136cdc..02136cf0 */
            seg += 0xc;
            rot += 0x1000;
            col += 0x3c;
        }
    }

    /* 02136cf4..02136d2c: the collider pass. EVERY one of the n colliders is
       cleared each frame, and only the live prefix (index < length) is put back
       on the active list -- so a retracted segment stops colliding without
       being destroyed. */
    if (n > 0) {
        char *c = thiz + 0xd4;
        s32 k;

        for (k = 0; k < n; k++) {
            _ZN12CylinderClsn5ClearEv(c);
            if (k < (s32)*(u8 *)(thiz + 0x465))
                _ZN12CylinderClsn6UpdateEv(c);
            c += 0x3c;
        }
    }

    /* 02136d30..02136d3c */
    return 1;
}
