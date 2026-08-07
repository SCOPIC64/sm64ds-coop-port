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
/* ---- BLUE_COIN_SWITCH (10, ov002) ---- */
extern unsigned char BlueCoinSwitch_SpawnInfo[];        /* ov002 0x0210b248 */
void *BlueCoinSwitch_Spawn(void);                       /* ov002 0x020f15cc */
void hal_fill_blue_coin_switch_vtable(void);
/* ---- STAR_MARKER (180, ov002) ---- */
extern unsigned char StarMarker_SpawnInfo[];            /* ov002 0x0210aa78 */
void *StarMarker_Spawn(void);                           /* ov002 0x020ebe8c */
void hal_fill_star_marker_vtable(void);
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
/* ---- gate 40: ov100's STAR_DOOR (354) ----
   NOT the DOOR above. Config swapped the two doors' names: the _ZN4Door*
   family is really the STAR door (RTTI daStarGate_c), so it has its own six
   methods and its own vtable fill (hal_fill_star_door_vtable), distinct from
   the real door's hal_fill_door_vtable. Its SpawnInfo is already mounted by
   ov100_syms.txt at 0x021483a8; the record's +4 halfword reads 354. See the
   head of slice_gate40.txt and the gate-40 section of hal/actor_classes.cpp. */
extern unsigned char StarDoor_SpawnInfo[];              /* ov100 0x021483a8 */
void *StarDoor_Spawn(void);                             /* ov100 0x021461d4 */
void hal_fill_star_door_vtable(void);
/* ---- gate 41: ov010's TRAP (36) and LIGHT_BEAM (37) ----
   Level 2's own overlay, mounted per symbol (ov010_syms.txt). LIGHT_BEAM has
   no fill of its own: its factory installs _ZTV4Trap, which the TRAP row
   fills. Both records' +4 halfwords read 36 / 37. */
extern unsigned char Trap_SpawnInfo[];                  /* ov010 0x02112ac0 */
void *Trap_Spawn(void);                                 /* ov010 0x02111998 */
void hal_fill_trap_vtable(void);
extern unsigned char LightBeam_SpawnInfo[];             /* ov010 0x02112b84 */
void *LightBeam_Spawn(void);                            /* ov010 0x02111dd0 */
/* ---- gate 42: ov010's PEACH_PAINTING (38) ----
   The prior "blocked on ov052/RollingRock" reading was the shared-base illusion
   -- both mislabeled references stay inside ov010 (func_ov010_02111e84 and the
   ov010 BSS SharedFilePtr data_ov010_02112d64), closed by /alternatename in
   hal/actor_classes.cpp. Its SpawnInfo is mounted (ov010_syms.txt, +4 = 38). */
extern unsigned char PeachPainting_SpawnInfo[];        /* ov010 0x02112c44 */
void *PeachPainting_Spawn(void);                       /* ov010 0x02112004 */
void hal_fill_peach_painting_vtable(void);
/* ---- gate 50: ov080's PAINTING (daPicGate_c, actor 307) ----
   A DIFFERENT class from PEACH_PAINTING, in a different overlay, with no
   cross-overlay spawn dependency. Its record's +4 halfword reads 307 and the
   +0 word is Painting_Spawn (ov080 0x02126f8c), which the registry repoints to
   the host factory. Its vtable is the 31-slot Actor table dsd left unnamed at
   data_ov080_021282b4; hal/actor_classes_painting.cpp declares that as the
   host array Painting_Spawn installs, and hal_fill_painting_vtable fills it. */
extern unsigned char Painting_SpawnInfo[];             /* ov080 0x02128290 */
void *Painting_Spawn(void);                            /* ov080 0x02126f8c */
void hal_fill_painting_vtable(void);
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
/* ---- gate 33: BOB-OMB BATTLEFIELD's mechanisms, terrain and pickups -------
   Level 6, ov014. Every id here was read out of that level's own object
   tables and resolved through the EU ACTOR_SPAWN_TABLE at arm9 0x02090864;
   hal/actor_classes_bob_world.cpp carries the evidence and the vtable fills.
   Kept as one contiguous block so the merge with the enemy stream, which
   appends its own block, is a straight concatenation. */
extern unsigned char Coin_SpawnInfo[];                  /* ov002 0x02108790 */
extern unsigned char RedCoin_SpawnInfo[];               /* ov002 0x021087ac */
extern unsigned char BlueCoin_SpawnInfo[];              /* ov002 0x021087c8 */
void *Coin_Spawn(void);                                 /* ov002 0x020b2b48 */
void *RedCoin_Spawn(void);                              /* ov002 0x020b2af0 */
void *BlueCoin_Spawn(void);                             /* ov002 0x020b2a98 */
void hal_fill_coin_vtable(void);
/* BRICK_BLOCK has no fill of its own: its factory installs
   _ZTV13BigBrickBlock, which gate 16 already fills for BLACK_BRICK_BLOCK. */
