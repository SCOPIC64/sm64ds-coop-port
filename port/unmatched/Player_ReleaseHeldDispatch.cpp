/* HOST COPY of src/func_ov002_020bdb50.cpp -- the Player "let go of the held
 * object" path that, for one held-object kind, hands the object to
 * func_ov002_020d5cec.
 *
 * THE CALLING-CONVENTION SEAM:
 *
 * func_ov002_020d5cec takes ONE argument -- the held object -- and dereferences
 * it (its host body does `mov r4, r0` then uses r4 throughout). On the DS the
 * caller leaves that object live in r0 at the call:
 *
 *     020bdbb8: ldr  r0, [r4, #0x360]   ; r0 = *(this+0x360) = the held object
 *     020bdbbc: ldrh r1, [r0, #0xc]     ; (r0 kept)
 *     ...
 *     020bdbd4: bl   func_ov002_020d5cec ; r0 still = the held object
 *
 * so the argument arrives for free in r0. The matched C spells the call with no
 * argument (`func_ov002_020d5cec(void)`); byte-identical on ARM because the
 * object is already in r0 from the immediately preceding load.
 *
 * On the host that zero-argument call pushes nothing; func_ov002_020d5cec's
 * object reads its first parameter from an unwritten stack slot and dereferences
 * garbage. THE FIX passes the held object explicitly -- exactly the value the
 * ROM leaves in r0: *(char **)(c + 0x360), which the surrounding matched code
 * already spells as `obj`.
 *
 * src/func_ov002_020bdb50.cpp is dropped from slice_gate10.txt in favour of this
 * file; the byte-locked source is unchanged.
 */

extern "C" {
extern void func_ov002_020bdc18(char* c);
extern void Player_ReleaseHeldActor(char* c);
extern void func_ov002_020d71a0(char* p);
extern int func_ov002_020d5cec(char* obj);   /* the real one-arg (held obj) shape */
// PORT_HOST_ABI: implicit-register-arg (callee takes the held object *(this+0x360); rode r0 on ARM).
void func_ov002_020bdb50(char* c, int arg) {
    func_ov002_020bdc18(c);
    Player_ReleaseHeldActor(c);
    if (arg != 0) return;
    if (*(int*)(c+0x360) == 0) return;
    func_ov002_020d71a0(c);
    {
        char* obj = *(char**)(c+0x360);
        char* vt = *(char**)obj;
        int r = (*(int(**)(char*))(vt+0x48))(obj);
        if (r == 0) return;
    }
    {
        char* obj = *(char**)(c+0x360);
        int b1 = (*(unsigned short*)(obj+0xc) == 0xbf);
        if (b1) {
            func_ov002_020d5cec(obj);   /* <-- the held object, ROM's r0 */
            (*(unsigned short *)(((int)c + 0x6ce))) &= ~2;
        } else {
            char* vt = *(char**)obj;
            (*(void(**)(char*, char*))(vt+0x4c))(obj, c);
        }
    }
    *(int*)(c+0x360) = 0;
}
}
