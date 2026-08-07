cmake_minimum_required(VERSION 3.16)
project(sm64ds-port C CXX)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MAP")
add_compile_options(/Oy-)

# Gate 1: compile the manifest slice out of ../src plus the HAL, link the
# smoke runner. 32-bit host only -- platform.h enforces it at compile time.

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate1.txt" SLICE_LINES)
set(SLICE_SOURCES "")
foreach(line IN LISTS SLICE_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke
    tests/smoke.cpp
    hal/cstd_div.c
    hal/os_time.cpp
    hal/shims.cpp
    ${SLICE_SOURCES})

target_include_directories(smoke PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_compile_definitions(smoke PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    # The decomp C uses guest idioms MSVC warns about; keep the build quiet
    # enough that a NEW warning is visible. /W3 stays on.
    target_compile_options(smoke PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 2: the allocator family + torture smoke, its own binary so gate 1
# stays a stable baseline.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate2.txt" SLICE2_LINES)
set(SLICE2_SOURCES "")
foreach(line IN LISTS SLICE2_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE2_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke_heap tests/smoke_heap.cpp hal/heap_globals.cpp ${SLICE2_SOURCES})
target_include_directories(smoke_heap PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_heap PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(smoke_heap PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 3a: root heap + Memory:: layer over the gate-2 allocator.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate3a.txt" SLICE3A_LINES)
set(SLICE3A_SOURCES "")
foreach(line IN LISTS SLICE3A_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE3A_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke_roots tests/smoke_roots.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES})
target_include_directories(smoke_roots PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_roots PRIVATE SM64DS_PLATFORM_PC)
if(MSVC)
    target_compile_options(smoke_roots PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# The ntr platform layer: imported host DS hardware (see ntr/README.md).
# Compiled as its own lib so every gate binary can link the pieces it needs.
add_library(ntr STATIC
    ntr/io.cpp ntr/gx.cpp ntr/texture.cpp ntr/ppu.cpp ntr/ppu_sub.cpp ntr/lz77.cpp
    ntr/rt.cpp ntr/runtime.cpp ntr/fs.cpp ntr/bmd.cpp)
target_include_directories(ntr PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
if(MSVC)
    target_compile_options(ntr PRIVATE /wd4311 /wd4312)
endif()

# 512x384 middle tier: 4x the pixels of native (~12ms software raster),
# half the chunk of DS-native. The interactive window's default.
add_library(ntr_2x STATIC
    ntr/io.cpp ntr/gx.cpp ntr/texture.cpp ntr/ppu.cpp ntr/ppu_sub.cpp ntr/lz77.cpp
    ntr/rt.cpp ntr/runtime.cpp ntr/fs.cpp ntr/bmd.cpp)
target_include_directories(ntr_2x PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
target_compile_definitions(ntr_2x PUBLIC NTR_HIRES2)
if(MSVC)
    target_compile_options(ntr_2x PRIVATE /wd4311 /wd4312)
endif()

# The same layer at 1024x768 for the interactive window; the smokes keep
# the DS-native lib (their reference pixel counts depend on 256x192).
add_library(ntr_hires STATIC
    ntr/io.cpp ntr/gx.cpp ntr/texture.cpp ntr/ppu.cpp ntr/ppu_sub.cpp ntr/lz77.cpp
    ntr/rt.cpp ntr/runtime.cpp ntr/fs.cpp ntr/bmd.cpp)
target_include_directories(ntr_hires PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/ntr/include")
target_compile_definitions(ntr_hires PUBLIC NTR_HIRES)
if(MSVC)
    target_compile_options(ntr_hires PRIVATE /wd4311 /wd4312)
endif()

# Gate 3b: SharedFilePtr over the host card seam; assets come from the
# repo's extracted/ tree + asset catalog, root baked in (env-overridable).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate3b.txt" SLICE3B_LINES)
set(SLICE3B_SOURCES "")
foreach(line IN LISTS SLICE3B_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE3B_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

get_filename_component(PORT_REPO_ROOT_ABS "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)

# Gate 4a: the game's display-list submission machinery over the ntr GX.
# These src/ files touch MMIO textually, so they build from hostgen-emitted
# copies under the build tree -- src/ itself is never edited.
set(GATE4A_SYMS
    func_0205532c func_02055388 func_020553a4 func_02055780
    func_0205a358 func_0205a290 func_0205a21c func_02056e98 func_02059fa8)
set(GATE4A_GEN "")
foreach(sym IN LISTS GATE4A_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE4A_GEN "${sym_out}")
endforeach()

add_executable(smoke_gx tests/smoke_gx.cpp ${GATE4A_GEN})
target_include_directories(smoke_gx PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_gx PRIVATE ntr)
target_compile_definitions(smoke_gx PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_gx PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4b: the whole Model pipeline. Adds the material-bind and matrix
# feeders to the hostgen set and the Model/render slice on top of everything.
set(GATE4B_SYMS func_02044b30 Geometry_MatrixMultiply3x3 func_02055998
    func_02044534 func_0204488c NormalizeVec3)
set(GATE4B_GEN "")
foreach(sym IN LISTS GATE4B_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE4B_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate4b.txt" SLICE4B_LINES)
set(SLICE4B_SOURCES "")
foreach(line IN LISTS SLICE4B_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE4B_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# ROM constants (sine table, default matrices): generated locally from the
# extracted arm9 image so Nintendo bytes never enter the repo.
set(ROMDATA_C "${CMAKE_BINARY_DIR}/host-src/romdata.c")
add_custom_command(OUTPUT "${ROMDATA_C}"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/romdata.py" "${ROMDATA_C}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/romdata.py"
    COMMENT "romdata from extracted/arm9_dec.bin")

add_executable(smoke_model tests/smoke_model.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_model PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_model PRIVATE ntr)
target_compile_definitions(smoke_model PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_model PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 5: the fiber frame loop -- game-shaped frames, host-rastered.
add_executable(smoke_frames tests/smoke_frames.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_frames PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_frames PRIVATE ntr)
target_compile_definitions(smoke_frames PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_frames PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4d: the catalog soak -- every model through the game pipeline.
add_executable(smoke_soak tests/smoke_soak.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_soak PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_soak PRIVATE ntr)
target_compile_definitions(smoke_soak PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_soak PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 8: collision -- the octree walk over real KCL data. cstd::sqrt
# drives the hardware sqrt unit through literal MMIO -> hostgen.
set(GATE8_SYMS _ZN4cstd4sqrtEy _ZN4cstd11fdiv_resultEv _ZN4cstd16reciprocal_asyncE5Fix12IiE
    Matrix4x3_ApplyInPlaceToScale)
set(GATE8_GEN "")
foreach(sym IN LISTS GATE8_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE8_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate8.txt" SLICE8_LINES)
set(SLICE8_SOURCES "")
foreach(line IN LISTS SLICE8_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE8_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Three files the SPHERE pass needs that later gates already carry: gate 10 has
# func_02039794 and func_02037e58, gate 13 has DotVec3. The binaries that stop
# at gate 8/9 (smoke_clsn, smoke_actor) get them here; smoke_player and
# walk_window get them from their own slices, and adding them in both places
# would be a duplicate source in one target.
set(GATE8_EXTRA_SOURCES
    "${PORT_REPO_ROOT_ABS}/src/func_02039794.c"
    "${PORT_REPO_ROOT_ABS}/src/func_02037e58.c"
    "${PORT_REPO_ROOT_ABS}/src/DotVec3.c")

add_executable(smoke_clsn tests/smoke_clsn.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE8_SOURCES} ${GATE8_EXTRA_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} "${ROMDATA_C}")
target_include_directories(smoke_clsn PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_clsn PRIVATE ntr)
target_compile_definitions(smoke_clsn PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_clsn PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 7: ModelAnim -- the game owns frame progression.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate7.txt" SLICE7_LINES)
set(SLICE7_SOURCES "")
foreach(line IN LISTS SLICE7_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE7_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 24: BlendModelAnim -- the cross-fading ModelAnim. Rides on gate 7's
# stack and is proven by the same smoke; see slice_gate24.txt.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate24.txt" SLICE24_LINES)
set(SLICE24_SOURCES "")
foreach(line IN LISTS SLICE24_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE24_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke_modelanim tests/smoke_modelanim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/anim_bridge.cpp
    hal/blend_vtable.cpp hal/cstd_div.c
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE24_SOURCES} ${GATE4A_GEN} ${GATE4B_GEN}
    # BlendModelAnim::D0 calls func_0203cbc0, whose src form is an ARM r0
    # ride-through. The host copy that replaces it defines func_02073244
    # beside it and otherwise only reaches targets through gate 31.
    "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/func_02073244.c"
    "${ROMDATA_C}")
target_include_directories(smoke_modelanim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_modelanim PRIVATE ntr)
target_compile_definitions(smoke_modelanim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_modelanim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 6: the 2D sprite engine's software side. OAM::Load reads POWCNT
# textually -> hostgen; the rest of the slice is plain.
set(GATE6_SYMS _ZN3OAM4LoadEv)
set(GATE6_GEN "")
foreach(sym IN LISTS GATE6_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE6_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate6.txt" SLICE6_LINES)
set(SLICE6_SOURCES "")
foreach(line IN LISTS SLICE6_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    if(line MATCHES "_ZN3OAM4LoadEv")
        continue()
    endif()
    list(APPEND SLICE6_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke_oam tests/smoke_oam.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE6_SOURCES} ${GATE4A_GEN} ${GATE4B_GEN} ${GATE6_GEN} "${ROMDATA_C}")
target_include_directories(smoke_oam PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_oam PRIVATE ntr)
target_compile_definitions(smoke_oam PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_oam PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 5b: the animation soak -- every compatible model+BCA pair.
add_executable(smoke_soak_anim tests/smoke_soak_anim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_soak_anim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_soak_anim PRIVATE ntr)
target_compile_definitions(smoke_soak_anim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_soak_anim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 4c: animation -- same slice (the 4c files live in slice_gate4b.txt),
# different driver: the piano posed by its BCA through the game's walk.
add_executable(smoke_anim tests/smoke_anim.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} "${ROMDATA_C}")
target_include_directories(smoke_anim PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_anim PRIVATE ntr)
target_compile_definitions(smoke_anim PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_anim PRIVATE /wd4311 /wd4312 /wd4068)
endif()

add_executable(smoke_fs tests/smoke_fs.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} "${ROMDATA_C}")
target_include_directories(smoke_fs PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_fs PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_fs PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 9: the actor framework -- ArrowSignRight's full lifecycle.
# One .c file carries C++ syntax (mwccarm accepted it); hostgen's
# emit-as-cpp with extern "C" wrap makes it legal.
set(GATE9_SYMS _ZN9ActorBase21AfterCleanupResourcesEj)
set(GATE9_GEN "")
foreach(sym IN LISTS GATE9_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE9_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate9.txt" SLICE9_LINES)
set(SLICE9_SOURCES "")
foreach(line IN LISTS SLICE9_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE9_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

add_executable(smoke_actor tests/smoke_actor.cpp
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/actor_vtables.cpp hal/anim_bridge.cpp
    hal/cxxname_bridge.cpp hal/megachar_stub.cpp
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE8_SOURCES} ${SLICE9_SOURCES} ${GATE8_EXTRA_SOURCES}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} ${GATE9_GEN} "${ROMDATA_C}")
target_include_directories(smoke_actor PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_actor PRIVATE ntr)
target_compile_definitions(smoke_actor PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_actor PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 10: Player spawns and stands (the walking campaign).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate10.txt" SLICE10_LINES)
set(SLICE10_SOURCES "")
foreach(line IN LISTS SLICE10_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE10_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Two Actor entry points are C++ inside a .c, but a different shape from the
# hostgen cases below: each is a real METHOD against Actor.h, so hostgen's
# wrap is wrong for them -- it would put the whole file, Actor.h include and
# all, inside extern "C". They compile as C++ instead, and
# hal/method_faces.cpp carries the C-linkage faces their callers spell. Both
# were plain C free functions until main rewrote them as methods.
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/_ZN5Actor13SpawnSoundObjEj.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/_ZN5Actor24KillAndTrackInDeathTableEv.c"
    PROPERTIES LANGUAGE CXX)

# func_ov002_020d708c is C++ inside a .c (a `virtual` vtable shadow), which
# MSVC will not compile as C. hostgen.py already emits .cpp with the symbols
# wrapped back to C linkage, so route it through hostgen rather than editing
# the byte-verified tree.
# engine/fader/StartExitFaderWipe.c is the same case: a `virtual` class in a
# .c, this time the FaderWipe the death path calls SetToStart on. hostgen
# resolves a path as well as a bare name, so the fader subdirectory works.
# func_ov002_020c897c does void-pointer arithmetic (the NoControl body's
# `*(void**)(c + id * 4 + 0xdc) + 0x50` anim probe), which MSVC C rejects
# with C2036; hostgen's VOIDPP_ARITH rewrite retypes the deref to char*.
# func_ov002_020cfbdc and func_ov002_020d6c60 are a third shape: decl_common.h
# declares each with a different pointer parameter type than its own TU defines
# it with (void* vs char*, and (char*,void*) vs (char*,char*)). Two separate
# symbols while the decl_*.h headers were C++-mangled, one extern "C" overload
# since main gave them C linkage -- error C2733. hostgen's HEADER_SHADOW hides
# the header's declaration of just those names, so neither src/ nor include/
# has to move.
set(GATE10_SYMS func_ov002_020dec70 func_ov002_020d708c
    engine/fader/StartExitFaderWipe func_ov002_020c897c
    engine/fader/StartEntranceFaderWipe func_ov002_020cfbdc
    func_ov002_020d6c60)
set(GATE10_GEN "")
foreach(sym IN LISTS GATE10_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE10_GEN "${sym_out}")
endforeach()

# Every ovdata mount also writes a <out>.c.map sidecar, and OV_DATA_MAPS
# collects them for the --cross pass at the end of this block. See the module
# docstring in tools/ovdata.py: one invocation cannot see another's symbols, so
# a pointer from one mounted overlay into another needs a pass that has all of
# them in hand. tools/ovsweep.py reads the same sidecars to check the result.
set(OV_DATA_MAPS "")

set(OV002DATA_C "${CMAKE_BINARY_DIR}/host-src/ov002_data.c")
add_custom_command(OUTPUT "${OV002DATA_C}" "${OV002DATA_C}.map"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" ov002
            "${OV002DATA_C}" --from-list "${CMAKE_CURRENT_SOURCE_DIR}/ov002_syms.txt"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/ov002_syms.txt"
    COMMENT "ovdata ov002")
list(APPEND OV_DATA_MAPS "${OV002DATA_C}.map")

# Gate 30: the LEVEL overlays, one whole-image mount each.
#
# A level overlay cannot be mounted symbol by symbol: its LVL_Overlay hangs
# nine object tables off one base and the loaders walk them by count, straight
# across whatever symbol boundaries dsd happened to name. --whole emits the
# entire image as one array and rebases its internal pointers from the reloc
# table.
#
# Every level overlay is linked at the SAME DS base (0x021111a0) because the
# DS only ever has one loaded. On the host they are separate arrays and
# port_ovNNN_at() is per-overlay, so hosting several at once is just several
# mounts; hal/level_boot.cpp's table picks which one the boot walks.
#
#   ov009 = level 1, castle grounds  (data/stage/main_castle), course 29
#   ov014 = level 6, Bob-omb Battlefield  (data/stage/bombhei_map), course 0
#   ov010 = level 2, a castle interior floor, course 29 (interior-shaped)
#   ov015 = level 7, Whomp's Fortress, course 1 (open course)
#
# Both facts come from the ROM, not from the overlay numbering: the arm9's
# level-to-overlay table data_020758c8 maps level -> overlay (it is level+8),
# and each overlay's LVL_Overlay names its own kcl/bmd/icg/icl OV0 HANDLES,
# resolved through build/assets/handles.tsv rather than by arithmetic. See the
# evidence note at the top of hal/level_boot.cpp, including what a derived
# handle constant got wrong; and the course numbers are the arm9
# SUBLEVEL_LEVEL_TABLE (0x02075298) which picks levels of two shapes here.
#
# ADDING A LEVEL IS MECHANICAL, and this list plus a row in hal/level_boot.cpp's
# port_level_table[] is the whole of it. The mount recipe lives in the header
# docstring of tools/ovdata.py; the level-boot recipe beside port_level_table[].
set(PORT_LEVEL_OVERLAYS ov009 ov014 ov010 ov015)
set(LEVEL_OV_DATA "")
foreach(ovn IN LISTS PORT_LEVEL_OVERLAYS)
    set(ovn_out "${CMAKE_BINARY_DIR}/host-src/${ovn}_data.c")
    add_custom_command(OUTPUT "${ovn_out}" "${ovn_out}.map"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" ${ovn}
                "${ovn_out}" --whole
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py"
        COMMENT "ovdata ${ovn} (whole image)")
    list(APPEND LEVEL_OV_DATA "${ovn_out}")
    list(APPEND OV_DATA_MAPS "${ovn_out}.map")
endforeach()

# Gate 17: the level overlay's own actors. ov009 is mounted WHOLE for the
# loaders; its actor code reaches its SharedFilePtrs, CLPS blocks and spawn
# tables BY NAME, so those symbols are mounted a second time per symbol.
# --pack keeps the runs that are really one array contiguous -- ov009's water
# spawn points are three seven-element Vector3 arrays that dsd split into
# twenty-one symbols and func_ov009_02111b1c walks by index.
set(OV009SYMS_C "${CMAKE_BINARY_DIR}/host-src/ov009_syms.c")
add_custom_command(OUTPUT "${OV009SYMS_C}" "${OV009SYMS_C}.map"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" ov009
            "${OV009SYMS_C}" --from-list
            "${CMAKE_CURRENT_SOURCE_DIR}/ov009_syms.txt" --pack
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/ov009_syms.txt"
    COMMENT "ovdata ov009 (per symbol, packed)")
list(APPEND OV_DATA_MAPS "${OV009SYMS_C}.map")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate17.txt" SLICE17_LINES)
set(SLICE17_SOURCES "")
foreach(line IN LISTS SLICE17_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE17_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 18 and up: the ACTOR overlays. Neither the engine overlay nor the
# level's own -- ov085/ov098/ov100/ov102 each carry a handful of classes that
# several levels pull in, and the castle grounds names some of each. They are
# mounted PER SYMBOL like ov002 (nothing walks a table across their symbol
# boundaries the way a level overlay's loaders do), --pack so the runs that
# are really one array stay contiguous, and each list is the overlay's WHOLE
# data span rather than a hand-picked subset -- see the header of any
# ovNNN_syms.txt for why.
# ov001 joins the per-symbol mounts for the HUD's sprite templates (gate 27):
# it is not an actor overlay, but it wants exactly the same treatment --
# packed run, relocations applied, interior pointers rebased to host.
# ov003 joins them for gate 31: not an actor overlay either, mounted for
# exactly one table -- the debug level select's row list. See ov003_syms.txt.
set(PORT_ACTOR_OVERLAYS ov085 ov098 ov100 ov089 ov102 ov001 ov003 ov084
    ov014 ov062 ov091 ov078)
# gate 41-42: ov010 is level 2's own overlay, already mounted whole in
# PORT_LEVEL_OVERLAYS; it joins the per-symbol mounts here so TRAP, LIGHT_BEAM
# and PEACH_PAINTING reach their SpawnInfo records by name -- the gate-17 shape
# of ov009/ov014 applied to the castle interior. See port/ov010_syms.txt.
list(APPEND PORT_ACTOR_OVERLAYS ov010)
# gate 50: ov080 is an ordinary actor overlay carrying the castle interior's
# PAINTING (daPicGate_c, 307) among three other classes; mounted per symbol so
# the actor reaches its SpawnInfo record, tuning tables and twelve state
# statics by name. See port/ov080_syms.txt.
list(APPEND PORT_ACTOR_OVERLAYS ov080)
# ov084 joins for gate 32: the first ENEMY overlay, GOOMBA and BOB_OMB_BUDDY.
# ...and ov014, ov062 and ov091 with them. ov014 is a LEVEL overlay (Bob-omb
# Battlefield's own) and is mounted a second time here, per symbol, because
# CHAIN_CHOMP and CHAIN_CHOMP_FENCE reach their tables by name -- the gate-17
# treatment of ov009 applied to the second level. ov062 is KOOPA_THE_QUICK's.
# ov091 is the one the chomp drags in: its InitResources spawns actor 27, the
# post it is chained to, and a null back from that spawn is a fault. ov078 is
# KING_BOB_OMB's, and level 6 is the only level of the fifty-two that loads it.
set(ACTOR_OV_SYMS "")
foreach(ovn IN LISTS PORT_ACTOR_OVERLAYS)
    set(ovn_out "${CMAKE_BINARY_DIR}/host-src/${ovn}_syms.c")
    add_custom_command(OUTPUT "${ovn_out}" "${ovn_out}.map"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" ${ovn}
                "${ovn_out}" --from-list
                "${CMAKE_CURRENT_SOURCE_DIR}/${ovn}_syms.txt" --pack
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py"
                "${CMAKE_CURRENT_SOURCE_DIR}/${ovn}_syms.txt"
        COMMENT "ovdata ${ovn} (per symbol, packed)")
    list(APPEND ACTOR_OV_SYMS "${ovn_out}")
    list(APPEND OV_DATA_MAPS "${ovn_out}.map")
endforeach()

# THE CROSS-MOUNT POINTER PASS, last because it needs every map.
#
# A pointer from one mounted overlay into another was left holding its DS
# address by the per-mount passes above, which can only see their own symbols.
# That is not a loud failure: ntr/io.cpp reserves DS main RAM at 0x02000000 as
# zeroed pages, so the read succeeds and hands the game an OamAttr list with no
# 0xffff terminator, and the walk in OAM::Render is unbounded the moment the
# sprite goes off screen. This pass reads every mount's <out>.c.map and emits
# one patch function, port_cross_patch(), for the words that cross.
#
# It reproduces the OAM::MM_PLAYER_ICONS table (ov002 -> ov001, sixteen entries)
# that hal/sub_actors.cpp used to keep by hand, entry for entry, and finds the
# genuinely-crossing pointers a new level's whole-image mount carries (Whomp's
# Fortress -> ov098's cannon data). What it CANNOT resolve is a target in a DS
# window two mounted overlays share -- ov084/ov085 and ov089/ov091 are alternate
# actor overlays linked at one base, and the target address alone cannot say
# which. Those stay hand-resolved from relocs.txt's module: field in
# hal/actor_overlays.cpp (the ov089 key models). tools/ovsweep.py checks that
# nothing crossing was left raw with no patch.
set(OVCROSS_C "${CMAKE_BINARY_DIR}/host-src/ov_cross.c")
add_custom_command(OUTPUT "${OVCROSS_C}"
    COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" --cross
            "${OVCROSS_C}" ${OV_DATA_MAPS}
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ovdata.py" ${OV_DATA_MAPS}
    COMMENT "ovdata --cross (pointers between mounted overlays)")

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate18.txt" SLICE18_LINES)
set(SLICE18_SOURCES "")
foreach(line IN LISTS SLICE18_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE18_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate19.txt" SLICE19_LINES)
set(SLICE19_SOURCES "")
foreach(line IN LISTS SLICE19_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE19_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate20.txt" SLICE20_LINES)
set(SLICE20_SOURCES "")
foreach(line IN LISTS SLICE20_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE20_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate21.txt" SLICE21_LINES)
set(SLICE21_SOURCES "")
foreach(line IN LISTS SLICE21_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE21_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate22.txt" SLICE22_LINES)
set(SLICE22_SOURCES "")
foreach(line IN LISTS SLICE22_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE22_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate23.txt" SLICE23_LINES)
set(SLICE23_SOURCES "")
foreach(line IN LISTS SLICE23_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE23_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 25: the bottom screen. The OAM lifecycle, Stage::LoadGraphics2D's
# closure and Stage::CheckCameraInput; see slice_gate25.txt. OAM::Load is not
# here -- it reads POWCNT textually and comes from GATE6_GEN, which this
# target picks up alongside the slice. (Renumbered from 24 at merge: the
# animation stream took 24 for BlendModelAnim the same night.)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate25.txt" SLICE25_LINES)
set(SLICE25_SOURCES "")
foreach(line IN LISTS SLICE25_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE25_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 26: the boot spine -- the real Stage actor. See the header of
# slice_gate26.txt; the HAL seam is hal/stage_bridges.cpp. (Renumbered from
# 24 at merge: three streams picked 24 the same night.)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate26.txt" SLICE26_LINES)
set(SLICE26_SOURCES "")
foreach(line IN LISTS SLICE26_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE26_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 27: the HUD actor (id 334). Its methods are all matched; what it needed
# was gate 25's sub-OAM path, a registry row, and the ov001 sprite templates.
# InitResources comes from port/unmatched/ -- see the head of slice_gate27.txt.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate27.txt" SLICE27_LINES)
set(SLICE27_SOURCES "")
foreach(line IN LISTS SLICE27_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE27_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
list(APPEND SLICE27_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/HUD_InitResources.cpp")

# Gate 28: the MINIMAP actor (id 335). Same shape as the HUD -- eighteen-slot
# vtable, constructor as factory -- plus the BG3-sub tilemap and the extended
# palette. See the head of slice_gate28.txt.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate28.txt" SLICE28_LINES)
set(SLICE28_SOURCES "")
foreach(line IN LISTS SLICE28_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE28_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 29: the particle subsystem -- the last subsystem the boot reported as
# stubbed. 101 matched files (the other 18 in its closure already ride on
# gates 10/13/16) plus hal/particle_vtable.cpp for the thirteen Callback
# vtables the tracker's constructor installs. See the head of
# slice_gate29.txt for what the dispatch turned out to be.
#
# THIS GATE GOES THROUGH HOSTGEN, unlike most slices, and it has to. Ten of
# its files reach the geometry engine through raw 0x04xxxxxx literals -- the
# particle render family (func_0204af3c, b028, b244, b460, b81c, bbd8, be40,
# c194, c24c) and func_02049ee8, which is what Particle::RenderAll calls.
# Compiled raw, every one of those writes would latch into dead mapped memory
# and the geometry engine would never see a particle: the same failure
# hostgen.py's own header describes for MTX_SCALE. hostgen also carries the
# HEADER_SHADOW entry the tracker's Initialise needs -- decl_Particle.h
# declares it (void*) with C++ linkage and the TU defines it extern "C" over
# a struct, which MSVC rejects as an overload on C linkage (C2733).
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate29.txt" SLICE29_LINES)
set(GATE29_GEN "")
foreach(line IN LISTS SLICE29_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    get_filename_component(sym "${line}" NAME_WE)
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../${line}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE29_GEN "${sym_out}")
endforeach()

# Gate 33: Bob-omb Battlefield's mechanisms, terrain objects and pickups --
# the classes of level 6 (ov014) that are not living creatures. The roster and
# the evidence behind every actor id are in the head of slice_gate33.txt; the
# vtable fills and the debug spawn hook are hal/actor_classes_bob_world.cpp.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate33.txt" SLICE33_LINES)
set(SLICE33_SOURCES "")
foreach(line IN LISTS SLICE33_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE33_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
list(APPEND SLICE33_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/Coin_Behavior.cpp")

# Gate 32: Bob-omb Battlefield's living cast -- the first ENEMY-family actor
# classes the port hosts, and the first vtables past twenty slots. See the head
# of slice_gate32.txt; the HAL seam is hal/actor_classes_bob_enemy.cpp and the
# direct-spawn probe that brings them up before their own level exists is
# hal/actor_spawn_probe_bob.cpp.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate32.txt" SLICE32_LINES)
set(SLICE32_SOURCES "")
foreach(line IN LISTS SLICE32_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE32_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 31: the level HANDOFF. Small on purpose -- almost all of it was already
# linked by gate 10 (see the head of slice_gate31.txt); what is here is
# StartFile's own closure, the Player's destroy slots and the collider
# registry reset. The driver is hal/level_change.cpp.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate31.txt" SLICE31_LINES)
set(SLICE31_SOURCES "")
foreach(line IN LISTS SLICE31_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE31_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
list(APPEND SLICE31_SOURCES
     "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/Player_CleanupResources.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/func_02073244.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/SetNumPlayers.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/unmatched/Door_CleanupResources.cpp")

# Gate 35: the course loop -- damage, death, respawn, coins, the star and the
# course's own sound. Small, because almost all of it was already linked; see
# the head of slice_gate35.txt for what was actually missing.
#
# func_02049018 is the 3D sound DISTANCE function and it drives the DS square-
# root unit directly (SQRTCNT/SQRT_PARAM/SQRT_RESULT at 0x40002b0). ntr maps
# that page as plain memory and hooks the write-triggered registers, but only
# through the NTR_MMIO macro hostgen substitutes -- a raw cast-deref writes the
# byte and never runs the unit, so the function returned whatever was last left
# in the result register. Measured before this: a player 95 scene units from
# the listener came back 8192, past every distance limit in the game, and every
# positional sound was culled. It goes through hostgen like every other TU that
# touches hardware; src/func_02049018.c comes out of slice 10.
set(GATE35_SYMS func_02049018)
set(GATE35_GEN "")
foreach(sym IN LISTS GATE35_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE35_GEN "${sym_out}")
endforeach()

set(GATE32_HOST_SOURCES
    hal/actor_classes_bob_enemy.cpp
    hal/actor_spawn_probe_bob.cpp
    hal/bob_enemy_bridges.cpp
    hal/bob_enemy_shadow_faces.cpp
    hal/bob_enemy_header_faces.cpp
    # the ModelAnim slot-5 collision, for the fourth gate running
    unmatched/BobEnemy_Renders.cpp
    # the mwcc pointer-to-member dispatch for the eighth time, and the first
    # one that belongs to a BASE class: Enemy's own death-type table
    unmatched/Enemy_UpdateDeath.cpp
    # ...and for the ninth: the bob-omb buddy's three-state machine
    unmatched/BobOmbBuddy_States.cpp
    # ...and for the tenth: the chain chomp's six
    unmatched/ChainChomp_States.cpp
    # the ARM r0 ride-through, in a spawn factory for the first time
    unmatched/ChainChomp_Spawn.cpp
    # ...and for the eleventh, the widest one yet: KING_BOB_OMB keeps the
    # ADDRESS of a two-PMF record rather than an index, so both his setter and
    # his Behavior do pointer arithmetic on a type MSVC doubles
    unmatched/KingBobOmb_States.cpp)

# The header/definition collisions in this gate's closure: decl_common.h
# declares each of these `(void*)` and its own TU defines it `(char*)`, which
# is one register on the ROM and C2733 to MSVC. hostgen's HEADER_SHADOW hides
# the header's declaration across the include; src/ and include/ stay untouched.
# func_ov084_021298d0 is here for a different reason: it is C++ inside a .c
# (the gate-9 case), which mwccarm accepted and MSVC will not compile as C.
# hostgen emits it as .cpp with the symbols wrapped back to C linkage.
# func_ov078_02124cf4 is the first kind again: decl_common.h declares
# func_0200fa8c `(void*, int)` and func_ov078_02125c24 `(char*, int)` while
# that TU declares both `(void*, int)`. One register on the ROM, two C2733s to
# MSVC.
set(GATE32_SYMS func_ov102_0214b248 func_ov084_021298d0 func_ov078_02124cf4)
set(GATE32_GEN "")
foreach(sym IN LISTS GATE32_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE32_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate35.txt" SLICE35_LINES)
set(SLICE35_SOURCES "")
foreach(line IN LISTS SLICE35_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    # (a stray `list(APPEND GATE32_GEN ...)` lived here from the six-stream
    # merge -- lesson 4's split-loop damage, masked because CMake dedupes
    # target sources. Caught by the post-merge cross-loop-append check.)
    list(APPEND SLICE35_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 13: the real Camera actor. Adds its own slice plus the HAL seam, and
# swaps three gate-10 files for host copies that only the camera needs
# (see port/unmatched/): ChangeArea calls HideArea() with no argument and
# relies on the ARM r0 ride-through, and func_0200cae4 / func_0200ca50
# dispatch through mwcc member-function pointers. smoke_player keeps the
# matched files -- it never arms data_0209f318, so nothing there dispatches.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate13.txt" SLICE13_LINES)
set(SLICE13_SOURCES "")
foreach(line IN LISTS SLICE13_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE13_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# THE POINTER SWEEP. port/tools/ptr_audit.py intersects romdata.py's NAMED
# list with config/arm9/relocs.txt and reports every named symbol whose ROM
# bytes contain a relocated word -- a DS address the port would emit raw and
# then call through. Two of its answers are tables that matched code indexes
# and CALLS: data_02086758 (the backup-media command table) and the 39 records
# at 0x02086bc0 (the kuppa script's camera commands). Both are hosted with real
# host function pointers in hal/ptr_tables.cpp, which needs their callees to
# exist as host code -- that is slice_ptrsweep.txt, 65 files of matched src
# that were in no slice at all, because the only thing naming them was a table
# of bytes. unmatched/func_02008550.cpp is the second table's consumer,
# host-copied for the same mwcc-PMF reason func_0200cae4 is.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_ptrsweep.txt" SLICE_PTR_LINES)
set(SLICE_PTR_SOURCES hal/ptr_tables.cpp unmatched/func_02008550.cpp)
foreach(line IN LISTS SLICE_PTR_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE_PTR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# gate 10 minus the two files the camera build replaces
set(SLICE10_CAM_SOURCES "")
foreach(src IN LISTS SLICE10_SOURCES)
    if(src MATCHES "/ChangeArea\\.c$" OR src MATCHES "/func_0200cae4\\.cpp$")
        continue()
    endif()
    # the pointer sweep: func_02008550 dispatches the camera script commands
    # through an mwcc pointer-to-member array, which MSVC cannot represent and
    # which was loading DS code addresses out of romdata besides. Host copy in
    # port/unmatched/, tables in hal/ptr_tables.cpp.
    if(src MATCHES "/func_02008550\\.cpp$")
        continue()
    endif()
    # gate 14: three more ARM ride-throughs, dormant until the level boot
    # reaches them (port/unmatched/ActorDerived_Spawn.cpp, PathPtr_FromID.cpp)
    if(src MATCHES "/_ZN12ActorDerived5SpawnEjP9ActorBaseii\\.cpp$"
       OR src MATCHES "/func_02042ffc\\.c$"
       OR src MATCHES "/_ZN7PathPtr6FromIDEj\\.cpp$"
       OR src MATCHES "/func_0204335c\\.c$")
        continue()
    endif()
    # gate 29: func_02021d1c is the particle spawn's tail, and the ONE place
    # in the subsystem that reaches a Callback vtable through a C++ `virtual`
    # -- a local shadow class, `p6->Run(...)`, which MSVC compiles thiscall
    # while every other dispatch on the same vtable is cdecl. It goes through
    # hostgen with a VIRTUAL_CALL patch instead, listed in slice_gate29.txt;
    # same treatment as func_ov002_020d708c above. Only the three targets that
    # take this filtered list exist, and all three carry gate 29.
    if(src MATCHES "/func_02021d1c\\.cpp$")
        continue()
    endif()
    # Sound: five ARM argument ride-throughs, replaced by hal/sdat/sound_abi.cpp
    # (declared with fewer parameters than their callers pass, which only
    # works when the extras sit in r1..r3), plus func_0205b5d4, the "read the
    # other core's progress" read that hal/sdat/consumer.cpp has to own so the
    # queue's spin-wait can terminate on a single-threaded host.
    if(src MATCHES "/func_0204f600\\.c$"
       OR src MATCHES "/func_0204f89c\\.c$"
       OR src MATCHES "/func_0204f7cc\\.c$"
       OR src MATCHES "/func_0204f86c\\.c$"
       OR src MATCHES "/func_0204fa2c\\.c$"
       OR src MATCHES "/func_0205b5d4\\.c$"
    # ...and Sound::Player::SetPlayableSeqCount, which writes through
    # data_020a4d84 = data_020a4d6c + 0x18, an adjacency host symbols do not
    # have. Same file, sound_abi.cpp.
       OR src MATCHES "/_ZN5Sound6Player19SetPlayableSeqCountEii\\.c$"
    # ...and Sound::Play, whose src calls func_02050cdc(void) and lets the
    # kind and id ride through in r0/r1.
       OR src MATCHES "/_ZN5Sound4PlayEjjRK7Vector3\\.c$"
    # ...and func_0203d974, "does anything still need loading off the card".
    # Its src reads the DS main-RAM mirror word 0x027ffc40 literally, which is
    # past MAIN_BASE+MAIN_SIZE (ntr/include/ntr/mmio.h) and unmapped -- and the
    # DS-faithful answer would then walk into a null sound heap. Hosted in
    # sound_abi.cpp; the header there has the whole argument. It is listed in
    # slice_gate14.txt as well, where it is commented out rather than filtered:
    # gate 14 is appended unfiltered, so a live line there puts it straight back
    # into the link (same as SetPlayableSeqCount above).
       OR src MATCHES "/func_0203d974\\.c$")
        continue()
    endif()
    # gate 35: HitDeathPlane is that shape one more time -- it calls
    # SetNextLevel with the argument riding in r0 across a call its own
    # declaration spells `(void)`. hal/star_flow.cpp names the rider.
    # func_0203d974 comes out with it (it is listed in both slice 10 and slice
    # 14): its 0x027ffc40 console-type read is in no region ntr maps, and the
    # course sound boot is its first caller.
    # ...and CollectStar, the WRITE half of the s8/int argument-width
    # disagreement port/unmatched/NumStars.cpp already carries the read half
    # of. port/unmatched/CollectStar.cpp takes the course id as s8.
    if(src MATCHES "/HitDeathPlane\\.c$"
       OR src MATCHES "/func_0203d974\\.c$"
       OR src MATCHES "/CollectStar\\.c$"
    # ...and func_02049018, the 3D sound distance function, which drives the
    # DS square-root unit through raw cast-derefs and so has to go through
    # hostgen's MMIO transform (GATE35_SYMS) rather than compile as it stands.
       OR src MATCHES "/func_02049018\\.c$")
        continue()
    endif()
    list(APPEND SLICE10_CAM_SOURCES "${src}")
endforeach()

# The hosted ARM7: native SDAT player behind the game's own command queue.
set(SOUND_HOST_SOURCES
    hal/sdat/sdat.cpp
    hal/sdat/mixer.cpp
    hal/sdat/sseq.cpp
    hal/sdat/out_win.cpp
    hal/sdat/consumer.cpp
    hal/sdat/sound_abi.cpp
    hal/sdat/sound_bss.cpp)

set(LEVELBOOT_HOST_SOURCES
    unmatched/ActorDerived_Spawn.cpp
    unmatched/PathPtr_FromID.cpp
    unmatched/func_0204335c.cpp
    unmatched/func_02043288.cpp
    unmatched/func_0204322c.cpp
    unmatched/func_02043fdc.cpp
    # ...and a fourth, dormant one: LoadEntranceObjects ends in
    # StartEntranceFaderWipe() spelled (void), with the wipe type riding in r0
    # from the `< 0` test right above it. Every entrance the port mounts takes
    # that return, so the call never fires today -- see the file header for
    # what it did and did not fix. Commented out of slice_gate14.txt in
    # exchange.
    unmatched/LoadEntranceObjects.cpp
    unmatched/func_02016ff4.cpp
    unmatched/ActorBase_AfterCleanupResources.cpp
    unmatched/func_ov002_020c6f3c.cpp
    # gate 16: two more mwcc pointer-to-member dispatches, both in actor
    # classes the registry carries. MSVC forms a PMF over an INCOMPLETE class
    # as the four-word general representation, which doubles the stride of the
    # table it indexes; see each file's header.
    unmatched/OneUpMushroom_Behavior.cpp
    unmatched/NumStars.cpp
    # gate 35: the WRITE half of the same s8/int argument-width disagreement
    unmatched/CollectStar.cpp
    unmatched/SignPost_StateDispatch.cpp
    # gate 18: the third, LakituBro's eleven-state machine
    unmatched/LakituBro_Behavior.cpp
    # ...and gate 17's ModelAnim slot-5 case with a body attached
    unmatched/Ov085_Renders.cpp
    # gate 19: the same slot problem one slot lower, on Model
    unmatched/Cannon_Render.cpp
    unmatched/Cannon_Behavior.cpp
    # gate 21: the pointer-to-member dispatch for the fifth time
    unmatched/Fish_Behavior.cpp
    # gate 22: the sixth, plus the Model slot the dual fill cannot serve
    unmatched/Door_Behavior.cpp
    unmatched/Door_Render.cpp
    # gate 23: the seventh, and the last on this level
    unmatched/QuestionBlock_States.cpp
    # gates 21 and 23: the ModelAnim slot-5 collision, three more times
    unmatched/ModelAnim_Renders.cpp)

set(CAMERA_HOST_SOURCES
    unmatched/ChangeArea.c
    unmatched/func_0200cae4.cpp
    unmatched/func_0200ca50.cpp
    unmatched/G3i_PerspectiveW.cpp
    unmatched/G3i_LookAt.cpp
    hal/camera_states.cpp
    hal/camera_bridges.cpp)

# Camera::Render writes CLEAR_COLOR at a literal address; LenVec3 drives the
# hardware sqrt unit and cstd::ldiv_result reads the divider result -- all
# three only work through the NTR_MMIO proxy.
# func_02009d90 carries a GCC __attribute__((unused)) MSVC will not parse;
# hostgen strips attributes as well as rewriting MMIO.
# _ZN12ActorDerived18AfterInitResourcesEj is C++ inside a .c (the gate-9 case
# again): hostgen emits it as .cpp with the C-linkage wrap, which MSVC accepts.
set(GATE13_SYMS _ZN6Camera6RenderEv LenVec3 _ZN4cstd11ldiv_resultEv
    func_02009d90 _ZN12ActorDerived18AfterInitResourcesEj)
set(GATE13_GEN "")
foreach(sym IN LISTS GATE13_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE13_GEN "${sym_out}")
endforeach()

# LenVec3 comes from the hostgen copy now, so drop the plain gate-9 entry.
set(SLICE9_CAM_SOURCES "")
foreach(src IN LISTS SLICE9_SOURCES)
    if(src MATCHES "/LenVec3\\.c$")
        continue()
    endif()
    list(APPEND SLICE9_CAM_SOURCES "${src}")
endforeach()

# Gate 14: two more C++-in-a-.c files (the gate-9 case): Actor's
# AfterInitResources, which the spawn spine's init Process dispatches, and
# StartEntranceFaderWipe's sibling already in GATE10_SYMS.
set(GATE14_SYMS _ZN5Actor18AfterInitResourcesEj func_ov002_020c6fe4)
set(GATE14_GEN "")
foreach(sym IN LISTS GATE14_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE14_GEN "${sym_out}")
endforeach()

# Gate 14: the real level boot. Stage::LoadClsnAndObjects over ov009's own
# LVL_Overlay, the fifteen sub-loaders, and the setters they store through.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate14.txt" SLICE14_LINES)
set(SLICE14_SOURCES "")
foreach(line IN LISTS SLICE14_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    # gate 35: func_0203d974 reads the console-type halfword at 0x027ffc40,
    # which no ntr region maps. It was harmless while nothing called it; the
    # course sound boot calls it. hal/star_flow.cpp hosts it.
    if(line MATCHES "/func_0203d974\\.c$")
        continue()
    endif()
    list(APPEND SLICE14_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 36: the real Stage::InitResources, to replace port_stage_a_boot. The
# function and all 45 of its direct callees are matched already; this is
# wiring, not new code. Linked but NOT yet called -- the boot swap comes after
# the closure resolves. See the head of slice_gate36.txt.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate36.txt" SLICE36_LINES)
set(SLICE36_SOURCES "")
foreach(line IN LISTS SLICE36_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE36_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gates 40-44: levels 1 and 2 (castle grounds ov009, castle interior ov010),
# the actor classes their object tables name that were still printing as
# skipped. Each gate's evidence is in the head of its slice_gateNN.txt and the
# registry rows are in hal/actor_classes.inc.
#   Gate 40: STAR_DOOR (354, ov100) -- the config's mislabeled _ZN4Door family.
#   Gate 41: TRAP (36) + LIGHT_BEAM (37), ov010, level 2's own overlay.
#   Gate 42: PEACH_PAINTING (38), ov010.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate40.txt" SLICE40_LINES)
set(SLICE40_SOURCES "")
foreach(line IN LISTS SLICE40_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE40_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate41.txt" SLICE41_LINES)
set(SLICE41_SOURCES "")
foreach(line IN LISTS SLICE41_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE41_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()
# Gate 42 (PEACH_PAINTING) is blocked on ov052/RollingRock and not built; no
# slice_gate42 is wired until that overlay is hosted.

# Gate 50: ov080's PAINTING (daPicGate_c, 307), the castle interior's portrait
# gate. A different class from PEACH_PAINTING, in a different overlay, with no
# cross-overlay spawn -- its closure is 28 matched ov080 TUs plus three sinits.
# See the head of slice_gate50.txt and hal/actor_classes_painting.cpp.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate50.txt" SLICE50_LINES)
set(SLICE50_SOURCES "")
foreach(line IN LISTS SLICE50_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE50_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 51: the CAP (WaterfallMist, 269), ov002. Built at gate 33, registered
# here now that its nine-state PMF table is seated. See slice_gate51.txt and
# unmatched/Cap_StateDispatch.cpp.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate51.txt" SLICE51_LINES)
set(SLICE51_SOURCES "")
foreach(line IN LISTS SLICE51_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE51_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 15: the per-frame actor spine (BeforeBehavior/Behavior/AfterBehavior),
# which is what makes the real WithMeshClsn tracking work -- see the header of
# slice_gate15.txt for why prev-pos is load-bearing.
#
# Actor::BeforeBehavior defines its eight engine BSS symbols at file scope the
# way mwccarm accepts (tentative definitions the linker merges). MSVC's C++
# front end makes them strong and they all belong to the HAL, so the file goes
# through hostgen with --extern-data.
set(GATE15_SYMS _ZN5Actor14BeforeBehaviorEv)
set(GATE15_GEN "")
foreach(sym IN LISTS GATE15_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" --extern-data ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE15_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate15.txt" SLICE15_LINES)
set(SLICE15_SOURCES "")
foreach(line IN LISTS SLICE15_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE15_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# Gate 16: real objects on the castle grounds -- the ROM's own actor frame
# (the five processing lists) plus one matched lifecycle closure per class the
# registry carries. See the header of slice_gate16.txt.
#
# The hostgen set is the gate-9 case each time: C++ syntax (a real method, or
# a `virtual` shadow class) inside a .c file, which mwccarm accepted and MSVC
# will not compile as C. hostgen emits .cpp with the symbols wrapped back to
# C linkage, so the byte-verified tree is never edited.
set(GATE16_SYMS _ZN5Actor22BeforeCleanupResourcesEv
    func_ov002_020aefb8
    # Gate 33: the WARP's Behavior is C++ inside a .c -- the same case.
    func_ov002_020ec410)
set(GATE16_GEN "")
foreach(sym IN LISTS GATE16_SYMS)
    if(EXISTS "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.c")
    else()
        set(sym_src "${PORT_REPO_ROOT_ABS}/src/${sym}.cpp")
    endif()
    set(sym_out "${CMAKE_BINARY_DIR}/host-src/src/${sym}.cpp")
    add_custom_command(OUTPUT "${sym_out}"
        COMMAND python "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
                --out "${CMAKE_BINARY_DIR}/host-src" ${sym}
        DEPENDS "${sym_src}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/hostgen.py"
        COMMENT "hostgen ${sym}")
    list(APPEND GATE16_GEN "${sym_out}")
endforeach()

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/slice_gate16.txt" SLICE16_LINES)
set(SLICE16_SOURCES "")
foreach(line IN LISTS SLICE16_LINES)
    string(STRIP "${line}" line)
    if(line MATCHES "^#" OR line STREQUAL "")
        continue()
    endif()
    list(APPEND SLICE16_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../${line}")
endforeach()

# The camera slice is on both binaries. The gate-13 State objects carry real
# DS function addresses, and Player::ChangeState already reaches
# func_0200d81c -> Camera::ChangeState on every state change, so the smoke
# needs the host dispatcher too or the first ChangeState calls a mwcc
# member-function pointer.
add_executable(smoke_player tests/smoke_player.cpp "${OV002DATA_C}" ${LEVEL_OV_DATA}
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/actor_vtables.cpp hal/anim_bridge.cpp
    hal/cxxname_bridge.cpp hal/megachar_stub.cpp hal/player_bridges.cpp hal/cxx_aliases.cpp hal/auto_bss.cpp hal/reverse_bridges.cpp hal/method_faces.cpp hal/fader_wipes.cpp hal/level_boot.cpp hal/message_boot.cpp hal/actor_registry.cpp hal/actor_classes.cpp hal/actor_classes_bob_world.cpp hal/actor_faces_bob.cpp hal/actor_class_faces.cpp hal/mmc_vtable.cpp hal/ov009_boot.cpp hal/actor_overlays.cpp hal/sub_screen.cpp hal/sub_actors.cpp hal/stage_bridges.cpp hal/level_change.cpp hal/star_flow.cpp hal/oam_lists.cpp hal/blend_vtable.cpp hal/particle_vtable.cpp hal/particle_bridges.cpp unmatched/Particle_RideThroughs.cpp
    ${CAMERA_HOST_SOURCES} ${LEVELBOOT_HOST_SOURCES} ${SOUND_HOST_SOURCES}
    ${SLICE_PTR_SOURCES}
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE8_SOURCES} ${SLICE9_CAM_SOURCES}
    ${SLICE10_CAM_SOURCES} ${SLICE13_SOURCES} ${SLICE14_SOURCES}
    ${SLICE15_SOURCES} ${SLICE16_SOURCES} ${SLICE17_SOURCES}
    ${SLICE18_SOURCES} ${SLICE19_SOURCES} ${SLICE20_SOURCES}
    ${SLICE21_SOURCES} ${SLICE22_SOURCES} ${SLICE23_SOURCES} ${SLICE25_SOURCES} ${SLICE26_SOURCES}
    ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE33_SOURCES} ${GATE6_GEN} ${SLICE31_SOURCES} ${SLICE35_SOURCES}
    ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE32_SOURCES}
    ${GATE32_HOST_SOURCES} ${GATE32_GEN} ${GATE6_GEN}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} ${GATE10_GEN}
    ${GATE13_GEN} ${GATE14_GEN} ${GATE15_GEN} ${GATE16_GEN} ${GATE35_GEN} ${GATE29_GEN}
    ${SLICE40_SOURCES} ${SLICE41_SOURCES}
    hal/actor_classes_painting.cpp ${SLICE50_SOURCES}
    unmatched/Cap_StateDispatch.cpp unmatched/ExclamationSwitch_StateDispatch.cpp ${SLICE51_SOURCES}
    unmatched/WaterBomb_StateDispatch.cpp unmatched/WaterBomb_State0.cpp
    "${ROMDATA_C}" "${OV009SYMS_C}" ${ACTOR_OV_SYMS} "${OVCROSS_C}")
target_include_directories(smoke_player PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(smoke_player PRIVATE ntr)
target_compile_definitions(smoke_player PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}"
    # this target links hal/cxx_aliases.cpp, which already carries
    # the Heap::_Deallocate face hal/blend_vtable.cpp repeats
    PORT_CXX_ALIASES_LINKED)
if(MSVC)
    target_compile_options(smoke_player PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# Gate 12: the interactive window (same stack, windowed loop).
add_executable(walk_window tests/walk_window.cpp "${OV002DATA_C}" ${LEVEL_OV_DATA}
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/actor_vtables.cpp hal/anim_bridge.cpp
    hal/cxxname_bridge.cpp hal/megachar_stub.cpp hal/player_bridges.cpp hal/cxx_aliases.cpp hal/auto_bss.cpp hal/reverse_bridges.cpp hal/method_faces.cpp hal/fader_wipes.cpp hal/level_boot.cpp hal/message_boot.cpp hal/actor_registry.cpp hal/actor_classes.cpp hal/actor_classes_bob_world.cpp hal/actor_faces_bob.cpp hal/actor_class_faces.cpp hal/mmc_vtable.cpp hal/ov009_boot.cpp hal/actor_overlays.cpp hal/sub_actors.cpp hal/sub_screen.cpp hal/stage_bridges.cpp hal/level_change.cpp hal/star_flow.cpp hal/oam_lists.cpp hal/blend_vtable.cpp hal/particle_vtable.cpp hal/particle_bridges.cpp unmatched/Particle_RideThroughs.cpp
    ${CAMERA_HOST_SOURCES} ${LEVELBOOT_HOST_SOURCES} ${SOUND_HOST_SOURCES}
    ${SLICE_PTR_SOURCES}
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE8_SOURCES} ${SLICE9_CAM_SOURCES}
    ${SLICE10_CAM_SOURCES} ${SLICE13_SOURCES} ${SLICE14_SOURCES}
    ${SLICE15_SOURCES} ${SLICE16_SOURCES} ${SLICE17_SOURCES}
    ${SLICE18_SOURCES} ${SLICE19_SOURCES} ${SLICE20_SOURCES}
    ${SLICE21_SOURCES} ${SLICE22_SOURCES} ${SLICE23_SOURCES}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES}
    ${SLICE33_SOURCES} ${GATE6_GEN}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES} ${SLICE31_SOURCES} ${GATE6_GEN}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES} ${SLICE35_SOURCES} ${GATE6_GEN}
    ${SLICE32_SOURCES} ${GATE32_HOST_SOURCES} ${GATE32_GEN} ${GATE6_GEN}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} ${GATE10_GEN}
    ${GATE13_GEN} ${GATE14_GEN} ${GATE15_GEN} ${GATE16_GEN} ${GATE35_GEN} ${GATE29_GEN}
    ${SLICE40_SOURCES} ${SLICE41_SOURCES}
    hal/actor_classes_painting.cpp ${SLICE50_SOURCES}
    unmatched/Cap_StateDispatch.cpp unmatched/ExclamationSwitch_StateDispatch.cpp ${SLICE51_SOURCES}
    unmatched/WaterBomb_StateDispatch.cpp unmatched/WaterBomb_State0.cpp
    "${ROMDATA_C}" "${OV009SYMS_C}" ${ACTOR_OV_SYMS} "${OVCROSS_C}")
target_include_directories(walk_window PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
# 2x (512x384, window ZOOM 2): the software raster is fill-bound; the
# hires 1024x768 tier burns ~40ms/frame on the level alone, native
# 256x192 reads too chunky. 2x is ~12ms -> 60fps with frame pacing.
target_link_libraries(walk_window PRIVATE ntr_2x)
target_compile_definitions(walk_window PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}"
    # this target links hal/cxx_aliases.cpp, which already carries
    # the Heap::_Deallocate face hal/blend_vtable.cpp repeats
    PORT_CXX_ALIASES_LINKED)
if(MSVC)
    target_compile_options(walk_window PRIVATE /wd4311 /wd4312 /wd4068)
endif()

# hires clone for screenshot sessions (4x resolution, ~1/16 the fps)
add_executable(walk_window_hires tests/walk_window.cpp "${OV002DATA_C}" ${LEVEL_OV_DATA}
    hal/heap_globals.cpp hal/os_arena.cpp hal/heap_vtable.cpp hal/fs.cpp
    hal/model_host.cpp hal/gx_upload_bridge.cpp hal/cstd_div.c
    hal/clsn_vtable.cpp hal/actor_vtables.cpp hal/anim_bridge.cpp
    hal/cxxname_bridge.cpp hal/megachar_stub.cpp hal/player_bridges.cpp hal/cxx_aliases.cpp hal/auto_bss.cpp hal/reverse_bridges.cpp hal/method_faces.cpp hal/fader_wipes.cpp hal/level_boot.cpp hal/message_boot.cpp hal/actor_registry.cpp hal/actor_classes.cpp hal/actor_classes_bob_world.cpp hal/actor_faces_bob.cpp hal/actor_class_faces.cpp hal/mmc_vtable.cpp hal/ov009_boot.cpp hal/actor_overlays.cpp hal/sub_actors.cpp hal/sub_screen.cpp hal/stage_bridges.cpp hal/level_change.cpp hal/star_flow.cpp hal/oam_lists.cpp hal/blend_vtable.cpp hal/particle_vtable.cpp hal/particle_bridges.cpp unmatched/Particle_RideThroughs.cpp
    ${CAMERA_HOST_SOURCES} ${LEVELBOOT_HOST_SOURCES} ${SOUND_HOST_SOURCES}
    ${SLICE_PTR_SOURCES}
    ${SLICE2_SOURCES} ${SLICE3A_SOURCES} ${SLICE3B_SOURCES} ${SLICE4B_SOURCES}
    ${SLICE7_SOURCES} ${SLICE8_SOURCES} ${SLICE9_CAM_SOURCES}
    ${SLICE10_CAM_SOURCES} ${SLICE13_SOURCES} ${SLICE14_SOURCES}
    ${SLICE15_SOURCES} ${SLICE16_SOURCES} ${SLICE17_SOURCES}
    ${SLICE18_SOURCES} ${SLICE19_SOURCES} ${SLICE20_SOURCES}
    ${SLICE21_SOURCES} ${SLICE22_SOURCES} ${SLICE23_SOURCES}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES}
    ${SLICE33_SOURCES} ${GATE6_GEN}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES} ${SLICE31_SOURCES} ${GATE6_GEN}
    ${SLICE25_SOURCES} ${SLICE27_SOURCES} ${SLICE28_SOURCES} ${SLICE26_SOURCES} ${SLICE35_SOURCES} ${GATE6_GEN}
    ${SLICE32_SOURCES} ${GATE32_HOST_SOURCES} ${GATE32_GEN} ${GATE6_GEN}
    ${GATE4A_GEN} ${GATE4B_GEN} ${GATE8_GEN} ${GATE10_GEN}
    ${GATE13_GEN} ${GATE14_GEN} ${GATE15_GEN} ${GATE16_GEN} ${GATE35_GEN} ${GATE29_GEN}
    ${SLICE40_SOURCES} ${SLICE41_SOURCES}
    hal/actor_classes_painting.cpp ${SLICE50_SOURCES}
    unmatched/Cap_StateDispatch.cpp unmatched/ExclamationSwitch_StateDispatch.cpp ${SLICE51_SOURCES}
    unmatched/WaterBomb_StateDispatch.cpp unmatched/WaterBomb_State0.cpp
    "${ROMDATA_C}" "${OV009SYMS_C}" ${ACTOR_OV_SYMS} "${OVCROSS_C}")
target_include_directories(walk_window_hires PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_link_libraries(walk_window_hires PRIVATE ntr_hires)
target_compile_definitions(walk_window_hires PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}"
    # this target links hal/cxx_aliases.cpp, which already carries
    # the Heap::_Deallocate face hal/blend_vtable.cpp repeats
    PORT_CXX_ALIASES_LINKED)
if(MSVC)
    target_compile_options(walk_window_hires PRIVATE /wd4311 /wd4312 /wd4068)
endif()


# The SDAT player on its own: parser, sequencer and mixer with no game code
# and no command queue, so a failure is localised to the player itself.
add_executable(smoke_sdat tests/smoke_sdat.cpp
    hal/sdat/sdat.cpp hal/sdat/mixer.cpp hal/sdat/sseq.cpp hal/sdat/out_win.cpp)
target_include_directories(smoke_sdat PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(smoke_sdat PRIVATE SM64DS_PLATFORM_PC
    PORT_REPO_ROOT="${PORT_REPO_ROOT_ABS}")
if(MSVC)
    target_compile_options(smoke_sdat PRIVATE /wd4311 /wd4312 /wd4068)
endif()