extern unsigned char BrickBlock_SpawnInfo[];            /* ov002 0x02108a24 */
void *BrickBlock_Spawn(void);                           /* ov002 0x020b4708 */
/* gate 48: BIG_BRICK_BLOCK (16) and FORTRESS_TOWER_WALL (46), Whomp's Fortress.
   Both factories install _ZTV13BigBrickBlock too -- no fill of their own, same
   as BRICK_BLOCK. */
extern unsigned char BigBrickBlock_SpawnInfo[];         /* ov002 0x02108a40 */
void *BigBrickBlock_Spawn(void);                        /* ov002 0x020b40fc */
extern unsigned char FortressTowerWall_SpawnInfo[];     /* ov002 0x02108a94 */
void *FortressTowerWall_Spawn(void);                    /* ov002 0x020b406c */
extern unsigned char MegaMushroomBlockTag_SpawnInfo[];  /* ov002 0x02108ba0 */
void *MegaMushroomBlockTag_Spawn(void);
void hal_fill_mega_mushroom_block_tag_vtable(void);
/* gate 49: ONE_UP_MUSHROOM_BLOCK_TAG (321), the same _ZTV10BrickBlock tag body
   as MEGA_MUSHROOM_BLOCK_TAG -- no fill of its own. */
extern unsigned char OneUpMushroomBlockTag_SpawnInfo[]; /* ov002 0x02108bf4 */
void *OneUpMushroomBlockTag_Spawn(void);                /* ov002 0x020b4670 */
extern unsigned char CannonHatch_SpawnInfo[];           /* ov002 0x02109d14 */
void *CannonHatch_Spawn(void);
void hal_fill_cannon_hatch_vtable(void);
extern unsigned char Cap_SpawnInfo[];                   /* ov002 0x021095cc */
void *Cap_Spawn(void);
void hal_fill_cap_vtable(void);
extern unsigned char HealingHeart_SpawnInfo[];          /* ov002 0x02109c50 */
void *HealingHeart_Spawn(void);
void hal_fill_healing_heart_vtable(void);
extern unsigned char ExclamationSwitch_SpawnInfo[];     /* ov002 0x02109900 */
void *ExclamationSwitch_Spawn(void);
void hal_fill_exclamation_switch_vtable(void);
extern unsigned char InvisibleSecret_SpawnInfo[];       /* ov002 0x0210b00c */
void *InvisibleSecret_Spawn(void);
void hal_fill_invisible_secret_vtable(void);
/* gate 46: INVISIBLE_POLE (287), RTTI daBar_c. Host-storage vtable, filled by
   hand (the InvisibleSecret case). */
extern unsigned char InvisiblePole_SpawnInfo[];         /* ov002 0x0210845c */
void *InvisiblePole_Spawn(void);                        /* ov002 0x020b0710 */
void hal_fill_invisible_pole_vtable(void);
extern unsigned char ArrowSignLeft_SpawnInfo[];         /* ov098 0x0213c398 */
void *ArrowSignLeft_Spawn(void);
extern unsigned char ArrowSignRight_SpawnInfo[];        /* ov098 0x0213c3b4 */
void *ArrowSignRight_Spawn(void);
void hal_fill_arrow_sign_vtable(void);
extern unsigned char WaterBomb_SpawnInfo[];             /* ov098 0x0213c74c */
void *WaterBomb_Spawn(void);
void hal_fill_water_bomb_vtable(void);
extern unsigned char RollingIronBall_SpawnInfo[];       /* ov100 0x02147f58 */
void *RollingIronBall_Spawn(void);
void hal_fill_rolling_iron_ball_vtable(void);
extern unsigned char WarpPipe_SpawnInfo[];              /* ov102 0x0214e134 */
void *WarpPipe_Spawn(void);
void hal_fill_warp_pipe_vtable(void);
extern unsigned char Warp_SpawnInfo[];                  /* ov002 0x0210ac98 */
void *Warp_Spawn(void);                                 /* ov002 0x020ec534 */
void hal_fill_warp_vtable(void);
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
/* gate 82: SHUTTER_BOB (40), the same ov014 mount as the chain chomp. A
   Platform subclass (RTTI daObjBSwdoor_c); SpawnInfo and data_ov014_021145c4
   already come from ov014_syms.txt, so this is a HAL-only addition. */
