/* GENERATED from config/arm9/overlays/ov002/symbols.txt: DS state-fn
 * address -> host method, for every St_ file the slice hosts.
 * St_LevelEnter stays out: its entrance-anim table is junk under the
 * smoke's fake spawn context (real level boot will stage it). */
    case 0x020cc228: return ((Player *)self)->Player::St_Shell_Cleanup();
    case 0x020d2404: return ((Player *)self)->Player::St_Wait_Main();
    case 0x020d32bc: return ((Player *)self)->Player::St_Wait_Init();
    case 0x020d38c0: return ((Player *)self)->Player::St_Walk_Main();
    case 0x020d4270: return ((Player *)self)->Player::St_Walk_Init();
    case 0x020dcc28: return ((Player *)self)->Player::St_StomachSlide_Main();
    case 0x020dd104: return ((Player *)self)->Player::St_StomachSlide_Init();
    case 0x020e04ec: return ((Player *)self)->Player::St_Land_Main();
    case 0x020e088c: return ((Player *)self)->Player::St_Land_Init();
    case 0x020e2118: return ((Player *)self)->Player::St_Jump_Main();
    /* St_Jump_Init defines at C linkage in src (explicit char* this) */
    case 0x020e22c0: return _ZN6Player12St_Jump_InitEv((char *)self);
    /* Mario's airborne gravity fn, dispatched as a PMF from the
       per-character rows in data_ov002_0211073c (St_Jump_Main) */
    case 0x020e200c: func_ov002_020e200c((char *)self); return 1;
    case 0x020e1af8: return ((Player *)self)->Player::St_Fall_Main();
    case 0x020e1b80: return ((Player *)self)->Player::St_Fall_Init();
