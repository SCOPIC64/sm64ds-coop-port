// UpdateKillByMegaChar: STUBBED for gate 9. The real body is the
// Mega-Mario squash check whose closure (RaycastLine methods,
// Actor::UpdatePos) belongs to the Player gates; with no mega char in
// play the real function takes its early-out, which is what 0 means.
//
// RULED (w6-c item 3): THIS IS NOT A HOST-ABI EXCEPTION AND MUST NOT BE
// TAGGED AS ONE. Nothing about the ROM's argument passing, its pointers to
// members or its hardware is unrepresentable here. It is a stub standing in
// for a matched TU that could link, which is the definition of a SHADOW, and
// a PORT_HOST_ABI tag would move it out of the replacement work list -- the
// one thing the queue exists to prevent. It stays in the queue. The ruling is
// still a ruling: someone asked whether it was an exception, and it is not.
//
// WHAT RETIRING IT COSTS, measured against walk_window.map at w6-c tip rather
// than estimated. The matched TU names nine externals. Six are already in the
// binary -- Matrix4x3_FromRotationY, MulVec3Mat4x3, Vec3_Add,
// DecIfAbove0_Byte, data_020a0e68, Actor::UpdatePos -- and three are not:
//
//     _ZN11RaycastLine14SetObjAndLineERK7Vector3S2_P5Actor
//     _ZN11RaycastLine11DetectClsnEv
//     func_ov002_020ee5d0
//
// So the gate-9 note above is still right about where the work lives: two
// RaycastLine methods and one ov002 body, not a scattered closure. That is a
// seat with a real cost and it wants its own lane. The seat lands first; the
// tag question does not get asked again instead of it.
extern "C" int _ZN8Platform20UpdateKillByMegaCharEsss5Fix12IiE(
    void *, short, short, short, int)
{
    return 0;
}