extern unsigned char ShutterBob_SpawnInfo[];      /* ov014 0x021145e4 */
void *ShutterBob_Spawn(void);                     /* host copy, unmatched/ */
void hal_fill_shutter_bob_vtable(void);
/* gate 83: SEESAW_BOB (39), the fresh ov095 mount. A Platform subclass (RTTI
   daObjSeesaw_c); SpawnInfo and the three file tables come from ov095_syms.txt,
   the sinits from hal/actor_overlays.cpp. */
extern unsigned char SeesawBob_SpawnInfo[];       /* ov095 0x02137484 */
void *SeesawBob_Spawn(void);                      /* ov095 0x02135f94 */
void hal_fill_seesaw_bob_vtable(void);
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
extern unsigned char KoopaFlag_SpawnInfo[];       /* ov062 0x0211dc30 */
void *KoopaFlag_Spawn(void);                      /* ov062 0x0211b208 */
void hal_fill_koopa_flag_vtable(void);
/* ov078 is KING_BOB_OMB's whole overlay, and level 6 is the only level of the
   fifty-two that loads it. */
extern unsigned char KingBobOmb_SpawnInfo[];      /* ov078 0x02126e28 */
void *KingBobOmb_Spawn(void);                     /* ov078 0x021265fc */
void hal_fill_king_bob_omb_vtable(void);
/* ---- gates 60-63: Whomp's Fortress' mechanical platforms (ov015) ----------
   Wired by the vtable each factory installs, not the class name the config
   carries (the ov015 name shift; see hal/actor_classes_wf.cpp). SpawnInfo
   records from the ov015 per-symbol mount (ov015_syms.txt); each +4 halfword
   is the id below. */
extern unsigned char TowerStep_SpawnInfo[];       /* ov015 0x0211462c, id 52 */
void *TowerStep_Spawn(void);                      /* ov015 0x0211290c */
void hal_fill_tower_step_vtable(void);
extern unsigned char RotatingBridge_SpawnInfo[];  /* ov015 0x021146f0, id 51 */
void *RotatingBridge_Spawn(void);                 /* ov015 0x02112ba0 */
void hal_fill_rotating_bridge_vtable(void);
extern unsigned char PoleBillboard_SpawnInfo[];   /* ov015 0x0211433c, id 42 */
void *PoleBillboard_Spawn(void);                  /* ov015 0x021112dc */
void hal_fill_pole_billboard_vtable(void);
extern unsigned char KnockDownPlank_SpawnInfo[];  /* ov015 0x021143fc, id 44 */
void *KnockDownPlank_Spawn(void);                 /* ov015 0x02111b68 */
void hal_fill_knock_down_plank_vtable(void);
/* MOVING_BAR_BIG (53) and MOVING_BAR_SMALL (54) share vtable 0x0211458c (the
   _ZN14KnockDownPlank* methods); both factories install it. KnockDownPlank::
   Behavior forms its per-frame pointer-to-member over a FORWARD-DECLARED struct,
   so a host copy (port/unmatched/KnockDownPlank_Behavior.cpp) reads the table as
   plain ints; the same file seats both {fn, delta} tables. See the note in
   hal/actor_classes_wf.cpp. */
extern unsigned char MovingBarBig_SpawnInfo[];    /* ov015 0x02114568, id 53 */
void *MovingBarBig_Spawn(void);                   /* ov015 0x02112230 */
extern unsigned char MovingBarSmall_SpawnInfo[];  /* ov015 0x0211454c, id 54 */
void *MovingBarSmall_Spawn(void);                 /* ov015 0x02112260 */
void hal_fill_moving_bar_vtable(void);            /* shared vtable 0x0211458c */
extern unsigned char RotatingPlatformWf_SpawnInfo[]; /* ov015 0x021147c4, id 50 */
/* its factory is a host wrapper: the raw RotatingPlatformWf_Spawn stores the
   VT1 placeholder rather than a named vtable, so the wrapper reseats the vptr
   onto the host table (hal/actor_classes_wf.cpp). */
void *port_factory_rotating_platform_wf(void);
void hal_fill_rotating_platform_wf_vtable(void);
/* ---- gates 70-74: Whomp's Fortress' level-7 enemies and platforms ----------
   Two ov084 piranhas and three ov091 platform/Platform-enemy classes, wired by
   the vtable each factory installs (the ov015 name shift twice more; see
   hal/actor_classes_l7.cpp). ov084 and ov091 are already mounted (gate 32); the
   SpawnInfo records come from ov084_syms.txt / ov091_syms.txt and each +4
   halfword is the id below. */
/* gate 72: THWOMP (161). Its factory leaves the vptr on the VT1 placeholder, so
   its factory is a host wrapper that reseats it, the RotatingPlatformWf case. */
