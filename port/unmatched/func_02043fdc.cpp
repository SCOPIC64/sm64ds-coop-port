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
