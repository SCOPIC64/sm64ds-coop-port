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
    case 0x020cb15c: return ((Player *)self)->Player::St_Headstand_Main();
    case 0x020cb2f0: return ((Player *)self)->Player::St_Headstand_Init();
    case 0x020cf714: return ((Player *)self)->Player::St_OnWall_Main();
    case 0x020cfa90: return ((Player *)self)->Player::St_OnWall_Init();
    case 0x020d0ee0: return ((Player *)self)->Player::St_Crawl_Main();
    case 0x020d112c: return ((Player *)self)->Player::St_Crawl_Init();
    case 0x020d1354: return ((Player *)self)->Player::St_Crouch_Main();
    case 0x020d15e0: return ((Player *)self)->Player::St_Crouch_Init();
    case 0x020d4de8: return ((Player *)self)->Player::St_Bonk_Main();
    case 0x020d4f8c: return ((Player *)self)->Player::St_Bonk_Init();
    case 0x020dc1d0: return ((Player *)self)->Player::St_SlideKick_Main();
    case 0x020dc3e0: return ((Player *)self)->Player::St_SlideKick_Init();
    case 0x020dd3c4: return ((Player *)self)->Player::St_PunchKick_Main();
    case 0x020dd79c: return ((Player *)self)->Player::St_PunchKick_Init();
    case 0x020dd9e0: return ((Player *)self)->Player::St_GroundPound_Cleanup();
    case 0x020dd9f8: return ((Player *)self)->Player::St_GroundPound_Main();
    case 0x020dddf0: return ((Player *)self)->Player::St_GroundPound_Init();
    case 0x020e0aa8: return ((Player *)self)->Player::St_SmallLaunchUp_Main();
    case 0x020e0adc: return ((Player *)self)->Player::St_SmallLaunchUp_Init();
    case 0x020e0b50: return ((Player *)self)->Player::St_SlopeJump_Main();
    case 0x020e0c04: return ((Player *)self)->Player::St_SlopeJump_Init();
    case 0x020e1238: return ((Player *)self)->Player::St_LongJump_Main();
    case 0x020e127c: return ((Player *)self)->Player::St_LongJump_Init();
    case 0x020e13c0: return ((Player *)self)->Player::St_SideFlip_Main();
    case 0x020e1a10: return ((Player *)self)->Player::St_SideFlip_Init();