extern unsigned char Thwomp_SpawnInfo[];          /* ov091 0x02135150, id 161 */
void *port_factory_thwomp(void);
void hal_fill_thwomp_vtable(void);
/* gate 74: SLIDING_PLATFORM_WF (55). Clean vtable install, no wrapper. */
extern unsigned char SlidingPlatformWf_SpawnInfo[]; /* ov091 0x02135008, id 55 */
void *SlidingPlatformWf_Spawn(void);
void hal_fill_sliding_platform_wf_vtable(void);
/* gate 73: ROTATING_UP_DOWN_PLATFORM (29). The ov015 name shift: its factory
   installs the RotatingUpDownPlatformUtm table, so it runs Utm's methods. */
extern unsigned char RotatingUpDownPlatform_SpawnInfo[]; /* ov091 0x02134bf8, id 29 */
void *RotatingUpDownPlatform_Spawn(void);
void hal_fill_rotating_up_down_platform_vtable(void);
/* gate 71: FIRE_PIRANHA_PLANT (253). The ov015 name shift: its factory installs
   the FirePiranhaPlantBig table, so it runs that class. No state seat. */
extern unsigned char FirePiranhaPlant_SpawnInfo[];  /* ov084 0x02130acc, id 253 */
void *FirePiranhaPlant_Spawn(void);
void hal_fill_fire_piranha_plant_vtable(void);
/* gate 70: PIRANHA_PLANT (250). Clean vtable install; its nine-state PMF
   Behavior is a host copy (port/unmatched/PiranhaPlant_States.cpp). */
extern unsigned char PiranhaPlant_SpawnInfo[];      /* ov084 0x02130c04, id 250 */
void *PiranhaPlant_Spawn(void);
void hal_fill_piranha_plant_vtable(void);
/* FALL_BLOCK_WF (45), vtable 0x021148dc (_ZTV11FallBlockWf). The inherited
   ov098-chain blocker was a phantom: the Behavior func_ov098_0213a36c is a plain
   switch machine whose same-id walk is null-safe. Every slot is matched src; a
   plain 32-slot fill and a slice reach it. See hal/actor_classes_wf.cpp. */
extern unsigned char FallBlockWf_SpawnInfo[];     /* ov015 0x021148b8, id 45 */
void *FallBlockWf_Spawn(void);                    /* ov015 0x02112dd0 */
void hal_fill_fall_block_wf_vtable(void);
/* ---- gates 64-69: Whomp's Fortress' enemy cast (ov079) --------------------
   Six classes, ids read from the arm9 ACTOR_SPAWN_TABLE (the ROM's numbers, not
   the task's shorthand) and each SpawnInfo's +4 halfword. WHOMP/WHOMP_KING
   share _ZTV5Whomp and FORTRESS_WALL/FORTRESS_WALL_BREAKABLE share
   _ZTV12FortressWall, so only the first of each pair carries the vtable fill;
   the second row's factory installs the same table (fill 0). SpawnInfo records
   from the ov079 per-symbol mount (ov079_syms.txt). See
   hal/actor_classes_wf_enemy.cpp. */
extern unsigned char Whomp_SpawnInfo[];           /* ov079 0x02127c5c, id 164 */
void *Whomp_Spawn(void);                          /* ov079 0x021265e8 */
void hal_fill_whomp_vtable_c(void);
extern unsigned char WhompKing_SpawnInfo[];       /* ov079 0x02127c40, id 165 */
void *WhompKing_Spawn(void);                      /* ov079 0x02126588 */
extern unsigned char BulletBill_SpawnInfo[];      /* ov079 0x02127ec4, id 222 */
void *BulletBill_Spawn(void);                     /* ov079 0x02126d64 */
void hal_fill_bullet_bill_vtable_c(void);
extern unsigned char BillBlaster_SpawnInfo[];     /* ov079 0x02127f94, id 43 */
void *BillBlaster_Spawn(void);                    /* ov079 0x021271b4 */
void hal_fill_bill_blaster_vtable_c(void);
extern unsigned char FortressWall_SpawnInfo[];    /* ov079 0x0212808c, id 47 */
void *FortressWall_Spawn(void);                   /* ov079 0x0212757c */
void hal_fill_fortress_wall_vtable_c(void);
extern unsigned char FortressWallBreakable_SpawnInfo[]; /* ov079 0x02128070, id 48 */
void *FortressWallBreakable_Spawn(void);          /* ov079 0x021275ac */
/* ---- gate 89: POWER_STAR (178, ov002) -------------------------------------
   An Enemy subclass whose vtable _ZTV9PowerStar (ov002 0x0210ab3c, 31 slots,
   RTTI daStar_c) PowerStar_Spawn installs directly. SpawnInfo record from the
   ov002 per-symbol mount (added to ov002_syms.txt); the record's +4 halfword
   reads 178. Its Behavior dispatches a fourteen-entry pointer-to-member state
   table, hosted in port/unmatched/PowerStar_States.cpp. See
   hal/actor_classes_star.cpp. */
