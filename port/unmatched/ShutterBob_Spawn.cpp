/* HOST COPY of src/ShutterBob_Spawn.c -- the vtable-store ORDER the shared-
 * header recovery lost.
 *
 * The matched source spells the factory
 *
 *     int *p = (int *)_ZN9ActorBasenwEj(804);
 *     if (p) {
 *         _ZN8PlatformC2Ev(p);
 *         p[0] = (int)_ZTV14daObjBSwdoor_c;   // the derived table, ov014 0x2114608
 *         p[0] = (int)VT1;                     // = data_ov002_021099e4, the base
 *     }
 *
 * On the ROM this is mwcc's derived-constructor idiom: Platform::Platform
 * installs the base vtable (data_ov002_021099e4, whose +4 is Actor::Before-
 * InitResources -- the base dBgActor_c table), then the derived body installs
 * its OWN vtable, _ZTV14daObjBSwdoor_c, LAST. The recovery emitted the two
 * pooled vtable constants in POOL address order (0x21099e4 then 0x2114608)
 * rather than STORE order, so the C ends with the base table winning and the
 * object dispatches through a table with no ShutterBob::InitResources/Behavior
 * -- slot 0 is null and the spawn spine calls through it. Measured: the object
 * came up with vtable = the zeroed host VT1 placeholder and faulted before its
 * InitResources ran.
 *
 * This copy writes the derived table last and once, exactly what the ROM does.
 * It is otherwise line-for-line the source; the vtable is the host array the
 * registry fills (hal/actor_classes_bob_world.cpp, hal_fill_shutter_bob_vtable),
 * reached here by its class name _ZTV10ShutterBob (the RTTI alias
 * _ZTV14daObjBSwdoor_c points at the same array).
 */
extern "C" {

void *_ZN9ActorBasenwEj(unsigned size);
void _ZN8PlatformC2Ev(void *self);
extern int _ZTV10ShutterBob[];

void *ShutterBob_Spawn(void)
{
    int *p = (int *)_ZN9ActorBasenwEj(804);
    if (p) {
        _ZN8PlatformC2Ev(p);
        p[0] = (int)_ZTV10ShutterBob;
    }
    return p;
}

}  /* extern "C" */
