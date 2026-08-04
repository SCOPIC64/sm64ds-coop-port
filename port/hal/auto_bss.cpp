// GENERATED-ACCUMULATED zero storage for gate-10/11 BSS ring symbols.
// Grown by the link-sweep loop; sizes are generous defaults.
extern "C" {
int MSG_GEN_TEXT_FUNCS[8];
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
int data_0209d710[8];
int data_0209d74c[8];
int data_0209f204[8];
int data_0209f20c[8];
int data_0209f27c[8];
int data_0209f284[8];
int data_0209f294[8];
int data_0209f2c4[8];
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
int data_0209f40c[8];
int data_0209f224[8];
int data_0209b274[8];
int data_0209b294[8];
/* data_0209f5bc (the installed fader) moved to hal/fader_wipes.cpp: the
   two FUN_0202xxxx wipe helpers deref it with no null check. */
int data_0209fc4c[8];
int data_020a0e58[8];
int data_0209b004[8];
int data_0209b138[8];
int data_0209b270[8];
int data_0209b284[8];
int data_0209b2a4[8];
int data_0209d4b4[8];
int data_0209f1f0[8];
int data_0209f24c[8];
int data_0209f268[8];
int data_0209f26c[8];
int data_0209f270[8];
int data_0209f5e8[8];
int data_020a4d84[8];
int data_02099fb0[8];
int data_0209d4ac[8];
int data_020a4c64[8];
int data_020a4c70[8];
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
int data_020a4c18[8];
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
int data_0209f4f8[16];   /* the per-level death table */
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
/* gate 20: the EXIT's own scratch. func_ov002_020b0a0c stores the spawn
   record's entrance byte here on its way into LoadLevel; config/arm9 calls
   it kind:bss, so zero is what the boot leaves it at. */
unsigned char data_0209f2c0[4];
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
}

/* Sound:: is a NAMESPACE in the TU that calls this one (YAX mangling) */
namespace Sound { void UnsetPlayerVoiceGroup(); }
void Sound::UnsetPlayerVoiceGroup() {}
