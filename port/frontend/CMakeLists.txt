cmake_minimum_required(VERSION 3.16)
# Match the game's self-contained CRT choice so checks also run with a portable
# MSVC toolchain that does not install the Visual Studio debug runtime DLLs.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(sm64ds_frontend_checks LANGUAGES CXX)

# This builds/tests the portable UI and input layer, NOT the game engine.
option(HANDHELD "Use the SteamOS/handheld menu profile" OFF)
add_executable(coop_frontend_test ../tests/coop_frontend_test.cpp)
target_include_directories(coop_frontend_test PRIVATE ..)
target_compile_features(coop_frontend_test PRIVATE cxx_std_11)
if(HANDHELD)
    target_compile_definitions(coop_frontend_test PRIVATE SM64DS_HANDHELD=1)
endif()
if(MSVC)
    # Standard C APIs keep this component portable. All copies are bounded by
    # the model's fixed field capacities; MSVC's *_s APIs are not portable.
    target_compile_definitions(coop_frontend_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(coop_frontend_test PRIVATE /W4 /WX /UNDEBUG)
else()
    target_compile_options(coop_frontend_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
endif()
enable_testing()
add_test(NAME menu_and_multitouch COMMAND coop_frontend_test)

if(WIN32 AND MSVC)
    # Separate processes catch differences between the saved cache and a fresh
    # load. Keep all files in an isolated fixture beside the test executable.
    add_executable(coop_settings_test ../tests/coop_settings_test.cpp ../hal/host_settings.cpp)
    target_include_directories(coop_settings_test PRIVATE ..)
    target_compile_features(coop_settings_test PRIVATE cxx_std_17)
    target_compile_definitions(coop_settings_test PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(coop_settings_test PRIVATE /UNDEBUG)
    set_target_properties(coop_settings_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/settings-fixture")
    foreach(rate IN ITEMS native 144)
        add_test(NAME settings_save_${rate} COMMAND coop_settings_test save ${rate})
        add_test(NAME settings_restart_${rate} COMMAND coop_settings_test read ${rate})
        set_tests_properties(settings_save_${rate} settings_restart_${rate} PROPERTIES
            WORKING_DIRECTORY "$<TARGET_FILE_DIR:coop_settings_test>"
            RUN_SERIAL TRUE)
        set_tests_properties(settings_restart_${rate} PROPERTIES DEPENDS settings_save_${rate})
    endforeach()
    set_tests_properties(settings_save_144 PROPERTIES DEPENDS settings_restart_native)
endif()
