add_library(Engine_Core STATIC
    include/engine/core/BaseTypes.h
    include/engine/core/Assert.h
    include/engine/core/Log.h
    include/engine/core/Profiler.h
    include/engine/core/Allocators.h
    include/engine/core/JobSystem.h
    src/Assert.cpp
    src/Log.cpp
    src/Profiler.cpp
    src/Allocators.cpp
    src/JobSystem.cpp
)

target_include_directories(Engine_Core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_set_compiler_warnings(Engine_Core)

if(ENABLE_TRACY)
    target_compile_definitions(Engine_Core PUBLIC TRACY_ENABLE)
endif()