extern unsigned char PowerStar_SpawnInfo[];       /* ov002 0x0210aa40, id 178 */
void *PowerStar_Spawn(void);                      /* ov002 0x020ebf30 */
void hal_fill_power_star_vtable(void);
/* ---- gate 90: STAR_CAMERA (177, ov002) ------------------------------------
   A bare Actor: StarCamera_Spawn installs NO vtable of its own, so it dispatches
   the base Actor vtable _ZTV5Actor (data_0208e3a4). hal_fill_actor_base_vtable
   fills that shared table -- provably safe, it is the only class that leaves it
   installed. SpawnInfo from the ov002 mount; the record's +4 halfword reads
   177. See hal/actor_classes_star.cpp. */
extern unsigned char StarCamera_SpawnInfo[];      /* ov002 0x0210aa94, id 177 */
void *StarCamera_Spawn(void);                     /* ov002 0x020ebe5c */
void hal_fill_actor_base_vtable(void);
/* ---- gate 102: FORTRESS_TOWER (49, ov102) ---------------------------------
   Whomp's Fortress' tower, a Platform subclass (RTTI daObjSimpleBg_c). Its
   factory FortressTower_Spawn installs _ZTV13FortressTower, the SAME table
   WARP_PIPE (id 298) installs and hal_fill_warp_pipe_vtable already fills, so
   this row carries no fill of its own. All five methods are already compiled
   (gate 33's WARP_PIPE slice) and FortressTower_SpawnInfo is already mounted
   (ov102_syms.txt); the record's +4 halfword reads 49. See slice_gate102.txt
   and the WARP_PIPE section in hal/actor_classes_bob_world.cpp. */
extern unsigned char FortressTower_SpawnInfo[];   /* ov102 0x0214e118, id 49 */
void *FortressTower_Spawn(void);                  /* ov102 0x02148f2c */
/* ---- gate 103: STAR_SWITCH (12, ov002) ------------------------------------
   Whomp's Fortress' star switch, a Platform subclass (RTTI daObjSwitch_c). Its
   factory StarSwitch_Spawn installs _ZTV10StarSwitch, the SAME table
   EXCLAMATION_SWITCH (id 11) installs and hal_fill_exclamation_switch_vtable
   already fills (including the five-state PMF seat), so this row carries no
   fill of its own. All five methods are already compiled (gate 33's
   EXCLAMATION_SWITCH slice). This gate mounts StarSwitch_SpawnInfo (added to
   ov002_syms.txt); the record's +4 halfword reads 12. See slice_gate103.txt
   and the EXCLAMATION_SWITCH section in hal/actor_classes_bob_world.cpp. */
extern unsigned char StarSwitch_SpawnInfo[];      /* ov002 0x0210991c, id 12 */
void *StarSwitch_Spawn(void);                     /* ov002 0x020baaac */
}

