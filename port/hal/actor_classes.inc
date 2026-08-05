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
/* ---- gate 17: the level overlay's own four ----
   The SpawnInfo records come from the PER-SYMBOL ov009 mount (ov009_syms.txt),
   not the whole one: the registry rewrites the factory word in place and the
   ActorBase constructor reads the two priorities back out of the same record,
   so it has to be storage the actor code and the registry agree on. */
extern unsigned char Bird_SpawnInfo[];                  /* ov009 0x02113934 */
void *Bird_Spawn(void);                                 /* ov009 0x02111a30 */
void hal_fill_bird_vtable(void);
extern unsigned char CastleWater_SpawnInfo[];           /* ov009 0x021139f4 */
void *CastleWater_Spawn(void);                          /* ov009 0x02111d8c */
void hal_fill_castle_water_vtable(void);
extern unsigned char MetalNet_SpawnInfo[];              /* ov009 0x02113abc */
void *MetalNet_Spawn(void);                             /* ov009 0x02112048 */
void hal_fill_metal_net_vtable(void);
extern unsigned char Flag_SpawnInfo[];                  /* ov009 0x02113b7c */
void *Flag_Spawn(void);                                 /* ov009 0x021121f0 */
void hal_fill_flag_vtable(void);
/* ---- gate 18: ov085, the first ACTOR overlay ----
   Not the engine overlay and not the level's -- six classes several levels
   share, of which the castle grounds names two. The SpawnInfo records come
   from the per-symbol ov085 mount (ov085_syms.txt). */
extern unsigned char Rabbit_SpawnInfo[];                /* ov085 0x021300d4 */
void *Rabbit_Spawn(void);                               /* ov085 0x0212cc2c */
void hal_fill_rabbit_vtable(void);
extern unsigned char LakituBro_SpawnInfo[];             /* ov085 0x02130320 */
void *LakituBro_Spawn(void);                            /* ov085 0x0212ed54 */
void hal_fill_lakitu_bro_vtable(void);
/* ---- gate 19: ov098's CANNON ---- */
extern unsigned char Cannon_SpawnInfo[];                /* ov098 0x0213c67c */
void *Cannon_Spawn(void);                               /* ov098 0x0213b43c */
void hal_fill_cannon_vtable(void);
/* ---- gate 20: ov002's last two ----
   Both SpawnInfo records come from the ov002 per-symbol mount that has
   carried Player/Tree/SignPost since gate 14. The class-method names are the
   neighbour's in both cases and the *_SpawnInfo names are the right ones --
   the ROM's RTTI settles it, see slice_gate20.txt. */
extern unsigned char Exit_SpawnInfo[];                  /* ov002 0x02108690 */
void *Exit_Spawn(void);                                 /* ov002 0x020b0f24 */
void hal_fill_exit_vtable(void);
extern unsigned char WaterfallMist_SpawnInfo[];         /* ov002 0x0210947c */
void *WaterfallMist_Spawn(void);                        /* ov002 0x020b6ee8 */
void hal_fill_waterfall_mist_vtable(void);
/* ---- gate 21: ov100's BUTTERFLY and FISH ----
   The third actor overlay (ov100_syms.txt). Both class names check out
   against the ROM's own RTTI; the one shifted pair in ov100 is Door/StarDoor
   and that is gate 22's problem. */
extern unsigned char Butterfly_SpawnInfo[];             /* ov100 0x02147e78 */
void *Butterfly_Spawn(void);                            /* ov100 0x02141ea4 */
void hal_fill_butterfly_vtable(void);
extern unsigned char Fish_SpawnInfo[];                  /* ov100 0x02148498 */
void *Fish_Spawn(void);                                 /* ov100 0x02146d38 */
void hal_fill_fish_vtable(void);
/* ---- gate 22: ov100's DOOR ----
   The last class the castle grounds names. Its vtable is the one config left
   as data_ov100_02148188 while giving `_ZTV4Door` to the star door; the ROM's
   RTTI settles it, see slice_gate22.txt. */
