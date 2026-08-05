// GATE 32: the faces whose class IS in include/, in a TU that includes nothing
// else.
//
// The split is forced rather than tidy: hal/bob_enemy_bridges.cpp has already
// spelled shadow Actor, Enemy, Player, ModelBase and ShadowModel, so including
// include/Player.h there would redefine every one of them. See the note at the
// foot of that file.
#include "Player.h"

extern "C" {
/* Both are reached from ov084 by their Itanium C names while their definitions
   are real methods: Goomba's death branch asks whether the player is riding a
   shell, and its collect path registers the coin against the egg count. */
int _ZN6Player9IsOnShellEv(void *self)
{ return ((Player *)self)->Player::IsOnShell(); }
void _ZN6Player20RegisterEggCoinCountEjbb(void *self, unsigned n, int b2, int b3)
{ ((Player *)self)->Player::RegisterEggCoinCount(n, b2 != 0, b3 != 0); }
}

/* Model::HideMaterial (arm9 0x02016688), which KOOPA_THE_QUICK's Render calls
   through its own shadow while the definition is a real method. */
#include "Model.h"
extern "C" void _ZN5Model12HideMaterialEii(void *self, int boneID, int listIdx)
{ ((Model *)self)->Model::HideMaterial(boneID, listIdx); }