#define PORT_ACTOR_CLASS_ROWS                                                 \
    {286, "TREE", Tree_SpawnInfo, Tree_Spawn, hal_fill_tree_vtable},          \
    {350, "AMBIENT_SOUND", AmbientSoundEffects_SpawnInfo,                     \
     AmbientSoundEffects_Spawn, hal_fill_ambient_sound_vtable},               \
    {17, "BLACK_BRICK_BLOCK", BlackBrickBlock_SpawnInfo,                      \
     BlackBrickBlock_Spawn, hal_fill_black_brick_block_vtable},               \
    /* ---- gate 78: BLUE_COIN_SWITCH (10), ov002 ---- */                      \
    {10, "BLUE_COIN_SWITCH", BlueCoinSwitch_SpawnInfo, BlueCoinSwitch_Spawn,   \
     hal_fill_blue_coin_switch_vtable},                                       \
    /* ---- gate 79: STAR_MARKER (180), ov002 ---- */                          \
    {180, "STAR_MARKER", StarMarker_SpawnInfo, StarMarker_Spawn,              \
     hal_fill_star_marker_vtable},                                            \
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
    /* ---- gate 33: Bob-omb Battlefield, one contiguous block ---- */         \
    {288, "COIN", Coin_SpawnInfo, Coin_Spawn, hal_fill_coin_vtable},           \
    {289, "RED_COIN", RedCoin_SpawnInfo, RedCoin_Spawn, hal_fill_coin_vtable}, \
    {290, "BLUE_COIN", BlueCoin_SpawnInfo, BlueCoin_Spawn,                     \
     hal_fill_coin_vtable},                                                    \
    {15, "BRICK_BLOCK", BrickBlock_SpawnInfo, BrickBlock_Spawn, 0},            \
    /* gate 48: two more of the same class body (installs _ZTV13BigBrickBlock,  \
       filled at gate 16), for Whomp's Fortress. No fill of their own, like     \
       BRICK_BLOCK. */                                                          \
    {16, "BIG_BRICK_BLOCK", BigBrickBlock_SpawnInfo, BigBrickBlock_Spawn, 0},   \
    {46, "FORTRESS_TOWER_WALL", FortressTowerWall_SpawnInfo,                    \
     FortressTowerWall_Spawn, 0},                                              \
    {322, "MEGA_MUSHROOM_BLOCK_TAG", MegaMushroomBlockTag_SpawnInfo,           \
     MegaMushroomBlockTag_Spawn, hal_fill_mega_mushroom_block_tag_vtable},     \
    /* gate 49: same tag body (installs _ZTV10BrickBlock, filled above), for   \
       Whomp's Fortress. No fill of its own. */                               \
    {321, "ONE_UP_MUSHROOM_BLOCK_TAG", OneUpMushroomBlockTag_SpawnInfo,        \
     OneUpMushroomBlockTag_Spawn, 0},                                         \
    {14, "CANNON_HATCH", CannonHatch_SpawnInfo, CannonHatch_Spawn,             \
     hal_fill_cannon_hatch_vtable},                                            \
    {297, "HEALING_HEART", HealingHeart_SpawnInfo, HealingHeart_Spawn,      \
     hal_fill_healing_heart_vtable},                                        \
    /* CAP (269), EXCLAMATION_SWITCH (11) and ROLLING_IRON_BALL (220) are     \
       registered at gate 51 (their PMF state tables are now seated). WATER_  \
       BOMB (208) is registered at gate 55 -- see the gate-55 note in         \
       actor_classes_bob_world.cpp. */                                        \
    {208, "WATER_BOMB", WaterBomb_SpawnInfo, WaterBomb_Spawn,                 \
     hal_fill_water_bomb_vtable},                                             \
    {329, "INVISIBLE_SECRET", InvisibleSecret_SpawnInfo, InvisibleSecret_Spawn,\
     hal_fill_invisible_secret_vtable},                                        \
    /* gate 46: the moving bar (RTTI daBar_c), Whomp's Fortress */             \
    {287, "INVISIBLE_POLE", InvisiblePole_SpawnInfo, InvisiblePole_Spawn,      \
     hal_fill_invisible_pole_vtable},                                          \
    {299, "ARROW_SIGN_LEFT", ArrowSignLeft_SpawnInfo, ArrowSignLeft_Spawn,     \
     hal_fill_arrow_sign_vtable},                                              \
    {300, "ARROW_SIGN_RIGHT", ArrowSignRight_SpawnInfo, ArrowSignRight_Spawn,  \
     hal_fill_arrow_sign_vtable},                                              \
    {347, "WARP", Warp_SpawnInfo, Warp_Spawn, hal_fill_warp_vtable},           \
    {298, "WARP_PIPE", WarpPipe_SpawnInfo, WarpPipe_Spawn,                     \
     hal_fill_warp_pipe_vtable},                                           \
    /* ---- gate 32, Bob-omb Battlefield's cast (append-only block) ---- */    \
    {206, "BOB_OMB", BobOmb_SpawnInfo, BobOmb_Spawn, hal_fill_bob_omb_vtable},  \
    {200, "GOOMBA", Goomba_SpawnInfo, Goomba_Spawn, hal_fill_goomba_vtable},    \
    {181, "BOB_OMB_BUDDY", BobOmbBuddy_SpawnInfo, BobOmbBuddy_Spawn,            \
     hal_fill_bob_omb_buddy_vtable},                                           \
    {219, "CHAIN_CHOMP", ChainChomp_SpawnInfo, ChainChomp_Spawn,               \
     hal_fill_chain_chomp_vtable},                                             \
    {41, "CHAIN_CHOMP_FENCE", ChainChompFence_SpawnInfo, ChainChompFence_Spawn,\
     hal_fill_chain_chomp_fence_vtable},                                       \
    {27, "STUMP", Stump_SpawnInfo, Stump_Spawn, hal_fill_stump_vtable},        \
    {188, "KOOPA_THE_QUICK", KoopaTheQuick_SpawnInfo, KoopaTheQuick_Spawn,     \
     hal_fill_koopa_the_quick_vtable},                                         \
    {205, "KOOPA_FLAG", KoopaFlag_SpawnInfo, KoopaFlag_Spawn,                  \
     hal_fill_koopa_flag_vtable},                                              \
    {189, "KING_BOB_OMB", KingBobOmb_SpawnInfo, KingBobOmb_Spawn,              \
     hal_fill_king_bob_omb_vtable},                                            \
    /* ---- gates 40-44: levels 1 and 2 (append-only block) ---- */            \
    {354, "STAR_DOOR", StarDoor_SpawnInfo, StarDoor_Spawn,                     \
     hal_fill_star_door_vtable},                                              \
    {36, "TRAP", Trap_SpawnInfo, Trap_Spawn, hal_fill_trap_vtable},            \
    /* LIGHT_BEAM has no fill of its own: its factory installs _ZTV4Trap,     \
       which the TRAP row above already fills. */                            \
    {37, "LIGHT_BEAM", LightBeam_SpawnInfo, LightBeam_Spawn, 0},               \
    /* ---- gate 50: ov080's PAINTING (append-only block) ---- */              \
    {307, "PAINTING", Painting_SpawnInfo, Painting_Spawn,                      \
     hal_fill_painting_vtable},                                               \
    /* ---- gate 51: CAP (269), the metal/vanish/wing cap on the ground ---- */\
    {269, "CAP", Cap_SpawnInfo, Cap_Spawn, hal_fill_cap_vtable},               \
    /* ---- gate 51: EXCLAMATION_SWITCH (11), the floor switch ---- */         \
    {11, "EXCLAMATION_SWITCH", ExclamationSwitch_SpawnInfo,                    \
     ExclamationSwitch_Spawn, hal_fill_exclamation_switch_vtable},             \
    /* ---- gate 51: ROLLING_IRON_BALL (220), Whomp's Fortress ---- */         \
    {220, "ROLLING_IRON_BALL", RollingIronBall_SpawnInfo, RollingIronBall_Spawn,\
     hal_fill_rolling_iron_ball_vtable},                                       \
    /* ---- gate 60: TOWER_STEP (52), Whomp's Fortress (ov015) ---- */         \
    {52, "TOWER_STEP", TowerStep_SpawnInfo, TowerStep_Spawn,                    \
     hal_fill_tower_step_vtable},                                              \
    /* ---- gate 61: ROTATING_BRIDGE (51), Whomp's Fortress (ov015) ---- */    \
    {51, "ROTATING_BRIDGE", RotatingBridge_SpawnInfo, RotatingBridge_Spawn,    \
     hal_fill_rotating_bridge_vtable},                                         \
    /* ---- gate 62: POLE_BILLBOARD (42), KNOCK_DOWN_PLANK (44) ---- */         \
    {42, "POLE_BILLBOARD", PoleBillboard_SpawnInfo, PoleBillboard_Spawn,       \
     hal_fill_pole_billboard_vtable},                                          \
    {44, "KNOCK_DOWN_PLANK", KnockDownPlank_SpawnInfo, KnockDownPlank_Spawn,   \
     hal_fill_knock_down_plank_vtable},                                        \
    /* ---- gate 63: ROTATING_PLATFORM_WF (50), Whomp's Fortress (ov015) ---- */\
    {50, "ROTATING_PLATFORM_WF", RotatingPlatformWf_SpawnInfo,                 \
     port_factory_rotating_platform_wf, hal_fill_rotating_platform_wf_vtable}, \
    /* ---- gate 76: MOVING_BAR_BIG (53), MOVING_BAR_SMALL (54) ---- */         \
    /* Both install vtable 0x0211458c (the _ZN14KnockDownPlank* methods) and    \
       run KnockDownPlank's lifecycle; the shared fill is idempotent so the     \
       second row re-runs it harmlessly. */                                    \
    {53, "MOVING_BAR_BIG", MovingBarBig_SpawnInfo, MovingBarBig_Spawn,         \
     hal_fill_moving_bar_vtable},                                              \
    {54, "MOVING_BAR_SMALL", MovingBarSmall_SpawnInfo, MovingBarSmall_Spawn,   \
     hal_fill_moving_bar_vtable},                                              \
    /* ---- gate 77: FALL_BLOCK_WF (45), Whomp's Fortress (ov015/ov098) ---- */ \
    {45, "FALL_BLOCK_WF", FallBlockWf_SpawnInfo, FallBlockWf_Spawn,            \
     hal_fill_fall_block_wf_vtable},                                          \
    /* ---- gate 42: ov010's PEACH_PAINTING (38) ---- */                       \
    {38, "PEACH_PAINTING", PeachPainting_SpawnInfo, PeachPainting_Spawn,       \
     hal_fill_peach_painting_vtable},                                          \
    /* ---- gates 70-74: Whomp's Fortress' level-7 cast (append-only) ---- */   \
    /* gate 72: THWOMP (161), ov091 */                                         \
    {161, "THWOMP", Thwomp_SpawnInfo, port_factory_thwomp,                     \
     hal_fill_thwomp_vtable},                                                  \
    /* gate 74: SLIDING_PLATFORM_WF (55), ov091 */                             \
    {55, "SLIDING_PLATFORM_WF", SlidingPlatformWf_SpawnInfo,                   \
     SlidingPlatformWf_Spawn, hal_fill_sliding_platform_wf_vtable},           \
    /* gate 73: ROTATING_UP_DOWN_PLATFORM (29), ov091 (Utm name shift) */      \
    {29, "ROTATING_UP_DOWN_PLATFORM", RotatingUpDownPlatform_SpawnInfo,        \
     RotatingUpDownPlatform_Spawn, hal_fill_rotating_up_down_platform_vtable}, \
    /* gate 71: FIRE_PIRANHA_PLANT (253), ov084 (FirePiranhaPlantBig shift) */ \
    {253, "FIRE_PIRANHA_PLANT", FirePiranhaPlant_SpawnInfo,                    \
     FirePiranhaPlant_Spawn, hal_fill_fire_piranha_plant_vtable},             \
    /* gate 70: PIRANHA_PLANT (250), ov084 */                                  \
    {250, "PIRANHA_PLANT", PiranhaPlant_SpawnInfo, PiranhaPlant_Spawn,         \
     hal_fill_piranha_plant_vtable}, \
    /* ---- gate 82: SHUTTER_BOB (40), Bob-omb Battlefield (ov014) ---- */ \
    {40, "SHUTTER_BOB", ShutterBob_SpawnInfo, ShutterBob_Spawn, \
     hal_fill_shutter_bob_vtable}, \
    /* ---- gate 83: SEESAW_BOB (39), Bob-omb Battlefield (ov095) ---- */ \
    {39, "SEESAW_BOB", SeesawBob_SpawnInfo, SeesawBob_Spawn, \
     hal_fill_seesaw_bob_vtable}, \
    /* ---- gate 89: POWER_STAR (178), ov002 ---- */ \
    {178, "POWER_STAR", PowerStar_SpawnInfo, PowerStar_Spawn, \
     hal_fill_power_star_vtable}, \
    /* ---- gate 90: STAR_CAMERA (177), ov002 (bare Actor, base vtable) ---- */ \
    {177, "STAR_CAMERA", StarCamera_SpawnInfo, StarCamera_Spawn, \
     hal_fill_actor_base_vtable}, \
    /* ---- gates 64-69: Whomp's Fortress' enemy cast (ov079), now live ---- */ \
    {164, "WHOMP", Whomp_SpawnInfo, Whomp_Spawn, hal_fill_whomp_vtable_c}, \
    {165, "WHOMP_KING", WhompKing_SpawnInfo, WhompKing_Spawn, 0}, \
    {222, "BULLET_BILL", BulletBill_SpawnInfo, BulletBill_Spawn, \
     hal_fill_bullet_bill_vtable_c}, \
    {43, "BILL_BLASTER", BillBlaster_SpawnInfo, BillBlaster_Spawn, \
     hal_fill_bill_blaster_vtable_c}, \
    {47, "FORTRESS_WALL", FortressWall_SpawnInfo, FortressWall_Spawn, \
     hal_fill_fortress_wall_vtable_c}, \
    {48, "FORTRESS_WALL_BREAKABLE", FortressWallBreakable_SpawnInfo, \
     FortressWallBreakable_Spawn, 0}, \
    /* ---- gate 102: FORTRESS_TOWER (49), Whomp's Fortress (ov102) ---- */ \
    /* Installs _ZTV13FortressTower, filled by the WARP_PIPE row above; no \
       fill of its own, the BIG_BRICK_BLOCK / LIGHT_BEAM shape. */ \
    {49, "FORTRESS_TOWER", FortressTower_SpawnInfo, FortressTower_Spawn, 0}, \
    /* ---- gate 103: STAR_SWITCH (12), Whomp's Fortress (ov002) ---- */ \
    /* Installs _ZTV10StarSwitch, filled by the EXCLAMATION_SWITCH row (with \
       its PMF state seat); no fill of its own. */ \
    {12, "STAR_SWITCH", StarSwitch_SpawnInfo, StarSwitch_Spawn, 0},
