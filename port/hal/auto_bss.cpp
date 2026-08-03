// GENERATED-ACCUMULATED zero storage for gate-10/11 BSS ring symbols.
// Grown by the link-sweep loop; sizes are generous defaults.
extern "C" {
int MSG_GEN_TEXT_FUNCS[8];
int VT1[8];
int data_0209b0d8[8];
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
int data_0209ee90[8];
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
int data_0209b078[8];
int data_0209fc5c[8];
int data_0209fc68[8];
int data_020a0d84[8];
int data_020a0d88[8];
int data_020a0db0[8];
int data_020a0de8[8];
int data_020a0de9[8];
int data_020a0deb[8];
int data_020a0e5a[8];
int data_020a1052[8];
int data_020a1164[8];
int data_020a1166[8];
int data_0209cab4[8];
int data_0209d6f8[8];
int data_0209e650[8];
int data_0209f37c[8];
int data_0209f40c[8];
int data_0209b0a8[8];
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
int data_0209b088[4];   /* Camera State object the slide path enters */
/* tier-2 state wave: DeadHit/Hurt read this arm9 bss word */
int data_0209f28c[8];
/* tier-2 round 2: arm9 bss the death/hurt/quicksand ring reads */
int data_0209b018[8];
int data_0209b038[8];
int data_0209b048[8];
int data_0209b058[8];
int data_0209b068[8];
int data_0209b098[8];
int data_0209b108[8];
int data_0209b128[8];
int data_0209b4b0[8];
int data_0209f330[8];
int data_0209b028[8];
int data_0209b470[8];
int data_0209b474[8];
int data_0209f354[8];
int data_0209f43c[8];
/* ov100 (the message-box overlay) bss reached from St_Talk_Main; bss is
   zero at load, so host storage is the whole of it. */
int data_ov100_02148708[8];
int data_0209f314[8];
int data_020a4bf8[8];
int data_020a4c18[8];
/* death states: KillPlayer's remaining-lives byte, and the pending-scene
   argument Scene::SetSceneToSpawn parks next to it */
int data_0209f2f4[8];
int data_0209f5b8[8];
}

/* Sound:: is a NAMESPACE in the TU that calls this one (YAX mangling) */
namespace Sound { void UnsetPlayerVoiceGroup(); }
void Sound::UnsetPlayerVoiceGroup() {}
