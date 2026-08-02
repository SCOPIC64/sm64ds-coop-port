/* GENERATED-BY-HAND from config/arm9/overlays/ov002/symbols.txt:
 * St_LevelEnter stays UNHOSTED for now: its entrance-anim table reads junk
 * under the smoke's fake spawn context (real level boot stages it) --
 * the loud no-op default covers it until gate 11 boots a real level.
 * DS state-function address -> host method. Grows with the slice. */
    case 0x020d2404: return ((Player *)self)->Player::St_Wait_Main();
    case 0x020d32bc: return ((Player *)self)->Player::St_Wait_Init();
    case 0x020d38c0: return ((Player *)self)->Player::St_Walk_Main();
    case 0x020d4270: return ((Player *)self)->Player::St_Walk_Init();
