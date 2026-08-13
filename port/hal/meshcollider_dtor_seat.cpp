// BATCH-3 LINKAGE SEAT: MeshCollider's own ROM destructor pair, seated for the
// walk_window family only.
//
// The ROM has the Itanium destructor pair: D1 (complete-object, arm9 0x02039864)
// and D0 (deleting, arm9 0x0203982c), both 2004/b56 byte-matches
// (src/_ZN12MeshColliderD1Ev.c and _ZN12MeshColliderD0Ev.c). D0 tears down the
// KCL octree object, calls func_02039658 to restore the base vptr, then frees
// via Memory::operator_delete2; D1 is the same without the free, the body every
// derived collider dtor calls for its base subobject.
//
// WHY A SEPARATE FILE, NOT slice_gate8. The concrete MeshCollider table lives in
// hal/clsn_vtable.cpp, which EVERY collision target links -- including the
// gate-8/9 smoke runners (smoke_clsn, smoke_actor) that carry no level teardown.
// D0's body drags func_02038224, func_02039658 and Memory::operator_delete2 (in
// turn _ZdlPv), none of which those minimal targets link, and none of which they
// need: they construct the level collider and exit, they never delete it. So the
// static slot 0 stays slot_trap0 there, and this file -- added only to the
// walk_window family in CMakeLists, next to meshcolliderbase_vtable.cpp -- seats
// the real deleting body at boot.
//
// ABI. The flat-C D0 is `int *D0(int *this)` (cdecl, this on the stack). The
// MSVC vtable slot is __thiscall (this in ecx), so it cannot point at the flat
// body directly; slot_mc_dtor is the ecx->arg adapter, the same convention every
// slot in clsn_vtable.cpp uses. Naming BOTH flat bodies here is the reference
// edge: D0 for the slot, D1 kept so the complete-object body links alongside it.

extern "C" {

extern void *_ZTV12MeshCollider[13];      /* storage in hal/clsn_vtable.cpp */

int *_ZN12MeshColliderD0Ev(void *self);   /* deleting, arm9 0x0203982c */
int *_ZN12MeshColliderD1Ev(void *self);   /* complete, arm9 0x02039864 */

}

static void __fastcall slot_mc_dtor(void *self, void *)
{
    _ZN12MeshColliderD0Ev(self);
}

/* Kept referenced so the complete-object body (called by derived colliders'
   base-subobject teardown) is pulled into the link with the deleting body. */
static void *const g_keep_mc_d1 = (void *)&_ZN12MeshColliderD1Ev;

