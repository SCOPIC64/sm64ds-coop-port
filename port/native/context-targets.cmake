# The existing Windows fiber implementation remains the game backend there.
# Native targets use the same-thread coroutine backend implemented in rt.cpp.
if(NOT WIN32)
    foreach(native_runtime_target IN ITEMS ntr ntr_wide_rt ntr_hires)
        if(TARGET ${native_runtime_target})
            target_sources(${native_runtime_target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ntr/frame_context.cpp")
        endif()
    endforeach()
endif()
