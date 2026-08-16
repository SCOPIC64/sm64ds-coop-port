//cpp
/* _ZN4Heap8_DestroyEv @ 0x203c74c (arm9) -- tail-call veneer to _ZN4Heap7DestroyEv (0x203c758).
 * ldr ip, [pc]; bx ip; .word 0x203c758
 *
 * The heap rides through untouched: in r0 on ARM, in the first stack slot on
 * the host. Spelling that parameter is what lets the host carry the receiver
 * at all. The ARM veneer keeps it by register convention, a property of bx ip
 * and not of this C, so a host build that named no argument dropped it.
 * Naming it costs no ARM byte, because r0 is already in place for the tail
 * call; match.py still reports 2004/b56 MATCH on the twelve bytes
 * 00c09fe51cff2fe158c70302.
 */
extern "C" {
extern void _ZN4Heap7DestroyEv(void *thiz);
void _ZN4Heap8_DestroyEv(void *thiz) {
    _ZN4Heap7DestroyEv(thiz);
}
}