extern unsigned char Door_SpawnInfo[];                  /* ov100 0x02148164 */
void *Door_Spawn(void);                                 /* ov100 0x0214589c */
void hal_fill_door_vtable(void);
/* ---- gate 23: ov102's QUESTION_BLOCK ----
   The fifth overlay, and the last id on the castle grounds' skip list that is
   not a 2D system. Its name checks out against the ROM's RTTI
   (daObjHatenaBlock_c) and its SpawnInfo's own +4 halfword. */
extern unsigned char QuestionBlock_SpawnInfo[];         /* ov102 0x0214e3e8 */
void *QuestionBlock_Spawn(void);                        /* ov102 0x0214a8ec */
void hal_fill_question_block_vtable(void);
/* ---- gate 27: the BOTTOM SCREEN'S OWN ------------------------------------
   HUD (334), one of the two ids the castle-grounds census has been printing
   as skipped. Its SpawnInfo is ov002 0x0210c210, which config names
   _ZN3HUD9spawnDataE, and the ROM puts the CONSTRUCTOR in the record's +0
   word -- there is no separate Spawn function to repoint. Its vtable is
   eighteen slots, not twenty; see the table read out of the ROM at the head
   of hal/sub_actors.cpp. */
extern unsigned char _ZN3HUD9spawnDataE[];      /* ov002 0x0210c210 */
int *_ZN3HUDC1Ev(void);                         /* ov002 0x020fe154 */
void hal_fill_hud_vtable(void);
/* ---- gate 28: the MINIMAP -------------------------------------------------
   The other id (335) the census printed as skipped, and the same shape as the
   HUD: SpawnInfo at ov002 0x0210c160 with the CONSTRUCTOR in the +0 word, and
   an eighteen-slot vtable. Its object is 600 bytes rather than the HUD's 124.
   It is also the actor that turns BG3-sub on -- InitResources ORs bit 3 into
   the layer mask hal/sub_screen.cpp publishes every frame. */
extern unsigned char _ZN7Minimap9spawnDataE[];  /* ov002 0x0210c160 */
int *_ZN7MinimapC1Ev(void);                     /* ov002 0x020fb8bc */
void hal_fill_minimap_vtable(void);
/* ---- gate 32: BOB-OMB BATTLEFIELD'S LIVING CAST ---------------------------
   The first ENEMY-family classes the port hosts, which is why they are in a
   file of their own (hal/actor_classes_bob_enemy.cpp): their vtables are
   THIRTY-ONE slots where every class above is twenty, because Actor appends
   thirteen virtuals of its own past ActorBase's eighteen.
   The ids are the ROM's own, cross-checked three ways: symbols/overlay_actors.md
   (built from ACTOR_SPAWN_TABLE at arm9 0x02090864), each SpawnInfo's +4
   halfword, and each class's RTTI name. */
extern unsigned char BobOmb_SpawnInfo[];        /* ov102 0x0214e534 */
void *BobOmb_Spawn(void);                       /* ov102 0x0214c6f8 */
void hal_fill_bob_omb_vtable(void);
extern unsigned char Goomba_SpawnInfo[];        /* ov084 0x021308ec */
void *Goomba_Spawn(void);                       /* ov084 0x0212c0b0 */
void hal_fill_goomba_vtable(void);
extern unsigned char BobOmbBuddy_SpawnInfo[];   /* ov084 0x02130a14 */
void *BobOmbBuddy_Spawn(void);                  /* ov084 0x0212d200 */
void hal_fill_bob_omb_buddy_vtable(void);
/* ov014 is Bob-omb Battlefield's OWN level overlay, mounted twice (whole for
   the loaders, per symbol for these two classes' tables) -- the gate-17 shape
   applied to a second level. */
