/* The registry's class rows, one line per hosted actor class.
 *
 * Included by hal/actor_registry.cpp into the table body. Each row is
 *
 *     { actorID, "NAME", <ROM SpawnInfo>, <host factory>, <vtable fill> }
 *
 * and each needs three things landed together: the SpawnInfo in
 * port/ov002_syms.txt (or the ov009 whole mount), the class's matched src
 * closure in slice_gate16.txt, and a vtable fill in hal/actor_classes.cpp
 * written to the vtable law.
 *
 * The factory column is the class's own matched <Class>_Spawn, which the ROM
 * stores in the SpawnInfo's +0 word; registering the class overwrites that
 * word with this host address and leaves the rest of Nintendo's record alone.
 */
extern "C" {
/* ---- TREE (286, ov002) x21 ---- */
extern unsigned char Tree_SpawnInfo[];      /* ov002 0x0210abdc */
void *Tree_Spawn(void);                     /* ov002 0x020ec32c */
void hal_fill_tree_vtable(void);
/* ---- AMBIENT_SOUND_EFFECTS (350, ov002) x5 ----
   The class whose methods and vtable the config names EnemySwitchTag; see the
   header of that section in hal/actor_classes.cpp for how the ROM settles it. */
extern unsigned char AmbientSoundEffects_SpawnInfo[];   /* ov002 0x0210b47c */
void *AmbientSoundEffects_Spawn(void);                  /* ov002 0x020f1b94 */
void hal_fill_ambient_sound_vtable(void);
/* ---- BLACK_BRICK_BLOCK (17, ov002) x1 ---- */
extern unsigned char BlackBrickBlock_SpawnInfo[];       /* ov002 0x02108a78 */
void *BlackBrickBlock_Spawn(void);                      /* ov002 0x020b409c */
void hal_fill_black_brick_block_vtable(void);
/* ---- SIGN_POST (184, ov002) x5 ---- */
extern unsigned char SignPost_SpawnInfo[];              /* ov002 0x02109ad4 */
void *SignPost_Spawn(void);                             /* ov002 0x020bc3c8 */
void hal_fill_sign_post_vtable(void);
/* ---- ONE_UP_MUSHROOM (276, ov002) x3 ---- */
extern unsigned char OneUpMushroom_SpawnInfo[];         /* ov002 0x02108388 */
void *OneUpMushroom_Spawn(void);                        /* ov002 0x020b0580 */
void hal_fill_one_up_mushroom_vtable(void);
}

#define PORT_ACTOR_CLASS_ROWS                                                 \
    {286, "TREE", Tree_SpawnInfo, Tree_Spawn, hal_fill_tree_vtable},          \
    {350, "AMBIENT_SOUND", AmbientSoundEffects_SpawnInfo,                     \
     AmbientSoundEffects_Spawn, hal_fill_ambient_sound_vtable},               \
    {17, "BLACK_BRICK_BLOCK", BlackBrickBlock_SpawnInfo,                      \
     BlackBrickBlock_Spawn, hal_fill_black_brick_block_vtable},               \
    {184, "SIGN_POST", SignPost_SpawnInfo, SignPost_Spawn,                    \
     hal_fill_sign_post_vtable},                                              \
    {276, "ONE_UP_MUSHROOM", OneUpMushroom_SpawnInfo, OneUpMushroom_Spawn,    \
     hal_fill_one_up_mushroom_vtable},
