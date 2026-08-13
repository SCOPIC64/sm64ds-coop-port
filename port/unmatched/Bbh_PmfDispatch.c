/* HOST COPIES of the three ov063 TUs that dispatch through mwcc
 * POINTER-TO-MEMBER tables -- the Painting_Dispatch.cpp case in its source
 * form: mwcc's PMF over an incomplete class is the ROM's 8-byte {fn, delta}
 * record, MSVC widens it to the 16-byte general form, so the compiled
 * &table[idx] strides wrong AND the call reads the neighbouring pair's words
 * as this-adjustments. The three matched sources stay untouched in src/ and
 * off the slice; each copy below spells the mwcc layout explicitly.
 *
 * The two tables are BSS the ov063 sinits fill by copying four 8-byte .data
 * pairs each -- so the port seats the .data pair fn words with host function
 * addresses BEFORE running the sinits (ov63_bringup in
 * hal/actor_classes_ov063.cpp), and the sinit's own matched copy then
 * propagates host pointers into the tables these dispatchers read. The
 * Painting recipe (hal/actor_overlays.cpp seats ov080's twelve statics the
 * same way). ROM pair values, read from overlay_0063.bin + relocs (delta 0
 * on all eight):
 *
 *   furniture table data_ov063_0211ef38[4] (sinit __sinit_ov063_0211e3cc):
 *     [0] <- data_ov063_0211e9b4 = {func_ov063_0211cc18, 0}  MansionSteps
 *     [1] <- data_ov063_0211e9bc = {func_ov063_0211cb54, 0}  TrapDoor
 *     [2] <- data_ov063_0211e9c4 = {func_ov063_0211c89c, 0}  Bookshelf
 *     [3] <- data_ov063_0211e9ac = {func_ov063_0211c7b0, 0}  MerryGoRound
 *   piano table data_ov063_0211efbc (2 Entries x 2 PMFs, sinit
 *   __sinit_ov063_0211e5fc):
 *     [0].pmf0 <- data_ov063_0211ecd8 = {func_ov063_0211dd78, 0}
 *     [0].pmf1 <- data_ov063_0211ece8 = {func_ov063_0211dbb8, 0}
 *     [1].pmf0 <- data_ov063_0211ece0 = {func_ov063_0211dba4, 0}
 *     [1].pmf1 <- data_ov063_0211ecf0 = {func_ov063_0211d8cc, 0}
 *   All eight bodies are matched TUs on slice_w5a.txt.
 */

extern unsigned char data_ov063_0211ef38[];
extern unsigned char data_ov063_0211efbc[];
extern int func_ov063_0211c684(char *c);
extern int func_ov063_0211c6f8(char *c);

typedef void (*BbhPmfFn)(char *self);

/* one mwcc {fn, delta} dispatch, layout spelled out */
static void bbh_pmf_call(unsigned char *pair, char *self)
{
    BbhPmfFn fn = (BbhPmfFn) *(void **)pair;
    fn(self + *(int *)(pair + 4));
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch (8-byte {fn,delta} vs
   MSVC's 16-byte form); the matched _ZN12MansionSteps8BehaviorEv.cpp strides
   data_ov063_0211ef38 wrong under MSVC. Layout hand-rolled here. */
int _ZN12MansionSteps8BehaviorEv(char *c)
{
    unsigned char before = *(unsigned char *)(c + 0x150);
    int idx = *(int *)(c + 0x140);
    bbh_pmf_call(data_ov063_0211ef38 + idx * 8, c);
    *(unsigned short *)(c + 0x14c) = (unsigned short)(*(unsigned short *)(c + 0x14c) + 1);
    if (before != *(unsigned char *)(c + 0x150))
        *(unsigned short *)(c + 0x14c) = 0;
    func_ov063_0211c684(c);
    func_ov063_0211c6f8(c);
    *(int *)(c + 0x124) = 0;
    return 1;
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch, same ruling -- the matched
   func_ov063_0211ddac.cpp reads Entry[idx].pmf[0] over the piano table. */
void func_ov063_0211ddac(char *c, int i)
{
    unsigned char *e;
    *(int *)(c + 0x6c8) = i;
    e = data_ov063_0211efbc + (*(int *)(c + 0x6c8)) * 16;
    bbh_pmf_call(e, c);
}

/* PORT_HOST_ABI: mwcc pointer-to-member dispatch, same ruling -- the matched
   func_ov063_0211ddf4.cpp reads Entry[idx].pmf[1]. */
void func_ov063_0211ddf4(char *c)
{
    unsigned char *e = data_ov063_0211efbc + (*(int *)(c + 0x6c8)) * 16;
    bbh_pmf_call(e + 8, c);
}
