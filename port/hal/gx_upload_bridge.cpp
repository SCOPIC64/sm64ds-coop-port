// MSVC-method bridges for gate 4b (the cross-linkage seam, gate-3a style).
//
// The Model TUs declare GX's upload entry points as STATIC MEMBERS of a
// struct GX (mangling ?LoadTex@GX@@SAX...), while the definitions are either
// namespace-style C++ (GX::LoadTex from src/) or C-named HAL stubs. Same
// story for the SharedFilePtr and ModelComponents methods whose real
// definitions are C-named .c files. Each bridge here exists because the two
// spellings mangle differently on MSVC; none of them add behavior.
#include <cstdio>

typedef unsigned int u32;
typedef unsigned short u16;
#include "SharedFilePtr.h"
#include "ModelBase.h"

// Three older callers still declare LoadFile as void. MSVC encodes return
// types, unlike Itanium here; the call ABI is otherwise identical and all three
// callers discard the real pointer result.
#pragma comment(linker, "/alternatename:?LoadFile@SharedFilePtr@@QAEXXZ=?LoadFile@SharedFilePtr@@QAEPAXXZ")
#pragma comment(linker, "/alternatename:__ZN2GX11LoadTexPlttEPKvjj=?LoadTexPltt@GX@@YAXPBXII@Z")
#pragma comment(linker, "/alternatename:?data_020a60b0@@3IA=_data_020a60b0")

// cstd::abs's C-spelling forwarder moved to hal/heap_globals.cpp, next to the
// other Itanium-spelling bridges; every target that links this file links that
// one, and the allocator layer needs the same symbol.

extern "C" {
void _ZN2GX12BeginLoadTexEv(void);
void _ZN2GX10EndLoadTexEv(void);
void _ZN2GX16BeginLoadTexPlttEv(void);
void _ZN2GX14EndLoadTexPlttEv(void);
void _ZN2GX7LoadTexEPKvjj(const void *, u32, u32);
void _ZN2GX11LoadTexPlttEPKvjj(const void *, u32, u32);
}

struct GX {
    static void BeginLoadTex();
    static void EndLoadTex();
    static void BeginLoadTexPltt();
    static void EndLoadTexPltt();
    static void LoadTex(const void *, u32, u32);
    static void LoadTexPltt(const void *, u32, u32);
};
void GX::BeginLoadTex() { _ZN2GX12BeginLoadTexEv(); }
void GX::EndLoadTex() { _ZN2GX10EndLoadTexEv(); }
void GX::BeginLoadTexPltt() { _ZN2GX16BeginLoadTexPlttEv(); }
void GX::EndLoadTexPltt() { _ZN2GX14EndLoadTexPlttEv(); }
void GX::LoadTex(const void *s, u32 o, u32 z) { _ZN2GX7LoadTexEPKvjj(s, o, z); }
/* SM64DS_TEX_LOG=1: the palette half of the upload ledger (hal_tex_log
   and the texel half live in hal/model_host.cpp). */
extern "C" int hal_tex_log(void);
void GX::LoadTexPltt(const void *s, u32 a, u32 z)
{
    if (hal_tex_log())
        printf("[palup]  off=%05x size=%05x -> pltt %04x\n", a, z, a >> 4);
    _ZN2GX11LoadTexPlttEPKvjj(s, a, z);
}

// Shrinks the file image to its post-parse size on the DS (a heap-space
// optimization). Skipped on host: the image simply stays at load size.
void SharedFilePtr::ReallocateModelFile() {}

// Historical C-name callers -> methods that are now real C++ definitions.
extern "C" {
void _ZN15ModelComponents21UpdateVertsUsingBonesEv(ModelComponents *self)
{ self->ModelComponents::UpdateVertsUsingBones(); }
void _ZN15ModelComponents11UpdateBonesEP8BCA_Filei(
    ModelComponents *self, BCA_File *file, int frame)
{ self->ModelComponents::UpdateBones(file, frame); }
}


// The compressed-texture loader keeps its C-named terminal-floor definition.
// RETIRED: src/_ZN5Model27LoadCompressedTextureToVramEPcjS0_.cpp is gated
// (gate4b) and provides the real MSVC static, including the pre-bump
// return this bridge used to reconstruct by hand. Keeping this wrapper
// meant link order (/FORCE:MULTIPLE keeps the first definition, and hal/
// sorts before src/) shadowed the real uploader with a forwarder onto the
// still-undefined Itanium name -- every format-5 texture jumped to the
// image base during Player::InitResources.
