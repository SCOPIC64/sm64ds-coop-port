/* spawn_aliases.cpp — thin C-linkage forwards from the old *_Spawn names
   (referenced by actor_classes.inc) to the upstream classInit functions.

   The upstream merge renamed every actor TU's factory from <Class>_Spawn
   to <Class>_classInit.  The registry still references the old names, so
   we provide these aliases here. */

extern "C" {

/* ---- gate 14 / ov002 classes ---- */
void *daTree_c_classInit();
void *Tree_Spawn(void) { return daTree_c_classInit(); }

void *daObjYajirusi_c_classInit_YAJIRUSI_R(void);
void *SignPost_Spawn(void) { return daObjYajirusi_c_classInit_YAJIRUSI_R(); }

void *da1up_c_classInit_ONEUPKINOKO();
void *OneUpMushroom_Spawn(void) { return da1up_c_classInit_ONEUPKINOKO(); }

/* BlackBrickBlock is daObjBlockS_c (Crate) in upstream */
void *daObjBlockS_c_classInit(void);
void *BlackBrickBlock_Spawn(void) { return daObjBlockS_c_classInit(); }

/* ---- gate 17 / ov009 classes ---- */
void *daSBird_c_classInit();
void *Bird_Spawn(void) { return daSBird_c_classInit(); }

void *daObjMcWater_c_classInit();
void *CastleWater_Spawn(void) { return daObjMcWater_c_classInit(); }

/* DockPole maps to daMcFlag_c in the ROM layout */
void *daMcFlag_c_classInit();
void *DockPole_Spawn(void) { return daMcFlag_c_classInit(); }

/* Flag also maps to daMcFlag_c (different vtable fill in actor_classes.cpp) */
void *Flag_Spawn(void) { return daMcFlag_c_classInit(); }

/* ---- gate 18 / ov085 classes ---- */
void *daMip_c_classInit(void);
void *Rabbit_Spawn(void) { return daMip_c_classInit(); }

void *daJgm_c_classInit();
void *LakituBro_Spawn(void) { return daJgm_c_classInit(); }

/* ---- gate 19 / ov098 CANNON ---- */
void *daCnn_c_classInit();
void *Cannon_Spawn(void) { return daCnn_c_classInit(); }

/* ---- gate 20 / ov002 last two ---- */
void *daChScene_c_classInit();
void *Exit_Spawn(void) { return daChScene_c_classInit(); }

void *daObjWaterfall_c_classInit();
void *WaterfallMist_Spawn(void) { return daObjWaterfall_c_classInit(); }

/* ---- gate 21 / ov100 BUTTERFLY + FISH ---- */
void *daBtfly_c_classInit(void);
void *Butterfly_Spawn(void) { return daBtfly_c_classInit(); }

void *daFish_c_classInit();
void *Fish_Spawn(void) { return daFish_c_classInit(); }

/* ---- gate 22 / ov100 DOOR ---- */
void *daChRoom_c_classInit();
void *Door_Spawn(void) { return daChRoom_c_classInit(); }

/* ---- gate 23 / ov102 QUESTION_BLOCK ---- */
void *daObjHatenaBlock_c_classInit_HATENA_BLOCK();
void *QuestionBlock_Spawn(void) { return daObjHatenaBlock_c_classInit_HATENA_BLOCK(); }

}  /* extern "C" */
