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
/* Player::Unk_020ca150 (ov002 0x020ca150), the state change the CHAIN_CHOMP's
   lunge asks for when it catches the player. Same shape: an Itanium C-named
   reference onto a definition that is a real method. */
int _ZN6Player12Unk_020ca150Eh(void *self, unsigned char a)
{ return ((Player *)self)->Player::Unk_020ca150(a); }
/* Player::Unk_020c4f40 (ov002 0x020c4f40), the state KOOPA_THE_QUICK puts the
   player into when he starts the race. */
int _ZN6Player12Unk_020c4f40Et(void *self, unsigned short a)
{ return ((Player *)self)->Player::Unk_020c4f40(a); }
}

/* Actor::GetSubtraction (arm9 0x0200f8d4), the absolute angle difference two
   of the koopa's states run. Its definition is a real method; his own TUs
   spell it by the Itanium C name. */
#include "Actor.h"
extern "C" int _ZN5Actor14GetSubtractionEss(void *self, short a, short b)
{ return ((Actor *)self)->Actor::GetSubtraction(a, b); }

/* Model::HideMaterial (arm9 0x02016a58), which KOOPA_THE_QUICK's Render calls
   through its own shadow while the definition is a real method. */
#include "Model.h"
extern "C" void _ZN5Model12HideMaterialEii(void *self, int boneID, int listIdx)
{ ((Model *)self)->Model::HideMaterial(boneID, listIdx); }
