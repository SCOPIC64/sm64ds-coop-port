// GENERATED-ACCUMULATED zero storage for gate-10/11 BSS ring symbols.
// Grown by the link-sweep loop; sizes are generous defaults.
//
// These are hosted DS BSS globals -- mutable game state (message and cutscene
// mode flags, the Process cleanup list, the particle cursors, and much more) --
// so they are routed into the .dsstate section the save state captures. The
// DSSTATE_BEGIN/END markers set the default bss/data segment for every global
// between them; the link-sweep loop that grows this file emits new symbols
// inside the bracket, so anything it adds is captured with no further work. See
// hal/dsstate_seg.h.
#include "dsstate_seg.h"
DSSTATE_BEGIN
extern "C" {
/* MSG_GEN_TEXT_FUNCS is NOT zeroed storage: it is a 3-entry function-pointer
   table func_0201b7cc calls through on a 0xfe message control byte. Seated with
   real host addresses in hal/message_gen_text.cpp -- a zeroed array here was a
   null call (eip=0) on every multi-page line carrying an embedded-text escape. */
int VT1[8];
int data_0209b454[8];
int data_0209b490[8];
int data_0209b494[8];
int data_0209b49c[8];
int data_0209cee8[8];
int data_0209d454[8];
int data_0209d45c[8];
int data_0209d480[8];
int data_0209d484[8];
int data_0209d48c[8];
int data_0209d490[8];
int data_0209d4a0[8];
int data_0209d4a4[8];
int data_0209d4c8[8];
int data_0209d64c[8];
int data_0209d650[8];
int data_0209d654[8];
int data_0209d658[8];
int data_0209d65c[8];
int data_0209d660[8];
int data_0209d664[8];
int data_0209d668[8];
int data_0209d66c[8];
int data_0209d670[8];
int data_0209d678[8];
int data_0209d67c[8];
int data_0209d680[8];
int data_0209d684[8];
int data_0209d688[8];
int data_0209d68c[8];
int data_0209d690[8];
int data_0209d694[8];
int data_0209d698[8];
int data_0209d69c[8];
int data_0209d6a0[8];
int data_0209d6a4[8];
int data_0209d6a8[8];
int data_0209d6ac[8];
int data_0209d6b0[8];
int data_0209d6b4[8];
int data_0209d6b8[8];
int data_0209d6bc[8];
int data_0209d6c0[8];
int data_0209d6c4[8];
int data_0209d6c8[8];
int data_0209d6cc[8];
int data_0209d6d0[8];
int data_0209d6d4[8];
int data_0209d6d8[8];
int data_0209d6dc[8];
int data_0209d6e0[8];
int data_0209d6e4[8];
int data_0209d6f0[8];
int data_0209d6f4[8];
int data_0209d6fc[8];
int data_0209d700[8];
int data_0209d704[8];
int data_0209d708[8];
int data_0209d70c[8];
/* SIZED BY ROM SPAN, not by the generous default. The eight symbols marked
   this way are the ones whose delta to the next ROM symbol is bigger than the
   32-byte default -- they are buffers, and a ROM routine that writes the WHOLE
   object runs off the end of the host copy and into whatever the linker put
   next. func_0201b7cc opens the message box with
   MultiStore_Int(0, &data_0209d74c, 0xf00) and cleared 3808 bytes past the
   default straight through hal_wipes[0]'s vtable pointer, which HUD::Behavior
   then dispatched IsAtStart on. Same class as the undersized data_0209f4ae
   that strayed into MovingCylinderClsn's vtable (hal/actor_vtables.cpp).
   Re-audit with the delta-to-next-symbol rule after adding any symbol here. */
int data_0209d710[0x3c / 4];    /* ROM span 0x3c */
int data_0209d74c[0xf00 / 4];   /* ROM span 0xf00 -- the message text buffer */
int data_0209f204[8];
int data_0209f20c[8];
int data_0209f27c[8];
int data_0209f284[8];
int data_0209f294[8];
int data_0209f2c4[8];
/* the red-coin counter NumRedCoins reads. Sized against the symbol table
   rather than left at this file's generic int[8]: config puts data_0209f310
   at 0x0209f310, so the extent is 3 bytes, and 32 was 29 bytes of slack over
   the next four symbols. Gate 32 defined its own copy of this in
   hal/bob_enemy_bridges.cpp; the definition lives here, which is where the
   link sweep grows BSS. */
unsigned char data_0209f30d[4];
int data_0209f4a2[8];
int data_0209f4a4[8];
int data_0209f4a6[8];
/* Stage::CheckInput's own view of the pad records: the matched TU
   accesses the whole 0x18-stride Ctrl block through this ONE symbol
   while older TUs read the per-field splits above -- the harness
   copies fields out after each CheckInput call (see walk_window) */
int data_0209f498[24];
int data_0209f350[8];
/* the actor the player is CARRYING. Rabbit::Behavior parks itself here when
   it is caught and Minimap::Behavior reads it back; engine BSS either way. */
int data_0209f33c[8];
int data_0209fc5c[8];
int data_0209fc68[8];
int data_020a0d84[8];
int data_020a0d88[8];
int data_020a0db0[8];
int data_020a0de8[8];
int data_020a0de9[8];
int data_020a0deb[8];
int data_020a0e5a[8];
/* data_020a1052 moved to hal/camera_bridges.cpp: it is a field INSIDE the
   local comms record at data_020a1040, not storage of its own */
/* data_0209cab4 moved to hal/level_boot.cpp: it is the second symbol of
   the save block, which the entrance loader reads across */
int data_0209d6f8[8];
int data_0209e650[8];
int data_0209f37c[8];
int data_0209f40c[0x30 / 4];    /* ROM span 0x30 */
int data_0209f224[8];
int data_0209b274[8];
int data_0209b294[8];
/* data_0209f5bc (the installed fader) moved to hal/fader_wipes.cpp: the
   two FUN_0202xxxx wipe helpers deref it with no null check. */
int data_0209fc4c[8];
int data_020a0e58[8];
int data_0209b004[8];
int data_0209b138[0x138 / 4];   /* ROM span 0x138 */
int data_0209b270[8];
int data_0209b284[8];
int data_0209b2a4[0x40 / 4];    /* ROM span 0x40 */
int data_0209d4b4[8];
int data_0209f1f0[8];
int data_0209f24c[8];
int data_0209f268[8];
int data_0209f26c[8];
int data_0209f270[8];
/* data_0209f5e8 (the COLOR fader) moved to hal/fader_wipes.cpp at gate 31:
   Scene::SetFaders dispatches two of its virtuals, so zeroed storage is a
   null vptr and the first LoadLevel faults on it. */
int data_020a4d84[0x368 / 4];   /* ROM span 0x368 */
/* data_02099fb0 moved to romdata (gate 35): it is file-backed arm9 data with
   the value 4 in it, and it is the COUNT func_02048720 walks when it looks
   for a free 3D voice. At 0 that loop never runs, pick stays -1, and every
   type-9 sound effect in the game comes back "no positional voice free" --
   which Sound::Play answers with a silent return. */
int data_0209d4ac[8];
int data_020a4c64[8];
int data_020a4c70[0xc0 / 4];    /* ROM span 0xc0 */
/* tier-2 state wave: DeadHit/Hurt read this arm9 bss word */
int data_0209f28c[8];
/* tier-2 round 2: arm9 bss the death/hurt/quicksand ring reads */
int data_0209b4b0[8];
int data_0209f330[8];
int data_0209b470[8];
int data_0209b474[8];
/* ov100 (the message-box overlay) bss reached from St_Talk_Main; bss is
   zero at load, so host storage is the whole of it. */
int data_020a4bf8[8];
/* Sound::Play's pooled 3D voice slots, 8 bytes each. func_02048f34 clears
   SIX of them (0x020a4c18..0x020a4c48 = 0x30) and func_02048720 indexes the
   pool by data_02099fb0, so int[8] was two entries short. */
int data_020a4c18[0x30 / 4];
/* death states: KillPlayer's remaining-lives byte, and the pending-scene
   argument Scene::SetSceneToSpawn parks next to it */
int data_0209f2f4[8];
int data_0209f5b8[8];
/* gate 14: the sound-group bookkeeping the kuppa tail's Sound::
   LoadGroupAndSetBank reads (dead on the port's boot, live at link time) */
int data_0209b47c[8];
int data_0209b4a8[8];
/* gate 14 A2: the entrance step handlers' own bss, arm9 and ov002. bss is
   zero at load, so host storage is the whole of it. */
int data_0209f2bc[8];
int data_0209f2ac[8];
int data_0209f4f8[0xc0 / 4];   /* the per-level death table; ROM span 0xc0 */
int data_ov002_0210e14c[8];
int data_ov002_0210f350[8];
int data_ov002_0210f3b0[8];
/* data_ov089_02132880 was the same fiction; gate 22 mounts ov089 for
   real and it is the overlay's own bytes now. */
/* gate 16: the two processing-list globals the other three did not already
   need. data_020a4ba8 is the cleanup list (head, tail, callback pair);
   data_020a4b5c is the id hook func_0204302c calls after a Process tears an
   actor down, and null is what the ROM's boot leaves it at. */
int data_020a4ba8[8];
int data_020a4b5c[4];
/* gate 22: the DOOR's two. data_020a0ebc is the zero Vector3
   func_ov100_02145370 rotates the player offset around. kind:bss in config,
   so zero is what the boot leaves it at; ov089's own bss comes from the
   overlay mount. */
int data_020a0ebc[3];
/* gate 180: the QUESTION_BLOCK content body func_ov102_021492d4 reads it.
   kind:bss at 0x020a0edc, real span 8 bytes (to data_020a0ee4); zero is the
   boot value. */
int data_020a0edc[2];
/* gate 20: the EXIT's own scratch. func_ov002_020b0a0c stores the spawn
   record's entrance byte here on its way into LoadLevel; config/arm9 calls
   it kind:bss, so zero is what the boot leaves it at. */
unsigned char data_0209f2c0[4];
/* gate 18 (RABBIT_KEY): StartMinigameMenu's return-to-rec-room flag -- the
   caught chain's 8th-catch terminal writes it before the scene fade.
   kind:bss at 0x0209f298, span 4 (to data_0209f29c), zero is the boot value. */
unsigned char data_0209f298[4];
/* gate 27, the HUD: the star-count cache HUD::Render and RenderStarCount
   share, the red-coin counter, and the VS-mode 'results are up' flag. All
   kind:bss, so zero is the boot value. */
unsigned char data_0209f2d4[4];
unsigned char data_0209f30c[4];
unsigned char data_0209fc9c[4];
unsigned char data_0209f248[4];
/* gate 25: the bottom screen. The three SetSubBgyOffset scroll shadows and
   SetBg0Offset's pair (the 2D layer's own copy of the BGxHOFS/VOFS words),
   the owner-language byte GetOwnerLanguage returns, the per-slot camera-
   button state Stage::CheckCameraInput latches, and the ov002 byte it sets
   when the zoom button is pressed. All kind:bss, so zero is the boot value. */
int data_0209d468[4];
int data_0209d46c[4];
int data_0209d470[4];
int data_0209d47c[4];
int data_0209d494[4];
int data_0209d498[4];
unsigned char data_020a0f00[4];
unsigned short data_0209f368[8];
unsigned char data_ov002_02111180[4];
/* gate 26, the boot spine: Stage::LoadModel's last line parks the Stage's own
   ModelComponents pointer (Stage+0x874) here, and CopyTexPalFromLevelModel
   reads it back. kind:bss in config, so zero until LoadModel runs. */
int data_0209f320;
/* gate 31: StartFile's last global before the scene fade. kind:bss in config,
   so zero is the boot value; the port's boot never set it because nothing
   called StartFile.

   Gate 35 also wants this one, as the id of the star just collected, and
   declared it int[8] in its own branch. FOUR BYTES IS THE RIGHT SIZE: config
   puts data_0209f22c at 0x0209f22c, so the extent is 4, and 32 would have
   covered seven adjacent symbols. star_flow.cpp reads it as a single
   `extern unsigned char`, which this serves. Same shape as the data_020a0e68
   stomp: when two streams want one symbol, the symbol table decides, not
   whichever declaration is more generous. */
unsigned char data_0209f228[4];
/* gate 31: the two SetNumPlayers seats beside data_0209fc5c, which is already
   above -- the player count and the per-slot controller index. kind:bss. */
unsigned char data_0209fc50[4];
char data_0209fc64[4];
/* gate 31: the second word CleanCommonModelDataArr resets. Its two siblings
   (the count at 0x0209cef8 and the array at 0x0209cefc) already have storage
   in hal/model_host.cpp; this one had no reader until the level teardown
   called the ROM's own reset. kind:bss, so zero is the boot value. */
int data_0209cef0;
}
DSSTATE_END

