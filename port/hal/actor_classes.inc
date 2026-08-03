/* The registry's class rows, one line per hosted actor class.
 *
 * Included by hal/actor_registry.cpp into the table body. Each row is
 *
 *     { actorID, "NAME", <ROM SpawnInfo>, <host factory>, <vtable fill> }
 *
 * and each needs three things landed together: the SpawnInfo in
 * port/ov002_syms.txt (or the ov009 whole mount), the class's matched src
 * closure in its slice file, and a vtable fill in hal/actor_vtables.cpp
 * written to the vtable law.
 *
 * The list is EMPTY on purpose until a tier lands: the pre-spawn hook turns
 * every unregistered id away cleanly, so an empty registry is a level with
 * no objects rather than a crash.
 */
#define PORT_ACTOR_CLASS_ROWS