// ---- the rest of the family's destructor pairs -----------------------------
//
// MeshCollider is not the only collision class whose ROM vtable the port hosts
// as a ZEROED array. Five more of them sit in the same band, each with the same
// Itanium two-slot destructor head, and in every one of them the port had the
// complete-object body (D1) linked and the deleting body (D0) missing entirely
// -- dsd never recovered a name for the D0s, so they are still spelled
// func_0203xxxx and no linked object mentioned them.
//
// SLOT NUMBERS ARE READ OFF THE ROM, not inferred. config/arm9/relocs.txt:
//
//   from:0x02099204 -> 0x020373f8   WithMeshClsn   D1  (_ZN12WithMeshClsnD1Ev)
//   from:0x02099208 -> 0x020373b8                  D0  (func_020373b8)
//   from:0x02099264 -> 0x02037534   RaycastGround  D1  (_ZN13RaycastGroundD1Ev)
//   from:0x02099268 -> 0x020374f0                  D0  (func_020374f0)
//   from:0x020992a4 -> 0x02037764   RaycastLine    D1  (_ZN11RaycastLineD1Ev)
//   from:0x020992a8 -> 0x02037710                  D0  (func_02037710)
//   from:0x02099338 -> 0x02037cb0   SphereClsn     D1  (_ZN10SphereClsnD1Ev)
//   from:0x0209933c -> 0x02037c40                  D0  (func_02037c40)
//   from:0x02099368 -> 0x02038144   ClsnResult     D1  (_ZN10ClsnResultD1Ev)
//   from:0x0209936c -> 0x02038114                  D0  (func_02038114)
//
// and config/arm9/symbols.txt puts each of those five D1 addresses on the class
// destructor the port already links, which is what identifies the class each
// zeroed table belongs to. The five D0 bodies read exactly as their class's
// deleting destructor does: restore the vptr, tear the subobjects down through
// the family's own D1 bodies, hand the storage back through
// Memory::operator_delete2.
//
// WHAT THIS SEAT DOES AND DOES NOT DO. Nothing in the port dispatches these five
// tables today -- they are used as identity tags (hal/actor_vtables.cpp and
// unmatched/Player_HeadBonk.cpp compare &data_02099368, they never call through
// it), and a dispatch would have hit a null slot and died. So this changes no
// behaviour: it makes the port's copy of each table carry the two words the ROM
// puts there, and it is the reference edge that pulls the five matched D0 TUs
// into the link. Slots 2 and up stay as the storage left them; this file has no
// relocation evidence for them and does not own that storage, so it does not
// invent fillers for them.
//
// ABI, same as slot_mc_dtor above: the flat-C bodies are `T *f(T *this)` with
// this on the stack, a ROM-shadow dispatch would arrive __thiscall with this in
// ecx, so each slot takes an ecx->arg adapter rather than the body directly.
extern "C" {

extern void *data_02099204[];   /* WithMeshClsn  -- storage hal/actor_vtables.cpp */
extern void *data_02099264[];   /* RaycastGround -- storage hal/actor_vtables.cpp */
extern void *data_020992a4[];   /* RaycastLine   -- storage hal/cxx_aliases.cpp */
extern void *data_02099338[];   /* SphereClsn    -- storage hal/actor_vtables.cpp */
extern void *data_02099368[];   /* ClsnResult    -- storage hal/actor_vtables.cpp */

void *_ZN12WithMeshClsnD1Ev(void *self);    /* 0x020373f8 */
void *func_020373b8(void *self);            /* 0x020373b8 */
void *_ZN13RaycastGroundD1Ev(void *self);   /* 0x02037534 */
void *func_020374f0(void *self);            /* 0x020374f0 */
void *_ZN11RaycastLineD1Ev(void *self);     /* 0x02037764 */
void *func_02037710(void *self);            /* 0x02037710 */
void *_ZN10SphereClsnD1Ev(void *self);      /* 0x02037cb0 */
void *func_02037c40(void *self);            /* 0x02037c40 */
void *_ZN10ClsnResultD1Ev(void *self);      /* 0x02038144 */
void *func_02038114(void *self);            /* 0x02038114 */

}

#define FAMILY_DTOR_SLOTS(tag, table, d1, d0)                            \
    static void __fastcall tag##_d1(void *self, void *) { d1(self); }    \
    static void __fastcall tag##_d0(void *self, void *) { d0(self); }    \
    static void tag##_seat(void)                                         \
    { table[0] = (void *)tag##_d1; table[1] = (void *)tag##_d0; }

FAMILY_DTOR_SLOTS(wmc, data_02099204, _ZN12WithMeshClsnD1Ev,  func_020373b8)
FAMILY_DTOR_SLOTS(rcg, data_02099264, _ZN13RaycastGroundD1Ev, func_020374f0)
FAMILY_DTOR_SLOTS(rcl, data_020992a4, _ZN11RaycastLineD1Ev,   func_02037710)
FAMILY_DTOR_SLOTS(sph, data_02099338, _ZN10SphereClsnD1Ev,    func_02037c40)
FAMILY_DTOR_SLOTS(clr, data_02099368, _ZN10ClsnResultD1Ev,    func_02038114)

#undef FAMILY_DTOR_SLOTS

extern "C" void hal_seat_meshcollider_dtor(void)
{
    (void)g_keep_mc_d1;
    _ZTV12MeshCollider[0] = (void *)slot_mc_dtor;

    wmc_seat();
    rcg_seat();
    rcl_seat();
    sph_seat();
    clr_seat();
}
