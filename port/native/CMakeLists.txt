cmake_minimum_required(VERSION 3.20)
project(sm64ds_native_core LANGUAGES CXX)

# This builds the real DS hardware/runtime library, not a playable game host.
# The game still needs its native window/audio/network layer and ABI bridges.
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The native DS core currently requires 32-bit Linux; use Clang with -m32")
endif()
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "Clang is required for the existing DS state-section declarations")
endif()

find_package(Threads REQUIRED)
set(PORT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
add_library(sm64ds_native_core STATIC
    ${PORT_ROOT}/ntr/io.cpp
    ${PORT_ROOT}/ntr/gx.cpp
    ${PORT_ROOT}/ntr/texture.cpp
    ${PORT_ROOT}/ntr/ppu.cpp
    ${PORT_ROOT}/ntr/ppu_sub.cpp
    ${PORT_ROOT}/ntr/ppu_audit.cpp
    ${PORT_ROOT}/ntr/lz77.cpp
    ${PORT_ROOT}/ntr/rt.cpp
    ${PORT_ROOT}/ntr/runtime.cpp
    ${PORT_ROOT}/ntr/fs.cpp
    ${PORT_ROOT}/ntr/bmd.cpp
    ${PORT_ROOT}/ntr/ipc.cpp
    ${PORT_ROOT}/ntr/hdtex.cpp
    ${PORT_ROOT}/ntr/smooth.cpp
    ${PORT_ROOT}/ntr/frame_context.cpp
    ${PORT_ROOT}/hal/dsstate_seg.cpp)
target_compile_features(sm64ds_native_core PUBLIC cxx_std_17)
target_include_directories(sm64ds_native_core PUBLIC ${PORT_ROOT}/ntr/include)
target_include_directories(sm64ds_native_core PRIVATE ${PORT_ROOT})
target_compile_options(sm64ds_native_core PRIVATE -fms-extensions -fno-pie)
target_link_libraries(sm64ds_native_core PUBLIC Threads::Threads)

add_executable(native_runtime_test ${PORT_ROOT}/tests/coop_native_runtime_test.cpp)
target_compile_options(native_runtime_test PRIVATE -UNDEBUG -fno-pie)
target_link_libraries(native_runtime_test PRIVATE sm64ds_native_core)
# Preserve the existing grouped DS globals and the adjacent IRQ callback bank.
target_link_options(native_runtime_test PRIVATE -no-pie "LINKER:-T,${CMAKE_CURRENT_SOURCE_DIR}/dsstate.ld")
enable_testing()
add_test(NAME actual_native_frame_loop_and_irq COMMAND native_runtime_test)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(NATIVE_TYPES "${CMAKE_CURRENT_BINARY_DIR}/native-types.h")
add_custom_command(OUTPUT "${NATIVE_TYPES}"
    COMMAND ${Python3_EXECUTABLE} ${PORT_ROOT}/tools/native_headers.py
        --root ${PORT_ROOT}/.. --out "${NATIVE_TYPES}"
    DEPENDS ${PORT_ROOT}/tools/native_headers.py ${PORT_ROOT}/../include/types.h)
add_custom_target(native_types DEPENDS "${NATIVE_TYPES}")
add_library(native_player_compile OBJECT
    ${PORT_ROOT}/hal/player_bridges.cpp
    ${PORT_ROOT}/../src/_ZN6Player12St_Wait_MainEv.cpp
    ${PORT_ROOT}/tests/coop_engine_abi_probe.cpp)
add_dependencies(native_player_compile native_types)
target_compile_features(native_player_compile PRIVATE cxx_std_17)
target_compile_definitions(native_player_compile PRIVATE SM64DS_PLATFORM_PC)
target_include_directories(native_player_compile PRIVATE ${PORT_ROOT}/../include ${PORT_ROOT} ${PORT_ROOT}/ntr/include)
target_compile_options(native_player_compile PRIVATE -fms-extensions -include "${NATIVE_TYPES}")

# Execute recovered game allocator methods, including their native class calls.
# A constructor-return bridge is necessary: SysV constructors do not return this.
set(NATIVE_CREATE_HEAP "${CMAKE_CURRENT_BINARY_DIR}/native-create-heap.cpp")
add_custom_command(OUTPUT "${NATIVE_CREATE_HEAP}"
    COMMAND ${Python3_EXECUTABLE} ${PORT_ROOT}/tools/native_heap_source.py
        --root ${PORT_ROOT}/.. --out "${NATIVE_CREATE_HEAP}"
    DEPENDS ${PORT_ROOT}/tools/native_heap_source.py
        ${PORT_ROOT}/../src/_ZN4Heap28CreateExpandingHeapAllocatorEPvjj.cpp)
file(STRINGS "${PORT_ROOT}/slice_gate2.txt" NATIVE_HEAP_ROWS REGEX "^src/")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${PORT_ROOT}/slice_gate2.txt")
set(NATIVE_HEAP_SOURCES "")
foreach(row IN LISTS NATIVE_HEAP_ROWS)
    if(NOT row STREQUAL "src/_ZN4Heap28CreateExpandingHeapAllocatorEPvjj.cpp")
        list(APPEND NATIVE_HEAP_SOURCES "${PORT_ROOT}/../${row}")
    endif()
endforeach()
add_executable(native_heap_test ${PORT_ROOT}/tests/smoke_heap.cpp
    heap_bridge.cpp ${PORT_ROOT}/hal/heap_globals.cpp ${NATIVE_HEAP_SOURCES} "${NATIVE_CREATE_HEAP}")
add_dependencies(native_heap_test native_types)
target_compile_features(native_heap_test PRIVATE cxx_std_17)
target_compile_definitions(native_heap_test PRIVATE SM64DS_PLATFORM_PC)
target_include_directories(native_heap_test PRIVATE ${PORT_ROOT}/../include ${PORT_ROOT} ${PORT_ROOT}/hal)
target_compile_options(native_heap_test PRIVATE -UNDEBUG -fno-pie -fms-extensions -include "${NATIVE_TYPES}")
target_link_options(native_heap_test PRIVATE -no-pie)
add_test(NAME native_game_allocator_5000_operations COMMAND native_heap_test)
