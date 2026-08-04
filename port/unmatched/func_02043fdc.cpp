/* HOST COPIES of src/func_02043fdc.cpp and src/func_020441cc.cpp -- the two
 * processing-list walks, with the mwcc pointer-to-member-function they
 * dispatch through read as a plain function pointer.
 *
 * Each list head carries its callback as an eight-byte PMF pair that
 * __sinit_02075154 copies out of arm9's five statics at 0x02099f48..0x02099f70.
 * All five are NONVIRTUAL PMFs -- { function address, 0 } -- naming
 * func_020432e4, func_0204335c, func_02043880, func_0204322c and
 * func_02043288, the four Process wrappers plus the scene-tree housekeeping
 * pass. MSVC has no representation for an mwcc PMF, so the port seats the
 * host function in the first word and reads it back as a function pointer;
 * hal/actor_registry.cpp's port_actor_lists_seat is the sinit's other half.
 *
 * Control flow is the matched sources', unchanged. func_02043fdc reads the
 * successor BEFORE the callback runs (a Process that destroys the actor
 * unlinks the node under it) and publishes the node it is on in
 * data_020a4b68, which is what func_020440e8/func_02044104 clear when a
 * destructor takes the walk's own cursor out. func_020441cc walks the scene
 * tree instead, whose successor comes from func_0203b394.
 */
extern "C" {

typedef int (*PortListFn)(void *self);

void *func_0203b394(void *node);
extern int data_020a4b68[];        /* the walk's published cursor
                                      (storage: hal/player_bridges.cpp) */

void port_scene_canary(const char *where);
}
#include <cstdio>
#include <cstdlib>
extern "C" {
/* SM64DS_SCENE_CANARY=1: walk the scene tree and report the first node whose
   owner back-pointer is not node-0x14.
 *
 * Every ActorBase writes its own address into its SceneNode's owner slot and
 * never touches it again, so that one invariant is a cheap tripwire for a
 * stray write into the actor heap -- which is what an eighteen-slot class
 * filled like a twenty-slot one produces (see hal/sub_actors.cpp). The walk
 * that dereferences the owner is func_020441cc below, and it faults a long way
 * from whoever actually did the damage; this says which phase to look in.
 * Off by default, and free when it is off. */
extern int data_020a4b6c[];
void port_scene_canary(const char *where)
{
    static int on = -1;
    if (on < 0) on = std::getenv("SM64DS_SCENE_CANARY") != 0;
    if (!on) return;
    int *node = (int *)(size_t)data_020a4b6c[0];
    int n = 0;
    while (node != 0 && n++ < 4000) {
        char *o = (char *)(size_t)node[4];
        if ((char *)node != o + 0x14) {
            std::printf("[canary] %s: node=%p owner=%p (skew %d)\n", where,
                        (void *)node, (void *)o, (int)((char *)node - o));
            std::fflush(stdout);
            return;
        }
        node = (int *)func_0203b394(node);
    }
    std::printf("[canary] %s: clean (%d nodes)\n", where, n);
    std::fflush(stdout);
}

/* {head, tail, callback, 0}; node is {prev, next, owner, ...} */
void *func_02043fdc(void *listv)
{
    int *list = (int *)listv;
    PortListFn fn = (PortListFn)(size_t)list[2];
    int *node;
    if (fn == 0)
        return (void *)1;
    node = (int *)(size_t)list[0];
    while (node != 0) {
        int *next;
        data_020a4b68[0] = (int)(size_t)node;
        next = (int *)(size_t)node[1];
        fn((void *)(size_t)node[2]);
        node = next;
    }
    data_020a4b68[0] = 0;
    return (void *)1;
}

/* {head, callback, 0}; scene node is 0x14 bytes with the owner at +0x10 */
void *func_020441cc(void *listv)
{
    int *list = (int *)listv;
    PortListFn fn = (PortListFn)(size_t)list[1];
    int *node;
    if (fn == 0)
        return (void *)1;
    node = (int *)(size_t)list[0];
    while (node != 0) {
        int *next = (int *)func_0203b394(node);
        fn((void *)(size_t)node[4]);
        node = next;
    }
    return (void *)1;
}

}