/* Sound:: is a NAMESPACE in the TU that calls this one (YAX mangling) */
namespace Sound { void UnsetPlayerVoiceGroup(); }
void Sound::UnsetPlayerVoiceGroup() {}

DSSTATE_BEGIN
/* ---- gate 29: the particle engine's own BSS -------------------------------
   data_0209ee78/7c/80 are the three cursors of the arena SysTracker::
   Initialise carves out (base, end, current) and func_02023178 bump-allocates
   from. 84/88/8c are the VRAM bases it caches off func_02045ce0/cf0/d10.
   data_020a4d30 is the engine's scratch slot.

   LCG_STATE_0204da4c is the particle RNG's state -- func_0204da4c multiplies
   by 0x5eedf715 and adds 0x1b0cb173 three times to pick a random emission
   direction. The additive constant means a zero seed is a perfectly good
   seed, and starting at zero is what keeps a selftest frame reproducible. */
extern "C" {
int data_0209ee78[8];
int data_0209ee7c[8];
int data_0209ee80[8];
int data_0209ee84[8];
int data_0209ee88[8];
int data_0209ee8c[8];
int data_020a4d30[8];
int LCG_STATE_0204da4c;
}
DSSTATE_END

/* gate 50: ov080's PAINTING (daPicGate_c, 307) bss is NOT here -- it is
   mounted with the rest of the overlay in ov080_syms.txt so the DATA table
   data_ov080_0212775c's pointers into it get rebased to host addresses. See
   the header of port/ov080_syms.txt for why auto_bss would break the Shared
   FilePtr walk. */
