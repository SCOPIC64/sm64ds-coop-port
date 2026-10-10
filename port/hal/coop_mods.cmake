set(COOP_MOD_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
file(READ "${COOP_MOD_SOURCE_DIR}/../mods/sm64-movement/main.lua" MOVEMENT_LUA)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${COOP_MOD_SOURCE_DIR}/../mods/sm64-movement/main.lua")
configure_file("${COOP_MOD_SOURCE_DIR}/coop_mod_builtin.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/coop_mods_generated/coop_mod_builtin.h" @ONLY)
function(coop_add_mods target lua_target)
    target_sources(${target} PRIVATE "${COOP_MOD_SOURCE_DIR}/coop_mods.cpp")
    target_include_directories(${target} PRIVATE "${COOP_MOD_SOURCE_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/coop_mods_generated")
    target_compile_features(${target} PRIVATE cxx_std_17)
    target_link_libraries(${target} PRIVATE ${lua_target})
endfunction()
