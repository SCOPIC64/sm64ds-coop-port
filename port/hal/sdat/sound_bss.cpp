// Storage the sound init needs, sized from the ROM.
//
// These four had no definition anywhere in the port because nothing reached
// them while sound was stubbed. Every size is the delta to the next symbol
// in config/arm9/symbols.txt, and each one is cross-checked against the code
// that walks it -- a too-small array here is silent corruption of whatever
// the linker happens to place next, not a crash, so the arithmetic is spelled
// out rather than rounded up.
extern "C" {

// The 16 voice records. func_0204fc40 runs i = 0..15 writing +0x2c and +0x3c
// and adding each to the free list, so the stride is 0x40 and 16 * 0x40 =
// 0x400; the symbol itself runs 0x020a50ec..0x020a552c = 0x440.
// The +0x3c byte it stores is i, and that is exactly the voice id every
// sound command carries in its first word.
unsigned char data_020a50ec[0x440];

// func_02048f34's three small voice-pool tables: 0x020a4bf0..0x020a4bf8 and
// 0x020a4c08..0x020a4c18, one and two 8-byte entries.
unsigned char data_020a4bf0[0x8];
unsigned char data_020a4c08[0x10];

// The SDAT root object func_02050f34 fills on hardware,
// 0x0209b4b4..0x0209b53c = 0x88 bytes: the 0x30 header, the FSFile at +0x30,
// and the three block pointers at +0x7c/+0x80/+0x84. hal/sdat/sdat.cpp
// builds its own equivalent and seats THAT in data_020a5bb8; this exists
// because func_02048f34 takes its address as the 3D sound owner.
unsigned char data_0209b4b4[0x88];

// The counters func_0204f1e8 and func_02050950 reset. Sizes are again the
// delta to the next symbol: three 4-byte callback slots on the voice side,
// and on the sound-thread side one word plus the 0x50-byte message queue
// data_020a5634 (only its head word is touched here -- the thread that would
// drain the rest is the one this port replaces).
int data_020a4d48, data_020a4d4c, data_020a4d50;
int data_020a55fc;
unsigned char data_020a5634[0x50];

}
