#include "Player.h"
#include "Model.h"
#include "Heap.h"

// These are storage/call prerequisites, not a claim of complete ABI parity.
static_assert(sizeof(void *) == 4, "DS code stores native pointers in 32-bit words");
static_assert(sizeof(Player) == 0x768, "Player must match DS object storage");
static_assert(sizeof(Player::State) == 0x18, "State table entries must stay 24 bytes");

void sm64ds_native_abi_probe() {}
