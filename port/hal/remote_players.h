#pragma once

/* Remote-player puppets: one spawned Player actor per net peer, posed
   from POS lines, never behavior-ticked. The camera follows only the
   local (its target latched at boot), the tint is shared with the
   local's outfit, and at most 3 puppets show (player slots 1..3). */
namespace sm64ds::remote {

/* a POS line arrived: create/refresh the named puppet */
void net_pos(const char *name, int x, int y, int z, int yaw, int chr,
             int anim);
/* peer left (or timed out): park the slot, abandon the actor */
void drop(const char *name);
/* per-frame: send local POS (~15 Hz), advance puppet anims, reap stale
   ones, and clear everything when the role falls back to offline */
void send_tick(void *local_player, int role);
/* render all live puppets (call right after the local player) */
void draw(void);
/* overlap touch: name of the puppet within reach, 5 s cooldown, else 0 */
const char *bump(int x, int y, int z);
int count(void);

}  // namespace sm64ds::remote
