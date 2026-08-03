/* AUTO-GENERATED from matched-function evidence by tools/gen_header.py
 * class ClsnResult: 5 matched functions, 9 evidenced fields.
 * Offsets/widths are observed, not guessed. Gaps are explicit padding.
 * Field NAMES are placeholders - renaming cannot change codegen. */
#ifndef CLSNRESULT_H
#define CLSNRESULT_H
#include "types.h"

/* unk_004 IS A 64-BIT FIELD AT A 4-ALIGNED OFFSET, and that is the ROM's
 * layout: CodeWarrior for ARM aligns `long long` to 4, so the first two words
 * of the record -- which are the surface's 8-byte CLPS entry -- sit at +4 and
 * +8 and the matched bodies read them as one doubleword.
 *
 * MSVC aligns it to 8. Left alone it slides unk_004 to +8 and every field
 * after it with it, while the SAME matched bodies address the rest of the
 * record by RAW BYTE OFFSET (ClsnResult::CopyTo reads &self->unk_004 and
 * writes dst + 4; SphereClsn::SetFloorResult is all raw offsets). Mixing the
 * two spellings across a 4-byte skew is silent: on the PC port it fed the
 * ground tracking a floor record whose CLPS entry started one word late, so
 * the surface type read back as 0x1f/3 and the path binding as 0 -- a value
 * castle grounds cannot produce, since all 22 of its CLPS entries name path
 * 0xff except 16 and 17 (paths 5 and 3).
 *
 * The pack is MSVC-only on purpose. mwccarm never defines _MSC_VER, so its
 * preprocessor never sees these lines and no byte of the decomp build moves.
 */
#ifdef _MSC_VER
#include <stddef.h>
#pragma pack(push, 4)
#endif

struct ClsnResult {
    u8  pad_000[0x4];
    s64 unk_004;            /* 0x004 */
    s32 unk_00c;            /* 0x00c */
    s32 unk_010;            /* 0x010 */
    s32 unk_014;            /* 0x014 */
    u16 unk_018;            /* 0x018 */
    u16 unk_01a;            /* 0x01a */
    s32 unk_01c;            /* 0x01c */
    s32 unk_020;            /* 0x020 */
    s32 unk_024;            /* 0x024 */
#ifdef __cplusplus
    /* methods */
    u32 GetClsnID() const;
#endif
};

#ifdef _MSC_VER
#pragma pack(pop)
/* Not a comment: the whole point of the pack above. Any host build that
   loses it fails here instead of corrupting floor records at runtime. */
typedef char ClsnResult_unk_004_must_be_at_4[
    (offsetof(struct ClsnResult, unk_004) == 4) ? 1 : -1];
#endif

#endif
