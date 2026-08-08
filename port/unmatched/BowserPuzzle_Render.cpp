/* HOST COPY of src/_ZN19BowserPuzzleManager6RenderEv.cpp (BOWSER_PUZZLE_MANAGER,
 * 79, ov064, gate 179) -- the ModelAnim/Model slot-5 collision, the Whomp /
 * Butterfly / Fish / QuestionBlock / Scuttlebug / RotatingFirebar case that gates
 * 17, 18, 21, 23, 64, 176 and 177 all wrote down, one overlay after another.
 *
 * The matched TU dispatches the model's slot 5 through a LOCAL six-virtual shadow
 * (`struct Base { v0..v4; m(int); }`, so `m` is the ROM's slot 5) on a plain Model
 * at +0xd4 (`struct Derived { char pad[0xd4]; Base base; }`, and the Manager
 * destructors call _ZN5ModelD1Ev at that same +0xd4, confirming a plain Model, not
 * a ModelAnim). The host _ZTV5Model is dual-filled -- Render sits in both slot 4
 * and slot 5 -- so this one is served by the qualified Model::Render, spelled out
 * so the file reads the same as its matched original.
 *
 * The matched original is `b->m(0)` -- slot 5 called with a null scale -- then
 * `return 1;`. Dropped from slice_gate179.txt and hosted here.
 *
 * PORT_HOST_ABI: ROM-order Model slot-5 dispatch, the Whomp/Fish case.
 */
#include "Model.h"

extern "C" int _ZN19BowserPuzzleManager6RenderEv(void *selfv)
{
    /* ((Base *)&((Derived *)this)->base)->m(0) -- Model's slot 5, null scale */
    ((Model *)((char *)selfv + 0xd4))->Model::Render(0);
    return 1;
}
