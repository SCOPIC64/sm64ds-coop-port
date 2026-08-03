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
}

#define PORT_ACTOR_CLASS_ROWS \
    {286, "TREE", Tree_SpawnInfo, Tree_Spawn, hal_fill_tree_vtable},
