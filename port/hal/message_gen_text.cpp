// MSG_GEN_TEXT_FUNCS, the message-box embedded-text dispatch table, host side.
//
// WHAT IT IS. func_0201b7cc (the message tokenizer) walks the glyph stream and,
// on a 0xfe control byte, reads an index and calls through a 3-entry
// function-pointer table at arm9 0x0208ee80:
//
//     Lfe:
//         u8 idx = cur[2];
//         data_0209d700 = idx;
//         (MSG_GEN_TEXT_FUNCS + idx)[-1]();   // MSG_GEN_TEXT_FUNCS[idx-1]()
//         data_0209d6f4 += cur[1];
//
// The three entries (config/arm9/relocs.txt, from:0x0208ee80/84/88):
//     [0] func_0201aca4  star / rabbit / bitcount decimal run  (uses NumStars,
//                        func_020138dc, SaveData::NumGlowingRabbitsFound, the
//                        digit-place table data_0208ee64 and glyph table ..74)
//     [1] func_0201ab6c  line-centering / tab bookkeeping       (no data table)
//     [2] func_0201adac  controller-button glyph pair           (glyph table ..6c)
//
// WHY IT LIVES HERE. The table had been zeroed auto_bss storage
// (MSG_GEN_TEXT_FUNCS[8] in hal/auto_bss.cpp), so a 0xfe escape called a null
// pointer and the process died at eip=0. Every REAL multi-page in-world line is
// id >= 0x2a and the common ones carry a 0xfe: the Bob-omb Buddy's msg 42
// ("...by pressing [FN]..."), the sign line 43, the star-count lines. So the
// crash blocked the whole multi-page display path, not one message.
//
// The three targets are matched src, joined to slice_gate10 beside func_0201b7cc
// (func_020138dc rides along as func_0201aca4's rabbit-bitcount dep). Their data
// tables data_0208ee64 / ..6c / ..74 are ROM constants, added to the romdata
// NAMED list (port/tools/romdata.py) so they come out as their real bytes --
// {100,10,1} place values, the button-glyph pairs and the 0..9 digit glyphs --
// rather than zeroed storage that would print all zeros.
//
// This is the same shape as the particle-vtable fix in hal/particle_vtable.cpp:
// a ROM function-pointer table that must hold real HOST addresses, never the DS
// addresses a romdata byte-emit would leave (those jump into unmapped memory).
// The three entries are __cdecl argless C functions here (their src is C,
// reached at their bare C names), so a plain function-pointer array is exact --
// no __thiscall/cdecl aliasing, no PMF widening.
extern "C" {

void func_0201aca4(void);   /* MSG_GEN_TEXT_FUNCS[0] (arm9 0x0201aca4) */
void func_0201ab6c(void);   /* MSG_GEN_TEXT_FUNCS[1] (arm9 0x0201ab6c) */
void func_0201adac(void);   /* MSG_GEN_TEXT_FUNCS[2] (arm9 0x0201adac) */

/* The table func_0201b7cc indexes. Three real host addresses, ROM order. */
void (*MSG_GEN_TEXT_FUNCS[3])(void) = {
    func_0201aca4,
    func_0201ab6c,
    func_0201adac,
};

}  /* extern "C" */
