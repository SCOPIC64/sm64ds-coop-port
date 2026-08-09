/* GATE 188 link aliases for Jolly Roger Bay's movers.
 *
 * Three bridges the recovered ov016 src needs:
 *
 * 1. func_ov018_02111bf0 -- a cross-overlay ALIAS TYPO in the recovered src:
 *    several UNAGI state bodies (func_ov016_02111758/021118b4/02111534) call
 *    "func_ov018_02111bf0", but the real function is func_ov016_02111bf0 (the
 *    SetState+ENTER dispatcher), which is host-copied in Unagi_StateDispatch.cpp.
 *    Bridge the ov018 spelling to the ov016 host body.
 *
 * 2. func_020b5e58 -- func_ov016_02112fa8 (id 60's daObjKi_Ita_c InitResources)
 *    calls it by the bare "func_020b5e58" C name, but the matched body is
 *    func_ov002_020b5e58 (ov002, in slice_gate188.txt). Bridge the bare name.
 *
 * 3. MeshColliderBase::UpdatePosWithTransform (the void* DATA form) -- ShipUp's
 *    InitResources declares it `extern void* _ZN16...S8_;` and takes its address
 *    as the BeforeClsn callback, which MSVC mangles as the data symbol
 *    ?_ZN16...S8_@@3PAXA. The real static method is
 *    ?UpdatePosWithTransform@MeshColliderBase@@SAX...@Z (hosted, gate 59). Alias
 *    the void* data spelling onto it. (FloatOnWater/id 313 declares it as a
 *    `void(void)` function whose &-address already resolves through the existing
 *    cxx_aliases.cpp bridge, so only ShipUp's void* form needs this.)
 */
#include <cstddef>

extern "C" {
int func_ov016_02111bf0(void *c, void *cell);
int func_ov002_020b5e58(char *self, char *fp);
}

/* #1: the ov018 typo -> the ov016 host dispatcher. Both C linkage. */
#pragma comment(linker, "/alternatename:_func_ov018_02111bf0=_func_ov016_02111bf0")
/* #2: the bare func_020b5e58 -> the ov002 matched body. Both C linkage. */
#pragma comment(linker, "/alternatename:_func_020b5e58=_func_ov002_020b5e58")
/* #3: ShipUp's void* data reference to UpdatePosWithTransform -> the real
   static method (the gate-59 host body). */
#pragma comment(linker, "/alternatename:?_ZN16MeshColliderBase22UpdatePosWithTransformERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_@@3PAXA=?UpdatePosWithTransform@MeshColliderBase@@SAXAAU1@PAUActor@@AAUClsnResult@@AAUVector3@@PAUVector3_16@@4@Z")
/* #4: func_ov002_020b5e58 (id 60's daObjKi_Ita_c init helper) takes the address
   of UpdatePosAndAngs declared `extern char _ZN16...S8_;` -> MSVC mangles that
   data spelling as ?_ZN16...S8_@@3DA (D = char); bridge it to the real static
   method the same way. */
#pragma comment(linker, "/alternatename:?_ZN16MeshColliderBase16UpdatePosAndAngsERS_P5ActorR10ClsnResultR7Vector3P10Vector3_16S8_@@3DA=?UpdatePosAndAngs@MeshColliderBase@@SAXAAU1@PAUActor@@AAUClsnResult@@AAUVector3@@PAUVector3_16@@4@Z")
