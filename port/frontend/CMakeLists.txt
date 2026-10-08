cmake_minimum_required(VERSION 3.16)
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
