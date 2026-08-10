/* Shared value types, consolidated from thousands of duplicate inline
 * declarations across the loose function files. */
#ifndef COMMON_H
#define COMMON_H
#include "types.h"

/* Guarded so math/Matrix.h, which needs this type in its own richer
 * {Matrix3x3 r; Vector3 t;} shape and cannot assume common.h was included
 * first, can define it too without colliding -- the same MATRIX4X3_DEFINED
 * convention VECTOR3_16_DEFINED already uses below. Both spellings are
 * layout-compatible (12 Fix12i words, 0x30 bytes); whichever header is seen
 * first wins and the other stands down. */
#ifndef MATRIX4X3_DEFINED
#define MATRIX4X3_DEFINED
struct Matrix4x3 { s32 m[12]; };
#endif
/* Vector3 lives in types.h (as {Fix12i x,y,z}); not redefined here. */
/* Guarded so MeshColliderBase.h, which needs this type and cannot assume common.h
 * was included first, can define it too without colliding. Both spell the guard
 * VECTOR3_16_DEFINED; whichever is seen first wins and the other stands down. */
#ifndef VECTOR3_16_DEFINED
#define VECTOR3_16_DEFINED
struct Vector3_16 { s16 x, y, z; };
#endif

#ifndef MATRIX4X3_TYPEDEF_DONE
#define MATRIX4X3_TYPEDEF_DONE
typedef struct Matrix4x3 Matrix4x3;
#endif
typedef struct Vector3_16 Vector3_16;

#endif