extern unsigned char ChainChomp_SpawnInfo[];      /* ov014 0x02114750 */
void *ChainChomp_Spawn(void);                     /* ov014 0x02112d1c */
void hal_fill_chain_chomp_vtable(void);
extern unsigned char ChainChompFence_SpawnInfo[]; /* ov014 0x0211488c */
void *ChainChompFence_Spawn(void);                /* ov014 0x0211307c */
void hal_fill_chain_chomp_fence_vtable(void);
extern unsigned char KoopaTheQuick_SpawnInfo[];   /* ov062 0x0211db78 */
void *KoopaTheQuick_Spawn(void);                  /* ov062 0x0211aee0 */
void hal_fill_koopa_the_quick_vtable(void);
/* Actor 27, the post the chomp is chained to. Its CLASS is daObjPile_c, not
   the Stump the config's _ZTV5Stump names -- see the section header in
   hal/actor_classes_bob_enemy.cpp. The SpawnInfo and factory names are the
   right ones: that record's +4 halfword reads 27. */
extern unsigned char Stump_SpawnInfo[];           /* ov091 0x02135298 */
void *Stump_Spawn(void);                          /* ov091 0x02133938 */
void hal_fill_stump_vtable(void);
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
     hal_fill_one_up_mushroom_vtable},                                        \
    {338, "CASTLE_WATER", CastleWater_SpawnInfo, CastleWater_Spawn,           \
     hal_fill_castle_water_vtable},                                           \
    {339, "METAL_NET", MetalNet_SpawnInfo, MetalNet_Spawn,                    \
     hal_fill_metal_net_vtable},                                              \
    {342, "FLAG", Flag_SpawnInfo, Flag_Spawn, hal_fill_flag_vtable},          \
    {343, "BIRD", Bird_SpawnInfo, Bird_Spawn, hal_fill_bird_vtable},          \
    {187, "RABBIT", Rabbit_SpawnInfo, Rabbit_Spawn, hal_fill_rabbit_vtable},  \
    {235, "LAKITU_BRO", LakituBro_SpawnInfo, LakituBro_Spawn,                 \
     hal_fill_lakitu_bro_vtable},                                             \
    {207, "CANNON", Cannon_SpawnInfo, Cannon_Spawn, hal_fill_cannon_vtable}, \
    {349, "EXIT", Exit_SpawnInfo, Exit_Spawn, hal_fill_exit_vtable},         \
    {197, "WATERFALL_MIST", WaterfallMist_SpawnInfo, WaterfallMist_Spawn,    \
     hal_fill_waterfall_mist_vtable},                                        \
    {336, "BUTTERFLY", Butterfly_SpawnInfo, Butterfly_Spawn,                 \
     hal_fill_butterfly_vtable},                                             \
    {344, "FISH", Fish_SpawnInfo, Fish_Spawn, hal_fill_fish_vtable},             {353, "DOOR", Door_SpawnInfo, Door_Spawn, hal_fill_door_vtable},             {20, "QUESTION_BLOCK", QuestionBlock_SpawnInfo, QuestionBlock_Spawn,          hal_fill_question_block_vtable},         \
    {334, "HUD", _ZN3HUD9spawnDataE, port_factory_hud, hal_fill_hud_vtable},   \
    {335, "MINIMAP", _ZN7Minimap9spawnDataE, port_factory_minimap,             \
     hal_fill_minimap_vtable},                                                 \
    /* ---- gate 32, Bob-omb Battlefield's cast (append-only block) ---- */    \
    {206, "BOB_OMB", BobOmb_SpawnInfo, BobOmb_Spawn, hal_fill_bob_omb_vtable},  \
    {200, "GOOMBA", Goomba_SpawnInfo, Goomba_Spawn, hal_fill_goomba_vtable},    \
    {181, "BOB_OMB_BUDDY", BobOmbBuddy_SpawnInfo, BobOmbBuddy_Spawn,            \
     hal_fill_bob_omb_buddy_vtable},                                           \
    {219, "CHAIN_CHOMP", ChainChomp_SpawnInfo, ChainChomp_Spawn,               \
     hal_fill_chain_chomp_vtable},                                             \
    {41, "CHAIN_CHOMP_FENCE", ChainChompFence_SpawnInfo, ChainChompFence_Spawn,\
     hal_fill_chain_chomp_fence_vtable},                                       \
    {27, "STUMP", Stump_SpawnInfo, Stump_Spawn, hal_fill_stump_vtable},
