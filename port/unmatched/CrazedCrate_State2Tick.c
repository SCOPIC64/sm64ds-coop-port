/* HOST MIRROR of the matched src/func_ov080_02124e60.c (the crate's state-2
 * tick) with the same one-call fix as CrazedCrate_InitResources.cpp: the
 * recovered source drops the state setter's second argument (the
 * r1-passthrough seam). The ROM's own call site (0x02124e94: `mov r1, #0;
 * str r1, [r0, #0xd0]; bl 0x212513c`) passes 0 -- measured, not guessed.
 * Nothing else differs.
 */
extern int func_ov080_0212513c(char *c, int i); /* host edit: true arity */
int func_ov080_02124e60(char *c) {
    int v = *(int*)(c + 0xb0);
    int b1 = (v & 0x20000) ? 1 : 0;
    if (b1 != 0) goto done;
    int b2 = (v & 0x40000) ? 1 : 0;
    if (b2 != 0) goto done;
    *(int*)(c + 0xd0) = 0;
    func_ov080_0212513c(c, 0); /* host edit: the ROM's r1=0 */
done:
    return 1;
}
